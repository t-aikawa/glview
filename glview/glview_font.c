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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include "glview.h"

// マルチサンプリング指定で文字を描画したときに、
// 下の離れた場所に不正にラインが描画される不具合を暫定対応
// 対処療法であり、原因不明 2021.05.09 by T.Aikawa
// 右側のゴミは、文字フォントを変えることで、現象が変わるので、
// 現時点では、対策しない。
#define TEST_2021_05_09_002


#define GLV_GL_PRINTF_FONT_SIZE	(18)

#define GLV_GL_PRINTF_LEFT		(0)
#define GLV_GL_PRINTF_CENTER	(1)

// ビットマップフォント
typedef struct _BITMAPFONT {
	u_int8_t	*pBitMap;
	int32_t		width;
	int32_t		height;
	int32_t		strWidth;
	int32_t		strHeight;
	float		rotation;
} T_BITMAPFONT;

// フォント情報
typedef struct _FONT_INFO {
	int			fontNum;
	float		fontSize;		// サイズ
	int			outLineFlg;		// 縁の有無 1縁有、0縁無
	int			bold;			// 
	int			center;			//
	int			lineSpace;		//
	//GLV_RGBACOLOR	color;			// 色
	//GLV_RGBACOLOR	outLineColor;	// 縁色
	//GLV_RGBACOLOR	bkgdColor;		// 背景色
	float		rotation;		// 回転角度
	int			lineBreak;		// 改行指定
} T_FONT_INFO;

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
	int				initFlag;
	FT_Library		library;
	FT_Face			face[GLV_FONT_NAME_MAX];
	//
	unsigned int	gColor;
	unsigned int	gOutLineColor;
	unsigned int	gBkgdColor;
	int32_t			baseHeight;
	//
	T_FONT_INFO		fontInfo;
	T_BITMAPFONT	bitmapFont;
	//
	int				x_ofs;
	int				y_ofs;
	int				x_pos;
} THREAD_SAFE_BUFFER_t;
#include "glview_thread_safe.h"
// =============================================================================
// =============================================================================

#define FONT_PATH_SIZE		(256)
static char font_path[FONT_PATH_SIZE]={};

void glvFont_thread_safe_init(void)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info;
	char file_path[FONT_PATH_SIZE*2];

	init_thread_safe_buffer();

	font_draw_info = get_thread_safe_buffer();

	memset(font_draw_info,0,sizeof(THREAD_SAFE_BUFFER_t));

	glvFont_DefaultStyle();

	//sprintf(file_path,"%s%s",font_path,"IPAexfont00201/ipaexg.ttf");
	sprintf(file_path,"%s","/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
	glvFont_LoadFont(GLV_FONT_NAME_NORMAL,file_path);

	//sprintf(file_path,"%s%s",font_path,"AP.ttf");
	sprintf(file_path,"%s","/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc");
	glvFont_LoadFont(GLV_FONT_NAME_TYPE1 ,file_path);
}

void glvFont_setDefaultFontPath(char *path)
{
	strcat(font_path,path);
}

void glvFont_SetPosition(int x_pos,int y_pos)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->x_pos = x_pos;
	font_draw_info->x_ofs = x_pos;
	font_draw_info->y_ofs = y_pos;
}

void glvFont_GetPosition(int *x_pos,int *y_pos)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	*x_pos = font_draw_info->x_ofs;
	*y_pos = font_draw_info->y_ofs;
}

void glvFont_setColor4i(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->gColor = ((a<<24) | (r<<16) | (g<<8) | (b));
}

void glvFont_SetBkgdColor4i(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->gBkgdColor = ((a<<24) | (r<<16) | (g<<8) | (b));
}

void glvFont_setOutLineColor4i(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->gOutLineColor = ((a<<24) | (r<<16) | (g<<8) | (b));
}

void glvFont_setColorRGBA(unsigned int color)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->gColor = color;
}

void glvFont_setOutLineColor(unsigned int color)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->gOutLineColor = color;
}

void glvFont_setBkgdColorRGBA(unsigned int color)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->gBkgdColor = color;
}

void glvFont_setFontPixelSize(int size)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	if(size <= 0) size = GLV_GL_PRINTF_FONT_SIZE;
	font_draw_info->fontInfo.fontSize = size;
}

void glvFont_setFontAngle(float angle)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->fontInfo.rotation = angle;
}

void glvFont_DefaultColor(void)
{
	glvFont_setColor4i			(  0,   0,   0, 255);
	glvFont_SetBkgdColor4i		(  0,   0,   0,   0);
	glvFont_setOutLineColor4i	(255,   0,   0, 255);
}

void glvFont_lineSpace(int n)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->y_ofs += n;
}

void glvFont_SetlineSpace(int n)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->fontInfo.lineSpace = n;
}

void glvFont_SetBaseHeight(int n)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	font_draw_info->baseHeight = n;
}

void glvFont_SetStyle(int font,int size,float angle,int lineSpace,int attr)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();

	if(attr & GLV_FONT_NAME){
		if(font < GLV_FONT_NAME_MAX){
			font_draw_info->fontInfo.fontNum = font;
		}
	}
	if(attr & GLV_FONT_NOMAL){
		glvFont_DefaultStyle();
	}
	if(attr & GLV_FONT_SIZE){
		glvFont_setFontPixelSize(size);
	}
	if(attr & GLV_FONT_ANGLE){
		glvFont_setFontAngle(angle);
	}
	if(attr & GLV_FONT_LEFT){
		font_draw_info->fontInfo.center = 0;
	}
	if(attr & GLV_FONT_CENTER){
		font_draw_info->fontInfo.center = 1;
	}
	if(attr & GLV_FONT_LINE_SPACE){
		glvFont_SetlineSpace(lineSpace);
	}
}

void glvFont_DefaultStyle(void)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();

	glvFont_setFontPixelSize(GLV_GL_PRINTF_FONT_SIZE);
	glvFont_setFontAngle(0.0f);
	font_draw_info->fontInfo.center = 0;
	font_draw_info->fontInfo.bold = 0;
	font_draw_info->fontInfo.outLineFlg = 1;
	font_draw_info->baseHeight = 0;
}

int glvFont_LoadFont(int font,char *fontPath)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	FT_Face face;
	FT_Error err;

	if(font >= GLV_FONT_NAME_MAX){
		return(GLV_ERROR);
	}
	if(font_draw_info->initFlag == 0){
		err = FT_Init_FreeType(&font_draw_info->library);
		if (err) {printf("glv_createBitmapFont:FT_Init_FreeType error\n"); }
		font_draw_info->initFlag = 1;
	}

	if(font_draw_info->face[font] != NULL){
		FT_Done_Face(font_draw_info->face[font]);
		font_draw_info->face[font] = NULL;
	}

	err = FT_New_Face(font_draw_info->library, fontPath, 0, &face);

	if (err) { printf("glv_createBitmapFont:FT_New_Face error\n"); }
	if(err){
		font_draw_info->face[font] = NULL;
		return(GLV_ERROR);
	}

	font_draw_info->face[font] = face;
	return(GLV_OK);
}

static int isPowerOf2(int val) {
    return val > 0 && (val & (val - 1)) == 0;
}

static int chgPowerOf2(int val) {
    int ret = 0;

    if (val <= 0) {
        return 0;
    }

    if (!isPowerOf2(val)) {
        for (int shift=2; shift<31; shift++) {
            // 32bit符号付整数なので30回シフト=2^31まで

            ret = 1 << shift;	// 1を左シフトして2のべき乗を生成
            if (ret > val) {
                return ret;
            }
        }
    } else {
        ret = val;
    }

    return ret;
}

int glvFont_UTF32toUTF8(int utf32,char *utf8,int *bytesInSequence) {
    if (utf32 < 0 || utf32 > 0x10FFFF) {
		*bytesInSequence = 0;
        return(0);
    }

    if (utf32 < 128){
        utf8[0] = (char)(utf32);
        utf8[1] = 0;
        utf8[2] = 0;
        utf8[3] = 0;
		*bytesInSequence = 1;
    } else if(utf32 < 2048){
        utf8[0] = 0xC0 | (char)(utf32 >> 6);
        utf8[1] = 0x80 | (char)((utf32) & 0x3F);
        utf8[2] = 0;
        utf8[3] = 0;
		*bytesInSequence = 2;
    } else if(utf32 < 65536){
        utf8[0] = 0xE0 | (char)(utf32 >> 12);
        utf8[1] = 0x80 | (char)((utf32 >> 6) & 0x3F);
        utf8[2] = 0x80 | (char)((utf32) & 0x3F);
        utf8[3] = 0;
		*bytesInSequence = 3;
    } else {
        utf8[0] = 0xF0 | (char)(utf32 >> 18);
        utf8[1] = 0x80 | (char)((utf32 >> 12) & 0x3F);
        utf8[2] = 0x80 | (char)((utf32 >> 6) & 0x3F);
        utf8[3] = 0x80 | (char)((utf32) & 0x3F);
		*bytesInSequence = 4;
    }
    return(1);
}

int glvFont_UTF8toUTF32(unsigned char *str, int *bytesInSequence)
{
	unsigned char c1, c2, c3, c4, c5, c6;

    *bytesInSequence = 1;
    if (!str){
        return 0;
    }

    //0xxxxxxx (ASCII)      7bit
    c1 = str[0];
    if ((c1 & 0x80) == 0x00)
    {
        return c1;
    }

    //10xxxxxx              high-order byte
    if ((c1 & 0xc0) == 0x80)
    {
        return 0;
    }

    //0xFE or 0xFF          BOM (not utf-8)
    if (c1 == 0xfe || c1 == 0xFF )
    {
        return 0;
    }

    //110AAAAA 10BBBBBB     5+6bit=11bit
    c2 = str[1];
    if (((c1 & 0xe0) == 0xc0) &&
        ((c2 & 0xc0) == 0x80))
    {
        *bytesInSequence = 2;


        return ((c1 & 0x1f) << 6) | (c2 & 0x3f);
    }

    //1110AAAA 10BBBBBB 10CCCCCC        4+6*2bit=16bit
    c3 = str[2];
    if (((c1 & 0xf0) == 0xe0) &&
        ((c2 & 0xc0) == 0x80) &&
        ((c3 & 0xc0) == 0x80))
    {
        *bytesInSequence = 3;
        return ((c1 & 0x0f) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f);
    }

    //1111 0AAA 10BBBBBB 10CCCCCC 10DDDDDD      3+6*3bit=21bit
    c4 = str[3];
    if (((c1 & 0xf8) == 0xf0) &&
        ((c2 & 0xc0) == 0x80) &&
        ((c3 & 0xc0) == 0x80) &&
        ((c4 & 0xc0) == 0x80))
    {
        *bytesInSequence = 4;
        return ((c1 & 0x07) << 18) | ((c2 & 0x3f) << 12) | ((c3 & 0x3f) << 6) | (c4 & 0x3f);
    }

    //1111 00AA 10BBBBBB 10CCCCCC 10DDDDDD 10EEEEEE     2+6*4bit=26bit
    c5 = str[4];
    if (((c1 & 0xfc) == 0xf0) &&
        ((c2 & 0xc0) == 0x80) &&
        ((c3 & 0xc0) == 0x80) &&
        ((c4 & 0xc0) == 0x80) &&
        ((c5 & 0xc0) == 0x80))
    {
        *bytesInSequence = 4;
        return ((c1 & 0x03) << 24) | ((c2 & 0x3f) << 18) | ((c3 & 0x3f) << 12) | ((c4 & 0x3f) << 6) | (c5 & 0x3f);
    }

    //1111 000A 10BBBBBB 10CCCCCC 10DDDDDD 10EEEEEE 10FFFFFF        1+6*5bit=31bit
    c6 = str[5];
    if (((c1 & 0xfe) == 0xf0) &&
        ((c2 & 0xc0) == 0x80) &&
        ((c3 & 0xc0) == 0x80) &&
        ((c4 & 0xc0) == 0x80) &&
        ((c5 & 0xc0) == 0x80) &&
        ((c6 & 0xc0) == 0x80))
    {
        *bytesInSequence = 4;
        return ((c1 & 0x01) << 30) | ((c2 & 0x3f) << 24) | ((c3 & 0x3f) << 18) | ((c4 & 0x3f) << 12) | ((c5 & 0x3f) << 6) | (c6 & 0x3f);
    }

    return 0;
}

int glvFont_string_to_utf32(char *str,int str_size,int *utf32_string,int max_chars)
{
	int	utf32,bytesInSequence;
	int i,n=0;

	for(i=0;i<str_size;i+=bytesInSequence){
		utf32 = glvFont_UTF8toUTF32((unsigned char *)str,&bytesInSequence);
		if(utf32 != 0){
			utf32_string[n++] = utf32;
			if(n >= max_chars){
				break;
			}
		}
		str += bytesInSequence;
	}
	return(n);
}

int glvFont_utf32_to_string(int *utf32_string,int str_size,char *str,int max_chars)
{
	int	bytesInSequence;
	int rc,i,k,n=0;
	char utf8[4];

	for(i=0;i<str_size;i++){
		rc = glvFont_UTF32toUTF8(utf32_string[i],utf8,&bytesInSequence);
		if(rc == 0) continue;
		if((n + bytesInSequence) <= max_chars){
			for(k=0;k<bytesInSequence;k++){
				*str++ = utf8[k];
			}
			n += bytesInSequence;
		}else{
			break;
		}
	}
	*str = 0;
	return(n);
}

int glvFont_createBitmapFont(int *utf32_string,int utf32_length,int16_t *advance_x)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();	
	unsigned int	gColor;
	unsigned int	gBkgdColor;
	int buffer_width;
	int buffer_height;
	int strWidth;
	int strHeight;
	int width;
	int height;
	int i;
	FT_Face face;
	FT_Error err;
	int max_height=0;
	int draw_height;
	// --------------------------------------
	float fontSize;
	int outLineFlg;
	unsigned char **ppBitMap;
	int* pWidth;
	int* pHeight;
	int* pStrWidth;
	int* pStrHeight;

	fontSize	= font_draw_info->fontInfo.fontSize;
	outLineFlg	= font_draw_info->fontInfo.outLineFlg;
	ppBitMap	= &font_draw_info->bitmapFont.pBitMap;
	pWidth		= &font_draw_info->bitmapFont.width;
	pHeight		= &font_draw_info->bitmapFont.height;
	pStrWidth	= &font_draw_info->bitmapFont.strWidth;
	pStrHeight	= &font_draw_info->bitmapFont.strHeight;

	*ppBitMap = NULL;

	gBkgdColor		= font_draw_info->gBkgdColor;
	gColor			= font_draw_info->gColor;

    int bg_a = (gBkgdColor & 0xFF000000) >> 24;
    int bg_r = (gBkgdColor & 0x00FF0000) >> 16;
    int bg_g = (gBkgdColor & 0x0000FF00) >> 8;
    int bg_b =  gBkgdColor & 0x000000FF;

    int fc_a = (gColor & 0xFF000000) >> 24;
    int fc_r = (gColor & 0x00FF0000) >> 16;
    int fc_g = (gColor & 0x0000FF00) >> 8;
    int fc_b =  gColor & 0x000000FF;

#define FONT_MARGIN_WIDTH		(6)
#define FONT_MARGIN_HEIGHT		(6)

	buffer_width  = chgPowerOf2((fontSize+FONT_MARGIN_WIDTH) * utf32_length);
	buffer_height = fontSize + FONT_MARGIN_HEIGHT;

	int bufferSize = buffer_width*buffer_height*2;
	unsigned char *buffer = (unsigned char*)calloc(bufferSize,1);

	if(font_draw_info->initFlag == 0){
		printf("glv_createBitmapFont: not initalized library\n");
		return(0);
	}

	face = font_draw_info->face[font_draw_info->fontInfo.fontNum];
	if(face == NULL){
		printf("glv_createBitmapFont:face[%d] == NULL\n",font_draw_info->fontInfo.fontNum);
	}

	err = FT_Set_Pixel_Sizes(face, fontSize, fontSize);
	if (err) { printf("glv_createBitmapFont:FT_Set_Pixel_Sizes\n"); }

	int xOffset = 0;
	if(advance_x != NULL) advance_x[0] = xOffset;
	for(i=0;i<utf32_length;i++){
		if(outLineFlg == 0){
			/* モノクロビットマップ */
			err = FT_Load_Char(face, utf32_string[i], 0);
			if (err) { printf("glv_createBitmapFont:FT_Load_Char\n"); }

			printf("face->size->metrics.y_ppem / face->units_per_EM  = %f\n",(double)face->size->metrics.y_ppem / (double)face->units_per_EM);
			printf("face->height = %d , face->descender = %d\n",face->height,face->descender);
			int	baseline = (face->height + face->descender) * (double)face->size->metrics.y_ppem / (double)face->units_per_EM;
			//baseline += (FONT_MARGIN_HEIGHT/2);
			//printf("1 baseline = %d\n",baseline);

			err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
			if (err) { printf("glv_createBitmapFont:FT_Render_Glyph\n"); }

			FT_Bitmap *bm = &face->glyph->bitmap;
			FT_GlyphSlot g = face->glyph;
			int row, col, bit, c,index;

			//printf("bitmap_left  = %d , bitmap_top  = %d\n",g->bitmap_left,g->bitmap_top);
			//printf("advance.x = %d , advance.y = %d\n",(int)g->advance.x >> 6,(int)g->advance.y >> 6);
			//printf("bitmap.width = %d , bitmap.rows = %d\n",bm->width,bm->rows);
			//printf("pitch = %d\n",bm->pitch);

			//printf("2 baseline        = %d\n",bm->rows * yMax / (yMax - yMin));

			/* モノクロビットマップの場合 */
			for (row = 0; row < (int)bm->rows; row ++) {
			    for (col = 0; col < bm->pitch; col ++) {
					c = bm->buffer[bm->pitch * row + col];
					for (bit = 7; bit >= 0; bit --) {
						if (((c >> bit) & 1) == 1){
							index = g->bitmap_left + (buffer_width * (row + baseline - g->bitmap_top)) + (col * 8 + 7 - bit)+ xOffset;
							if((index < 0) || (index >= bufferSize)){
								printf("glv_createBitmapFont: buffer size over %d  (%d:%x)\n",index,i,utf32_string[i]);
							}else{
								*(buffer + index) = 0xff;
								draw_height = (index + buffer_width)/buffer_width;
								max_height  = (max_height >= draw_height)?(max_height):(draw_height);
							}
						}
					}
			    }
			}
			if(g->advance.x == 0){
				xOffset += (g->bitmap_left + bm->width);
				if((g->bitmap_left + bm->width) == 0){
					// draw space
					xOffset += (fontSize*0.3);	// 見た目で設定
				}
			}else{
				xOffset += (g->advance.x >> 6);
			}
		}else{
		/* アンチエイリアスフォント */
#if 1
			/* ノーマルフォント */
			err = FT_Load_Char(face, utf32_string[i],0);
			if (err) { printf("FT_Load_Char\n"); }
#else
			/* ボールド */
			err = FT_Load_Char(face, utf32_string[i], FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
			if (err) { printf("FT_Load_Char\n"); }
			if(face->glyph->format != FT_GLYPH_FORMAT_OUTLINE){
			    printf("glv_createBitmapFont: FT_Load_Char format != FT_GLYPH_FORMAT_OUTLINE\n"); // エラー！ アウトラインでなければならない
			}
			int strength = 1 << 6;    // 適当な太さ
			FT_Outline_Embolden(&face->glyph->outline, strength);
#endif

			//printf("face->size->metrics.y_ppem / face->units_per_EM  = %f\n",(double)face->size->metrics.y_ppem / (double)face->units_per_EM);
			//printf("face->height = %d , face->descender = %d\n",face->height,face->descender);
			int	baseline = (face->height + face->descender) * face->size->metrics.y_ppem / face->units_per_EM;
			//printf("3-1 baseline = %d\n",baseline);
			//baseline += (FONT_MARGIN_HEIGHT/2);
			baseline += 1;
			//printf("3-2 baseline = %d\n",baseline);

			err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
			if (err) { printf("glv_createBitmapFont:FT_Render_Glyph error\n"); }

			FT_Bitmap *bm = &face->glyph->bitmap;
			FT_GlyphSlot g = face->glyph;
			int row, col, index;
			unsigned char c;

			//printf("bitmap_left  = %d , bitmap_top  = %d\n",g->bitmap_left,g->bitmap_top);
			//printf("advance.x = %d , advance.y = %d\n",(int)g->advance.x >> 6,(int)g->advance.y >> 6);
			//printf("bitmap.width = %d , bitmap.rows = %d\n",bm->width,bm->rows);
			//printf("pitch = %d\n",bm->pitch);

			for (row = 0; row < (int)bm->rows; row ++) {
			    for (col = 0; col < bm->pitch; col ++) {
					c = (unsigned char)bm->buffer[bm->pitch * row + col];
					if(c > 0){
						index = g->bitmap_left + (buffer_width * (row + baseline - g->bitmap_top)) + col + xOffset;
						if((index < 0) || (index >= bufferSize)){
							printf("glv_createBitmapFont: buffer size over %d  (%d:%x)\n",index,i,utf32_string[i]);
						}else{
							*(buffer + index) = c;
							draw_height = (index + buffer_width)/buffer_width;
							max_height  = (max_height >= draw_height)?(max_height):(draw_height);
						}
					}
			    }
			}
			if(g->advance.x == 0){
				xOffset += (g->bitmap_left + bm->width);
				if((g->bitmap_left + bm->width) == 0){
					// draw space
					xOffset += (fontSize*0.3);	// 見た目で設定
				}
			}else{
				xOffset += (g->advance.x >> 6);
			}
		}
		if(advance_x != NULL) advance_x[i+1] = xOffset;
	}
	//printf("buffer_height = %d , max_height = %d\n",buffer_height,max_height);

    strWidth = xOffset;
    //strHeight = buffer_height;
	if(font_draw_info->baseHeight == 0){
		strHeight = max_height;
	}else if(font_draw_info->baseHeight > max_height){
    	strHeight = font_draw_info->baseHeight;
	}else{
 	   strHeight = max_height;
	}

    width = chgPowerOf2(strWidth);

#ifdef TEST_2021_05_09_002
	height = strHeight + 2;
#else
    height = chgPowerOf2(strHeight);
#endif

	int fontBufferSize = width*height;
	unsigned char *fontBuffer = (unsigned char*)calloc(fontBufferSize,sizeof(int));

    //背景色を描画
    for(int h = 0; h<strHeight; h++){
        for(int w = 0; w<strWidth; w++){
            int out = w * 4 + (width * 4)*h;
            int in  = w     + (buffer_width)*h;
            int alpha;

            alpha = buffer[in];
#if 0
            if(alpha != 0x00){
                fontBuffer[out] = bg_r;
                fontBuffer[out+1] = bg_g;
                fontBuffer[out+2] = bg_b;
                fontBuffer[out+3] = bg_a;


            	alpha = (alpha * fc_a) / 255;

            	fontBuffer[out]   = (((fc_r - bg_r) * alpha) / 255) + bg_r;
            	fontBuffer[out+1] = (((fc_g - bg_g) * alpha) / 255) + bg_g;
            	fontBuffer[out+2] = (((fc_b - bg_b) * alpha) / 255) + bg_b;
                fontBuffer[out+3] = (((alpha - bg_a) * alpha) / 255) + bg_a;
            }else{
                fontBuffer[out] = bg_r;
                fontBuffer[out+1] = bg_g;
                fontBuffer[out+2] = bg_b;
                fontBuffer[out+3] = bg_a;
            }
#else
         	if(alpha == 0x00){
                fontBuffer[out] = bg_r;
                fontBuffer[out+1] = bg_g;
                fontBuffer[out+2] = bg_b;
                fontBuffer[out+3] = bg_a;
			}else if(alpha == 0xFF){
                fontBuffer[out] = fc_r;
                fontBuffer[out+1] = fc_g;
                fontBuffer[out+2] = fc_b;
                fontBuffer[out+3] = alpha;
			}else{
			   int a,r,g,b;
                fontBuffer[out] = bg_r;
                fontBuffer[out+1] = bg_g;
                fontBuffer[out+2] = bg_b;
                fontBuffer[out+3] = bg_a;

            	alpha = (alpha * fc_a) / 255;

            	r = (((fc_r - bg_r) * alpha) / 255) + bg_r;
            	g = (((fc_g - bg_g) * alpha) / 255) + bg_g;
            	b = (((fc_b - bg_b) * alpha) / 255) + bg_b;
				a = alpha + bg_a * (0xff - alpha);	

            	fontBuffer[out]   = (r <= 0xff)?(r):(0xff);
            	fontBuffer[out+1] = (g <= 0xff)?(g):(0xff);
            	fontBuffer[out+2] = (b <= 0xff)?(b):(0xff);
				fontBuffer[out+3] = (a <= 0xff)?(a):(0xff);			   
			}
#endif
        }
    }

	free(buffer);

	*ppBitMap = fontBuffer;

	*pWidth = width;
	*pHeight =height;
	*pStrWidth = strWidth;
	*pStrHeight =strHeight;

    return 1;
}

static int glvFont_deleteBitmapFont(void)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	if(NULL == font_draw_info->bitmapFont.pBitMap) {
		return -1;
	}

	free(font_draw_info->bitmapFont.pBitMap);
	font_draw_info->bitmapFont.pBitMap = NULL;

	return 0;
}

#if 0
static void glvGl_DrawTexturesEx(uint32_t textureId,float x, float y, float width, float height, float spot_x, float spot_y, float rotation)
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
#endif

static int glvFont_DrawTexture(float spotX,float spotY)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	uint32_t textureID;

	if (NULL == font_draw_info->bitmapFont.pBitMap){
		return(-1);
	}
	// テクスチャ生成
	textureID = glvGl_GenTextures(font_draw_info->bitmapFont.pBitMap, font_draw_info->bitmapFont.width, font_draw_info->bitmapFont.height);
	if(0 != textureID){
		// 描画
		glvGl_DrawTexturesEx(textureID,
			font_draw_info->x_ofs, font_draw_info->y_ofs,
			(float)font_draw_info->bitmapFont.width,
			(float)font_draw_info->bitmapFont.height,
			spotX, spotY,
			font_draw_info->fontInfo.rotation);
		// テクスチャ解放
		glvGl_DeleteTextures(&textureID);
	}
	return(0);
}

int glvFont_DrawUTF32String(int *utf32_string,int utf32_length,int16_t *advance_x)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();

	glvGl_BeginBlend();

	font_draw_info->bitmapFont.pBitMap = NULL;

	if(advance_x != NULL){
		advance_x[0] = 0;
		advance_x[1] = 0;
	}

	if(utf32_length == 0){
		return(0);
	}

	// ビットマップフォント生成
	glvFont_createBitmapFont(utf32_string,utf32_length,advance_x);

#if 0
	if(advance_x != NULL){
		int drawIndex;
		printf("advance_x(%d):",utf32_length);
		for(drawIndex=0;drawIndex<utf32_length+1;drawIndex++){
			printf("%d ",advance_x[drawIndex]);
		}
		printf("\n");
	}
#endif

	glvFont_DrawTexture(0.0f,0.0f);
	glvFont_deleteBitmapFont();

	font_draw_info->x_ofs += font_draw_info->bitmapFont.strWidth;

	glvGl_EndBlend();
	return(font_draw_info->bitmapFont.strHeight + font_draw_info->fontInfo.lineSpace);
}

int glvFont_DrawUTF8String(char* pStr)
{
	THREAD_SAFE_BUFFER_t	*font_draw_info = get_thread_safe_buffer();
	float spotX = 0.0f;
	float spotY = 0.0f;
	int strLength,n,start,lineBreak/*,drawIndex*/;
	int		*utf32_string;
	int16_t *advance_x;
	int		utf32_length;
	char	*draw_String;
	int		draw_length;

	glvGl_BeginBlend();

	font_draw_info->bitmapFont.pBitMap = NULL;

	strLength = strlen(pStr) + 1;
	//printf(" strLength = %d\n",strLength);

	lineBreak = 0;
	start = 0;
	for(n=0;n<strLength;n++){
		if(*(pStr + n) != 0x00){
			if(*(pStr + n) == '\n'){
				lineBreak = 1;
			}else{
				continue;
			}
		}

#if 0
		font_draw_info->String = pStr + start;
		font_draw_info->length = n - start;
		font_draw_info->utf32_string[0] = 0;
		font_draw_info->utf32_length = 0;
		font_draw_info->advance_x[0] = 0;
		font_draw_info->advance_x[1] = 0;

		if(font_draw_info->length == 0){
			break;
		}
		font_draw_info->utf32_length = glvFont_string_to_utf32(font_draw_info->String,font_draw_info->length,font_draw_info->utf32_string,DRAW_CHAR_SIZE);
		if((font_draw_info->utf32_length) == 0){
			break;
		}

		//printf(" start = %d , length = %d\n",start,(n - start));
		// ビットマップフォント生成

		glvFont_createBitmapFont(font_draw_info->utf32_string,font_draw_info->utf32_length,font_draw_info->advance_x);
#if 0
		printf("advance_x(%d):",font_draw_info->utf32_length);
		for(drawIndex=0;drawIndex<font_draw_info->utf32_length+1;drawIndex++){
			printf("%d ",font_draw_info->advance_x[drawIndex]);
		}
		printf("\n");
#endif
#else
		draw_String = pStr + start;
		draw_length = n - start;

		if(draw_length == 0){
			break;
		}

		utf32_string = malloc(sizeof(int) * (draw_length + 1));
		utf32_length = glvFont_string_to_utf32(draw_String,draw_length,utf32_string,draw_length);

		if(utf32_length == 0){
			free(utf32_string);
			break;
		}

		//printf(" start = %d , length = %d\n",start,draw_length);
		// ビットマップフォント生成
		advance_x = malloc(sizeof(int16_t) * (draw_length + 2));
		glvFont_createBitmapFont(utf32_string,utf32_length,advance_x);

#if 0
		printf("advance_x(%d):",utf32_length);
		for(drawIndex=0;drawIndex<utf32_length+1;drawIndex++){
			printf("%d ",advance_x[drawIndex]);
		}
		printf("\n");
#endif
		free(utf32_string);
		free(advance_x);
#endif

		// 表示位置
		if (GLV_GL_PRINTF_LEFT == font_draw_info->fontInfo.center) {
			spotX = 0.0f;
			spotY = 0.0f;
		} else if (GLV_GL_PRINTF_CENTER == font_draw_info->fontInfo.center) {
			spotX = (float)font_draw_info->bitmapFont.strWidth/2.0;
			spotY = (float)font_draw_info->bitmapFont.strHeight/2.0;
		}

		glvFont_DrawTexture(spotX,spotY);
		glvFont_deleteBitmapFont();

		if(lineBreak == 1){
			font_draw_info->x_ofs = font_draw_info->x_pos;
			font_draw_info->y_ofs += (font_draw_info->bitmapFont.strHeight + font_draw_info->fontInfo.lineSpace);
			start = n + 1;
			lineBreak = 0;
		}else{
			font_draw_info->x_ofs += font_draw_info->bitmapFont.strWidth;
		}
	}
	glvGl_EndBlend();
	return (font_draw_info->bitmapFont.strHeight + font_draw_info->fontInfo.lineSpace);
}

int glvFont_printf(char * fmt,...)
{
	int rc;
	char str[1024];
    va_list args;

    va_start( args, fmt );

    rc = vsnprintf(str, 1024, fmt, args );

	if(rc != -1){
		glvFont_DrawUTF8String(str);
	}

    va_end( args );

	return(rc);
}

int glvFont_getCursorPosition(int *utf32_string,int utf32_length,int16_t *advance_x,int cursor_pos)
{
	int i,width=0;
	for(i=0;i<utf32_length;i++){
		width = advance_x[i+1];
		if(width > cursor_pos){
			break;
		}
	}
	return(i);
}

int glvFont_insertCharacter(int *utf32_string,int *utf32_length,int utf32,int cursor_index)
{
	int i;
	for(i=*utf32_length;i>cursor_index;i--){
		utf32_string[i] = utf32_string[i-1];
	}
	utf32_string[cursor_index] = utf32;
	utf32_string[*utf32_length+1] = 0;
	*utf32_length = *utf32_length + 1;
	return(cursor_index + 1);
}

int glvFont_deleteCharacter(int *utf32_string,int *utf32_length,int cursor_index)
{
	int i;
	if(*utf32_length == 0){
		return(0);
	}
	if(cursor_index == 0){
		return(0);
	}
	for(i=cursor_index;i<*utf32_length;i++){
		utf32_string[i-1] = utf32_string[i];
	}
	utf32_string[*utf32_length] = 0;
	*utf32_length = *utf32_length - 1;
	return(cursor_index-1);
}

int glvFont_setCharacter(int *utf32_string,int *utf32_length,int utf32,int cursor_index)
{
	utf32_string[cursor_index] = utf32;
	if(*utf32_length == cursor_index){
		*utf32_length = *utf32_length + 1;
		utf32_string[*utf32_length] = 0;
	}
	return(cursor_index+1);
}
