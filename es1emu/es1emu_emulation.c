/*
 * Copyright © 2016 Hitachi, Ltd.
 * Copyright © 2021 T.Aikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include "es1emu_emulation.h"
#include "es1emu_shader.h"
#include "es1emu_matrix.h"

#ifdef _GLES1_EMULATION

#define _GL_PTHREAD_SAFE
//------------------------------------------------------------------------------
// 定数
//------------------------------------------------------------------------------
#if 0
/**
 * @brief		シェーダー：色をグローバル指定
 */
#define VSHADER_VERTEX_ARRAY  "\
	precision lowp float;\
	attribute vec4 a_pos;\
	uniform mat4 u_pmvMatrix;\
	void main(void)\
	{\
		gl_Position = u_pmvMatrix * a_pos;\
	}"

#define FSHADER_VERTEX_ARRAY  "\
	precision lowp float;\
	uniform vec4 u_color;\
	void main (void)\
	{\
		gl_FragColor = u_color;\
	}"


/**
 * @brief		シェーダー：色を個別指定
 */
#define VSHADER_COLOR_ARRAY  "\
	precision lowp float;\
	attribute vec4 a_pos;\
	attribute vec4 a_color;\
	uniform mat4 u_pmvMatrix;\
	varying vec4 v_color;\
	void main(void)\
	{\
		gl_Position = u_pmvMatrix * a_pos;\
		v_color = a_color;\
	}"

#define FSHADER_COLOR_ARRAY  "\
	precision lowp float;\
	varying vec4 v_color;\
	void main (void)\
	{\
		gl_FragColor = v_color;\
	}"


/**
 * @brief		シェーダー：テクスチャ用
 */
#define VSHADER_TEXTURE_ARRAY  "\
	precision lowp float;\
	attribute vec4 a_pos;\
	attribute vec2 a_texture;\
	uniform mat4 u_pmvMatrix;\
	varying vec2 v_texcoord;\
	void main(void)\
	{\
		gl_Position = u_pmvMatrix * a_pos;\
		v_texcoord = a_texture;\
	}"

#define FSHADER_TEXTURE_ARRAY  "\
	precision lowp float;\
	varying vec2 v_texcoord;\
	uniform sampler2D texture0;\
	void main (void)\
	{\
		gl_FragColor = texture2D(texture0, v_texcoord);\
	}"
#else
#define VSHADER_VERTEX_ARRAY  "\
	precision highp float;\
	attribute vec4 a_pos;\
	attribute vec4 a_color;\
	attribute vec2 a_texture;\
	uniform mat4 u_pmvMatrix;\
	uniform vec4 u_color;\
	uniform bool u_color_arrey;\
	uniform bool u_texture_arrey;\
	varying vec4 v_color;\
	varying vec2 v_texcoord;\
	void main (void) \
	{\
		gl_Position = u_pmvMatrix * a_pos;\
		if (u_texture_arrey) {\
			v_texcoord = a_texture;\
		} else {\
			if (u_color_arrey) {\
				v_color = a_color;\
			} else {\
				v_color = u_color;\
			}\
		}\
	}"

#define FSHADER_VERTEX_ARRAY  "\
	precision highp float;\
	uniform bool u_texture_arrey;\
	uniform sampler2D texture0;\
	varying vec2 v_texcoord;\
	varying vec4 v_color;\
	void main (void)\
	{\
		if(u_texture_arrey) {\
			gl_FragColor = texture2D(texture0, v_texcoord);\
		} else {\
			gl_FragColor = v_color;\
		}\
	}"
#endif
// スタックサイズ
#define STACK_SIZE					(10)

// attributeインデックス
#define ATTR_LOC_POS				(0)		// attribute vec4 a_pos;
#define ATTR_LOC_COLOR				(1)		// attribute vec4 a_color;
#define ATTR_LOC_TEXTURE			(2)		// attribute vec2 a_texture;


//------------------------------------------------------------------------------
// 構造体
//------------------------------------------------------------------------------
// シェーダ情報
typedef struct {
	GLuint		programId;		// プログラムID

	// attributeロケーション
	// リンク前に"attributeインデックス"に関連付け済み

	// uniformロケーション
	GLint		uPMVMatrix;		// プロジェクション*モデルビューマトリクス
	GLint		uColor;			// グローバルカラー
	GLint		uTexture0;		// テクスチャー
	GLint		uColorArrey;
	GLint		uTextureArrey;
} PROGRAM_INFO;

// カラー情報
typedef struct {
	float		r;
	float		g;
	float		b;
	float		a;
} COLOR_INFO;

// マトリクススタック
typedef struct {
	MPMatrix	mat[STACK_SIZE];			// スタック
	GLint		sp;							// スタックポインタ
	MPMatrix	cur;						// カレント
} MATRIX_FILO;

// OpenGL ES パラメータ
typedef struct {
	PROGRAM_INFO	program[ES1EMU_PROGRAM_MAX];		// プログラム情報
	COLOR_INFO		color;						// グローバルカラー情報
	GLint			mode;						// マトリクスモード
	MATRIX_FILO		filo[2];					// スタック [0]:モデルビュー、[1]プロジェクション
	MPMatrix		*pMat;						// カレントマトリクス
} ES1PARAMS;


//------------------------------------------------------------------------------
// 静的変数
//------------------------------------------------------------------------------
// スレッド固有バッファのキー
static pthread_key_t buffer_key;
// 1 回限りのキーの初期化
static pthread_once_t buffer_key_once = PTHREAD_ONCE_INIT;


//------------------------------------------------------------------------------
// 内部関数プロトタイプ
//------------------------------------------------------------------------------
static void buffer_destroy(void *buf);
static void buffer_key_alloc();
static void threadBuffer_alloc(GLint size);
static void initParams();
static ES1PARAMS *getParams();


//------------------------------------------------------------------------------
// 関数
//------------------------------------------------------------------------------
// スレッド固有のバッファを解放する
static void buffer_destroy(void *buf)
{
	free(buf);
}

// キーを確保する */
static void buffer_key_alloc()
{
	pthread_key_create(&buffer_key, buffer_destroy);
}

// スレッド固有のバッファを確保する
static void threadBuffer_alloc(GLint size)
{
	pthread_once(&buffer_key_once, buffer_key_alloc);
	pthread_setspecific(buffer_key, malloc(size));
}

static void initParams()
{

#ifdef _GL_PTHREAD_SAFE
	// スレッド固有のバッファを確保
	threadBuffer_alloc(sizeof(ES1PARAMS));
#endif	// _GL_PTHREAD_SAFE

	ES1PARAMS* param = getParams();

	// 初期化
	memset(param, 0, sizeof(ES1PARAMS));

}

static ES1PARAMS *getParams()
{
#ifdef _GL_PTHREAD_SAFE
	// スレッド固有バッファから取得
	return ((ES1PARAMS*)pthread_getspecific(buffer_key));
#else
	static ES1PARAMS mParams;
	return (&mParams);
#endif	// _GL_PTHREAD_SAFE
}

int es1emu_Init()
{
	GLuint shaderProg;
	int ret = 1;

	// パラメータ初期化
	initParams();

	ES1PARAMS *param = getParams();

	do {
#if 0
		// SHADER_VERTEX_ARRAY
		{
			// シェーダプログラム生成
			shaderProg = es1emu_CreateProgram(VSHADER_VERTEX_ARRAY, FSHADER_VERTEX_ARRAY);
			if (shaderProg == 0) {
				ret = 0;
				break;
			}

			// AttribLocationのindexを明示的に指定(リンク前にglBindAttribLocation())
			glBindAttribLocation(shaderProg, ATTR_LOC_POS, "a_pos");

			// シェーダプログラムリンク
			shaderProg = es1emu_LinkShaderProgram(shaderProg);
			if (shaderProg == 0) {
				ret = 0;
				break;
			}

			param->program[ES1EMU_PROGRAM_VERTEX_ARRAY].programId = shaderProg;
			param->program[ES1EMU_PROGRAM_VERTEX_ARRAY].uPMVMatrix = glGetUniformLocation(shaderProg, "u_pmvMatrix");
			param->program[ES1EMU_PROGRAM_VERTEX_ARRAY].uColor = glGetUniformLocation(shaderProg, "u_color");
		}

		// SHADER_COLOR_ARRAY
		{
			// シェーダプログラム生成
			shaderProg = es1emu_CreateProgram(VSHADER_COLOR_ARRAY, FSHADER_COLOR_ARRAY);
			if (shaderProg == 0) {
				ret = 0;
				break;
			}

			// AttribLocationのindexを明示的に指定(リンク前にglBindAttribLocation())
			glBindAttribLocation(shaderProg, ATTR_LOC_POS, "a_pos");
			glBindAttribLocation(shaderProg, ATTR_LOC_COLOR, "a_color");

			// シェーダプログラムリンク
			shaderProg = es1emu_LinkShaderProgram(shaderProg);
			if (shaderProg == 0) {
				ret = 0;
				break;
			}

			param->program[ES1EMU_PROGRAM_COLOR_ARRAY].programId = shaderProg;
			param->program[ES1EMU_PROGRAM_COLOR_ARRAY].uPMVMatrix = glGetUniformLocation(shaderProg, "u_pmvMatrix");
		}

		// SHADER_TEXTURE_ARRAY
		{
			// シェーダプログラム生成
			shaderProg = es1emu_CreateProgram(VSHADER_TEXTURE_ARRAY, FSHADER_TEXTURE_ARRAY);
			if (shaderProg == 0) {
				ret = 0;
				break;
			}
			// AttribLocationのindexを明示的に指定(リンク前にglBindAttribLocation())
			glBindAttribLocation(shaderProg, ATTR_LOC_POS, "a_pos");
			glBindAttribLocation(shaderProg, ATTR_LOC_TEXTURE, "a_texture");

			// シェーダプログラムリンク
			shaderProg = es1emu_LinkShaderProgram(shaderProg);
			if (shaderProg == 0) {
				ret = 0;
				break;
			}

			param->program[ES1EMU_PROGRAM_TEXTURE_ARRAY].programId = shaderProg;
			param->program[ES1EMU_PROGRAM_TEXTURE_ARRAY].uPMVMatrix = glGetUniformLocation(shaderProg, "u_pmvMatrix");
			param->program[ES1EMU_PROGRAM_TEXTURE_ARRAY].uTexture0 = glGetUniformLocation(shaderProg, "texture0");
		}
#else
		// SHADER_VERTEX_ARRAY
		{
			// シェーダプログラム生成
			shaderProg = es1emu_CreateProgram(VSHADER_VERTEX_ARRAY, FSHADER_VERTEX_ARRAY);
			if (shaderProg == 0) {
				fprintf(stderr,"es1emu_Init:es1emu_CreateProgram err\n");
				ret = 0;
				break;
			}

			// AttribLocationのindexを明示的に指定(リンク前にglBindAttribLocation())
			glBindAttribLocation(shaderProg, ATTR_LOC_POS, "a_pos");
			glBindAttribLocation(shaderProg, ATTR_LOC_COLOR, "a_color");
			glBindAttribLocation(shaderProg, ATTR_LOC_TEXTURE, "a_texture");

			// シェーダプログラムリンク
			shaderProg = es1emu_LinkShaderProgram(shaderProg);
			if (shaderProg == 0) {
				fprintf(stderr,"es1emu_Init:es1emu_LinkShaderProgram err\n");
				ret = 0;
				break;
			}

			param->program[ES1EMU_PROGRAM_VERTEX_ARRAY].programId = shaderProg;
			param->program[ES1EMU_PROGRAM_VERTEX_ARRAY].uPMVMatrix = glGetUniformLocation(shaderProg, "u_pmvMatrix");
			param->program[ES1EMU_PROGRAM_VERTEX_ARRAY].uColor = glGetUniformLocation(shaderProg, "u_color");
			param->program[ES1EMU_PROGRAM_VERTEX_ARRAY].uTexture0 = glGetUniformLocation(shaderProg, "texture0");
			param->program[ES1EMU_PROGRAM_VERTEX_ARRAY].uColorArrey = glGetUniformLocation(shaderProg, "u_color_arrey");
			param->program[ES1EMU_PROGRAM_VERTEX_ARRAY].uTextureArrey = glGetUniformLocation(shaderProg, "u_texture_arrey");
		}

		es1emu_UseProgram(0);
#endif
	} while (0);

	return (ret);
}

int es1emu_Finish()
{
	GLuint shaderProg;
	int ret = 1;
	ES1PARAMS *param = getParams();

	shaderProg = param->program[ES1EMU_PROGRAM_VERTEX_ARRAY].programId;

	if(shaderProg != 0){
		es1emu_DeleteProgram(shaderProg);
	}

	return (ret);
}

void es1emu_UseProgram(int programType)
{
	ES1PARAMS *param = getParams();

	PROGRAM_INFO* program = &param->program[programType];

	// シェーダプログラムを選択
	glUseProgram(program->programId);
#if 0
	// プロジェクション*モデルビュー
	MPMatrix mat;
	es1emu_MultMatrix(&mat, &param->filo[1].cur, &param->filo[0].cur);

	// マトリクスUniform転送
	glUniformMatrix4fv(program->uPMVMatrix, 1, GL_FALSE, (GLfloat*)&mat);

	// シェーダ毎のパラメータ転送

	switch (programType)
	{
	case ES1EMU_PROGRAM_VERTEX_ARRAY:
		glUniform4fv(program->uColor, 1, (GLfloat*)&param->color);
		glEnableVertexAttribArray(ATTR_LOC_POS);
		break;
	case ES1EMU_PROGRAM_COLOR_ARRAY:
		glEnableVertexAttribArray(ATTR_LOC_POS);
		glEnableVertexAttribArray(ATTR_LOC_COLOR);
		break;
	case ES1EMU_PROGRAM_TEXTURE_ARRAY:
		glEnableVertexAttribArray(ATTR_LOC_POS);
		glEnableVertexAttribArray(ATTR_LOC_TEXTURE);
		glUniform1i(program->uTexture0, 0);
		break;
	default:
		break;
	}
#else
	glUniform1i(program->uTextureArrey, 0);
	glUniform1i(program->uColorArrey, 0);
	glUniform1i(program->uTexture0, 0);
	glEnableVertexAttribArray(ATTR_LOC_POS);
#endif
}

void es1emu_LoadMatrix()
{
	ES1PARAMS *param = getParams();
	PROGRAM_INFO* program = &param->program[0];

	MPMatrix mat;
	es1emu_MultMatrix(&mat, &param->filo[1].cur, &param->filo[0].cur);

	// マトリクスUniform転送
	glUniformMatrix4fv(program->uPMVMatrix, 1, GL_FALSE, (GLfloat*)&mat);

	//glUniform4fv(program->uColor, 1, (GLfloat*)&param->color);
}

void GL_APIENTRY glEnableClientState (GLenum array)
{
	ES1PARAMS* param = getParams();
	PROGRAM_INFO* program = &param->program[0];

	switch (array)
	{
	case GL_COLOR_ARRAY:			/* カラー配列。glColorPointer() を参照 */
		glUniform1i(program->uColorArrey, 1);
		glEnableVertexAttribArray(ATTR_LOC_COLOR);
		break;
	case GL_NORMAL_ARRAY:			/* 法線配列。glNormalPointer() を参照 */
		break;
	case GL_TEXTURE_COORD_ARRAY:	/* テクスチャ座標配列。glTexCoordPointer() を参照 */
		glUniform1i(program->uTextureArrey, 1);
		glEnableVertexAttribArray(ATTR_LOC_TEXTURE);
		break;
	case GL_VERTEX_ARRAY:			/* 頂点配列。glVertexPointer() を参照 */
		glEnableVertexAttribArray(ATTR_LOC_POS);
		break;
	default:
		break;
	}
}
void GL_APIENTRY glDisableClientState (GLenum array)
{
	ES1PARAMS* param = getParams();
	PROGRAM_INFO* program = &param->program[0];

	switch (array)
	{
	case GL_COLOR_ARRAY:			/* カラー配列。glColorPointer() を参照 */
		glUniform1i(program->uColorArrey, 0);
		glDisableVertexAttribArray(ATTR_LOC_COLOR);
		break;
	case GL_NORMAL_ARRAY:			/* 法線配列。glNormalPointer() を参照 */
		break;
	case GL_TEXTURE_COORD_ARRAY:	/* テクスチャ座標配列。glTexCoordPointer() を参照 */
		glUniform1i(program->uTextureArrey, 0);
		glDisableVertexAttribArray(ATTR_LOC_TEXTURE);
		break;
	case GL_VERTEX_ARRAY:			/* 頂点配列。glVertexPointer() を参照 */
		glDisableVertexAttribArray(ATTR_LOC_POS);
		break;
	default:
		break;
	}
}

void GL_APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	ES1PARAMS* param = getParams();
	PROGRAM_INFO* program = &param->program[0];

	param->color.r = red;
	param->color.g = green;
	param->color.b = blue;
	param->color.a = alpha;

	glUniform4fv(program->uColor, 1, (GLfloat*)&param->color);
}

void GL_APIENTRY glPushMatrix(void)
{
	ES1PARAMS	*param = getParams();
	MATRIX_FILO	*filo = &param->filo[param->mode];

	if (filo->sp < STACK_SIZE) {
		memcpy(&filo->mat[(filo->sp)++], &filo->cur, sizeof(MPMatrix));
	}
}

void GL_APIENTRY glPopMatrix(void)
{
	ES1PARAMS	*param = getParams();
	MATRIX_FILO	*filo = &param->filo[param->mode];

	if (filo->sp > 0) {
		memcpy(&filo->cur, &filo->mat[--(filo->sp)], sizeof(MPMatrix));
	}
}

void GL_APIENTRY glMatrixMode(GLenum mode)
{
	ES1PARAMS *param = getParams();

	param->mode = (GL_MODELVIEW^mode);
	param->pMat = &param->filo[param->mode].cur;
}

void GL_APIENTRY glLoadIdentity(void)
{
	es1emu_LoadIdentityMatrix(getParams()->pMat);
}

void GL_APIENTRY glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
	es1emu_OrthoMatrix(getParams()->pMat, left, right, bottom,top, zNear, zFar);
}

void GL_APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	es1emu_RotateMatrix(getParams()->pMat, angle, x, y, z);
}

void GL_APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z)
{
	es1emu_ScaleMatrix(getParams()->pMat, x, y, z);
}

void GL_APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
	es1emu_TranslateMatrix(getParams()->pMat, x, y, z);
}

void GL_APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	glVertexAttribPointer(ATTR_LOC_POS, size, type, GL_FALSE, sizeof(float)*size, pointer);
}

void GL_APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	glVertexAttribPointer(ATTR_LOC_COLOR, size, type, GL_TRUE, sizeof(char)*size, pointer);
}

void GL_APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	glVertexAttribPointer(ATTR_LOC_TEXTURE, size, type, GL_FALSE, sizeof(float)*size, pointer);
}

#endif // _GLES1_EMULATION
