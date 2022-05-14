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
#include <linux/input.h>

#include "glview.h"

// 三角形頂点数
#define MP_TRIANGLE_CNT				3

//------------------------------------------------------------------------------
// 定数
//------------------------------------------------------------------------------
#define GLV_GL_ACCY				1.0E-15
#define GLV_GL_LINE_OFF_SIZE	(4)
#define GLV_GL_DRAW_POINT		(1000)
#define GLV_GL_BUF_SIZE			(GLV_GL_LINE_OFF_SIZE*GLV_GL_DRAW_POINT)

//------------------------------------------------------------------------------
// マクロ
//------------------------------------------------------------------------------
#define GLV_GL_HALF(_data)		(float)((_data)*0.5f)

//------------------------------------------------------------------------------
// 静的変数
//------------------------------------------------------------------------------
// 汎用バッファ
//static GLV_T_POINT_t glv_gPointBuf[GLV_GL_BUF_SIZE];

// =============================================================================
// thread safe buffer 確保処理
// 
// 初期化:
//   void init_thread_safe_buffer(void);
// バッファーアドレス取得:
//  THREAD_SAFE_BUFFER_t *get_thread_safe_buffer(void);
// =============================================================================
typedef struct _thread_safe_buffer{
	// 確保する領域を下記に記述してください
	EGLDisplay		egl_dpy;
	EGLContext		egl_ctx;
	GLV_T_POINT_t	glv_gPointBuf[GLV_GL_BUF_SIZE];
	//
} THREAD_SAFE_BUFFER_t;
#include "glview_thread_safe.h"
// =============================================================================
// =============================================================================

//------------------------------------------------------------------------------
// 関数
//------------------------------------------------------------------------------
/**
 * @brief		初期化:thread作成時に呼び出してください
 */
void glvGl_thread_safe_init(void)
{
	init_thread_safe_buffer();
}

void glvGl_setEglContextInfo(EGLDisplay egl_dpy,EGLContext egl_ctx)
{
	THREAD_SAFE_BUFFER_t *thread_buffer = get_thread_safe_buffer();
	thread_buffer->egl_dpy = egl_dpy;
	thread_buffer->egl_ctx = egl_ctx;
}

EGLContext glvGl_GetEglContext(void)
{
	THREAD_SAFE_BUFFER_t *thread_buffer = get_thread_safe_buffer();
	return(thread_buffer->egl_ctx);
}

EGLDisplay glvGl_GetEglDisplay(void)
{
	THREAD_SAFE_BUFFER_t *thread_buffer = get_thread_safe_buffer();
	return(thread_buffer->egl_dpy);
}

/**
 * @brief		初期化
 */
void glvGl_init(void)
{
#ifndef _GLES1_EMULATION
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_FLAT);
#endif

	glDisable(GL_DITHER);		// ディザを無効化
	glDisable(GL_DEPTH_TEST);

	// 頂点配列の使用を許可
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

    //glClearColor(0.0, 0.0, 0.0, 0.0);
	// 画面クリア
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
 * @brief		平方根(ルート)計算
 * @return		結果
 */
float glvGl_sqrtF(const float x)
{
#if 1
	return (sqrtf(x));
#else
	float xHalf = 0.5f * x;
	int32_t tmp = 0x5F3759DF - ( *(int32_t*)&x >> 1 ); //initial guess
	float xRes  = *(float*)&tmp;

	xRes *= ( 1.5f - ( xHalf * xRes * xRes ) );
	return (xRes * x);
#endif
}

/**
 * @brief		2点から指定した距離にオフセットした4点算出
 * @param[in]	pV0 座標1
 * @param[in]	pV1 座標2
 * @param[in]	dist 幅(片側)
 * @param[out]	pPos 変換座標
 * @return		結果(成功:0, 失敗:-1)
 */
int32_t glvGl_lineOff(const GLV_T_POINT_t *pV0, const GLV_T_POINT_t *pV1, float dist, GLV_T_POINT_t* pPos)
{
	float	xlk;
	float	ylk;
	float	rsq;
	float	rinv;
	float	a;
	float	b;

	xlk = pV1->x - pV0->x;
	ylk = pV1->y - pV0->y;
	rsq = (xlk * xlk) + (ylk * ylk);

	if (rsq < GLV_GL_ACCY) {
		return (-1);
	} else {
		rinv = 1.0f / glvGl_sqrtF(rsq);

		a = -ylk * rinv * dist;
		b =  xlk * rinv * dist;
	}

	pPos[0].x = pV0->x + a;
	pPos[0].y = pV0->y + b;
	pPos[1].x = pV0->x - a;
	pPos[1].y = pV0->y - b;
	pPos[2].x = pV1->x + a;
	pPos[2].y = pV1->y + b;
	pPos[3].x = pV1->x - a;
	pPos[3].y = pV1->y - b;

	return (0);
}

/**
 * @brief		縮退三角形連結
 */
void glvGl_degenerateTriangle(const GLV_T_POINT_t* pPos, int32_t pointCnt, float width, GLV_T_POINT_t* pOutBuf, int32_t* pIndex)
{
	int32_t idx = *pIndex;
	int32_t ret = 0;
	int32_t i = 0;
	//float w = width * 0.5f;

	for (i=0; i<pointCnt-1; i++) {
		if (0 == i && 0 != idx) {
			// 座標の並びが先頭　且つ　データの並びが先頭以外
			ret = glvGl_lineOff(&pPos[i], &pPos[i+1], width, &pOutBuf[idx+1]);
			if (-1 == ret) {
				continue;
			}

			// 先頭と同じ座標を先頭に格納
			pOutBuf[idx] = pOutBuf[idx+1];

			idx += 5;
		} else {
			ret = glvGl_lineOff(&pPos[i], &pPos[i+1], width, &pOutBuf[idx]);
			if (-1 == ret) {
				continue;
			}

			idx += 4;
		}
	}

	if (idx != *pIndex) {
		// リンクの最後の座標を最後尾に設定
		pOutBuf[idx] = pOutBuf[idx-1];
		idx++;

		*pIndex = idx;
	}
}

/**
 * @brief		2点間の距離算出
 * @param[in]	pV0 座標1
 * @param[in]	pV1 座標2
 * @return		距離
 */
float glvGl_distance(const GLV_T_POINT_t *v0, const GLV_T_POINT_t *v1)
{
	float x = 0.0f;
	float y = 0.0f;
	float distance = 0.0f;

	x = v1->x - v0->x;
	y = v1->y - v0->y;

	// 距離rを求める
	distance = glvGl_sqrtF((x*x) + (y*y));

	// 絶対値
	distance = fabs(distance);

	return (distance);
}

/**
 * @brief		2点間の角度算出
 * 				(座標1から座標2への角度)　右0°で反時計回り
 * @param[in]	pV0 座標1
 * @param[in]	pV1 座標2
 * @return		角度
 */
float glvGl_degree(const GLV_T_POINT_t *v0, const GLV_T_POINT_t *v1)
{
	float rad = 0.0f;
	float deg = 0.0f;

	// 2点間から表示角度算出
	rad = atan2f((v1->y - v0->y), (v1->x - v0->x));

	// 0から2π(0度から360度)を求める為、マイナス値補正
	if(rad < 0.0f) {
		rad = rad + (2.0f * M_PI);
	}

	// ラジアンから度に変換
	deg = rad * 180.0f / M_PI;

	return (deg);
}

/**
 * @brief		指定した頂点配列を描画
 * @param[in]	mode 描画モード
 * @param[in]	pPos 頂点座標
 * @param[in]	cnt 頂点座標数
 */
void glvGl_draw(const int32_t mode, const GLV_T_POINT_t* pPos, int32_t cnt)
{
	glVertexPointer(2, GL_FLOAT, 0, pPos);

#ifdef _GLES1_EMULATION
	es1emu_LoadMatrix();
#endif

	glDrawArrays(mode, 0, cnt);
}

/**
 * @brief		指定した頂点配列をLINE_STRIPで描画
 * @param[in]	pPos 頂点座標
 * @param[in]	cnt 頂点座標数
 * @param[in]	width 幅
 */
void glvGl_drawLineStrip(const GLV_T_POINT_t* pPos, int32_t cnt, float width)
{
	glLineWidth(width);
	glvGl_draw(GL_LINE_STRIP, pPos, cnt);
}

/**
 * @brief		線を描画
 * @param[in]	pPos 頂点座標
 * @param[in]	cnt 頂点座標数
 * @param[in]	width 太さ
 */
void glvGl_drawLines(const GLV_T_POINT_t* pPos, int32_t cnt, float width)
{
	THREAD_SAFE_BUFFER_t *thread_buffer = get_thread_safe_buffer();
	int32_t idx = 0;
	int32_t ret = 0;
	int32_t i = 0;
	float w = 0.0f;

	if (GLV_GL_BUF_SIZE < (cnt-1)*GLV_GL_LINE_OFF_SIZE) {
		// 描画しない
		return;
	}

	// 太さの半分
	w = GLV_GL_HALF(width);

	// 点数分ループ
	for (i=0; i<cnt-1; i++) {
		ret = glvGl_lineOff(&pPos[i], &pPos[i+1], w, &thread_buffer->glv_gPointBuf[idx]);
		if (-1 == ret) {
			continue;
		}
		idx += GLV_GL_LINE_OFF_SIZE;
	}

	glvGl_draw(GL_TRIANGLE_STRIP, thread_buffer->glv_gPointBuf, idx);
}

/**
 * @brief		線を描画
 * @param[in]	pPos 頂点座標(ポリゴン化済み)
 * @param[in]	cnt 頂点座標数
 */
void glvGl_drawPolyLine(const GLV_T_POINT_t* pPos, int32_t cnt)
{
	glvGl_draw(GL_TRIANGLE_STRIP, pPos, cnt);
}

/**
 * @brief		破線を描画
 * @param[in]	pPos 頂点座標
 * @param[in]	cnt 頂点座標数
 * @param[in]	width 太さ
 * @param[in]	dot 破線間隔
 */
void glvGl_drawDotLines(const GLV_T_POINT_t* pPos, int32_t cnt, float width, float dot)
{
	THREAD_SAFE_BUFFER_t *thread_buffer = get_thread_safe_buffer();
	GLV_T_POINT_t		vv[2];			// 破線線分
	GLV_T_POINT_t		v0;				// 始点
	GLV_T_POINT_t		v1;				// 終点
	GLV_T_POINT_t		dv;				// 単位ベクトル
	float		len = 0.0f;		// 2点間の距離
	float		vpos = 0.0f;
	float		dLen = 0.0f;
	int		drawF = 1;
	float		chk_len = 0.0f;
	float		w = 0.0f;
	int32_t		index = 0;
	int32_t		i = 0;

	// 太さの半分
	w = width * 0.5f;

	for (i=0; i < cnt-1; i++) {
		v0 = pPos[i];
		v1 = pPos[i + 1];

		dv.x = v1.x - v0.x;
		dv.y = v1.y - v0.y;

		// 2点間の距離
		len = fabs(glvGl_sqrtF((dv.x)*(dv.x) + (dv.y)*(dv.y)));
		if (len < GLV_GL_ACCY) {
			continue;
		}

		// 2点間単位ベクトル計算
		dv.x *= 1.0f/len;
		dv.y *= 1.0f/len;

		vpos = 0.0f;
		dLen = 0.0f;

		while (len > GLV_GL_ACCY) {
			if (chk_len >= dot - GLV_GL_ACCY) {
				drawF = (drawF) ? 0 : 1;
				chk_len = 0.0f;
			}

			if (chk_len + len <= dot) {
				dLen = len - chk_len;
				if (dLen < 0.0f) dLen = len;
				if (drawF) {
					vv[0].x = (dv.x * vpos) + v0.x;
					vv[0].y = (dv.y * vpos) + v0.y;
					vv[1].x = (dv.x * (vpos + dLen)) + v0.x;
					vv[1].y = (dv.y * (vpos + dLen)) + v0.y;

					glvGl_degenerateTriangle(vv, 2, w, thread_buffer->glv_gPointBuf, &index);
				}
				chk_len += dLen;
				len -= dLen;
				break;
			}

			dLen = dot - chk_len;
			if (drawF) {
				vv[0].x = (dv.x * vpos) + v0.x;
				vv[0].y = (dv.y * vpos) + v0.y;
				vv[1].x = (dv.x * (vpos + dLen)) + v0.x;
				vv[1].y = (dv.y * (vpos + dLen)) + v0.y;

				glvGl_degenerateTriangle(vv, 2, w, thread_buffer->glv_gPointBuf, &index);
			}
			chk_len += dLen;
			len  -= dLen;

			vpos += dLen;
		}
	}

	if (index >= MP_TRIANGLE_CNT) {
		glvGl_draw(GL_TRIANGLE_STRIP, thread_buffer->glv_gPointBuf, index);
	}
}

/**
 * @brief		円を描画
 * @param[in]	center 中心位置
 * @param[in]	radius 半径
 * @param[in]	divCnt 分割数
 */
void glvGl_drawCircle(const GLV_T_POINT_t* pCenter, float radius, int32_t divCnt, float width)
{
	GLV_T_POINT_t buf[360+2];
	float		dPos = 0.0f;
	float		dd   = (M_PI * 2.0f) / (float)divCnt;
	GLV_T_POINT_t		v0;
	int32_t		i = 0;
	float		w = 0.0f;

	// 太さの半分
	w = width * 0.5f;

	for (i=0; i <= divCnt; i++) {
		v0.x = cos(dPos) * radius;
		v0.y = sin(dPos) * radius;
		buf[i].x = v0.x + pCenter->x;
		buf[i].y = v0.y + pCenter->y;
		dPos += dd;
	}
	glvGl_drawLines(buf, divCnt+1, w);
}

/**
 * @brief		円(塗りつぶし)を描画
 * @param[in]	center 中心位置
 * @param[in]	radius 半径
 * @param[in]	divCnt 分割数
 */
void glvGl_drawCircleFill(const GLV_T_POINT_t* pCenter, float radius, int32_t divCnt)
{
	GLV_T_POINT_t buf[360+2];
	float dPos = 0.0f;
	int32_t i = 0;
	float dd = (M_PI * 2.0f) / (float)divCnt;

	buf[0].x = pCenter->x;
	buf[0].y = pCenter->y;

	for (i=0; i<=divCnt; i++) {
		buf[i+1].x = pCenter->x + cos(dPos) * radius;
		buf[i+1].y = pCenter->y + sin(dPos) * radius;
		dPos += dd;
	}

	glvGl_draw(GL_TRIANGLE_FAN, buf, divCnt+2);
}

/**
 * @brief		円(塗りつぶし)を描画 バッファリング
 * @param[in]	center 中心位置
 * @param[in]	radius 半径
 */
void glvGl_drawCircleFillEx(const GLV_T_POINT_t* pCenter, float radius)
{
	static GLV_T_POINT_t g_point[16];
	static int gPolycircle = 0;

	GLV_T_POINT_t point[16];
	float dPos = 0.0f;
	int32_t divCnt = 8;
	int32_t i = 0;

	if (gPolycircle == 0) {
		float dd = (M_PI * 2.0f) / (float)divCnt;
		for (i=0; i<=divCnt; i++) {
			g_point[i].x = cos(dPos);
			g_point[i].y = sin(dPos);
			dPos += dd;
		}
		gPolycircle = 1;
	}

	point[0].x = pCenter[0].x;
	point[0].y = pCenter[0].y;

	for (i=0; i<=divCnt; i++) {
		point[i+1].x = point[0].x + (g_point[i].x * radius);
		point[i+1].y = point[0].y + (g_point[i].y * radius);
	}

	glvGl_draw(GL_TRIANGLE_FAN, point, divCnt+2);
}

/**
 * @brief		四角描画
 * @param[in]	x X座標
 * @param[in]	y Y座標
 * @param[in]	width 幅
 * @param[in]	height 高さ
 */
void glvGl_drawRectangle(float x, float y, float width, float height)
{
	GLV_T_POINT_t squares[4];

	squares[0].x = x;
	squares[0].y = y;
	squares[1].x = x;
	squares[1].y = y + height;
	squares[2].x = x + width;
	squares[2].y = y;
	squares[3].x = x + width;
	squares[3].y = y + height;

	glvGl_draw(GL_TRIANGLE_STRIP, squares, 4);
}

/**
 * @brief		2点から指定した距離にオフセットした4点算出(片側へシフト)
 * @param[in]	pV0 座標1
 * @param[in]	pV1 座標2
 * @param[in]	dist 幅(片側)
 * @param[in]	shift シフト量(-:左/+:右)
 * @param[out]	pPos 変換座標
 * @return		結果(成功:0, 失敗:-1)
 */
int32_t glvGl_lineOffShift(const GLV_T_POINT_t *pV0, const GLV_T_POINT_t *pV1, float dist, float shift, GLV_T_POINT_t* pPos)
{
	float	xlk;
	float	ylk;
	float	rsq;
	float	rinv;
	float	a;
	float	b;
	float	aSft;
	float	bSft;

	xlk = pV1->x - pV0->x;
	ylk = pV1->y - pV0->y;
	rsq = (xlk * xlk) + (ylk * ylk);

	if (rsq < GLV_GL_ACCY) {
		return (-1);
	} else {
		rinv = 1.0f / glvGl_sqrtF(rsq);

		a = -ylk * rinv * dist;
		b =  xlk * rinv * dist;
		aSft = -ylk * rinv * (shift);
		bSft =  xlk * rinv * (shift);
	}

	pPos[0].x = pV0->x + aSft + a;
	pPos[0].y = pV0->y + bSft + b;
	pPos[1].x = pV0->x + aSft - a;
	pPos[1].y = pV0->y + bSft - b;
	pPos[2].x = pV1->x + aSft + a;
	pPos[2].y = pV1->y + bSft + b;
	pPos[3].x = pV1->x + aSft - a;
	pPos[3].y = pV1->y + bSft - b;

	return (0);
}

/**
 * @brief		縮退三角形連結(シフト)
 */
void glvGl_degenerateTriangleShift(const GLV_T_POINT_t* pPos, int32_t pointCnt, float width, float shift, uint8_t dir, int arrow, GLV_T_POINT_t* pOutBuf, int32_t* pIndex)
{
	int32_t idx = *pIndex;
	int32_t ret = 0;
	int32_t i = 0;

	if (0 == dir) {
		// 順方向左にシフト
		for (i=0; i<pointCnt-1; i++) {
			if (0 == i && 0 != idx) {
				// 座標の並びが先頭　且つ　データの並びが先頭以外
				ret = glvGl_lineOffShift(&pPos[i], &pPos[i+1], width, -shift, &pOutBuf[idx+1]);
				if (-1 == ret) {
					continue;
				}
				// 先頭と同じ座標を先頭に格納
				pOutBuf[idx] = pOutBuf[idx+1];
				idx += 5;
			} else {
				ret = glvGl_lineOffShift(&pPos[i], &pPos[i+1], width, -shift, &pOutBuf[idx]);
				if (-1 == ret) {
					continue;
				}
				idx += 4;
			}
		}
	} else {
		// 逆方向左にシフト
		for (i=pointCnt-1; i>0; i--) {
			if ((pointCnt-1) == i && 0 != idx) {
				// 座標の並びが先頭　且つ　データの並びが先頭以外
				ret = glvGl_lineOffShift(&pPos[i], &pPos[i-1], width, -shift, &pOutBuf[idx+1]);
				if (-1 == ret) {
					continue;
				}
				// 先頭と同じ座標を先頭に格納
				pOutBuf[idx] = pOutBuf[idx+1];
				idx += 5;
			} else {
				ret = glvGl_lineOffShift(&pPos[i], &pPos[i-1], width, -shift, &pOutBuf[idx]);
				if (-1 == ret) {
					continue;
				}
				idx += 4;
			}
		}
	}

	if (idx != *pIndex) {
		// 矢印追加
		if (arrow) {
			idx += glvGl_addHalfArrow(&pOutBuf[idx-3], &pOutBuf[idx-1], shift*3, shift, &pOutBuf[idx]);
		}

		// リンクの最後の座標を最後尾に設定
		pOutBuf[idx] = pOutBuf[idx-1];
		idx++;

		*pIndex = idx;
	}
}

/**
 * @brief		指定した頂点配列を描画(色配列指定)
 * @param[in]	mode 描画モード
 * @param[in]	pPos 頂点座標
 * @param[in]	pColor 頂点に対応した色
 * @param[in]	cnt 頂点座標数
 */
void glvGl_drawColor(const int32_t mode, const GLV_T_POINT_t* pPos, const GLV_T_Color_t* pColor, int32_t cnt)
{
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, pPos);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, pColor);

#ifdef _GLES1_EMULATION
	es1emu_LoadMatrix();
#endif

	glDrawArrays(mode, 0, cnt);

	glDisableClientState(GL_COLOR_ARRAY);
}

/**
 * @brief		線を描画(色配列指定)
 * @param[in]	pPos 頂点座標(ポリゴン化済み)
 * @param[in]	pColor 頂点に対応した色
 * @param[in]	cnt 頂点座標数
 */
void glvGl_drawPolyLineColor(const GLV_T_POINT_t* pPos, GLV_T_Color_t* pColor, int32_t cnt)
{
	glvGl_drawColor(GL_TRIANGLE_STRIP, pPos, pColor, cnt);
}

/**
 * @brief		半矢印座標追加
 * 				与えられた2点から半矢印座標を生成
 * @param[in]	pPos1 始点座標
 * @param[in]	pPos2 終点座標
 * @param[in]	length 矢印の長さ
 * @param[in]	width 傘の幅
 * @return		生成に成功した場合2
 */

int32_t glvGl_addHalfArrow(const GLV_T_POINT_t* pPos1, const GLV_T_POINT_t* pPos2, float length, float width, GLV_T_POINT_t* pOut)
{
	float rad;
	float sn;
	float cs;

	// 2点間の距離チェック
	if (length > glvGl_distance(pPos1, pPos2)/3.0f) {
		return (0);
	}

	rad = atan2f(pPos2->y-pPos1->y, pPos2->x-pPos1->x);
	sn = sin(rad);
	cs = cos(rad);

	pOut[0].x = pPos2->x - (length*cs);
	pOut[0].y = pPos2->y - (length*sn);
	pOut[1].x = pOut[0].x + (width*sn);
	pOut[1].y = pOut[0].y - (width*cs);

	return (2);
}

/**
 * @brief		VBO設定
 * @param[io]	pVbo VBO情報2
 * @param[in]	pPoint 頂点配列
 * @param[in]	pColor カラー配列
 * @return		結果(成功:1, 失敗:0)
 */
int glvGl_SetVBO(GLV_T_VBO_INFO_t *pVbo, const GLV_T_POINT_t *pPoint, const GLV_T_Color_t *pColor)
{
	int32_t pointSize = sizeof(GLV_T_POINT_t) * pVbo->pointCnt;
	int32_t colorSize = sizeof(GLV_T_Color_t) * pVbo->pointCnt;

	pVbo->type = GL_TRIANGLES;

	glGenBuffers(1, &pVbo->vboID);
	glBindBuffer(GL_ARRAY_BUFFER, pVbo->vboID);

	glBufferData(GL_ARRAY_BUFFER, (pointSize + colorSize), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0,         pointSize, pPoint);	// 頂点
	glBufferSubData(GL_ARRAY_BUFFER, pointSize, colorSize, pColor);	// 色

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return (1);
}

/**
 * @brief		VBO削除
 * @param[io]	pVbo VBO情報2
 * @return		結果(成功:1, 失敗:0)
 */
int glvGl_DeleteVBO(GLV_T_VBO_INFO_t *pVbo)
{
	glDeleteBuffers(1, &pVbo->vboID);
	pVbo->vboID = 0;
	pVbo->pointCnt = 0;
	pVbo->type = 0;

	return (1);
}

/**
 * @brief		VBO描画
 * @param[in]	pVbo VBO情報
 * @return		結果(成功:1, 失敗:0)
 */
int glvGl_DrawVBO(const GLV_T_VBO_INFO_t *pVbo)
{
	if (0 == pVbo->vboID){
		return (0);
	}

	glEnableClientState(GL_COLOR_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, pVbo->vboID);

	glVertexPointer(2, GL_FLOAT, 0, 0);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, (void*)(sizeof(GLV_T_POINT_t) * pVbo->pointCnt));

#ifdef _GLES1_EMULATION
	es1emu_LoadMatrix();
#endif

	glDrawArrays(pVbo->type, 0, pVbo->pointCnt);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDisableClientState(GL_COLOR_ARRAY);

	return (1);
}

/**
 * @brief		テクスチャ設定
 */
uint32_t glvGl_GenTextures(const uint8_t* pByteArray, int32_t width, int32_t height)
{
	GLuint textureID;

	if(NULL == pByteArray) {
		return (0);
	}

	glGenTextures(1, &textureID);

#ifdef _GLES1_EMULATION
	glActiveTexture(GL_TEXTURE0);
#endif

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#ifdef _GLES1_EMULATION
#else
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLint)width, (GLint)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)pByteArray);

	return ((uint32_t)textureID);
}

/**
 * @brief		テクスチャ解放
 */
void glvGl_DeleteTextures(uint32_t *pTextureID)
{
	glDeleteTextures(1, (GLuint*)pTextureID);
	*pTextureID = 0;
}

/**
 * @brief		テクスチャ描画
 */
void glvGl_DrawTextures(uint32_t pTextureID, const GLV_T_POINT_t *pSquares)
{
	static GLV_T_POINT_t textureCoords[4] = {
		{0.0f, 0.0f},
		{1.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 1.0f}
	};

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, pTextureID);

	glVertexPointer(2, GL_FLOAT, 0, pSquares);
	glTexCoordPointer(2, GL_FLOAT, 0, textureCoords);

#ifdef _GLES1_EMULATION
	es1emu_LoadMatrix();
#endif

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_TEXTURE_2D);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void glvGl_DrawTexturesEx(uint32_t textureId,float x, float y, float width, float height, float spot_x, float spot_y, float rotation)
{
	GLV_T_POINT_t squares[4] = {
		{0,		0},
		{width,	0},
		{0,		height},
		{width,	height}
	};

	glvGl_PushMatrix();

	// 指定座標に移動
	glvGl_Translatef(x, y, 0.0);
	glvGl_Rotatef(rotation, 0.0, 0.0, 1.0);

	// スポットにオフセット
	if(0 != spot_x || 0 != spot_y) {
		glvGl_Translatef(-spot_x, -spot_y, 0.0);
	}

	glvGl_DrawTextures(textureId, squares);

	glvGl_PopMatrix();
}

/**
 * @brief		LoadIdentity
 */
void glvGl_LoadIdentity()
{
	glLoadIdentity();
}

/**
 * @brief		Viewport設定
 */
void glvGl_Viewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
	glViewport(x, y, width, height);
}

/**
 * @brief		ClearColor
 */
void glvGl_ClearColor(float r, float g, float b, float a)
{
	glClearColor(r,g,b,a);
}

/**
 * @brief		glClear
 */
void glvGl_Clear(uint32_t mask)
{
	glClear(mask);
}

/**
 * @brief		色設定
 */
void glvGl_Color4f(float r, float g, float b, float a)
{
	glColor4f(r, g, b, a);
}

/**
 * @brief		色設定
 * @param[in]	rgba rgba
 */
void glvGl_ColorRGBA(GLV_RGBACOLOR rgba)
{
	glColor4f(GLV_GET_FR(rgba), GLV_GET_FG(rgba), GLV_GET_FB(rgba), GLV_GET_FA(rgba));
}

/**
 * @brief		色設定(透過色置き換え)
 * @param[in]	rgba rgba
 */
void glvGl_ColorRGBATrans(GLV_RGBACOLOR rgba, float a)
{
	glColor4f(GLV_GET_FR(rgba), GLV_GET_FG(rgba), GLV_GET_FB(rgba), a);
}

/**
 * @brief		BLEND開始
 */
void glvGl_BeginBlend()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/**
 * @brief		BLEND終了
 */
void glvGl_EndBlend()
{
	glDisable(GL_BLEND);
}

/**
 * @brief		PushMatrix
 */
void glvGl_PushMatrix()
{
	glPushMatrix();
}

/**
 * @brief		PopMatrix
 */
void glvGl_PopMatrix()
{
	glPopMatrix();
}

/**
 * @brief		Rotatef
 */
void glvGl_Rotatef(float angle, float x, float y, float z)
{
	glRotatef(angle, x, y, z);
}

/**
 * @brief		Translatef
 */
void glvGl_Translatef(float x, float y, float z)
{
	glTranslatef(x, y, z);
}

/**
 * @brief		Scalef
 */
void glvGl_Scalef(float x, float y, float z)
{
	glScalef(x, y, z);
}

/**
 * @brief		PROJECTIONモード
 */
void glvGl_MatrixProjection()
{
	// プロジェクション行列の設定
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
}

/**
 * @brief		MODELVIEWモード
 */
void glvGl_MatrixModelView()
{
	// モデルビュー行列の設定
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/**
 * @brief		Flush
 */
void glvGl_Flush()
{
	glFlush();
}

/**
 * @brief		Orthof
 */
void glvGl_Orthof(float left, float right, float bottom, float top, float zNear, float zFar)
{
#ifdef _GLES1_EMULATION
	glOrthof(left, right, bottom, top, zNear, zFar);
#else
	glOrtho(left, right, bottom, top, zNear, zFar);
#endif
}

void glvGl_GL_Init(void)
{
#ifndef _GLES1_EMULATION
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glDisable(GL_LIGHTING);
	glShadeModel(GL_FLAT);
#endif

	glDisable(GL_DITHER);		// ディザを無効化
	glDisable(GL_DEPTH_TEST);

	// 頂点配列の使用を許可
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

    //glClearColor(0.0, 0.0, 0.0, 0.0);
	// 画面クリア
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
