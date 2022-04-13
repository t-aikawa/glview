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

#include <stdio.h>
#include <GLES2/gl2.h>
//#include <GLES2/gl2ext.h>
#include "es1emu_emulation.h"
#include "es1emu_shader.h"

#ifdef _GLES1_EMULATION

//------------------------------------------------------------------------------
// 内部関数プロトタイプ
//------------------------------------------------------------------------------
/**
 * @brief		シェーダプログラムコンパイル
 * @param[in]	type GL_VERTEX_SHADER:頂点シェーダ / GL_FRAGMENT_SHADER:フラグメントシェーダ
 * @param[in]	pProgram プログラム文字列
 * @return		コンパイル結果生成されるシェーダオブジェクト / 失敗時は0を返却
 */
static GLuint MP_CompileShaderProgram(GLint type, const char *pProgram);

//------------------------------------------------------------------------------
// 関数
//------------------------------------------------------------------------------
static GLuint MP_CompileShaderProgram(GLint type, const char *pProgram)
{
	GLuint shaderId;
	GLint shaderCompiled;
	const GLchar* sources[2] = {
			(const GLchar*)GLSL_VERSION,
			(const GLchar*)pProgram
		};

	// シェーダオブジェクトの作成
	shaderId = glCreateShader((GLenum)type);

	// ソースコードをシェーダオブジェクトに変換
	glShaderSource(shaderId, 2, (const char**)sources, NULL);

	// コンパイル
	glCompileShader(shaderId);

	// コンパイルエラーチェック
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &shaderCompiled);
	if (!shaderCompiled) {
		fprintf(stderr,"MP_CompileShaderProgram:MP_CompileShaderProgram err\n");
		return (0);
	}

	return (shaderId);
}

GLuint es1emu_CreateProgram(const char* pVShader, const char* pFShader)
{
	GLuint vshaderId;
	GLuint fshaderId;

	// 頂点シェーダコンパイル
	vshaderId = MP_CompileShaderProgram(GL_VERTEX_SHADER, pVShader);
	if (vshaderId == 0) {
		fprintf(stderr,"es1emu_CreateProgram:MP_CompileShaderProgram err\n");
		return (0);
	}

	// フラグメントシェーダコンパイル
	fshaderId = MP_CompileShaderProgram(GL_FRAGMENT_SHADER, pFShader);
	if (fshaderId == 0) {
		fprintf(stderr,"es1emu_CreateProgram:MP_CompileShaderProgram err\n");
		return (0);
	}

	// シェーダプログラム作成
	GLuint programId = glCreateProgram();

	// シェーダプログラムにシェーダオブジェクト登録
	glAttachShader(programId, fshaderId);
	glAttachShader(programId, vshaderId);

	// シェーダオブジェクトは不要な為削除
	glDeleteShader(vshaderId);
	glDeleteShader(fshaderId);

	return (programId);
}

GLuint es1emu_LinkShaderProgram(const GLuint programId)
{
	GLint bLinked;

	// プログラムをリンク
	glLinkProgram(programId);

	// リンクエラー判定
	glGetProgramiv(programId, GL_LINK_STATUS, &bLinked);
	if (!bLinked) {
		fprintf(stderr,"es1emu_LinkShaderProgram:es1emu_LinkShaderProgram err\n");
		return (0);
	}

	return (programId);
}

void es1emu_DeleteProgram(const GLuint programId)
{
	glUseProgram(0);
	glDeleteProgram(programId);
}

#endif // _GLES1_EMULATION
