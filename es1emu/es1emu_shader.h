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

#ifndef _ES1EMU_SHADER_H
#define _ES1EMU_SHADER_H

#ifdef _GLES1_EMULATION

//------------------------------------------------------------------------------
// 定数
//------------------------------------------------------------------------------
#define GLSL_VERSION	"#version 100\n"	// OpenGL ES 2.0 GLSL 1.0

//------------------------------------------------------------------------------
// プロトタイプ宣言
//------------------------------------------------------------------------------
/**
 * @brief		シェーダプログラム生成
 * @param[in]	pVShader 頂点シェーダプログラム
 * @param[in]	pFShader フラグメントシェーダプログラム
 * @return		プログラムID / 失敗時は0を返却
 */
GLuint es1emu_CreateProgram(const char* pVShader, const char* pFShader);

/**
 * @brief		シェーダプログラムリンク
 * @param[in]	programId プログラムID
 * @return		シェーダオブジェクト / 失敗時は0を返却
 */
GLuint es1emu_LinkShaderProgram(const GLuint programId);

/**
 * @brief		シェーダプログラム解放
 * @param[in]	programId プログラムID
 */
void es1emu_DeleteProgram(const GLuint programId);

#endif // _GLES1_EMULATION

#endif	// _ES1EMU_SHADER_H
