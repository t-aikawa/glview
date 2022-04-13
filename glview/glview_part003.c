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

typedef struct _window_list_box_user_data{
	GLV_W_MENU_t	*menu;
	glvWindow		window_list_box;
	glvSheet		sheet_list_box;
	glvWiget		wiget_list_box;
	int				x,y,w,h;
} WINDOW_LIST_BOX_USER_DATA_t;

typedef struct _wiget_list_box_user_data{
	int			select;
	int			selectStatus;
	glvWindow	window_select_list;
	glvInstanceId		window_select_list_id;
	GLV_W_MENU_t	menu;
	int			item_height;
} WIGET_LIST_BOX_USER_DATA_t;

#if 0
static int list_box_window_setMenu(glvWindow glv_win,GLV_W_MENU_t *menu)
{
	WINDOW_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(glv_win);
	user_data->menu = menu;
	return(GLV_OK);
}
#endif

static int list_box_window_getMenu(glvWindow glv_win,GLV_W_MENU_t **menu)
{
	WINDOW_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(glv_win);
	*menu = user_data->menu;
	return(GLV_OK);
}

// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
typedef struct _wiget_list_box_select_text_user_data{
	int		lineSpace;
	int		select;
	int		focus;
	int		item_height;
} WIGET_LIST_BOX_SELECT_TEXT_USER_DATA_t;

static int wiget_list_box_select_text_getSelectNo(glvWiget wiget,int *select)
{
	WIGET_LIST_BOX_SELECT_TEXT_USER_DATA_t *user_data = glv_getUserData(wiget);

	*select = user_data->select;

	return(GLV_OK);
}

static int wiget_list_box_select_text_focus(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int focus_stat,glvWiget in_wiget)
{
	WINDOW_LIST_BOX_USER_DATA_t *list_box_window_user_data = glv_getUserData(glv_win);
	WIGET_LIST_BOX_USER_DATA_t *list_box_user_data = glv_getUserData(list_box_window_user_data->wiget_list_box);

	printf("wiget_list_box_select_text_focus\n");
	if(focus_stat == GLV_STAT_OUT_FOCUS){
		list_box_user_data->selectStatus = 1;
		glvOnReDraw(list_box_window_user_data->window_list_box);
	}
	return(GLV_OK);
}

static int wiget_list_box_select_text_mousePointer(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int glv_mouse_event_type,glvTime glv_mouse_event_time,int glv_mouse_event_x,int glv_mouse_event_y,int pointer_left_stat)
{
	WIGET_LIST_BOX_SELECT_TEXT_USER_DATA_t *user_data = glv_getUserData(wiget);
	int		old_focus;

	//printf("wiget_list_box_select_text_mouse glv_mouse_event_y = %d\n",glv_mouse_event_y);

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

static int wiget_list_box_select_text_redraw(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	WIGET_LIST_BOX_SELECT_TEXT_USER_DATA_t *user_data = glv_getUserData(wiget);
	WINDOW_LIST_BOX_USER_DATA_t *list_box_window_user_data = glv_getUserData(glv_win);
	//WIGET_LIST_BOX_USER_DATA_t *list_box_user_data = glv_getUserData(list_box_window_user_data->wiget_list_box);
	GLV_WIGET_GEOMETRY_t	geometry;
	int		kind,attr,num;
	float x,y,w,h;
	GLV_WIGET_STATUS_t	wigetStatus;
	GLV_W_MENU_t	*menu;
	//int select = list_box_user_data->select;

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

	glv_getValue(list_box_window_user_data->wiget_list_box,"ListBox gBaseBkgdColor"		,"C",&gBaseBkgdColor);
	glv_getValue(list_box_window_user_data->wiget_list_box,"ListBox gFocusBkgdColor"	,"C",&gFocusBkgdColor);
	glv_getValue(list_box_window_user_data->wiget_list_box,"ListBox gPressBkgdColor"	,"C",&gPressBkgdColor);
	glv_getValue(list_box_window_user_data->wiget_list_box,"ListBox gReleaseBkgdColor"	,"C",&gReleaseBkgdColor);
	glv_getValue(list_box_window_user_data->wiget_list_box,"ListBox gSelectBkgdColor"	,"C",&gSelectBkgdColor);
	glv_getValue(list_box_window_user_data->wiget_list_box,"ListBox gSelectColor"		,"C",&gSelectColor);
	glv_getValue(list_box_window_user_data->wiget_list_box,"ListBox gFontColor"			,"C",&gFontColor);
	glv_getValue(list_box_window_user_data->wiget_list_box,"ListBox gBkgdColor"			,"C",&gBkgdColor);
	glv_getValue(list_box_window_user_data->wiget_list_box,"ListBox font size"			,"i",&fontSize);

	int		lineSpace		= user_data->lineSpace;
	int		select			= user_data->select;
	int		focus			= user_data->focus;
	int		item_height		= user_data->item_height;

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

	//printf("wiget_list_box_select_text_redraw focus = %d , kind = %d\n",focus,kind);
	
	//attr = GLV_FONT_LEFT;
	attr = GLV_FONT_CENTER;
	glvFont_SetStyle(GLV_FONT_NAME_TYPE1,fontSize,0.0f,0,GLV_FONT_NAME | GLV_FONT_NOMAL | GLV_FONT_SIZE | attr);
	glvFont_setColor(gFontColor);
	glvFont_setBkgdColor(gBkgdColor);
	glvFont_SetlineSpace(lineSpace);

	list_box_window_getMenu(glv_win,&menu);
	if(menu != NULL){
		for(num=0;num<menu->num;num++){
			if(attr == GLV_FONT_LEFT){
				glvFont_SetPosition(x,y);
			}else{
				glvFont_SetPosition(x + w / 2,y + item_height / 2);
			}
			if(select == num){
				glvFont_setColor(gSelectColor);
				gBkgdColor = gSelectBkgdColor;
				glColor4f_RGBA(gSelectBkgdColor);
				glvGl_drawRectangle(x,y,w,item_height);

			}else if(focus == num){
				//printf("wiget_list_box_select_text_redraw focus = %d , kind = %d\n",focus,kind);
				glvFont_setColor(gSelectColor);
				switch(kind){
					case GLV_WIGET_STATUS_FOCUS:
						//glColor4f_RGBA(gFocusBkgdColor);
						gBkgdColor = gReleaseBkgdColor;
						glColor4f_RGBA(gReleaseBkgdColor);
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
			glvFont_printf("%s",menu->item[num].text);
			y += item_height;
		}
	}

	//glDisable(GL_BLEND);
	glPopMatrix();

	return(GLV_OK);
}

static int wiget_list_box_select_text_init(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	glv_allocUserData(wiget,sizeof(WIGET_LIST_BOX_SELECT_TEXT_USER_DATA_t));
	WIGET_LIST_BOX_SELECT_TEXT_USER_DATA_t *user_data = glv_getUserData(wiget);

	user_data->lineSpace	= 2;

	return(GLV_OK);
}

static int wiget_list_box_select_text_terminate(glvWiget wiget)
{
	printf("wiget_list_box_select_text_terminate\n");
	return(GLV_OK);
}

static const struct glv_wiget_listener _wiget_listBoxSelectText_listener = {
	.attr			= (GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_MOTION),
//	.attr			= GLV_WIGET_ATTR_NO_OPTIONS,
	.init			= wiget_list_box_select_text_init,
	.redraw			= wiget_list_box_select_text_redraw,
	.mousePointer	= wiget_list_box_select_text_mousePointer,
	.focus			= wiget_list_box_select_text_focus,
	.terminate		= wiget_list_box_select_text_terminate
};
const struct glv_wiget_listener *wiget_listBoxSelectText_listener = &_wiget_listBoxSelectText_listener;

// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
#if 1
typedef struct _sheet_list_box_user_data{
	glvWiget wiget_list_box_select_list;
} SHEET_LIST_BOX_USER_DATA_t;

static int list_box_sheet_action(glvWindow glv_win,glvSheet sheet,int action,glvInstanceId selectId)
{
	SHEET_LIST_BOX_USER_DATA_t *sheet_user_data = glv_getUserData(sheet);
	WINDOW_LIST_BOX_USER_DATA_t *list_box_window_user_data = glv_getUserData(glv_win);
	int select;

	if(selectId == glv_getInstanceId(sheet_user_data->wiget_list_box_select_list)){
		wiget_list_box_select_text_getSelectNo(sheet_user_data->wiget_list_box_select_list,&select);
		//printf("list_box_sheet_action:wiget_list_box_select_list select = %d\n",select);
		WIGET_LIST_BOX_USER_DATA_t *list_box_user_data = glv_getUserData(list_box_window_user_data->wiget_list_box);
		list_box_user_data->select = select;
		list_box_user_data->selectStatus = 1;
		glvOnAction(list_box_window_user_data->sheet_list_box,GLV_ACTION_WIGET,glv_getInstanceId(list_box_window_user_data->wiget_list_box));
		glvOnReDraw(list_box_window_user_data->window_list_box);
	}

	return(GLV_OK);
}

static void list_box_sheet_init_params(glvSheet sheet,int WinWidth,int WinHeight)
{
	GLV_WIGET_GEOMETRY_t	geometry;
	SHEET_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(sheet);

	geometry.scale	= 1.0;
	geometry.width	= WinWidth;
	geometry.height	= WinHeight;
	geometry.x	= 0;
	geometry.y	= 0;
	glvWiget_setWigetGeometry(user_data->wiget_list_box_select_list,&geometry);
}

static int list_box_sheet_reshape(glvWindow glv_win,glvSheet sheet,int window_width, int window_height)
{
	//printf("hmi_sheet_reshape2\n");
	list_box_sheet_init_params(sheet,window_width,window_height);
	glvSheet_reqDrawWigets(sheet);
	glvSheet_reqSwapBuffers(sheet);
	return(GLV_OK);
}

static int list_box_sheet_update(glvWindow glv_win,glvSheet sheet,int drawStat)
{
	//printf("hmi_sheet_update2\n");

	glvSheet_reqDrawWigets(sheet);
	glvSheet_reqSwapBuffers(sheet);

	return(GLV_OK);
}

static int list_box_sheet_init(glvWindow glv_win,glvSheet sheet,int window_width, int window_height)
{
	glv_allocUserData(sheet,sizeof(SHEET_LIST_BOX_USER_DATA_t));
	SHEET_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(sheet);
	WINDOW_LIST_BOX_USER_DATA_t *list_box_window_user_data = glv_getUserData(glv_win);

	user_data->wiget_list_box_select_list	= glvCreateWiget(sheet,wiget_listBoxSelectText_listener,(GLV_WIGET_ATTR_POINTER_FOCUS));
	WIGET_LIST_BOX_SELECT_TEXT_USER_DATA_t *wiget_list_box_select_list_user_data = glv_getUserData(user_data->wiget_list_box_select_list);
	WIGET_LIST_BOX_USER_DATA_t *list_box_user_data = glv_getUserData(list_box_window_user_data->wiget_list_box);
	wiget_list_box_select_list_user_data->focus = wiget_list_box_select_list_user_data->select = list_box_user_data->select;
	wiget_list_box_select_list_user_data->item_height = list_box_user_data->item_height;

	glvWiget_setFocus(user_data->wiget_list_box_select_list);
	glvWiget_setWigetVisible(user_data->wiget_list_box_select_list,GLV_VISIBLE);

	list_box_sheet_init_params(sheet,window_width,window_height);

	return(GLV_OK);
}

static const struct glv_sheet_listener _list_box_sheet_listener = {
	.init			= list_box_sheet_init,
	.reshape		= list_box_sheet_reshape,
	.redraw			= list_box_sheet_update,
	.update 		= list_box_sheet_update,
	.timer			= NULL,
	.mousePointer	= NULL,
	.action			= list_box_sheet_action,
	.terminate		= NULL,
};

static const struct glv_sheet_listener *list_box_sheet_listener = &_list_box_sheet_listener;

static int list_box_window_init(glvWindow glv_win,int width, int height)
{
	glvGl_init();
	glvWindow_setViewport(glv_win,width,height);

	glvSheet glv_sheet;
	glv_sheet = glvCreateSheet(glv_win,list_box_sheet_listener,"list box sheet");
	glvWindow_activeSheet(glv_win,glv_sheet);

	return(GLV_OK);
}

static int list_box_window_update(glvWindow glv_win,int drawStat)
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//printf("list_box_window_update\n");

	glvReqSwapBuffers(glv_win);
	return(GLV_OK);
}

static int list_box_window_reshape(glvWindow glv_win,int width, int height)
{
	glvWindow_setViewport(glv_win,width,height);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glvReqSwapBuffers(glv_win);
	return(GLV_OK);
}
#endif

static int list_box_window_terminate(glvWindow glv_win)
{
	printf("list_box_window_terminate\n");
	return(GLV_OK);
}

static const struct glv_window_listener _list_box_window_listener = {
	.init		= list_box_window_init,
	.reshape	= list_box_window_reshape,
	.redraw		= list_box_window_update,
	.update 	= list_box_window_update,
	.timer		= NULL,
	.gesture	= NULL,
	.userMsg	= NULL,
	.terminate	= list_box_window_terminate,
};
static const struct glv_window_listener *list_box_window_listener = &_list_box_window_listener;
// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
static int wiget_list_box_mousePointer(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int glv_mouse_event_type,glvTime glv_mouse_event_time,int glv_mouse_event_x,int glv_mouse_event_y,int pointer_left_stat)
{
	GLV_WIGET_GEOMETRY_t	geometry;
	WIGET_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(wiget);
	float x,y;
	glvWiget_getWigetGeometry(wiget,&geometry);

	x	= geometry.x;
	y	= geometry.y;

	if(glv_mouse_event_type == GLV_MOUSE_EVENT_PRESS){
		int frame2_width = geometry.width;
		int frame2_height = user_data->menu.num * user_data->item_height;
		int offset_x = x;
		int offset_y = y - user_data->select * user_data->item_height;

		if(glvWindow_isAliveWindow(glv_win,user_data->window_select_list_id) == GLV_INSTANCE_DEAD){
			user_data->window_select_list = glvCreateChildWindow(glv_win,list_box_window_listener,"list box window",
							offset_x, offset_y, frame2_width, frame2_height,GLV_WINDOW_ATTR_DEFAULT,&user_data->window_select_list_id);
			//user_data->window_select_list_id = glv_getInstanceId(user_data->window_select_list);
			glv_allocUserData(user_data->window_select_list,sizeof(WINDOW_LIST_BOX_USER_DATA_t));
			WINDOW_LIST_BOX_USER_DATA_t *list_box_window_user_data = glv_getUserData(user_data->window_select_list);
			list_box_window_user_data->menu  = &user_data->menu;
			list_box_window_user_data->window_list_box = glv_win;
			list_box_window_user_data->sheet_list_box = sheet;
			list_box_window_user_data->wiget_list_box = wiget;
			list_box_window_user_data->x = offset_x;
			list_box_window_user_data->y = offset_y;
			list_box_window_user_data->x = frame2_width;
			list_box_window_user_data->y = frame2_height;
			//list_box_window_setMenu(user_data->window_select_list,&user_data->menu);
			glvOnReDraw(user_data->window_select_list);
			printf("list_box window create\n");
		}else{
			if(user_data->window_select_list != NULL){
				glvDestroyWindow(&user_data->window_select_list);
				printf("list_box window delete\n");
			}
		}
	}

	return(GLV_OK);
}

static int wiget_list_box_redraw(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	WIGET_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(wiget);
	GLV_WIGET_GEOMETRY_t	geometry;
	int		kind,attr,num;
	float x,y,w,h;
	GLV_WIGET_STATUS_t	wigetStatus;

	glvSheet_getSelectWigetStatus(sheet,&wigetStatus);
	glvWiget_getWigetGeometry(wiget,&geometry);

	w	= geometry.width;
	h	= geometry.height;
	x	= geometry.x;
	y	= geometry.y;

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

#if 0	// test.test.test
	long	test01;
	uint8_t  *test02;
	int	test03;
	double test04;
	glv_getValue(wiget,"glv_getValue test","LiSR",&test01,&test03,&test02,&test04);
	printf("glv_getValue test(1) = %lx\n",test01 * -1);
	printf("glv_getValue test(2) = %s\n",test02);
	printf("glv_getValue test(3) = %d\n",test03);
	printf("glv_getValue test(4) = %f\n",test04);
#endif

	int	select	= user_data->select;

	glPushMatrix();
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	kind =  glvWiget_kindSelectWigetStatus(wiget,&wigetStatus);

	if(user_data->selectStatus == 1){
		// リスト選択後にフォーカスが残る不具合の暫定対策
		kind = GLV_WIGET_STATUS_RELEASE;
	}
	switch(kind){
		case GLV_WIGET_STATUS_FOCUS:
			gBkgdColor = gFocusBkgdColor;
			glColor4f_RGBA(gFocusBkgdColor);
			glvGl_drawRectangle(x,y,w,h);
			break;
		case GLV_WIGET_STATUS_PRESS:
			gBkgdColor = gPressBkgdColor;
			glColor4f_RGBA(gPressBkgdColor);
			glvGl_drawRectangle(x,y,w,h);
			break;
		case GLV_WIGET_STATUS_RELEASE:
		default:
			gBkgdColor = gReleaseBkgdColor;
			glColor4f_RGBA(gReleaseBkgdColor);
			glvGl_drawRectangle(x,y,w,h);
			break;
	}

	//attr = GLV_FONT_LEFT;
	attr = GLV_FONT_CENTER;
	glvFont_SetStyle(GLV_FONT_NAME_TYPE1,fontSize,0.0f,0,GLV_FONT_NAME | GLV_FONT_NOMAL | GLV_FONT_SIZE | attr);
	if(attr == GLV_FONT_LEFT){
		glvFont_SetPosition(x,y+2);
	}else{
		glvFont_SetPosition(x + w / 2,y + (fontSize+6) / 2);
	}
	glvFont_setColor(gFontColor);
	glvFont_setBkgdColor(gBkgdColor);
	//glvFont_SetlineSpace(lineSpace);

	for(num=0;num<user_data->menu.num;num++){
		if(select == num){
			glvFont_printf("%s\n",user_data->menu.item[num].text);
			break;
		}
	}
	//glDisable(GL_BLEND);
	glPopMatrix();

	if(user_data->selectStatus == 1){
		user_data->selectStatus = 0;
		if(user_data->window_select_list != NULL){
			glvDestroyWindow(&user_data->window_select_list);
			printf("list_box window delete\n");
		}
	}

	return(GLV_OK);
}

static int  wiget_value_cb_listBox_list(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "list";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_listBox_list: key error [%s][%s]\n",value->key,key_string);
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
        if((menu == 0) && (n > 0)){
            glv_menu_setItem(&user_data->menu,n,text,attr,next,functionId);
        }
	}
	return(GLV_OK);
}

static int  wiget_value_cb_listBox_FunctionId(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "select function id";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_listBox_FunctionId: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		value->n[0].v.int32 = glv_menu_getFunctionId(&user_data->menu,user_data->select);
	}else{
		// set
		user_data->select = glv_menu_searchFunctionId(&user_data->menu,value->n[0].v.int32);
	}
	return(GLV_OK);
}

static int  wiget_value_cb_listBox_ItemHeight(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "item height";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_listBox_ItemHeight: key error [%s][%s]\n",value->key,key_string);
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

#define DEFAULT_LIST_BOX_ITEM_HEIGHT	(30)

static int wiget_list_box_init(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	glv_allocUserData(wiget,sizeof(WIGET_LIST_BOX_USER_DATA_t));
	WIGET_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(wiget);

	int fontSize = 16;

	glv_menu_init(&user_data->menu);

#if 1
	glv_setAbstract(wiget,"list","リスト文字列を設定する","iiSiii",
								"0固定","リスト番号(1以上)","文字列","0固定","0固定","選択時に返却される番号");
	glv_setAbstract(wiget,"item height","リスト１アイテムの描画高さを設定する","i","高さ(dot)");
#endif

	glv_setValue(wiget,"list"				,"iiSiiiT",0,0,NULL,0,0,0,wiget_value_cb_listBox_list);

	glv_setValue(wiget,"select function id"	,"iT",0,wiget_value_cb_listBox_FunctionId);
	glv_setValue(wiget,"item height"		,"iT",fontSize+4,wiget_value_cb_listBox_ItemHeight);

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

#if 0	// test.test.test
	glv_setValue(wiget,"glv_getValue test","LiSR",-1 * 0x1234567800005678,100,"書式定義文字列とそれに引き続く可変長引数列を",3.1415);
#endif

	user_data->select		= 0;
	user_data->window_select_list		= NULL;
	user_data->window_select_list_id	= 0;
	user_data->item_height	= DEFAULT_LIST_BOX_ITEM_HEIGHT;

	return(GLV_OK);
}

static int wiget_list_box_terminate(glvWiget wiget)
{
	WIGET_LIST_BOX_USER_DATA_t *user_data = glv_getUserData(wiget);

	//printf("wiget_list_box_terminate\n");

	if(user_data->window_select_list != NULL){
		glvDestroyWindow(&user_data->window_select_list);
		printf("list_box window delete\n");
	}

	glv_menu_free(&user_data->menu);

	return(GLV_OK);
}

static const struct glv_wiget_listener _wiget_listBox_listener = {
	.attr			= (GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_FOCUS),
	.init			= wiget_list_box_init,
	.redraw			= wiget_list_box_redraw,
	.mousePointer	= wiget_list_box_mousePointer,
	.terminate		= wiget_list_box_terminate
};
const struct glv_wiget_listener *wiget_listBox_listener = &_wiget_listBox_listener;
