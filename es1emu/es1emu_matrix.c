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

#include <math.h>
#include <string.h>
#include <GLES2/gl2.h>
//#include <GLES2/gl2ext.h>
#include "es1emu_emulation.h"
#include "es1emu_matrix.h"

#define ES1EMU_ANGLE_180			180
// 角度 → ラジアンへ変換
#define ES1EMU_DEG_TO_RAD(_deg)			(float)((_deg) * M_PI / (float)ES1EMU_ANGLE_180)

// OpenGLの行列の並び(○の数字が順番)
//　　行→→→
//　　 0 1 2 3
//列 0①⑤⑨⑬
//↓ 1②⑥⑩⑭
//↓ 2③⑦⑪⑮
//↓ 3④⑧⑫⑯

//------------------------------------------------------------------------------
// 関数
//------------------------------------------------------------------------------
void es1emu_LoadIdentityMatrix(MPMatrix *mat)
{
	memset(mat, 0x00, sizeof(MPMatrix));
	mat->m[0][0] = 1.0f;
	mat->m[1][1] = 1.0f;
	mat->m[2][2] = 1.0f;
	mat->m[3][3] = 1.0f;
}

void es1emu_CopyMatrix(MPMatrix *dst, const MPMatrix *src)
{
	memcpy(dst, src, sizeof(MPMatrix));
}

void es1emu_MultMatrix(MPMatrix *dst, const MPMatrix *src1, const MPMatrix *src2)
{
	GLint x;
	GLint y;
	MPMatrix tmp;

	for (x=0; x<4; x++) {
		for (y=0; y<4; y++) {
			tmp.m[x][y] = (src1->m[0][y] * src2->m[x][0]) +
						  (src1->m[1][y] * src2->m[x][1]) +
						  (src1->m[2][y] * src2->m[x][2]) +
						  (src1->m[3][y] * src2->m[x][3]);
		}
	}

	memcpy(dst, &tmp, sizeof(MPMatrix));
}

void es1emu_RotateMatrix(MPMatrix *mat, const float angle, const float x, const float y, const float z)
{
	MPMatrix rotate;
	float rad = ES1EMU_DEG_TO_RAD(angle);
	float c = cosf(rad);
	float s = sinf(rad);

	es1emu_LoadIdentityMatrix(&rotate);

	// X軸回転
	if (x>0.0f) {
		rotate.m[1][1] = c;
		rotate.m[1][2] = s;
		rotate.m[2][1] = -s;
		rotate.m[2][2] = c;
	}
	// Y軸回転
	if (y>0.0f) {
		rotate.m[0][0] = c;
		rotate.m[0][2] = -s;
		rotate.m[2][0] = s;
		rotate.m[2][2] = c;
	}
	// Z軸回転
	if (z>0.0f) {
		rotate.m[0][0] = c;
		rotate.m[0][1] = s;
		rotate.m[1][0] = -s;
		rotate.m[1][1] = c;
	}

	es1emu_MultMatrix(mat, mat, &rotate);
}

void es1emu_TranslateMatrix(MPMatrix *mat, const float x, const float y, const float z)
{
	MPMatrix translate;

	es1emu_LoadIdentityMatrix(&translate);
	translate.m[3][0] = x;
	translate.m[3][1] = y;
	translate.m[3][2] = z;

	es1emu_MultMatrix(mat, mat, &translate);
}

void es1emu_ScaleMatrix(MPMatrix *mat, const float x, const float y, const float z)
{
	MPMatrix scale;

	es1emu_LoadIdentityMatrix(&scale);
	scale.m[0][0] = x;
	scale.m[1][1] = y;
	scale.m[2][2] = z;

	es1emu_MultMatrix(mat, mat, &scale);
}

void es1emu_OrthoMatrix(MPMatrix *mat, const float left, const float right, const float bottom, const float top, const float zNear, const float zFar)
{
	MPMatrix ortho;
	float dx = right - left;
	float dy = top - bottom;
	float dz = zFar - zNear;

	es1emu_LoadIdentityMatrix(&ortho);
	ortho.m[0][0] =  2.0f / dx;
	ortho.m[1][1] =  2.0f / dy;
	ortho.m[2][2] =- 2.0f / dz;
	ortho.m[3][0] =- (right + left) / dx;
	ortho.m[3][1] =- (top + bottom) / dy;
	ortho.m[3][2] =- (zFar + zNear) / dz;

	es1emu_MultMatrix(mat, mat, &ortho);
}
