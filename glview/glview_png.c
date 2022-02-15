/*
 * Copyright © 2022 T.Aikawa
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <png.h>
#include "glview.h"

#define PNG_IF_DEBUG_PRINT	if(1)

typedef struct memory_read_interface {
	uint8_t *data;	// PNGデータ
	int data_offset;  // 読み込み時のオフセット
} memory_read_interface_t;

// データ読込コールバック
static void memory_read_func(png_structp png_ptr,png_bytep out_buf,png_size_t size)
{
	memory_read_interface_t *memory = (memory_read_interface_t*)png_get_io_ptr(png_ptr);
	memcpy(out_buf,(memory->data + memory->data_offset),size);
	memory->data_offset += size;
}

static int initialize(char* data,png_structpp png_ptr_ptr,png_infopp info_ptr_ptr)
{
	if(NULL == data){
		return(EXIT_FAILURE);
	}
	if(NULL == png_ptr_ptr){
		return(EXIT_FAILURE);
	}
	if(NULL == info_ptr_ptr){
		return(EXIT_FAILURE);
	}
	// PNG file signature チエック
	// 89 50 4E 47 0D 0A 1A 0A
	if(png_sig_cmp( data, 0, 8 ) != 0){
		return(EXIT_FAILURE);
	}
	// 読み込み構造体の初期化
	*png_ptr_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
	if(*png_ptr_ptr == NULL){
		return(EXIT_FAILURE);
	}
	// 画像情報構造体の初期化
	*info_ptr_ptr = png_create_info_struct(*png_ptr_ptr);
	if(*png_ptr_ptr == NULL){
		return(EXIT_FAILURE);
	}
	// PNGファイルのsignature(8バイト)をスキップ
	png_set_sig_bytes(*png_ptr_ptr,8);

	return(EXIT_SUCCESS);
}

static uint8_t *decode(png_structp png_ptr,png_infop info_ptr,int* p_Width,int* p_Height)
{
	uint8_t *img = NULL;
	png_byte color_type;
	png_bytep in;
	uint8_t *out;
	int x, y;
	int width;
	int height;

	// void png_read_png(png_structrp png_ptr, png_inforp info_ptr, int transforms, png_voidp params)
	// transforms パラメタ一覧
	// PNG_TRANSFORM_STRIP_16		: 16ビットサンプルを8ビットにストリップします
	// PNG_TRANSFORM_STRIP_ALPHA	: アルファチャネルを破棄します
	// PNG_TRANSFORM_PACKING		: 1、2、および4ビットのサンプルをバイトに展開します
	// PNG_TRANSFORM_PACKSWAP		: パックされたピクセルの順序を最初にLSBに変更します
	// PNG_TRANSFORM_EXPAND			: パレット画像をRGBに、グレースケールを8ビット画像に、tRNSチャンクをアルファチャネルに拡張します
	// PNG_TRANSFORM_INVERT_MONO	: モノクロ画像を反転する
	// PNG_TRANSFORM_SHIFT			: ピクセルをsBIT深度に正規化します
	// PNG_TRANSFORM_BGR			: RGBをBGRに、RGBAをBGRAに反転します
	// PNG_TRANSFORM_SWAP_ALPHA		: RGBAをARGBに、またはGAをAGに反転します
	// PNG_TRANSFORM_INVERT_ALPHA	: アルファを不透明から透明に変更します

	// PNGファイルを読み込む
	png_read_png(png_ptr,info_ptr,(PNG_TRANSFORM_PACKING | PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_EXPAND),NULL);

	width      = png_get_image_width(png_ptr,info_ptr);
	height     = png_get_image_height(png_ptr,info_ptr);
	color_type = png_get_color_type(png_ptr,info_ptr);

	// 画像データを行単位で読み出す
	png_bytepp row_pointers = png_get_rows(png_ptr,info_ptr);

	//printf("glv_decodePngData: width = %d , height = %d , color_type = %d\n",width,height,color_type);

	if(color_type == PNG_COLOR_TYPE_PALETTE){
		// PNG_TRANSFORM_EXPAND を指定している為、パレットになる事はない。
		printf("glv_decodePngData(decode): PNG_COLOR_TYPE_PALETTE width = %d , height = %d , color_type = %d\n",width,height,color_type);
		return(NULL);
	}
	img = calloc((width * height),sizeof(uint32_t*));
	if(img == NULL){
		return(NULL);
	}
	switch(color_type){
#if 0
		case PNG_COLOR_TYPE_PALETTE:  // インデックスカラー
			// PNG_TRANSFORM_EXPAND を指定している為、パレットになる事はない。
			printf("glv_decodePngData: PNG_COLOR_TYPE_PALETTE width = %d , height = %d , color_type = %d\n",width,height,color_type);
			break;
#endif
		case PNG_COLOR_TYPE_GRAY:  // グレースケール
			PNG_IF_DEBUG_PRINT printf("glv_decodePngData: PNG_COLOR_TYPE_GRAY width = %d , height = %d , color_type = %d\n",width,height,color_type);
			out = img;
			for(y =0; y<height;y++){
				in = row_pointers[y];
				for(x = 0;x<width;x++){
					uint8_t g = *in++;
					*out++ = g;	 	// r
					*out++ = g;	 	// g
					*out++ = g;	 	// b
					*out++ = 0xff;  // a
				}
			}
			break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:  // グレースケール+α
			PNG_IF_DEBUG_PRINT printf("glv_decodePngData: PNG_COLOR_TYPE_GRAY_ALPHA width = %d , height = %d , color_type = %d\n",width,height,color_type);
			out = img;
			for(y =0; y<height;y++){
				in = row_pointers[y];
				for(x = 0;x<width;x++){
					uint8_t g = *in++;
					*out++ = g;	 	// r
					*out++ = g;	 	// g
					*out++ = g;	 	// b
					*out++ = *in++; // a
				}
			}
			break;
		case PNG_COLOR_TYPE_RGB:  // RGB
			PNG_IF_DEBUG_PRINT printf("glv_decodePngData: PNG_COLOR_TYPE_RGB width = %d , height = %d , color_type = %d\n",width,height,color_type);
			out = img;
			for(y =0; y<height;y++){
				in = row_pointers[y];
				for(x = 0;x<width;x++){
					*out++ = *in++; // r
					*out++ = *in++; // g
					*out++ = *in++; // b
					*out++ = 0xff;  // a
				}
			}
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:  // RGBA
			PNG_IF_DEBUG_PRINT printf("glv_decodePngData: PNG_COLOR_TYPE_RGB_ALPHA width = %d , height = %d , color_type = %d\n",width,height,color_type);
			out = img;
			for(y=0;y<height;y++){
				in = row_pointers[y];
				for(x=0;x<width;x++){
					*out++ = *in++; // r
					*out++ = *in++; // g
					*out++ = *in++; // b
					*out++ = *in++; // a
				}
			}
			break;
		default:
			PNG_IF_DEBUG_PRINT printf("glv_decodePngData: unknown width = %d , height = %d , color_type = %d\n",width,height,color_type);
			free(img);
			img = NULL;
			break;
	}
	*p_Width = width;
	*p_Height = height;
	return(img);
}

uint8_t *glv_decodePngDataForMemory(char* data,int* p_Width,int* p_Height)
{
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	memory_read_interface_t memory; 
	uint8_t *img = NULL;

	// 初期化
	if(initialize(data,&png_ptr,&info_ptr) != EXIT_SUCCESS){
		goto error;
	}

	if(setjmp(png_jmpbuf(png_ptr))){
		goto error;
	}

	memory.data = data;
	memory.data_offset = 8;	// signature(8バイト)分をスキップ
	// pngデータをメモリから読み込む場合のコールバック処理を設定
	png_set_read_fn(png_ptr,(png_voidp)&memory,memory_read_func);

	// pngデータをデコードする
	img = decode(png_ptr,info_ptr,p_Width,p_Height);

	error:
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return(img);
}

uint8_t *glv_decodePngDataForFilePath(char* file_path,int* p_Width, int* p_Height)
{
	FILE *fp;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	memory_read_interface_t memory; 
	uint8_t *img = NULL;
	png_byte sig_bytes[8];

	fp = fopen(file_path,"rb");
	if(fp == NULL){
		fprintf(stderr,"glv_decodePngDataForFilePath : not found [%s]\n",file_path);
		goto error;
	}

	if(fread(sig_bytes,sizeof(sig_bytes),1,fp) != 1){
		goto error;
	}

	// 初期化
	if(initialize(sig_bytes,&png_ptr,&info_ptr) != EXIT_SUCCESS){
		goto error;
	}

	if(setjmp(png_jmpbuf(png_ptr))){
		goto error;
	}

	// ファイルから読み込む
	png_init_io(png_ptr,fp);

	// pngデータをデコードする
	img = decode(png_ptr,info_ptr,p_Width,p_Height);

	error:
	png_destroy_read_struct(&png_ptr, &info_ptr,NULL);

	if(fp != NULL){
		fclose(fp);
	}
	return(img);
}

#define PATH_NAME_SIZE (512)

void glv_createCsourcePngDataForFilePath(char* file_path,char* out_name)
{
	FILE *ifp;
	FILE *wfp;
	int n;
	int	ch;
	char path_name[PATH_NAME_SIZE];	// ファイルパス
	char *path;

	ifp = fopen(file_path,"rb");
	if(ifp == NULL){
		fprintf(stderr,"glv_createCsourcePngDataForFilePath : not found [%s]\n",file_path);
		return;
	}

	wfp = fopen(out_name,"wb");
	if(wfp == NULL) {
		fclose(ifp);
		fprintf(stderr,"glv_createCsourcePngDataForFilePath: can not create. [%s]\n",out_name);
		return;
	}

	n = 0;
	fprintf(wfp,"static char *%s =",out_name);
	while((ch = fgetc(ifp)) != EOF){
		if(n == 0){
			fprintf(wfp,"\n\"");
		}
		fprintf(wfp,"\\x%02x",ch);
		n++;
		if(n == 32){
			fprintf(wfp,"\"");
			n = 0;
		}
	}
	if(n != 0){
		fprintf(wfp,"\"");
	}
	fprintf(wfp,";\n");

	path_name[0] = '\0';
    path = getcwd(path_name,PATH_NAME_SIZE);	// カレントディレクトリ取得

	if(path != NULL){
		printf("glv_createCsourcePngDataForFilePath: create c source file [%s/%s]\n",path_name,out_name);
	}

	fclose(wfp);
	fclose(ifp);
}
