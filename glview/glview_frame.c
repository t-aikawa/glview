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

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "xdg-shell-client-protocol.h"
#include "xdg-shell-unstable-v6-client-protocol.h"
#include "ivi-application-client-protocol.h"

#include "glview.h"
#include "weston-client-window.h"
#include "glview_local.h"

#define FRAME_SIZE				        (6)
#define FREAME_SHADOW_SIZE		        (12)

#define FRAME_TOP_NAME_SIZE		        (25)
#define FRAME_TOP_CMD_MENU_SIZE		    (26)
#define FRAME_TOP_PULL_DOWN_MENU_SIZE	(26)
#define FRAME_BOTTOM_STATUS_AREA_SIZE	(16)

typedef struct _sheet_user_data{
	glvWiget wiget_button_close;
	glvWiget wiget_button_pullDownMenu;
	glvWiget wiget_button_cmdMenu;
} SHEET_USER_DATA_t;

typedef struct _window_user_data{
	int	back;
	int	shadow;
} WINDOW_USER_DATA_t;

static int button_close_redraw(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	GLV_WIGET_GEOMETRY_t	geometry;
	int		kind;
	float x,y,w,h;
	GLV_WIGET_STATUS_t	wigetStatus;

	glvSheet_getSelectWigetStatus(sheet,&wigetStatus);
	glvWiget_getWigetGeometry(wiget,&geometry);

	w		= geometry.width;
	h		= geometry.height;
	x		= geometry.x;
	y		= geometry.y;

	glPushMatrix();
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glColor4f(0.0, 0.0, 0.0, 1.0);
	//glvGl_drawRectangle(x,y,w,h);

	kind =  glvWiget_kindSelectWigetStatus(wiget,&wigetStatus);

	switch(kind){
		case GLV_WIGET_STATUS_FOCUS:
			glColor4f(1.0, 0.0, 0.0, 1.0);
			glvGl_drawRectangle(x,y,w,h);
			break;
		case GLV_WIGET_STATUS_PRESS:
			glColor4f(0.0, 1.0, 0.0, 1.0);
			glvGl_drawRectangle(x,y,w,h);
			break;
		case GLV_WIGET_STATUS_RELEASE:
		default:
			glColor4f(0.5, 0.5, 0.5, 1.0);
			glvGl_drawRectangle(x,y,w,h);
			break;
	}

	//printf("button_close_redraw\n");
	//glDisable(GL_BLEND);
	glPopMatrix();

	return(GLV_OK);
}

static int frame_sheet_action(glvWindow glv_win,glvSheet sheet,int action,glvInstanceId selectId)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	SHEET_USER_DATA_t *user_data = glv_getUserData(sheet);
	struct _glv_window *tmp;
	struct _glv_window *glv_all_window;
	struct _glv_display *glv_dpy;

	if(selectId == glv_getInstanceId(user_data->wiget_button_close)){
		// window close
		//printf("frame_sheet_action\n");
		glv_dpy = glv_window->glv_dpy;
		if(glv_window->parent == NULL){
			//printf("frame_sheet_action: top level frame close %s\n",glv_window->name);
			glvEscapeEventLoop(glv_dpy);
		}else{
#if 0
			//printf("frame_sheet_action: child frame close %s\n",glv_window->name);
			pthread_mutex_lock(&glv_dpy->display_mutex);							// display
			wl_list_for_each_safe(glv_all_window, tmp, &glv_window->glv_dpy->window_list, link){
				if(glv_all_window->instance.Id != glv_window->instance.Id){
					if(glv_all_window->myFrame->instance.Id == glv_window->instance.Id){
						glvTerminateThreadSurfaceView(glv_all_window);
						_glvDestroyWindow(glv_all_window);
						pthread_mutex_destroy(&glv_all_window->window_mutex);
						free(glv_all_window);
					}
				}
			}
			glvDestroyWindow(&glv_win);
			pthread_mutex_unlock(&glv_dpy->display_mutex);							// display
#else
			glvDestroyWindow(&glv_win);
#endif
		}
	}else if(selectId == glv_getInstanceId(user_data->wiget_button_pullDownMenu)){
		int functionId;
		glv_getValue(user_data->wiget_button_pullDownMenu,"select function id","i",&functionId);
		//printf("frame_sheet_action:wiget_pullDownMenu functionId = %d\n",functionId);
		glvOnAction(glv_win,GLV_ACTION_PULL_DOWN_MENU,functionId);
	}else if(selectId == glv_getInstanceId(user_data->wiget_button_cmdMenu)){
		int functionId;
		int	next;
		glv_getValue(user_data->wiget_button_cmdMenu,"select function id","ii",&functionId,&next);
		//printf("frame_sheet_action:wiget_button_cmdMenu functionId = %d\n",functionId);
		glv_setValue(glv_win,"cmdMenu number","i",next);
		glvOnReDraw(glv_win);
		glvOnAction(glv_win,GLV_ACTION_CMD_MENU,functionId);

	}
	return(GLV_OK);
}

static void frame_init_params(glvSheet sheet,int WinWidth,int WinHeight)
{
	GLV_WIGET_GEOMETRY_t	geometry;
	SHEET_USER_DATA_t *user_data = glv_getUserData(sheet);
	GLV_WINDOW_t	*glv_window = ((GLV_SHEET_t *)sheet)->glv_window;

	geometry.scale = 1.0;
	geometry.width	= 16;
	geometry.height	= 16;
	geometry.x	= WinWidth  - geometry.width - FRAME_SIZE - FREAME_SHADOW_SIZE;
	geometry.y	= FRAME_SIZE;
	glvWiget_setWigetGeometry(user_data->wiget_button_close,&geometry);

	if(user_data->wiget_button_pullDownMenu != NULL){
		geometry.scale = 1.0;
		geometry.width	= glv_window->frameInfo.inner_width + 2;
		geometry.height	= glv_window->frameInfo.top_pulldown_menu_size - 1;
		geometry.x	= FRAME_SIZE - 1;
		geometry.y	= glv_window->frameInfo.top_name_size + glv_window->frameInfo.top_cmd_menu_size + glv_window->frameInfo.top_shadow_size + glv_window->frameInfo.top_edge_size;
		glvWiget_setWigetGeometry(user_data->wiget_button_pullDownMenu,&geometry);
	}
	if(user_data->wiget_button_cmdMenu != NULL){
		geometry.scale = 1.0;
		geometry.width	= glv_window->frameInfo.inner_width + 2;
		geometry.height	= glv_window->frameInfo.top_cmd_menu_size - 1;
		geometry.x	= FRAME_SIZE - 1;
		geometry.y	= glv_window->frameInfo.top_name_size + glv_window->frameInfo.top_shadow_size + glv_window->frameInfo.top_edge_size;
		glvWiget_setWigetGeometry(user_data->wiget_button_cmdMenu,&geometry);
	}
}

static int frame_sheet_reshape(glvWindow glv_win,glvSheet sheet,int window_width, int window_height)
{
	//printf("frame_sheet_reshape\n");
	frame_init_params(sheet,window_width,window_height);
	glvSheet_reqDrawWigets(sheet);
	glvSheet_reqSwapBuffers(sheet);
	return(GLV_OK);
}

static int frame_sheet_update(glvWindow glv_win,glvSheet sheet,int drawStat)
{
	//printf("frame_sheet_update\n");

	glvSheet_reqDrawWigets(sheet);
	glvSheet_reqSwapBuffers(sheet);

	return(GLV_OK);
}

static int frame_sheet_init(glvWindow glv_win,glvSheet sheet,int window_width, int window_height)
{
	glv_allocUserData(sheet,sizeof(SHEET_USER_DATA_t));
	SHEET_USER_DATA_t *user_data = glv_getUserData(sheet);

	user_data->wiget_button_close	= glvCreateWiget(sheet,NULL,GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_FOCUS);
		glvWiget_setHandler_init(user_data->wiget_button_close,NULL);
		glvWiget_setHandler_redraw(user_data->wiget_button_close,button_close_redraw);
		glvWiget_setHandler_terminate(user_data->wiget_button_close,NULL);
	glvWiget_setWigetVisible(user_data->wiget_button_close,GLV_VISIBLE);

	if(glv_isPullDownMenu(glv_win) == 1){
		user_data->wiget_button_pullDownMenu = glvCreateWiget(sheet,wiget_pullDownMenu_listener,GLV_WIGET_ATTR_NO_OPTIONS);
		glv_setValue(user_data->wiget_button_pullDownMenu,"item height","i",22);
		glvWiget_setWigetVisible(user_data->wiget_button_pullDownMenu,GLV_VISIBLE);
	}

	if(glv_isCmdMenu(glv_win) == 1){
		user_data->wiget_button_cmdMenu = glvCreateWiget(sheet,wiget_cmdMenu_listener,GLV_WIGET_ATTR_NO_OPTIONS);
		glvWiget_setWigetVisible(user_data->wiget_button_cmdMenu,GLV_VISIBLE);
	}

	frame_init_params(sheet,window_width,window_height);

	return(GLV_OK);
}

static int  window_value_cb_pullDownMenu_list(int io,struct _glv_r_value *value)
{
	GLV_WINDOW_t	*glv_window = value->instance;
	GLV_SHEET_t		*sheet;
	sheet = glv_window->active_sheet;
	if(sheet == NULL){
		return(GLV_ERROR);
	}
	SHEET_USER_DATA_t *user_data = glv_getUserData(sheet);
	if(user_data == NULL){
		return(GLV_ERROR);
	}
	if(user_data->wiget_button_pullDownMenu == NULL){
		return(GLV_ERROR);
	}
	char *key_string = "pullDownMenu";
	if(strcmp(value->key,key_string) != 0){
		printf("window_value_cb_pullDownMenu_list: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		
	}else{
		// set
        int menu        = value->n[0].v.int32;
        int n           = value->n[1].v.int32;
        char *text      = value->n[2].v.string;
        int attr        = value->n[3].v.int32;
        int next        = value->n[4].v.int32;
        int functionId  = value->n[5].v.int32;

		glv_setValue(user_data->wiget_button_pullDownMenu,"list","iiSiii",menu,n,text,attr,next,functionId);
	}
	return(GLV_OK);
}

static int  window_value_cb_cmdMenu_list(int io,struct _glv_r_value *value)
{
	GLV_WINDOW_t	*glv_window = value->instance;
	GLV_SHEET_t		*sheet;
	sheet = glv_window->active_sheet;
	if(sheet == NULL){
		return(GLV_ERROR);
	}
	SHEET_USER_DATA_t *user_data = glv_getUserData(sheet);
	if(user_data == NULL){
		return(GLV_ERROR);
	}
	if(user_data->wiget_button_cmdMenu == NULL){
		return(GLV_ERROR);
	}
	char *key_string = "cmdMenu";
	if(strcmp(value->key,key_string) != 0){
		printf("window_value_cb_cmdMenu_list: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		
	}else{
		// set
        int menu        = value->n[0].v.int32;
        int n           = value->n[1].v.int32;
        char *text      = value->n[2].v.string;
        int attr        = value->n[3].v.int32;
        int next        = value->n[4].v.int32;
        int functionId  = value->n[5].v.int32;

		glv_setValue(user_data->wiget_button_cmdMenu,"list","iiSiii",menu,n,text,attr,next,functionId);
	}
	return(GLV_OK);
}

static int function_key_input_proc(GLV_WIGET_t *glv_wiget,unsigned int key, unsigned int state)
{
	//GLV_WIGET_t *glv_wiget;
	int rc;
	int cmdMenu;

	//glv_getValue(glv_dpy,"cmdMenu wiget","P",&glv_wiget);

	if(glv_wiget == NULL){
		printf(":コマンドメニューが表示されていない glv_wiget = NULL\n");
		return(GLV_ERROR);
	}

	cmdMenu = glv_isCmdMenu(glv_wiget);

	if(cmdMenu == 0){
		printf(":コマンドメニューが表示されていない cmdMenu = 0\n");
		return(GLV_ERROR);
	}

	GLV_SHEET_t *glv_sheet = glv_wiget->glv_sheet;
	GLV_WINDOW_t *glv_window = glv_sheet->glv_window;

	int focus,new_focus;
	if((key < XKB_KEY_F1) && (key > XKB_KEY_F12)){
		return(GLV_ERROR);
	}
	focus = key - XKB_KEY_F1;

	rc = glv_setValue(glv_wiget,"function key","i",focus);
	if(rc == GLV_ERROR){
		printf(":focusが設定できない\n");
		return(GLV_ERROR);
	}
	glv_getValue(glv_wiget,"function key","i",&new_focus);
	if(focus != new_focus){
		return(GLV_ERROR);
	}
	if(state == GLV_KEYBOARD_KEY_STATE_PRESSED){
		glv_sheet->pointer_focus_wiget_Id = glv_wiget->instance.Id;
		glv_sheet->select_wiget_Id = glv_wiget->instance.Id;
		glv_sheet->select_wiget_button_status = GLV_MOUSE_EVENT_PRESS;
		glvOnReDraw(glv_window);
	}else{
		glv_sheet->pointer_focus_wiget_Id = 0;
		glv_sheet->select_wiget_Id = glv_wiget->instance.Id;
		glv_sheet->select_wiget_button_status = GLV_MOUSE_EVENT_RELEASE;
		glvOnReDraw(glv_window);
		_glv_sheet_action_front(glv_window);
	}
	return(GLV_OK);
}

static int  display_value_cb_cmdMenu_proc(int io,struct _glv_r_value *value)
{
	int rc;

	char *key_string = "cmdMenu key action";
	if(strcmp(value->key,key_string) != 0){
		printf("display_value_cb_cmdMenu_proc: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		
	}else{
		// set
		GLV_WIGET_t *glv_wiget	= value->n[0].v.pointer;
        int key					= value->n[1].v.int32;
        int state				= value->n[2].v.int32;
		if(glv_wiget == NULL){
			return(GLV_OK);
		}
		rc = function_key_input_proc(glv_wiget,key,state);
		return(rc);
	}
	return(GLV_OK);
}

static int frame_init(glvWindow glv_win,int width,int height)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t *)glv_win;

	glv_allocUserData(glv_win,sizeof(WINDOW_USER_DATA_t));
	WINDOW_USER_DATA_t *user_data = glv_getUserData(glv_win);

	glv_getValue(glv_win,"back"		,"i",&user_data->back);
	glv_getValue(glv_win,"shadow"	,"i",&user_data->shadow);

#ifdef GLV_OPENGL_ES1
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

	glViewport(0, 0, width, height);

	// プロジェクション行列の設定
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// 座標体系設定
	glOrthof(0, width, height, 0, -1, 1);

	// モデルビュー行列の設定
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glvSheet glv_sheet;
	glv_sheet = glvCreateSheet(glv_win,NULL,"frame sheet");
		glvSheet_setHandler_init(glv_sheet,frame_sheet_init);
		glvSheet_setHandler_reshape(glv_sheet,frame_sheet_reshape);
		glvSheet_setHandler_redraw(glv_sheet,frame_sheet_update);
		glvSheet_setHandler_update(glv_sheet,frame_sheet_update);
		glvSheet_setHandler_timer(glv_sheet,NULL);
		glvSheet_setHandler_mousePointer(glv_sheet,NULL);
		glvSheet_setHandler_action(glv_sheet,frame_sheet_action);
		glvSheet_setHandler_terminate(glv_sheet,NULL);
	
	glvWindow_activeSheet(glv_win,glv_sheet);

	if(glv_isPullDownMenu(glv_win) == 1){
		glv_setValue(glv_win,"pullDownMenu","iiSiiiT",0,0,NULL,0,0,0,window_value_cb_pullDownMenu_list);
	}
	if(glv_window->parent == NULL){
		glv_setValue(glv_getDisplay(glv_win),"cmdMenu key action","PiiT",NULL,0,0,display_value_cb_cmdMenu_proc);
	}
	if(glv_isCmdMenu(glv_win) == 1){
		glv_setValue(glv_win,"cmdMenu","iiSiiiT",0,0,NULL,0,0,0,window_value_cb_cmdMenu_list);
	}

	return(GLV_OK);
}

static int frame_update(glvWindow glv_win,int drawStat)
{
	GLV_FRAME_INFO_t	*frameInfo;
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	uint32_t	gFontColor;
	uint32_t	gBkgdColor;
	WINDOW_USER_DATA_t *user_data = glv_getUserData(glv_win);

	frameInfo = &glv_window->frameInfo;

	glPushMatrix();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if((user_data->shadow == 0) || (user_data->shadow == GLV_FRAME_SHADOW_DRAW_ON)){
    	// 影
		if(glv_window->top == 1){
			glColor4f(0.0, 0.0, 0.0, 0.6);
			glvGl_drawRectangle(frameInfo->left_shadow_size + frameInfo->left_edge_size - 1,
								frameInfo->top_shadow_size  + frameInfo->top_edge_size,
								frameInfo->inner_width + frameInfo->left_size + 2,
								frameInfo->inner_height + frameInfo->top_size + frameInfo->bottom_user_area_size + 2);
			glColor4f(0.0, 0.0, 0.0, 0.4);
			glvGl_drawRectangle(frameInfo->left_shadow_size + frameInfo->left_edge_size - 1,
								frameInfo->top_shadow_size  + frameInfo->top_edge_size,
								frameInfo->inner_width + frameInfo->left_size + 6,
								frameInfo->inner_height + frameInfo->top_size + frameInfo->bottom_user_area_size + 6);
			glColor4f(0.0, 0.0, 0.0, 0.2);
			glvGl_drawRectangle(frameInfo->left_shadow_size + frameInfo->left_edge_size - 1,
								frameInfo->top_shadow_size  + frameInfo->top_edge_size,
								frameInfo->inner_width + frameInfo->left_size + 10,
								frameInfo->inner_height + frameInfo->top_size + frameInfo->bottom_user_area_size + 10);
		}
	}
    // 下地
    if(glv_window->top == 1){
		gBkgdColor = GLV_SET_RGBA(222,222,222,255);
    }else{
		gBkgdColor = GLV_SET_RGBA(250,250,250,255);
    }

	switch(user_data->back){
		case GLV_FRAME_BACK_DRAW_OFF:
			break;
		case GLV_FRAME_BACK_DRAW_INNER:
			glColor4f_RGBA(gBkgdColor);
			glvGl_drawRectangle(frameInfo->left_shadow_size + frameInfo->left_size,
								frameInfo->top_shadow_size  + frameInfo->top_size,
								frameInfo->inner_width,
								frameInfo->inner_height);
			break;
		case GLV_FRAME_BACK_DRAW_FULL:
		default:
			glColor4f_RGBA(gBkgdColor);
			glvGl_drawRectangle(frameInfo->left_shadow_size,
								frameInfo->top_shadow_size,
								frameInfo->inner_width + frameInfo->left_size + frameInfo->right_edge_size,
								frameInfo->inner_height + frameInfo->top_size + frameInfo->bottom_user_area_size + frameInfo->bottom_edge_size);
			break;
	}
#if 0
	glColor4f_RGBA(gBkgdColor);
	glvGl_drawRectangle(frameInfo->left_shadow_size,
						frameInfo->top_shadow_size,
						frameInfo->inner_width + frameInfo->left_size + frameInfo->right_edge_size,
						frameInfo->inner_height + frameInfo->top_size + frameInfo->bottom_user_area_size + frameInfo->bottom_edge_size);
#endif
#if 0
	glColor4f_RGBA(gBkgdColor);
	glvGl_drawRectangle(frameInfo->left_shadow_size + frameInfo->left_size,
						frameInfo->top_shadow_size  + frameInfo->top_size,
						frameInfo->inner_width,
						frameInfo->inner_height);
#endif

	// ウィンドウタイトル
    if(glv_window->top == 1){
		gFontColor = GLV_SET_RGBA(  0,  0,  0,255);
    }else{
		gFontColor = GLV_SET_RGBA(100,100,100,255);
    }
	glvFont_setColor(gFontColor);
	glvFont_setBkgdColor(gBkgdColor);
	glvFont_SetStyle(GLV_FONT_NAME_NORMAL,16,0.0f,0,GLV_FONT_NAME | GLV_FONT_SIZE | GLV_FONT_CENTER);
	glvFont_SetPosition(frameInfo->frame_width/2,(2+frameInfo->top_name_size/2 + frameInfo->top_shadow_size + frameInfo->top_edge_size/2));
	glvFont_printf("%s(%d)",glv_window->title,glv_window->drawCount+1);

	{   // edge
		GLV_T_POINT_t point[2];
		glColor4f(0.0, 0.0, 0.0, 1.0);

        // 外側の枠
        // top line
		point[0].x = frameInfo->left_shadow_size;
		point[0].y = frameInfo->top_shadow_size + 1;
		point[1].x = point[0].x + frameInfo->left_edge_size + frameInfo->inner_width + frameInfo->right_edge_size;
		point[1].y = point[0].y;
		glvGl_drawLineStrip(point,2,1);
        // left line
		point[0].x = frameInfo->left_shadow_size + 1;
		point[0].y = frameInfo->top_shadow_size;
		point[1].x = point[0].x;
		point[1].y = point[0].y + frameInfo->top_edge_size + frameInfo->top_user_area_size + frameInfo->inner_height + frameInfo->bottom_user_area_size + frameInfo->bottom_edge_size;
		glvGl_drawLineStrip(point,2,1);
        // bottom line
		point[0].x = frameInfo->left_shadow_size;
		point[0].y = frameInfo->top_size + frameInfo->inner_height + frameInfo->bottom_user_area_size + frameInfo->bottom_edge_size;
		point[1].x = point[0].x + frameInfo->left_edge_size + frameInfo->inner_width + frameInfo->right_edge_size;
		point[1].y = point[0].y;
		glvGl_drawLineStrip(point,2,1);
        // right line
		point[0].x = frameInfo->inner_width + frameInfo->left_size + frameInfo->right_edge_size;
		point[0].y = frameInfo->top_shadow_size;
		point[1].x = point[0].x;
		point[1].y = point[0].y + frameInfo->top_edge_size + frameInfo->top_user_area_size + frameInfo->inner_height + frameInfo->bottom_user_area_size + frameInfo->bottom_edge_size;
		glvGl_drawLineStrip(point,2,1);

        // 内側の枠
        // top line
		glColor4f(0.5, 0.5, 0.5, 1.0);
		point[0].x = frameInfo->left_size - 1;
		point[0].y = frameInfo->top_size;
		point[1].x = frameInfo->left_size + frameInfo->inner_width  + 1;
		point[1].y = point[0].y;
		glvGl_drawLineStrip(point,2,1);
        // left line
		point[0].x = frameInfo->left_size;
		point[0].y = frameInfo->top_size - 1;
		point[1].x = point[0].x;
		point[1].y = point[0].y + frameInfo->inner_height + 1;
		glvGl_drawLineStrip(point,2,1);
        // bottom line
		point[0].x = frameInfo->left_size - 1;
		point[0].y = frameInfo->inner_height + frameInfo->top_size + 1;
		point[1].x = point[0].x + frameInfo->inner_width + 1;
		point[1].y = point[0].y;
		glvGl_drawLineStrip(point,2,1);
        // right line
		point[0].x = frameInfo->inner_width + frameInfo->left_size + 1;
		point[0].y = frameInfo->top_size - 1;
		point[1].x = point[0].x;
		point[1].y = point[0].y + frameInfo->inner_height + 1;
		glvGl_drawLineStrip(point,2,1);
	}

	glDisable(GL_BLEND);
	glPopMatrix();
	glvReqSwapBuffers(glv_win);
	
	return(GLV_OK);
}

int frame_reshape(glvWindow glv_win,int width, int height)
{
	glViewport(0, 0, width, height);

	// プロジェクション行列の設定
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// 座標体系設定
	glOrthof(0, width, height, 0, -1, 1);

	// モデルビュー行列の設定
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	frame_update(glv_win,GLV_STAT_DRAW_REDRAW);
	return(GLV_OK);
}

glvInstanceId glvCreateFrameWindow(void *glv_instance,const struct glv_frame_listener *listener,glvWindow *glv_win,char *name,char *title,int width, int height)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)*glv_win;
	GLV_FRAME_INFO_t	frameInfo;
	glvDisplay glv_dpy;
	glvWindow glv_win_parent;
	int rc;
	int instanceType;

	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == GLV_INSTANCE_TYPE_DISPLAY){
		glv_dpy = glv_instance;
		glv_win_parent = NULL;
	}else if(instanceType == GLV_INSTANCE_TYPE_WINDOW){
		glv_dpy = glv_getDisplay(glv_instance);
		glv_win_parent = glv_instance;
	}else{
		printf("glvCreateFrameWindow:bad instance type %08x\n",instanceType);
		return(0);
	}

	glvAllocWindowResource(glv_dpy,glv_win,name,NULL);
	glv_window = (GLV_WINDOW_t*)*glv_win;

	memset(&frameInfo,0,sizeof(GLV_FRAME_INFO_t));

    // input 
	frameInfo.top_shadow_size		= 0;
	frameInfo.bottom_shadow_size	= FREAME_SHADOW_SIZE;
	frameInfo.left_shadow_size		= 0;
	frameInfo.right_shadow_size		= FREAME_SHADOW_SIZE;

    frameInfo.top_name_size				= FRAME_TOP_NAME_SIZE;
    frameInfo.top_cmd_menu_size			= 0;
    frameInfo.top_pulldown_menu_size	= 0;
    frameInfo.bottom_status_bar_size	= 0;

	frameInfo.top_edge_size			= FRAME_SIZE;
	frameInfo.bottom_edge_size		= FRAME_SIZE;
	frameInfo.left_edge_size		= FRAME_SIZE;
	frameInfo.right_edge_size		= FRAME_SIZE;

	glv_setValue(glv_window,"back"		,"i",0);
	glv_setValue(glv_window,"shadow"	,"i",0);

	if(listener != NULL){
		if(listener->pullDownMenu != 0){
			glv_window->pullDownMenu = 1;
			frameInfo.top_pulldown_menu_size = FRAME_TOP_PULL_DOWN_MENU_SIZE;
		}
		if((listener->cmdMenu != 0) && (glv_win_parent == NULL)){
			// コマンドメニューは、最初のフレームしか作成できにない
			glv_window->cmdMenu = 1;
			frameInfo.top_cmd_menu_size = FRAME_TOP_CMD_MENU_SIZE;
		}
		if(listener->statusBar != 0){
			glv_window->statusBar = 1;
			frameInfo.bottom_status_bar_size = FRAME_BOTTOM_STATUS_AREA_SIZE;
		}
		if(listener->back != 0){
			glv_setValue(glv_window,"back"	,"i",listener->back);
		}
		if(listener->shadow != 0){
			glv_setValue(glv_window,"shadow","i",listener->shadow);
		}
	}

    // output
    frameInfo.top_user_area_size    = frameInfo.top_name_size + frameInfo.top_cmd_menu_size + frameInfo.top_pulldown_menu_size;
    frameInfo.bottom_user_area_size = frameInfo.bottom_status_bar_size;

	frameInfo.top_size				= frameInfo.top_user_area_size + frameInfo.top_shadow_size + frameInfo.top_edge_size;
	frameInfo.bottom_size			= frameInfo.bottom_status_bar_size + frameInfo.bottom_shadow_size + frameInfo.bottom_edge_size;
	frameInfo.left_size				= frameInfo.left_shadow_size   + frameInfo.left_edge_size;
	frameInfo.right_size			= frameInfo.right_shadow_size  + frameInfo.right_edge_size;

	frameInfo.inner_width			= width;
	frameInfo.inner_height			= height;
	frameInfo.frame_width			= frameInfo.inner_width + frameInfo.left_size + frameInfo.right_size;
	frameInfo.frame_height			= frameInfo.inner_height + frameInfo.top_size + frameInfo.bottom_size;

	rc = _glvCreateWindow(glv_window,name,
					GLV_TYPE_THREAD_FRAME,title, 0, 0, frameInfo.frame_width, frameInfo.frame_height,glv_win_parent,GLV_WINDOW_ATTR_DEFAULT);
	if(rc != GLV_OK){
		return(0);
	}
	memcpy(&glv_window->frameInfo,&frameInfo,sizeof(GLV_FRAME_INFO_t));

	if(listener != NULL){
		glvWindow_setHandler_action((glvWindow)glv_window,listener->action);
		glvWindow_setHandler_configure((glvWindow)glv_window,listener->configure);
		glvWindow_setHandler_terminate((glvWindow)glv_window,listener->terminate);
	}
	glvWindow_setHandler_init((glvWindow)glv_window,frame_init);
	glvWindow_setHandler_reshape((glvWindow)glv_window,frame_reshape);
	glvWindow_setHandler_redraw((glvWindow)glv_window,frame_update);
	glvWindow_setHandler_update((glvWindow)glv_window,frame_update);
	glvWindow_setHandler_timer((glvWindow)glv_window,NULL);
	glvWindow_setHandler_mousePointer((glvWindow)glv_window,NULL);
	glvWindow_setHandler_mouseButton((glvWindow)glv_window,NULL);
	glvWindow_setHandler_mouseAxis((glvWindow)glv_window,NULL);
	glvWindow_setHandler_gesture((glvWindow)glv_window,NULL);
	glvWindow_setHandler_cursor((glvWindow)glv_window,NULL);
	glvWindow_setHandler_endDraw((glvWindow)glv_window,NULL);

	glvCreateThreadSurfaceView((glvWindow)glv_window);

	glvOnReDraw(glv_window);

	return(glv_window->instance.Id);
}
