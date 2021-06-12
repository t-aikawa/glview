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

#ifndef _ES1EMU_MATRIX_H
#define _ES1EMU_MATRIX_H

//------------------------------------------------------------------------------
// 構造体
//------------------------------------------------------------------------------
/**
 * @brief  マトリクス
 */
typedef struct {
	float   m[4][4];
} MPMatrix;

//------------------------------------------------------------------------------
// プロトタイプ宣言
//------------------------------------------------------------------------------
/**
 * @brief		単位行列をロード
 * @param[out]	mat マトリクス
 */
void es1emu_LoadIdentityMatrix(MPMatrix *mat);

/**
 * @brief		行列をコピー
 * @param[out]	dst コピー先マトリクス
 * @param[in]	src コピー元マトリクス
 */
void es1emu_CopyMatrix(MPMatrix *dst, const MPMatrix *src);

/**
 * @brief		現在の行列と指定した行列との積を算出
 * @param[out]	dst src1とsrc2の積
 * @param[in]	src1 現在マトリクス
 * @param[in]	src2 指定マトリクス
 */
void es1emu_MultMatrix(MPMatrix *dst, const MPMatrix *src1, const MPMatrix *src2);

/**
 * @brief		現在の行列を回転した行列を算出
 * @param[io]	mat 現在のマトリクス→回転後のマトリクス
 * @param[in]	angle 角度
 * @param[in]	x X方向
 * @param[in]	y Y方向
 * @param[in]	z Z方向
 */
void es1emu_RotateMatrix(MPMatrix *mat, const float angle, const float x, const float y, const float z);

/**
 * @brief		現在の行列を平行移動した行列を算出
 * @param[io]	mat 現在のマトリクス→平行移動後のマトリクス
 * @param[in]	x X方向
 * @param[in]	y Y方向
 * @param[in]	z Z方向
 */
void es1emu_TranslateMatrix(MPMatrix *mat, const float x, const float y, const float z);

/**
 * @brief		現在の行列を拡大縮小した行列を算出
 * @param[io]	mat 現在のマトリクス→拡大縮小後のマトリクス
 * @param[in]	x X方向拡大率
 * @param[in]	y Y方向拡大率
 * @param[in]	z Z方向拡大率
 */
void es1emu_ScaleMatrix(MPMatrix *mat, const float x, const float y, const float z);

/**
 * @brief		平行投影変換行列を算出
 * @param[io]	mat 現在のマトリクス→平行投影変換後のマトリクス
 * @param[in]	left Xの左端
 * @param[in]	right Xの右端
 * @param[in]	bottom Yの下端
 * @param[in]	top Yの上端
 * @param[in]	zNear Zの手前
 * @param[in]	zFar Zの奥
 */
void es1emu_OrthoMatrix(MPMatrix *mat, const float left, const float right, const float bottom, const float top, const float zNear, const float zFar);


#endif	// _ES1EMU_MATRIX_H
