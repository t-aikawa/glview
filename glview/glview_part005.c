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

#define GLV_W_CMD_MENU_MAX	(12)

typedef struct _wiget_cmd_menu_user_data{
	int				pos_x[GLV_W_CMD_MENU_MAX];
	int				focus;
	int				menuNumber;
	GLV_W_MENU_t	cmdMenu[GLV_W_CMD_MENU_MAX];
} WIGET_CMD_MENU_USER_DATA_t;

// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
static int wiget_cmd_menu_mousePointer(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int glv_mouse_event_type,glvTime glv_mouse_event_time,int glv_mouse_event_x,int glv_mouse_event_y,int pointer_left_stat)
{
	GLV_WIGET_GEOMETRY_t	geometry;
	WIGET_CMD_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	float x,w,h;
	int		i,focus;
	int		old_focus;
	int		start_x;

	glvWiget_getWigetGeometry(wiget,&geometry);

	w	= geometry.width;
	h	= geometry.height;
	x	= geometry.x;
	//y	= geometry.y;

	//printf("cmd_menu_select_text_mouse glv_mouse_event_y = %d\n",glv_mouse_event_y);

	old_focus = user_data->focus;

	focus = -1;
	start_x = 0;
	if((0 <= glv_mouse_event_x) && (w >= glv_mouse_event_x) && (0 <= glv_mouse_event_y) && (h >= glv_mouse_event_y)){
		//areaFlag = 1;
		for(i=0;i<user_data->cmdMenu[user_data->menuNumber].num;i++){
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

	//if(glv_mouse_event_type == GLV_MOUSE_EVENT_PRESS){

	return(GLV_OK);
}

static int wiget_cmd_menu_redraw(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	WIGET_CMD_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
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
	uint32_t	gSelectColor;
	uint32_t	gFontColor;
	uint32_t	gBkgdColor;
	int			fontSize;
	int			menuNumber;
	int			next;
	int			redraw=0;

	glv_getValue(wiget,"gFocusBkgdColor"	,"C",&gFocusBkgdColor);
	glv_getValue(wiget,"gPressBkgdColor"	,"C",&gPressBkgdColor);
	glv_getValue(wiget,"gReleaseBkgdColor"	,"C",&gReleaseBkgdColor);
	glv_getValue(wiget,"gSelectColor"		,"C",&gSelectColor);
	glv_getValue(wiget,"gFontColor"			,"C",&gFontColor);
	glv_getValue(wiget,"gBkgdColor"			,"C",&gBkgdColor);
	glv_getValue(wiget,"font size"			,"i",&fontSize);

	glv_getValue(glv_win,"cmdMenu number"	,"i",&menuNumber,&next);

	// printf("wiget_cmd_menu_redraw: menuNumber = %d\n",menuNumber);

	// 	メニューが選択できなかったときは、直前のメニューを表示する
	// append 2021/12/20 by T.Aikawa
	if(menuNumber == -1){
		menuNumber = user_data->menuNumber;
	}

	int focus	= user_data->focus;

	if(user_data->menuNumber != menuNumber){
		int i;
		//printf("change cmd menu\n");
		for(i=0;i<GLV_W_CMD_MENU_MAX;i++){
			user_data->pos_x[i] = 0;
		}
		redraw = 1;
	}
	user_data->menuNumber = menuNumber;

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
#if 0
	if(attr == GLV_FONT_LEFT){
		glvFont_SetPosition(x + 2,y + 2);
	}else{
		glvFont_SetPosition(x + w / 2,y + fontSize / 2);
	}
#endif
	glvFont_setColor(gFontColor);
	glvFont_setBkgdColor(gBkgdColor);

	start_x = x;
	for(num=0;num<user_data->cmdMenu[user_data->menuNumber].num;num++){
		if(attr == GLV_FONT_LEFT){
			glvFont_SetPosition(start_x + 2,y + 2);
		}else{
			glvFont_SetPosition(start_x + w / 2,y + fontSize / 2);
		}
		if(redraw == 1){
		}else{
			glColor4f_RGBA(GLV_SET_RGBA(   0,  0,  0,255));
		
			if(user_data->pos_x[num] > (frameInfo.left_size + frameInfo.inner_width /* + frameInfo.right_edge_size */)){
				width = frameInfo.left_size + frameInfo.inner_width /* + frameInfo.right_edge_size */ - start_x - 2;
			}else{
				width = user_data->pos_x[num] - start_x;
			}
			glvGl_drawRectangle(start_x,y,width,h);
		}
		if(focus == num){
			//printf("cmd_menu_select_text_redraw focus = %d , kind = %d\n",focus,kind);
			//glColor4f_RGBA(GLV_SET_RGBA(   0,  0,  0,255));
			//glvGl_drawRectangle(start_x,y,user_data->pos_x[num] - start_x,h);
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
			//	glvGl_drawRectangle(start_x+1,y+1,user_data->pos_x[num] - start_x-2,h-2);
			//}
		}else{
			//printf("cmd_menu_select_text_redraw other = %d , kind = %d\n",num,kind);
			glvFont_setColor(gFontColor);
			gBkgdColor = gReleaseBkgdColor;
			glColor4f_RGBA(gReleaseBkgdColor);
			//if(user_data->pos_x[num] > 0){
			//	glvGl_drawRectangle(start_x+1,y+1,user_data->pos_x[num] - start_x-2,h-2);
			//}
		}
		
		if(user_data->pos_x[num] > 0){
#if 1
			if(user_data->pos_x[num] > (frameInfo.left_size + frameInfo.inner_width /* + frameInfo.right_edge_size */)){
				width = frameInfo.left_size + frameInfo.inner_width + /* frameInfo.right_edge_size */ - start_x - 2;
			}else{
				width = user_data->pos_x[num] - start_x - 2;
			}
#else
			width = user_data->pos_x[num] - start_x - 2;
#endif
			glvGl_drawRectangle(start_x+1,y+1,width,h-2);
		}
		glvFont_setBkgdColor(gBkgdColor);
		if(user_data->pos_x[num] > (frameInfo.left_size + frameInfo.inner_width  /* + frameInfo.right_edge_size */)){
			glvFont_printf("%s","");
			break;
		}else{
			glvFont_printf(" %s ",user_data->cmdMenu[user_data->menuNumber].item[num].text);
		}
		glvFont_GetPosition(&x_pos,&y_pos);
		user_data->pos_x[num] = x_pos + 2;
		start_x = user_data->pos_x[num] - 1;
		if((num > 0) && (((num + 1) % 4) == 0)){
			start_x += 8;
		}
	}

	//glDisable(GL_BLEND);
	glPopMatrix();

	if(redraw == 1){
		glvOnReDraw(glv_win);
	}
	return(GLV_OK);
}

static int  wiget_value_cb_cmdMenu_list(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_CMD_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "list";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_cmdMenu_list: key error [%s][%s]\n",value->key,key_string);
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

		glv_menu_setItem(&user_data->cmdMenu[menu],n,text,attr,next,functionId);
	}
	return(GLV_OK);
}

static int  wiget_value_cb_cmdMenu_FunctionId(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_CMD_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "select function id";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_cmdMenu_FunctionId: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		//value->n[0].v.int32 = glv_menu_getFunctionId(&user_data->menu,user_data->select);

		if(user_data->focus > -1){
			value->n[0].v.int32 = glv_menu_getFunctionId(&user_data->cmdMenu[user_data->menuNumber],user_data->focus);
			value->n[1].v.int32 = glv_menu_getNext(&user_data->cmdMenu[user_data->menuNumber],user_data->focus);
		}else{
			value->n[0].v.int32 = -1;
			value->n[1].v.int32 = -1;
		}
	}else{
		// set
		//user_data->select = glv_menu_searchFunctionId(&user_data->menu,value->n[0].v.int32);
	}
	return(GLV_OK);
}

static int  wiget_value_cb_cmdMenu_focus(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_CMD_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "function key";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_cmdMenu_focus: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		value->n[0].v.int32 = user_data->focus;
	}else{
		// set
		int focus = value->n[0].v.int32;
		if((focus >= -1) && (focus < user_data->cmdMenu[user_data->menuNumber].num)){
			user_data->focus = focus;
		}else{
			return(GLV_ERROR);
		}
	}
	return(GLV_OK);
}

static int wiget_cmd_menu_init(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	glv_allocUserData(wiget,sizeof(WIGET_CMD_MENU_USER_DATA_t));
	WIGET_CMD_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	int i;

	int fontSize = 14;

	for(i=0;i<GLV_W_CMD_MENU_MAX;i++){
		glv_menu_init(&user_data->cmdMenu[i]);
	}

	glv_setValue(glv_getDisplay(glv_win),"cmdMenu wiget","P",wiget);

	user_data->menuNumber = 0;

	glv_setValue(glv_win,"cmdMenu number"	,"i",user_data->menuNumber);

	glv_setValue(wiget,"function key"		,"iT",-1,wiget_value_cb_cmdMenu_focus);
	glv_setValue(wiget,"list"				,"iiSiiiT",0,0,NULL,0,0,0,wiget_value_cb_cmdMenu_list);

	glv_setValue(wiget,"select function id"	,"iiT",0,0,wiget_value_cb_cmdMenu_FunctionId);

	glv_setValue(wiget,"gFocusBkgdColor"	,"C",GLV_SET_RGBA(255,  0,  0,255));
	glv_setValue(wiget,"gPressBkgdColor"	,"C",GLV_SET_RGBA(  0,255,  0,255));
	glv_setValue(wiget,"gReleaseBkgdColor"	,"C",GLV_SET_RGBA(178,178,178,255));
	glv_setValue(wiget,"gSelectColor"		,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"gFontColor"			,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"gBkgdColor"			,"C",GLV_SET_RGBA(  0,  0,  0,  0));
	glv_setValue(wiget,"font size"			,"i",fontSize);

	user_data->focus	= -1;

	return(GLV_OK);
}

static int wiget_cmd_menu_terminate(glvWiget wiget)
{
	WIGET_CMD_MENU_USER_DATA_t *user_data = glv_getUserData(wiget);
	int i;

	glv_setValue(glv_getDisplay(wiget),"cmdMenu wiget","P",NULL);

	for(i=0;i<GLV_W_CMD_MENU_MAX;i++){
		glv_menu_free(&user_data->cmdMenu[i]);
	}

	//printf("wiget_cmd_menu_terminate\n");

	return(GLV_OK);
}

static const struct glv_wiget_listener _wiget_cmdMenu_listener = {
	.attr			= (GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_FOCUS | GLV_WIGET_ATTR_POINTER_MOTION),
	.init			= wiget_cmd_menu_init,
	.redraw			= wiget_cmd_menu_redraw,
	.mousePointer	= wiget_cmd_menu_mousePointer,
	.terminate		= wiget_cmd_menu_terminate
};
const struct glv_wiget_listener *wiget_cmdMenu_listener = &_wiget_cmdMenu_listener;
