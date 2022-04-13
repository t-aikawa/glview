/*
 * glview sample
   Written by T.Aikawa. 2021/01/08
 */

#include "glview.h"

int static size = 26;

static void redraw(glvWindow glv_win)
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glvFont_SetStyle(GLV_FONT_NAME_NORMAL,size,0.0f,0,GLV_FONT_NAME | GLV_FONT_NOMAL | GLV_FONT_SIZE | GLV_FONT_LEFT);
	glvFont_setColorRGBA(255,  0, 0,255);
	glvFont_SetBkgdColorRGBA(255,255,255,255);
	glvFont_SetPosition(0,0);

	glvFont_printf("%s","ABC");

	glvReqSwapBuffers(glv_win);
}

static int window_init(glvWindow glv_win,int width, int height)
{
	glvGl_init();	// openglを使用するための初期化処理
	glvWindow_setViewport(glv_win,width,height);

	return(GLV_OK);
}

static int window_reshape(glvWindow glv_win,int width, int height)
{
	size = (width>height)?(height*0.8):(width*0.4);		// 描画する文字フォントのサイズ

	glvWindow_setViewport(glv_win,width,height);
    redraw(glv_win);
	
	return(GLV_OK);
}


static int window_redraw(glvWindow glv_win,int drawStat)
{
    redraw(glv_win);
	return(GLV_OK);
}

static const struct glv_window_listener _window_listener = {
	.init			= window_init,
	.reshape		= window_reshape,
	.redraw			= window_redraw,
};
const struct glv_window_listener *window_listener = &_window_listener;

glvWindow	main_window = NULL;

int main_frame_start(glvWindow frame_window,int width, int height)
{
	// 描画用のウインドウを作成する
	main_window = glvCreateWindow(frame_window,window_listener,"window",0, 0, width, height,GLV_WINDOW_ATTR_DEFAULT,NULL);
	glvOnReDraw(main_window);	// 	描画要求
	return(GLV_OK);
}

static const struct glv_frame_listener _frame_window_listener = {
	.start	= main_frame_start,		// 	フレーム作成時、最初に呼び出される
};
static const struct glv_frame_listener *frame_window_listener = &_frame_window_listener;

int main(int argc, char *argv[])
{
	glvDisplay	glv_dpy;
	glvWindow	frame_window = NULL;

	glv_dpy = glvOpenDisplay(NULL);
	if(!glv_dpy){
		fprintf(stderr,"Error: glvOpenDisplay() failed\n");
		return(-1);
	}

	// フレームを作成する
	frame_window = glvCreateFrameWindow(glv_dpy,frame_window_listener,"frame","sample 001",400, 550,NULL);

	/* ----------------------------------------------------------------------------------------------- */
	glvEnterEventLoop(glv_dpy);		// event loop
	/* ----------------------------------------------------------------------------------------------- */

	// 	終了処理
	glvDestroyWindow(&main_window);
	glvDestroyWindow(&frame_window);
	glvCloseDisplay(glv_dpy);

	printf("all terminated.\n");

	return(0);
}
