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

typedef struct _window_pullDown_menu_user_data{
	GLV_W_MENU_t	*menu;
	glvWindow		window_top_pullDown_menu;
	glvSheet		sheet_top_pullDown_menu;
	glvWiget		wiget_top_pullDown_menu;
	int				x,y,w,h;
	int				item_width;
	int				endDraw;
} WINDOW_PULLDOWN_MENU_USER_DATA_t;

#define GLV_W_PULL_DOWN_MENU_MAX	(12)

typedef struct _wiget_pullDown_menu_user_data{
	int				selectPullMenu;
	int				selectPullMenuStatus;
	glvWindow		window_pull_down_list;
	glvInstanceId	window_pull_down_list_id;
	GLV_W_MENU_t	menu;
	int				pos_x[GLV_W_PULL_DOWN_MENU_MAX];
	int				focus;
	int				select;
	int				item_height;
	GLV_W_MENU_t	pullMenu[GLV_W_PULL_DOWN_MENU_MAX];
} WIGET_PULLDOWN_MENU_USER_DATA_t;

#if 0
static int pullDown_menu_window_setMenu(glvWindow glv_win,GLV_W_MENU_t *menu)
{
	WINDOW_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(glv_win);
	user_data->menu = menu;
	return(GLV_OK);
}
#endif

static int pullDown_menu_window_getMenu(glvWindow glv_win,GLV_W_MENU_t **menu)
{
	WINDOW_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(glv_win);
	*menu = user_data->menu;
	return(GLV_OK);
}

// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
typedef struct _wiget_pullDown_menu_select_text_user_data{
	int		lineSpace;
	int		select;
	int		focus;
	int		item_height;
	int		item_width;
} WIGET_PULLDOWN_MENU_SELECT_TEXT_USER_DATA_t;

static int wiget_pullDown_menu_select_text_getSelectNo(glvWiget wiget,int *select)
{
	WIGET_PULLDOWN_MENU_SELECT_TEXT_USER_DATA_t *user_data = glv_getUserData(wiget);

	*select = user_data->select;

	return(GLV_OK);
}

static int wiget_pullDown_menu_select_text_focus(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int focus_stat,glvWiget in_wiget)
{
	WINDOW_PULLDOWN_MENU_USER_DATA_t *pullDown_menu_window_user_data = glv_getUserData(glv_win);
	WIGET_PULLDOWN_MENU_USER_DATA_t *pullDown_menu_user_data = glv_getUserData(pullDown_menu_window_user_data->wiget_top_pullDown_menu);

	if(focus_stat == GLV_STAT_OUT_FOCUS){
		if(in_wiget == pullDown_menu_window_user_data->wiget_top_pullDown_menu){
			// プルダウンメニューのメニューリストにフォーカスを設定する
			//printf("プルダウンメニューのメニューリストにフォーカスを設定する\n");
			//glvWiget_setFocus(wiget);
			pullDown_menu_user_data->focus = -1;
		}else{
			//printf("wiget_pullDown_menu_select_text_focus:selectPullMenuStatus = 1\n");
			pullDown_menu_user_data->focus = -1;
			pullDown_menu_user_data->selectPullMenuStatus = 1;
			glvOnReDraw(pullDown_menu_window_user_data->window_top_pullDown_menu);
		}
	}
	return(GLV_OK);
}

static int wiget_pullDown_menu_select_text_mousePointer(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int glv_mouse_event_type,glvTime glv_mouse_event_time,int glv_mouse_event_x,int glv_mouse_event_y,int pointer_left_stat)
{
	WIGET_PULLDOWN_MENU_SELECT_TEXT_USER_DATA_t *user_data = glv_getUserData(wiget);
	int		old_focus;

	//printf("wiget_pullDown_menu_select_text_mouse glv_mouse_event_y = %d\n",glv_mouse_event_y);

	old_focus = user_data->focus;
	user_data->focus = glv_mouse_event_y / user_data->item_height;

	if(glv_mouse_event_type == GLV_MOUSE_EVENT_PRESS){
		user_data->select = user_data->focus;
	}

	if(old_focus != user_data->focus){
		glvOnReDraw(glv_win);
	}

	return(GLV_OK);
}

static int wiget_pullDown_menu_select_text_redraw(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	WIGET_PULLDOWN_MENU_SELECT_TEXT_USER_DATA_t *user_data = glv_getUserData(wiget);
	WINDOW_PULLDOWN_MENU_USER_DATA_t *pullDown_menu_window_user_data = glv_getUserData(glv_win);
	//WIGET_PULLDOWN_MENU_USER_DATA_t *pullDown_menu_user_data = glv_getUserData(pullDown_menu_window_user_data->wiget_top_pullDown_menu);
	GLV_WIGET_GEOMETRY_t	geometry;
	int		kind,attr,num;
	float x,y,w,h;
	GLV_WIGET_STATUS_t	wigetStatus;
	GLV_W_MENU_t	*menu;
	int				x_pos,y_pos;

	glvSheet_getSelectWigetStatus(sheet,&wigetStatus);
	glvWiget_getWigetGeometry(wiget,&geometry);

	w	= geometry.width;
	h	= geometry.height;
	x	= geometry.x;
	y	= geometry.y;

	uint32_t	gBaseBkgdColor;
	uint32_t	gFocusBkgdColor;
	uint32_t	gPressBkgdColor;
	uint32_t	gReleaseBkgdColor;
	uint32_t	gSelectBkgdColor;
	uint32_t	gSelectColor;
	uint32_t	gFontColor;
	uint32_t	gBkgdColor;
	int			fontSize;

	glv_getValue(pullDown_menu_window_user_data->wiget_top_pullDown_menu,"ListBox gBaseBkgdColor"	,"C",&gBaseBkgdColor);
	glv_getValue(pullDown_menu_window_user_data->wiget_top_pullDown_menu,"ListBox gFocusBkgdColor"	,"C",&gFocusBkgdColor);
	glv_getValue(pullDown_menu_window_user_data->wiget_top_pullDown_menu,"ListBox gPressBkgdColor"	,"C",&gPressBkgdColor);
	glv_getValue(pullDown_menu_window_user_data->wiget_top_pullDown_menu,"ListBox gReleaseBkgdColor","C",&gReleaseBkgdColor);
	glv_getValue(pullDown_menu_window_user_data->wiget_top_pullDown_menu,"ListBox gSelectBkgdColor"	,"C",&gSelectBkgdColor);
	glv_getValue(pullDown_menu_window_user_data->wiget_top_pullDown_menu,"ListBox gSelectColor"		,"C",&gSelectColor);
	glv_getValue(pullDown_menu_window_user_data->wiget_top_pullDown_menu,"ListBox gFontColor"		,"C",&gFontColor);
	glv_getValue(pullDown_menu_window_user_data->wiget_top_pullDown_menu,"ListBox gBkgdColor"		,"C",&gBkgdColor);
	glv_getValue(pullDown_menu_window_user_data->wiget_top_pullDown_menu,"ListBox font size"		,"i",&fontSize);

	int	lineSpace	= user_data->lineSpace;
	//int	select		= user_data->select;
	int	focus		= user_data->focus;
	int	item_height	= user_data->item_height;
	int item_width = 0;

	glPushMatrix();
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	kind =  glvWiget_kindSelectWigetStatus(wiget,&wigetStatus);

	switch(kind){
		case GLV_WIGET_STATUS_FOCUS:
			break;
		case GLV_WIGET_STATUS_PRESS:
			break;
		case GLV_WIGET_STATUS_RELEASE:
		default:
		focus = -1;
			break;
	}

	glColor4f_RGBA(gBaseBkgdColor);
	glvGl_drawRectangle(x,y,w,h);

	//printf("wiget_pullDown_menu_select_text_redraw focus = %d , kind = %d\n",focus,kind);
	
	//attr = GLV_FONT_CENTER;
	attr = GLV_FONT_LEFT;
	glvFont_SetStyle(GLV_FONT_NAME_NORMAL,fontSize,0.0f,0,GLV_FONT_NAME | GLV_FONT_NOMAL | GLV_FONT_SIZE | attr);
	glvFont_setColor(gFontColor);
	glvFont_setBkgdColor(gBkgdColor);
	glvFont_SetlineSpace(lineSpace);

	pullDown_menu_window_getMenu(glv_win,&menu);
	if(menu != NULL){
		for(num=0;num<menu->num;num++){
			if(attr == GLV_FONT_LEFT){
				glvFont_SetPosition(x,y);
			}else{
				glvFont_SetPosition(x + w / 2,y + item_height / 2);
			}
			/*if(select == num){
				glvFont_setColor(gSelectColor);
				gBkgdColor = gSelectBkgdColor;
				glColor4f_RGBA(gSelectBkgdColor);
				glvGl_drawRectangle(x,y,w,item_height);

			}else */ if(focus == num){
				//printf("wiget_pullDown_menu_select_text_redraw focus = %d , kind = %d\n",focus,kind);
				glvFont_setColor(gSelectColor);
				switch(kind){
					case GLV_WIGET_STATUS_FOCUS:
						//glColor4f_RGBA(gFocusBkgdColor);
						glvFont_setColor(gSelectColor);
						gBkgdColor = gSelectBkgdColor;
						glColor4f_RGBA(gSelectBkgdColor);
						break;
					case GLV_WIGET_STATUS_PRESS:
						gBkgdColor = gPressBkgdColor;
						glColor4f_RGBA(gPressBkgdColor);
						break;
					case GLV_WIGET_STATUS_RELEASE:
					default:
						gBkgdColor = gReleaseBkgdColor;
						glColor4f_RGBA(gReleaseBkgdColor);
						break;
				}
				glvGl_drawRectangle(x,y,w,item_height);
			}else{
				glvFont_setColor(gFontColor);
				gBkgdColor = gBaseBkgdColor;
				glColor4f_RGBA(gBaseBkgdColor);			
			}

			glvFont_setBkgdColor(gBkgdColor);
			glvFont_printf(" %s ",menu->item[num].text);
			glvFont_GetPosition(&x_pos,&y_pos);
			if(item_width < x_pos){
				//printf("item_width %d , x_pos %d\n",item_width,x_pos);
				item_width = x_pos;
			}
			y += item_height;
		}
	}

	if(user_data->item_width < item_width){
		//printf("glvOnReShape\n");
		pullDown_menu_window_user_data->endDraw = 2; // endDrawdで２回再描画する  windows11+WSL2+wslgではなぜか、２回描画しないと正しく表示されない
		user_data->item_width = item_width;
		pullDown_menu_window_user_data->item_width = item_width;
		//glvOnReShape(glv_win,-1,-1,item_width,-1);
	}

	//glDisable(GL_BLEND);
	glPopMatrix();

	return(GLV_OK);
}

static int wiget_pullDown_menu_select_text_init(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	glv_allocUserData(wiget,sizeof(WIGET_PULLDOWN_MENU_SELECT_TEXT_USER_DATA_t));
	WIGET_PULLDOWN_MENU_SELECT_TEXT_USER_DATA_t *user_data = glv_getUserData(wiget);

	user_data->lineSpace	= 2;

	return(GLV_OK);
}

static int wiget_pullDown_menu_select_text_terminate(glvWiget wiget)
{
	//printf("wiget_pullDown_menu_select_text_terminate\n");
	return(GLV_OK);
}

static const struct glv_wiget_listener _wiget_pullDownMenuSelectText_listener = {
	.attr			= (GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_MOTION),
//	.attr			= GLV_WIGET_ATTR_NO_OPTIONS,
	.init			= wiget_pullDown_menu_select_text_init,
	.redraw			= wiget_pullDown_menu_select_text_redraw,
	.mousePointer	= wiget_pullDown_menu_select_text_mousePointer,
	.focus			= wiget_pullDown_menu_select_text_focus,
	.terminate		= wiget_pullDown_menu_select_text_terminate
};
const struct glv_wiget_listener *wiget_pullDownMenuSelectText_listener = &_wiget_pullDownMenuSelectText_listener;

// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
#if 1
typedef struct _sheet_pullDown_menu_user_data{
	glvWiget wiget_pullDown_menu_select_list;
} SHEET_PULLDOWN_MENU_USER_DATA_t;

static int pullDown_menu_sheet_action(glvWindow glv_win,glvSheet sheet,int action,glvInstanceId selectId)
{
	SHEET_PULLDOWN_MENU_USER_DATA_t *sheet_user_data = glv_getUserData(sheet);
	WINDOW_PULLDOWN_MENU_USER_DATA_t *pullDown_menu_window_user_data = glv_getUserData(glv_win);
	int select;

	if(selectId == glv_getInstanceId(sheet_user_data->wiget_pullDown_menu_select_list)){
		wiget_pullDown_menu_select_text_getSelectNo(sheet_user_data->wiget_pullDown_menu_select_list,&select);
		//printf("pullDown_menu_sheet_action:wiget_pullDown_menu_select_list select = %d\n",select);
		WIGET_PULLDOWN_MENU_USER_DATA_t *pullDown_menu_user_data = glv_getUserData(pullDown_menu_window_user_data->wiget_top_pullDown_menu);
		pullDown_menu_user_data->selectPullMenu = select;
		pullDown_menu_user_data->selectPullMenuStatus = 1;
		pullDown_menu_user_data->focus = -1;  // test.test.test
		glvOnAction(pullDown_menu_window_user_data->sheet_top_pullDown_menu,GLV_ACTION_WIGET,glv_getInstanceId(pullDown_menu_window_user_data->wiget_top_pullDown_menu));
		glvOnReDraw(pullDown_menu_window_user_data->window_top_pullDown_menu);
	}

	return(GLV_OK);
}

static void pullDown_menu_sheet_init_params(glvSheet sheet,int WinWidth,int WinHeight)
{
	GLV_WIGET_GEOMETRY_t	geometry;
	SHEET_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(sheet);

	geometry.scale = 1.0;
	geometry.width	= WinWidth;
	geometry.height	= WinHeight;
	geometry.x	= 0;
	geometry.y	= 0;
	glvWiget_setWigetGeometry(user_data->wiget_pullDown_menu_select_list,&geometry);
}

static int pullDown_menu_sheet_reshape(glvWindow glv_win,glvSheet sheet,int window_width, int window_height)
{
	//printf("hmi_sheet_reshape2\n");
	pullDown_menu_sheet_init_params(sheet,window_width,window_height);
	glvSheet_reqDrawWigets(sheet);
	glvSheet_reqSwapBuffers(sheet);
	return(GLV_OK);
}

static int pullDown_menu_sheet_update(glvWindow glv_win,glvSheet sheet,int drawStat)
{
	//printf("hmi_sheet_update2\n");

	glvSheet_reqDrawWigets(sheet);
	glvSheet_reqSwapBuffers(sheet);

	return(GLV_OK);
}

static int pullDown_menu_sheet_init(glvWindow glv_win,glvSheet sheet,int window_width, int window_height)
{
	//char *string;
	glv_allocUserData(sheet,sizeof(SHEET_PULLDOWN_MENU_USER_DATA_t));
	SHEET_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(sheet);
	WINDOW_PULLDOWN_MENU_USER_DATA_t *pullDown_menu_window_user_data = glv_getUserData(glv_win);

	user_data->wiget_pullDown_menu_select_list	= glvCreateWiget(sheet,wiget_pullDownMenuSelectText_listener,(GLV_WIGET_ATTR_POINTER_FOCUS));
	WIGET_PULLDOWN_MENU_SELECT_TEXT_USER_DATA_t *wiget_pullDown_menu_select_list_user_data = glv_getUserData(user_data->wiget_pullDown_menu_select_list);
	WIGET_PULLDOWN_MENU_USER_DATA_t *pullDown_menu_user_data = glv_getUserData(pullDown_menu_window_user_data->wiget_top_pullDown_menu);
	wiget_pullDown_menu_select_list_user_data->focus = wiget_pullDown_menu_select_list_user_data->select = pullDown_menu_user_data->select;
	wiget_pullDown_menu_select_list_user_data->item_height	= pullDown_menu_user_data->item_height;
	wiget_pullDown_menu_select_list_user_data->item_width	= window_width;

	glvWiget_setFocus(user_data->wiget_pullDown_menu_select_list);
	glvWiget_setWigetVisible(user_data->wiget_pullDown_menu_select_list,GLV_VISIBLE);

	pullDown_menu_sheet_init_params(sheet,window_width,window_height);

	return(GLV_OK);
}

static const struct glv_sheet_listener _pullDown_menu_sheet_listener = {
	.init			= pullDown_menu_sheet_init,
	.reshape		= pullDown_menu_sheet_reshape,
	.redraw			= pullDown_menu_sheet_update,
	.update 		= pullDown_menu_sheet_update,
	.timer			= NULL,
	.mousePointer	= NULL,
	.action			= pullDown_menu_sheet_action,
	.terminate		= NULL,
};

static const struct glv_sheet_listener *pullDown_menu_sheet_listener = &_pullDown_menu_sheet_listener;

static int pullDown_menu_window_init(glvWindow glv_win,int width, int height)
{
	glvGl_init();
	glvWindow_setViewport(glv_win,width,height);

	glvSheet glv_sheet;
	glv_sheet = glvCreateSheet(glv_win,pullDown_menu_sheet_listener,"list box sheet");
	glvWindow_activeSheet(glv_win,glv_sheet);

	return(GLV_OK);
}

static int pullDown_menu_window_update(glvWindow glv_win,int drawStat)
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//printf("pullDown_menu_window_update\n");

	glvReqSwapBuffers(glv_win);
	return(GLV_OK);
}

static int pullDown_menu_window_reshape(glvWindow glv_win,int width, int height)
{
	glvWindow_setViewport(glv_win,width,height);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glvReqSwapBuffers(glv_win);
	return(GLV_OK);
}
#endif

static int pullDown_menu_window_terminate(glvWindow glv_win)
{
	//printf("pullDown_menu_window_terminate\n");
	return(GLV_OK);
}

static int pullDown_menu_window_endDraw(glvWindow glv_win,glvTime time)
{
	WINDOW_PULLDOWN_MENU_USER_DATA_t *pullDown_menu_window_user_data = glv_getUserData(glv_win);

	//printf("pullDown_menu_window_endDraw\n");

	if(pullDown_menu_window_user_data->endDraw > 0){
		pullDown_menu_window_user_data->endDraw--;
		//printf("pullDown_menu_window_endDraw:req endDraw %d\n",pullDown_menu_window_user_data->item_width);
		glvOnReShape(glv_win,-1,-1,pullDown_menu_window_user_data->item_width,-1);
	}

	return(GLV_OK);
}

static const struct glv_window_listener _pullDown_menu_window_listener = {
	.init		= pullDown_menu_window_init,
	.reshape	= pullDown_menu_window_reshape,
	.redraw		= pullDown_menu_window_update,
	.update 	= pullDown_menu_window_update,
	.timer		= NULL,
	.gesture	= NULL,
	.userMsg	= NULL,
	.terminate	= pullDown_menu_window_terminate,
	.endDraw	= pullDown_menu_window_endDraw,
};
static const struct glv_window_listener *pullDown_menu_window_listener = &_pullDown_menu_window_listener;
// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
#if 0
static int wiget_pullDown_menu_focus(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int focus_stat,glvWiget in_wiget)
{
	WIGET_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);

	printf("wiget_pullDown_menu_focus\n");
	if(focus_stat == GLV_STAT_OUT_FOCUS){
		user_data->selectPullMenuStatus = 1;
		glvOnReDraw(glv_win);
	}
	return(GLV_OK);
}
#endif

static int wiget_pullDown_menu_mousePointer(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int glv_mouse_event_type,glvTime glv_mouse_event_time,int glv_mouse_event_x,int glv_mouse_event_y,int pointer_left_stat)
{
	GLV_WIGET_GEOMETRY_t	geometry;
	WIGET_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	float x,y,w,h;
	int		i,focus;
	int		old_focus,old_select,areaFlag=0;
	int		start_x;

	glvWiget_getWigetGeometry(wiget,&geometry);

	w	= geometry.width;
	h	= geometry.height;
	x	= geometry.x;
	y	= geometry.y;

	//printf("wiget_pullDown_menu_select_text_mouse glv_mouse_event_x =  %d , glv_mouse_event_y = %d\n",glv_mouse_event_x,glv_mouse_event_y);

	old_focus = user_data->focus;
	old_select = user_data->select;

	focus = -1;
	start_x = 0;
	if((0 <= glv_mouse_event_x) && (w >= glv_mouse_event_x) && (0 <= glv_mouse_event_y) && (h >= glv_mouse_event_y)){
		areaFlag = 1;
		for(i=0;i<user_data->menu.num;i++){
			if((start_x <= glv_mouse_event_x) && ((user_data->pos_x[i] - x) >= glv_mouse_event_x)){
				focus = i;
				break;
			}
			start_x = user_data->pos_x[i] - x;
		}
	}

	user_data->focus = focus;

	//printf("user_data->focus = %d\n",user_data->focus);

	if(old_focus != user_data->focus){
		glvOnReDraw(glv_win);
	}

	if((areaFlag == 1) && (focus == -1) && (glv_mouse_event_type == GLV_MOUSE_EVENT_PRESS)){
		if(user_data->window_pull_down_list != NULL){
			glvDestroyWindow(&user_data->window_pull_down_list);
			//printf("pullDown_menu window delete step1\n");
		}
		user_data->focus = -1;
		user_data->select = -1;
	}

	if(focus == -1){
		return(GLV_OK);
	}

	glvDisplay glv_dpy = glv_getDisplay(glv_win);

	if((glv_mouse_event_type == GLV_MOUSE_EVENT_PRESS) ||
		((glv_mouse_event_type == GLV_MOUSE_EVENT_MOTION) && (glvWindow_isAliveWindow(glv_win,user_data->window_pull_down_list_id) == GLV_INSTANCE_ALIVE) && (old_select != focus))){
		int offset_x = start_x + x;
		int offset_y = y + geometry.height;
		int frame2_width = /* geometry.width */ 10;
		int frame2_height = user_data->pullMenu[focus].num * user_data->item_height;

		if(glv_getWindowType(glv_win) == GLV_TYPE_THREAD_FRAME){
			// frame window内にwindowを生成する場合、位置がずれるため、位置補正を行う
			GLV_FRAME_INFO_t frameInfo;
			glv_getFrameInfo(glv_win,&frameInfo);
			offset_x -= (frameInfo.left_size);
			offset_y -= (frameInfo.top_size);
		}

		if(old_select != focus){
			if(glvWindow_isAliveWindow(glv_win,user_data->window_pull_down_list_id) == GLV_INSTANCE_ALIVE){
				if(user_data->window_pull_down_list != NULL){
					glvDestroyWindow(&user_data->window_pull_down_list);
					//printf("pullDown_menu window delete step3\n");
				}
				user_data->select = -1;
			}
		}

		if(glvWindow_isAliveWindow(glv_win,user_data->window_pull_down_list_id) == GLV_INSTANCE_DEAD){
			//user_data->select = focus;
			user_data->selectPullMenu = -1;
			if(user_data->pullMenu[focus].num > 0){
				user_data->select = focus;
				user_data->window_pull_down_list_id = glvCreateChildWindow(glv_win,pullDown_menu_window_listener,&user_data->window_pull_down_list,"list window",
								offset_x, offset_y, frame2_width, frame2_height,GLV_WINDOW_ATTR_DEFAULT);
				glv_allocUserData(user_data->window_pull_down_list,sizeof(WINDOW_PULLDOWN_MENU_USER_DATA_t));
				WINDOW_PULLDOWN_MENU_USER_DATA_t *pullDown_menu_window_user_data = glv_getUserData(user_data->window_pull_down_list);
				pullDown_menu_window_user_data->menu  = &user_data->pullMenu[focus];
				pullDown_menu_window_user_data->window_top_pullDown_menu = glv_win;
				pullDown_menu_window_user_data->sheet_top_pullDown_menu = sheet;
				pullDown_menu_window_user_data->wiget_top_pullDown_menu = wiget;
				pullDown_menu_window_user_data->x = offset_x;
				pullDown_menu_window_user_data->y = offset_y;
				pullDown_menu_window_user_data->x = frame2_width;
				pullDown_menu_window_user_data->y = frame2_height;
				//pullDown_menu_window_setMenu(user_data->window_pull_down_list,&user_data->menu);
				glvOnReDraw(user_data->window_pull_down_list);
				//printf("pullDown_menu window create\n");
			}
		}else{
			if(glv_mouse_event_type == GLV_MOUSE_EVENT_PRESS){
				if(user_data->window_pull_down_list != NULL){
					glvDestroyWindow(&user_data->window_pull_down_list);
					//printf("pullDown_menu window delete step2\n");
				}
				user_data->select = -1;
			}
		}
	}

	return(GLV_OK);
}

static int wiget_pullDown_menu_redraw(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	WIGET_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	GLV_WIGET_GEOMETRY_t	geometry;
	int		kind,attr,num;
	float x,y,w,h,width;
	GLV_WIGET_STATUS_t	wigetStatus;
	int x_pos,y_pos;
	int start_x;

	glvSheet_getSelectWigetStatus(sheet,&wigetStatus);
	glvWiget_getWigetGeometry(wiget,&geometry);

	GLV_FRAME_INFO_t frameInfo;
	glv_getFrameInfo(glv_win,&frameInfo);

	//scale	= geometry.scale;
	w		= geometry.width;
	h		= geometry.height;
	x		= geometry.x;
	y		= geometry.y;

	uint32_t	gFocusBkgdColor;
	uint32_t	gPressBkgdColor;
	uint32_t	gReleaseBkgdColor;
	uint32_t	gSelectBkgdColor;
	uint32_t	gSelectColor;
	uint32_t	gFontColor;
	uint32_t	gBkgdColor;
	int			fontSize;

	glv_getValue(wiget,"gFocusBkgdColor"	,"C",&gFocusBkgdColor);
	glv_getValue(wiget,"gPressBkgdColor"	,"C",&gPressBkgdColor);
	glv_getValue(wiget,"gReleaseBkgdColor"	,"C",&gReleaseBkgdColor);
	glv_getValue(wiget,"gSelectBkgdColor"	,"C",&gSelectBkgdColor);
	glv_getValue(wiget,"gSelectColor"		,"C",&gSelectColor);
	glv_getValue(wiget,"gFontColor"			,"C",&gFontColor);
	glv_getValue(wiget,"gBkgdColor"			,"C",&gBkgdColor);
	glv_getValue(wiget,"font size"			,"i",&fontSize);

	int	select	= user_data->select;
	int focus	= user_data->focus;

	glPushMatrix();
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gBkgdColor = gReleaseBkgdColor;
	glColor4f_RGBA(gReleaseBkgdColor);
	glvGl_drawRectangle(x,y,w,h);

	kind =  glvWiget_kindSelectWigetStatus(wiget,&wigetStatus);

	switch(kind){
		case GLV_WIGET_STATUS_FOCUS:
			break;
		case GLV_WIGET_STATUS_PRESS:
			break;
		case GLV_WIGET_STATUS_RELEASE:
		default:
			break;
	}

	attr = GLV_FONT_LEFT;
	glvFont_SetStyle(GLV_FONT_NAME_NORMAL,fontSize,0.0f,0,GLV_FONT_NAME | GLV_FONT_NOMAL | GLV_FONT_SIZE | attr);
	if(attr == GLV_FONT_LEFT){
		glvFont_SetPosition(x,y + 2);
	}else{
		glvFont_SetPosition(x + w / 2,y + fontSize / 2);
	}
	glvFont_setColor(gFontColor);
	glvFont_setBkgdColor(gBkgdColor);

	start_x = x;
	for(num=0;num<user_data->menu.num;num++){
		if((select == num) && (user_data->selectPullMenuStatus == 0) && (glvWindow_isAliveWindow(glv_win,user_data->window_pull_down_list_id) == GLV_INSTANCE_ALIVE)){
			//printf("wiget_pullDown_menu_select_text_redraw select = %d , kind = %d\n",select,kind);
			glvFont_setColor(gSelectColor);
			gBkgdColor = gSelectBkgdColor;
			glColor4f_RGBA(gSelectBkgdColor);
			if(user_data->pos_x[num] > 0){
				if(user_data->pos_x[num] > (frameInfo.left_size + frameInfo.inner_width /* + frameInfo.right_edge_size */)){
					width = frameInfo.left_size + frameInfo.inner_width /* + frameInfo.right_edge_size */ - start_x - 2;
				}else{
					width = user_data->pos_x[num] - start_x;
				}
				glvGl_drawRectangle(start_x,y,width,h);
			}
		}else if(focus == num){
			//printf("wiget_pullDown_menu_select_text_redraw focus = %d , kind = %d\n",focus,kind);
			glvFont_setColor(gSelectColor);
			switch(kind){
				case GLV_WIGET_STATUS_FOCUS:
					//glColor4f_RGBA(gFocusBkgdColor);
					gBkgdColor = gFocusBkgdColor;
					glColor4f_RGBA(gFocusBkgdColor);
					break;
				case GLV_WIGET_STATUS_PRESS:
					gBkgdColor = gPressBkgdColor;
					glColor4f_RGBA(gPressBkgdColor);
					break;
				case GLV_WIGET_STATUS_RELEASE:
				default:
					gBkgdColor = gReleaseBkgdColor;
					glColor4f_RGBA(gReleaseBkgdColor);
					break;
			}
			//if(user_data->pos_x[num] > 0){
			//	glvGl_drawRectangle(start_x,y,user_data->pos_x[num] - start_x,h);
			//}
		}else{
			//printf("wiget_pullDown_menu_select_text_redraw other = %d , kind = %d\n",num,kind);
			glvFont_setColor(gFontColor);
			gBkgdColor = gReleaseBkgdColor;
			glColor4f_RGBA(gReleaseBkgdColor);			
		}
		if(user_data->pos_x[num] > 0){
#if 1
			if(user_data->pos_x[num] > (frameInfo.left_size + frameInfo.inner_width /* + frameInfo.right_edge_size */)){
				width = frameInfo.left_size + frameInfo.inner_width + /* frameInfo.right_edge_size */ - start_x;
			}else{
				width = user_data->pos_x[num] - start_x;
			}
#else
			width = user_data->pos_x[num] - start_x;
#endif
			glvGl_drawRectangle(start_x,y,width,h);
		}
		glvFont_setBkgdColor(gBkgdColor);
		if(user_data->pos_x[num] > (frameInfo.left_size + frameInfo.inner_width  /* + frameInfo.right_edge_size */)){
			glvFont_printf("%s","");
			break;
		}else{
			glvFont_printf(" %s ",user_data->menu.item[num].text);
		}
		glvFont_GetPosition(&x_pos,&y_pos);
		user_data->pos_x[num] = x_pos;
		start_x = user_data->pos_x[num];
	}

	//glDisable(GL_BLEND);
	glPopMatrix();

	if(user_data->selectPullMenuStatus == 1){
		user_data->selectPullMenuStatus = 0;
		if(user_data->window_pull_down_list != NULL){
			glvDestroyWindow(&user_data->window_pull_down_list);
			//printf("pullDown_menu window delete\n");
		}
	}

	return(GLV_OK);
}

static int  wiget_value_cb_pullDownMenu_list(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "list";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_pullDownMenu_list: key error [%s][%s]\n",value->key,key_string);
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
        if(menu == 0){
			if((n > 0) && (n < GLV_W_PULL_DOWN_MENU_MAX)){
				glv_menu_setItem(&user_data->menu,n,text,attr,next,functionId);
			}
        }else{
			glv_menu_setItem(&user_data->pullMenu[menu-1],n,text,attr,next,functionId);
		}
	}
	return(GLV_OK);
}

static int  wiget_value_cb_pullDownMenu_FunctionId(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "select function id";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_pullDownMenu_FunctionId: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		//value->n[0].v.int32 = glv_menu_getFunctionId(&user_data->menu,user_data->select);

		if((user_data->select > -1) && (user_data->selectPullMenu > -1)){
			value->n[0].v.int32 = glv_menu_getFunctionId(&user_data->pullMenu[user_data->select],user_data->selectPullMenu);
		}else if(user_data->focus > -1){
			value->n[0].v.int32 = glv_menu_getFunctionId(&user_data->menu,user_data->focus);
		}else{
			value->n[0].v.int32 = -1;
		}
	}else{
		// set
		//user_data->select = glv_menu_searchFunctionId(&user_data->menu,value->n[0].v.int32);
	}
	return(GLV_OK);
}

static int  wiget_value_cb_pullDownMenu_ItemHeight(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "item height";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_pullDownMenu_ItemHeight: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		value->n[0].v.int32 = user_data->item_height;
	}else{
		// set
		user_data->item_height = value->n[0].v.int32;
	}
	return(GLV_OK);
}

#define DEFAULT_PULLDOWN_MENU_ITEM_HEIGHT	(30)

static int wiget_pullDown_menu_init(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	glv_allocUserData(wiget,sizeof(WIGET_PULLDOWN_MENU_USER_DATA_t));
	WIGET_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	int i;

	int fontSize = 14;

	glv_menu_init(&user_data->menu);

	for(i=0;i<GLV_W_PULL_DOWN_MENU_MAX;i++){
		glv_menu_init(&user_data->pullMenu[i]);
	}

	glv_setValue(wiget,"list"				,"iiSiiiT",0,0,NULL,0,0,0,wiget_value_cb_pullDownMenu_list);

	glv_setValue(wiget,"select function id"	,"iT",0,wiget_value_cb_pullDownMenu_FunctionId);
	glv_setValue(wiget,"item height"		,"iT",fontSize+4,wiget_value_cb_pullDownMenu_ItemHeight);

	glv_setValue(wiget,"gFocusBkgdColor"	,"C",GLV_SET_RGBA(255,  0,  0,255));
	glv_setValue(wiget,"gPressBkgdColor"	,"C",GLV_SET_RGBA(  0,255,  0,255));
	glv_setValue(wiget,"gReleaseBkgdColor"	,"C",GLV_SET_RGBA(178,178,178,255));
	glv_setValue(wiget,"gSelectBkgdColor"	,"C",GLV_SET_RGBA(255,255,  0,255));
	glv_setValue(wiget,"gSelectColor"		,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"gFontColor"			,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"gBkgdColor"			,"C",GLV_SET_RGBA(  0,  0,  0,  0));
	glv_setValue(wiget,"font size"			,"i",fontSize);

	glv_setValue(wiget,"ListBox gBaseBkgdColor"		,"C",GLV_SET_RGBA(255,240,240,255));
	glv_setValue(wiget,"ListBox gFocusBkgdColor"	,"C",GLV_SET_RGBA(255,  0,  0,255));
	glv_setValue(wiget,"ListBox gPressBkgdColor"	,"C",GLV_SET_RGBA(  0,255,  0,255));
	glv_setValue(wiget,"ListBox gReleaseBkgdColor"	,"C",GLV_SET_RGBA(178,178,178,255));
	glv_setValue(wiget,"ListBox gSelectBkgdColor"	,"C",GLV_SET_RGBA(255,212,  0,255));
	glv_setValue(wiget,"ListBox gSelectColor"		,"C",GLV_SET_RGBA(  0,  0,255,255));
	glv_setValue(wiget,"ListBox gFontColor"			,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"ListBox gBkgdColor"			,"C",GLV_SET_RGBA(  0,  0,  0,  0));
	glv_setValue(wiget,"ListBox font size"			,"i",fontSize);

	user_data->select		= -1;
	user_data->focus		= -1;
	user_data->window_pull_down_list		= NULL;
	user_data->item_height	= DEFAULT_PULLDOWN_MENU_ITEM_HEIGHT;

	user_data->selectPullMenu = -1;
	user_data->selectPullMenuStatus = 0;

	return(GLV_OK);
}

static int wiget_pullDown_menu_terminate(glvWiget wiget)
{
	WIGET_PULLDOWN_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	int i;

	//printf("wiget_pullDown_menu_terminate\n");

	if(user_data->window_pull_down_list != NULL){
		glvDestroyWindow(&user_data->window_pull_down_list);
		//printf("pullDown_menu window delete\n");
	}

	glv_menu_free(&user_data->menu);

	for(i=0;i<GLV_W_PULL_DOWN_MENU_MAX;i++){
		glv_menu_free(&user_data->pullMenu[i]);
	}

	return(GLV_OK);
}

static const struct glv_wiget_listener _wiget_pullDownMenu_listener = {
	.attr			= (GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_FOCUS | GLV_WIGET_ATTR_POINTER_MOTION),
	.init			= wiget_pullDown_menu_init,
	.redraw			= wiget_pullDown_menu_redraw,
	.mousePointer	= wiget_pullDown_menu_mousePointer,
//	.focus			= wiget_pullDown_menu_focus,
	.terminate		= wiget_pullDown_menu_terminate
};
const struct glv_wiget_listener *wiget_pullDownMenu_listener = &_wiget_pullDownMenu_listener;
