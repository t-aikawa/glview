/*
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "glview.h"

#define APP_VERSION_TEXT	"Version 0.1.3 (" __DATE__ ")"
#define APP_TITLE_TEXT		"glview test"
#define APP_NAME_TEXT		APP_TITLE_TEXT " " APP_VERSION_TEXT


static 	char *dpyName = NULL;
glvDisplay	glv_dpy;
glvWindow	glv_frame_window = NULL;
glvWindow	glv_main_window = NULL;

extern int sample_hmi_keyboard_function_key(unsigned int key,unsigned int state);
extern int test_keyboard_handle_key(unsigned int key,unsigned int state);
extern const struct glv_window_listener *simple_egl_main_window_listener;
void test_set_pulldownMenu(glvWindow frame);
/* ----------------------------------------------------------------------- */

static int WinWidth = 1280, WinHeight = 720;	// HD		( 720p)

int main_frame_start(glvWindow glv_frame_window,int width, int height)
{
	int offset_x = 0;
	int offset_y = 0;

	printf("main_frame_start [%s] width = %d , height = %d\n",glvWindow_getWindowName(glv_frame_window),width,height);

	test_set_pulldownMenu(glv_frame_window);
	glvCreateWindow(glv_frame_window,simple_egl_main_window_listener,&glv_main_window,"simple-egl-window",
			0, 0, width, height,GLV_WINDOW_ATTR_DEFAULT);
	glvOnReDraw(glv_main_window);

	return(GLV_OK);
}

int main_frame_configure(glvWindow glv_win,int width, int height)
{
	//printf("main_frame_configure width = %d , height = %d\n",width,height);
	glvOnReShape(glv_main_window,-1,-1,width,height);
	return(GLV_OK);
}

int main_frame_action(glvWindow glv_win,int action,glvInstanceId functionId)
{
	if(action == GLV_ACTION_PULL_DOWN_MENU){
		printf("main_frame_action:GLV_ACTION_PULL_DOWN_MENU functionId = %ld\n",functionId);
	}
	if(action == GLV_ACTION_CMD_MENU){
		printf("main_frame_action:GLV_ACTION_CMD_MENU functionId = %ld\n",functionId);
		if(functionId == 1001){
			glvDisplay glv_dpy;
			glv_dpy = glv_getDisplay(glv_win);
			glvEscapeEventLoop(glv_dpy);
		}else if(functionId == 1005){
			sample_hmi_keyboard_function_key(XKB_KEY_F5,GLV_KEYBOARD_KEY_STATE_PRESSED);
		}else if(functionId == 1006){
			sample_hmi_keyboard_function_key(XKB_KEY_F6,GLV_KEYBOARD_KEY_STATE_PRESSED);
		}else if(functionId == 1007){
			sample_hmi_keyboard_function_key(XKB_KEY_F7,GLV_KEYBOARD_KEY_STATE_PRESSED);
		}else if(functionId == 1008){
			sample_hmi_keyboard_function_key(XKB_KEY_F8,GLV_KEYBOARD_KEY_STATE_PRESSED);
		}else if(functionId == 1009){
			sample_hmi_keyboard_function_key(XKB_KEY_F9,GLV_KEYBOARD_KEY_STATE_PRESSED);
		}
	}
	return(GLV_OK);
}

static const struct glv_frame_listener _main_frame_window_listener = {
	.start			= main_frame_start,
//	.configure		= main_frame_configure,
	.pullDownMenu	= 1,
	.cmdMenu		= 1,
	.action			= main_frame_action,
	.back			= GLV_FRAME_BACK_DRAW_OFF,
};
static const struct glv_frame_listener *main_frame_window_listener = &_main_frame_window_listener;

// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
int main(int argc, char *argv[])
{
	int rc;

	fprintf(stdout,"%s\n",APP_NAME_TEXT);

	glvSetDebugFlag(GLV_DEBUG_ON);

	glv_dpy = glvOpenDisplay(dpyName);
	if(!glv_dpy){
		fprintf(stderr,"Error: glvOpenDisplay() failed\n");
		return(-1);
	}

	/* ----------------------------------------------------------------------------------------------- */
	glv_input_func.keyboard_function_key	= sample_hmi_keyboard_function_key;
	glv_input_func.touch_down   			= NULL;
	glv_input_func.touch_up     			= NULL;

	glvCreateFrameWindow(glv_dpy,main_frame_window_listener,&glv_frame_window,"frame window",APP_NAME_TEXT,WinWidth, WinHeight);

	/* ----------------------------------------------------------------------------------------------- */
	glvEnterEventLoop(glv_dpy);		// event loop
	/* ----------------------------------------------------------------------------------------------- */

	// 	終了処理

	glvDestroyWindow(&glv_main_window);

	glvDestroyWindow(&glv_frame_window);

	glvCloseDisplay(glv_dpy);

	printf("all terminated.\n");

	return(0);
}

int sample_hmi_keyboard_function_key(
                    unsigned int key,
                    unsigned int state)
{
    //printf("Key is %d state is %d\n", key, state);
    if(state == GLV_KEYBOARD_KEY_STATE_PRESSED){
    	switch(key){
		case XKB_KEY_F1: 		// XKB_KEY_F1
			glvEscapeEventLoop(glv_dpy);
			break;
		case XKB_KEY_F5:		// XKB_KEY_F5
		case XKB_KEY_F6:		// XKB_KEY_F6
		case XKB_KEY_F7:		// XKB_KEY_F7
		case XKB_KEY_F8:		// XKB_KEY_F8
		case XKB_KEY_F9:		// XKB_KEY_F9
		default:
			test_keyboard_handle_key(key,state);
			break;
    	}
    }
    return(0);
}
