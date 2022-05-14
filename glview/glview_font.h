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

#ifndef _GLVIEW_FONT_H
#define _GLVIEW_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
	GLV_FONT_NAME_NORMAL,
    GLV_FONT_NAME_TYPE1,
    GLV_FONT_NAME_TYPE2,
    GLV_FONT_NAME_TYPE3,
	GLV_FONT_NAME_MAX
};

#define GLV_FONT_NAME       (1<<0)
#define GLV_FONT_SIZE       (1<<1)
#define GLV_FONT_NOMAL      (1<<2)
#define GLV_FONT_LEFT       (1<<3)
#define GLV_FONT_CENTER     (1<<4)
#define GLV_FONT_ANGLE      (1<<5)
#define GLV_FONT_LINE_SPACE (1<<6)

void glvFont_thread_safe_init(void);
void glvFont_setDefaultFontPath(char *path);
void glvFont_SetPosition(int x_ofs,int y_ofs);
void glvFont_GetPosition(int *x_ofs,int *y_ofs);
void glvFont_setColor4i(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void glvFont_SetBkgdColor4i(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void glvFont_setOutLineColor4i(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void glvFont_setColorRGBA(unsigned int color);
void glvFont_setBkgdColorRGBA(unsigned int color);
void glvFont_setFontPixelSize(int size);
void glvFont_setFontAngle(float angle);
void glvFont_DefaultColor(void);
void glvFont_DefaultStyle(void);
void glvFont_SetStyle(int font,int size,float angle,int lineSpace,int attr);
int glvFont_LoadFont(int font,char *fontPath);
void glvFont_lineSpace(int n);
void glvFont_SetlineSpace(int n);
void glvFont_SetBaseHeight(int n);
int glvFont_printf(char * fmt,...);

int glvFont_string_to_utf32(char *str,int str_size,int *utf32_string,int max_chars);
int glvFont_utf32_to_string(int *utf32_string,int str_size,char *str,int max_chars);
int glvFont_DrawUTF32String(int *utf32_string,int utf32_length,int16_t *advance_x);
int glvFont_getCursorPosition(int *utf32_string,int utf32_length,int16_t *advance_x,int cursor_pos);
int glvFont_insertCharacter(int *utf32_string,int *utf32_length,int utf32,int cursor_index);
int glvFont_deleteCharacter(int *utf32_string,int *utf32_length,int cursor_index);
int glvFont_setCharacter(int *utf32_string,int *utf32_length,int utf32,int cursor_index);

#ifdef __cplusplus
}
#endif

#endif	// _GLVIEW_FONT_H
