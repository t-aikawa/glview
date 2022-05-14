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

#ifndef _GLVIEW_GL_H
#define _GLVIEW_GL_H

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// 構造体
//------------------------------------------------------------------------------
// 座標
typedef struct glv_POINT {
	float	x;	// X座標
	float	y;	// Y座標
} GLV_T_POINT_t;

// rgbaカラー
typedef struct glv_Color {
	uint8_t	r;
	uint8_t	g;
	uint8_t	b;
	uint8_t	a;
} GLV_T_Color_t;

// VBO情報
typedef struct glv_VBO_INFO {
	uint32_t	vboID;			// VBO ID
	uint32_t	type;			// 頂点タイプ(GL_TRIANGLES, GL_TRIANGLE_STRIP)
	int32_t	pointCnt;		// 頂点座標数
} GLV_T_VBO_INFO_t;

// カラー
typedef uint32_t				GLV_RGBACOLOR;	// カラーRGBA値

#define GLV_SET_RGBA(r,g,b,a)	(GLV_RGBACOLOR)((uint8_t)(b)|((uint16_t)((uint8_t)(g))<<8)|(((uint32_t)(uint8_t)(r))<<16)|(((uint32_t)(uint8_t)(a))<<24))
#define GLV_GET_B(rgba)			((uint8_t)(rgba))
#define GLV_GET_G(rgba)			((uint8_t)((rgba)>> 8))
#define GLV_GET_R(rgba)			((uint8_t)((rgba)>>16))
#define GLV_GET_A(rgba)			((uint8_t)((rgba)>>24))

#define GLV_GET_FR(rgba)		((float)((float)GLV_GET_R(rgba)/255.0f))
#define GLV_GET_FG(rgba)		((float)((float)GLV_GET_G(rgba)/255.0f))
#define GLV_GET_FB(rgba)		((float)((float)GLV_GET_B(rgba)/255.0f))
#define GLV_GET_FA(rgba)		((float)((float)GLV_GET_A(rgba)/255.0f))

#define GLV_RGBA_NON			GLV_SET_RGBA(  0,  0,  0,  0)
#define GLV_RGBA_BLACK			GLV_SET_RGBA(  0,  0,  0,255)
#define GLV_RGBA_WHITE			GLV_SET_RGBA(255,255,255,255)
#define GLV_RGBA_RED			GLV_SET_RGBA(255,  0,  0,255)
#define GLV_RGBA_GREEN			GLV_SET_RGBA(  0,255,  0,255)
#define GLV_RGBA_BLUE			GLV_SET_RGBA(  0,  0,255,255)
#define GLV_RGBA_YELLOW		    GLV_SET_RGBA(255,255,  0,255)
//------------------------------------------------------------------------------
// マクロ
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// プロトタイプ宣言
//------------------------------------------------------------------------------
float glvGl_sqrtF(const float x);
int32_t glvGl_lineOff(const GLV_T_POINT_t *pV0, const GLV_T_POINT_t *pV1, float dist, GLV_T_POINT_t* pPos);
void glvGl_degenerateTriangle(const GLV_T_POINT_t* pPoint, int32_t pointCnt, float width, GLV_T_POINT_t* pOutBuf, int32_t* pIndex);
float glvGl_distance(const GLV_T_POINT_t *v0, const GLV_T_POINT_t *v1);
float glvGl_degree(const GLV_T_POINT_t *v0, const GLV_T_POINT_t *v1);

void glvGl_draw(const int32_t mode, const GLV_T_POINT_t* pPos, int32_t cnt);
void glvGl_drawLineStrip(const GLV_T_POINT_t* pPos, int32_t cnt, float width);
void glvGl_drawLines(const GLV_T_POINT_t* pPos, int32_t cnt, float width);
void glvGl_drawPolyLine(const GLV_T_POINT_t* pPos, int32_t cnt);
void glvGl_drawDotLines(const GLV_T_POINT_t* pPos, int32_t cnt, float width, float dot);
void glvGl_drawCircle(const GLV_T_POINT_t* pCenter, float radius, int32_t divCnt, float width);
void glvGl_drawCircleFill(const GLV_T_POINT_t* pCenter, float radius, int32_t divCnt);
void glvGl_drawCircleFillEx(const GLV_T_POINT_t* pCenter, float radius);
void glvGl_drawRectangle(float x, float y, float width, float height);

int32_t glvGl_lineOffShift(const GLV_T_POINT_t *pV0, const GLV_T_POINT_t *pV1, float dist, float shift, GLV_T_POINT_t* pPos);
void glvGl_degenerateTriangleShift(const GLV_T_POINT_t* pPos, int32_t pointCnt, float width, float shift, uint8_t dir, int arrow, GLV_T_POINT_t* pOutBuf, int32_t* pIndex);
void glvGl_drawColor(const int32_t mode, const GLV_T_POINT_t* pPos, const GLV_T_Color_t* pColor, int32_t cnt);
//void glvGl_drawLinesShift(const GLV_T_POINT_t* pPos, int32_t cnt, float width, float shift, int16_t dir);
void glvGl_drawPolyLineColor(const GLV_T_POINT_t* pPos, GLV_T_Color_t* pColor, int32_t cnt);
int32_t glvGl_addHalfArrow(const GLV_T_POINT_t* pPos1, const GLV_T_POINT_t* pPos2, float length, float width, GLV_T_POINT_t* pOut);

int glvGl_SetVBO(GLV_T_VBO_INFO_t *pVbo, const GLV_T_POINT_t *pPoint, const GLV_T_Color_t *pColor);
int glvGl_DeleteVBO(GLV_T_VBO_INFO_t *pVbo);
int glvGl_DrawVBO(const GLV_T_VBO_INFO_t *pVbo);

uint32_t glvGl_GenTextures(const uint8_t* pByteArray, int32_t width, int32_t height);
void glvGl_DeleteTextures(uint32_t *pTextureID);
void glvGl_DrawTextures(uint32_t textureId, const GLV_T_POINT_t *pSquares);
void glvGl_DrawTexturesEx(uint32_t textureId,float x, float y, float width, float height, float spot_x, float spot_y, float rotation);

void glvGl_thread_safe_init(void);

void glvGl_init(void);
void glvGl_ColorRGBA(GLV_RGBACOLOR rgba);
void glvGl_ColorRGBATrans(GLV_RGBACOLOR rgba, float a);
void glvGl_BeginBlend();
void glvGl_EndBlend();
void glvGl_MatrixProjection();
void glvGl_MatrixModelView();

void glvGl_LoadIdentity();
void glvGl_Viewport(int32_t x, int32_t y, int32_t width, int32_t height);
void glvGl_ClearColor(float r, float g, float b, float a);
void glvGl_Clear(uint32_t mask);
void glvGl_Flush();
void glvGl_Color4f(float r, float g, float b, float a);
void glvGl_PushMatrix();
void glvGl_PopMatrix();
void glvGl_Orthof(float left, float right, float bottom, float top, float zNear, float zFar);
void glvGl_Rotatef(float angle, float x, float y, float z);
void glvGl_Scalef(float x, float y, float z);
void glvGl_Translatef(float x, float y, float z);

void glvGl_setEglContextInfo(EGLDisplay egl_dpy,EGLContext egl_ctx);
EGLContext glvGl_GetEglContext(void);
EGLDisplay glvGl_GetEglDisplay(void);

#ifdef __cplusplus
}
#endif

#endif	// _GLVIEW_GL_H
