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

// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
typedef struct _wiget_slider_bar_user_data{
	int min;
	int max;
	int position;
	int range;
} WIGET_SLIDER_BAR_USER_DATA_t;

static int wiget_slider_bar_input(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int kind,int state,uint32_t kyesym,int *utf32,uint8_t *attr,int length)
{
	WIGET_SLIDER_BAR_USER_DATA_t *user_data = glv_getUserData(wiget);
	int drawFlag=0;
	if((kind == GLV_KEY_KIND_CTRL) && (kyesym == XKB_KEY_Left)){
		// Move left, left arrow
		if(user_data->position > user_data->min){
			user_data->position--;
			drawFlag = 1;
		}
	}else if((kind == GLV_KEY_KIND_CTRL) && (kyesym == XKB_KEY_Right)){
		// Move right, right arrow
		if(user_data->position < user_data->max){
			user_data->position++;
			drawFlag = 1;
		}
	}
	if(drawFlag == 1){
		glvOnReDraw(glv_win);
	}
	return(GLV_OK);
}

static int wiget_slider_bar_mousePointer(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int glv_mouse_event_type,glvTime glv_mouse_event_time,int glv_mouse_event_x,int glv_mouse_event_y,int pointer_left_stat)
{
	GLV_WIGET_GEOMETRY_t	geometry;
	WIGET_SLIDER_BAR_USER_DATA_t *user_data = glv_getUserData(wiget);
	glvWiget_getWigetGeometry(wiget,&geometry);

	user_data->position = user_data->range * glv_mouse_event_x / (geometry.width + geometry.width / user_data->range) + user_data->min;

	printf("slider_bar_mouse (%d,%d) position = %d\n",glv_mouse_event_x,glv_mouse_event_y,user_data->position);

	if(glv_mouse_event_type == GLV_MOUSE_EVENT_MOTION){
		glvOnReDraw(glv_win);
	}
	return(GLV_OK);
}

static int wiget_slider_bar_redraw(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	WIGET_SLIDER_BAR_USER_DATA_t *user_data = glv_getUserData(wiget);
	GLV_WIGET_GEOMETRY_t	geometry;
	int		kind;
	float x,y,w,h;
	GLV_WIGET_STATUS_t	wigetStatus;

	glvSheet_getSelectWigetStatus(sheet,&wigetStatus);
	glvWiget_getWigetGeometry(wiget,&geometry);

	//scale	= geometry.scale;
	w		= geometry.width;
	h		= geometry.height;
	x		= geometry.x;
	y		= geometry.y;

	uint32_t	gFocusBkgdColor;
	uint32_t	gPressBkgdColor;
	uint32_t	gReleaseBkgdColor;

	glv_getValue(wiget,"gFocusBkgdColor"	,"C",&gFocusBkgdColor);
	glv_getValue(wiget,"gPressBkgdColor"	,"C",&gPressBkgdColor);
	glv_getValue(wiget,"gReleaseBkgdColor"	,"C",&gReleaseBkgdColor);

	glPushMatrix();
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	kind =  glvWiget_kindSelectWigetStatus(wiget,&wigetStatus);

	switch(kind){
		case GLV_WIGET_STATUS_FOCUS:
			glColor4f_RGBA(gFocusBkgdColor);
			glvGl_drawRectangle(x,y,w,h);
			break;
		case GLV_WIGET_STATUS_PRESS:
			glColor4f_RGBA(gPressBkgdColor);
			glvGl_drawRectangle(x,y,w,h);
			break;
		case GLV_WIGET_STATUS_RELEASE:
		default:
			glColor4f_RGBA(gReleaseBkgdColor);
			glvGl_drawRectangle(x,y,w,h);
			break;
	}

	//glColor4f(1.0, 1.0, 1.0, 1.0);
	glColor4f_RGBA(GLV_SET_RGBA(255,212,  0,255));
	glvGl_drawRectangle(geometry.x + (user_data->position - user_data->min) * geometry.width / (user_data->range - user_data->min) - 20/2,geometry.y,20,geometry.height);

	//printf("wiget_slider_bar_redraw\n");

	//glDisable(GL_BLEND);
	glPopMatrix();

	return(GLV_OK);
}

static int wiget_value_cb_sliderBar_Params(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_SLIDER_BAR_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "params";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_sliderBar_Position: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		value->n[0].v.int32 = user_data->min;
		value->n[1].v.int32 = user_data->max;
	}else{
		// set
		user_data->min = value->n[0].v.int32;
		user_data->max = value->n[1].v.int32;
		user_data->range = user_data->max - user_data->min + 1;
	}
	return(GLV_OK);
}

static int  wiget_value_cb_sliderBar_Position(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_SLIDER_BAR_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "position";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_sliderBar_Position: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		value->n[0].v.int32 = user_data->position;
	}else{
		// set
		user_data->position = value->n[0].v.int32;
	}
	return(GLV_OK);
}

static int wiget_slider_bar_init(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	glv_allocUserData(wiget,sizeof(WIGET_SLIDER_BAR_USER_DATA_t));
	WIGET_SLIDER_BAR_USER_DATA_t *user_data = glv_getUserData(wiget);

	user_data->min = 1;
	user_data->position = 5;
	user_data->max = 9;
	user_data->range = user_data->max - user_data->min + 1;

	glv_setValue(wiget,"gFocusBkgdColor"	,"C",GLV_SET_RGBA(255,  0,  0,255));
	glv_setValue(wiget,"gPressBkgdColor"	,"C",GLV_SET_RGBA(  0,255,  0,255));
	glv_setValue(wiget,"gReleaseBkgdColor"	,"C",GLV_SET_RGBA(178,178,178,255));
	glv_setValue(wiget,"position"			,"iT",user_data->position,wiget_value_cb_sliderBar_Position);
	glv_setValue(wiget,"params"				,"iiT",user_data->min,user_data->max,wiget_value_cb_sliderBar_Params);

	return(GLV_OK);
}

static const struct glv_wiget_listener _wiget_sliderBar_listener = {
	.attr			= (GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_FOCUS | GLV_WIGET_ATTR_TEXT_INPUT_FOCUS),
	.init			= wiget_slider_bar_init,
	.redraw			= wiget_slider_bar_redraw,
	.mousePointer	= wiget_slider_bar_mousePointer,
	.input			= wiget_slider_bar_input,
	.terminate		= NULL,
};
const struct glv_wiget_listener *wiget_sliderBar_listener = &_wiget_sliderBar_listener;

// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
typedef struct _wiget_check_box_user_data{
	int			check;
} WIGET_CHECK_BOX_USER_DATA_t;

static int wiget_check_box_redraw(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	WIGET_CHECK_BOX_USER_DATA_t *user_data = glv_getUserData(wiget);
	GLV_WIGET_GEOMETRY_t	geometry;
	int		kind;
	float x,y,w,h;
	GLV_WIGET_STATUS_t	wigetStatus;

	glvSheet_getSelectWigetStatus(sheet,&wigetStatus);
	glvWiget_getWigetGeometry(wiget,&geometry);

	//scale	= geometry.scale;
	w		= geometry.width;
	h		= geometry.height;
	x		= geometry.x;
	y		= geometry.y;

	int			check				= user_data->check;

	uint32_t	gFocusBkgdColor;
	uint32_t	gPressBkgdColor;
	uint32_t	gReleaseBkgdColor;
	uint32_t	gBoxColor;

	glv_getValue(wiget,"gFocusBkgdColor"	,"C",&gFocusBkgdColor);
	glv_getValue(wiget,"gPressBkgdColor"	,"C",&gPressBkgdColor);
	glv_getValue(wiget,"gReleaseBkgdColor"	,"C",&gReleaseBkgdColor);
	glv_getValue(wiget,"gBoxColor"			,"C",&gBoxColor);

	glPushMatrix();
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	kind =  glvWiget_kindSelectWigetStatus(wiget,&wigetStatus);

	switch(kind){
		case GLV_WIGET_STATUS_FOCUS:
			glColor4f_RGBA(gFocusBkgdColor);
			glvGl_drawRectangle(x,y,w,h);
			break;
		case GLV_WIGET_STATUS_PRESS:
			glColor4f_RGBA(gPressBkgdColor);
			glvGl_drawRectangle(x,y,w,h);
			if(check == 0){
				check = 1;
			}else{
				check = 0;
			}
			break;
		case GLV_WIGET_STATUS_RELEASE:
		default:
			glColor4f_RGBA(gReleaseBkgdColor);
			glvGl_drawRectangle(x,y,w,h);
			break;
	}

	if(check == 1){
		int offset = 2;
		GLV_T_POINT_t point[2];
		glColor4f_RGBA(gBoxColor);
		//glColor4f(0.0, 0.0, 0.0, 1.0);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		point[0].x = x + offset;
		point[0].y = y + offset;
		point[1].x = point[0].x + w - offset * 2;
		point[1].y = point[0].y + h - offset * 2;
		glvGl_drawLines(point,2,4);
		point[0].x = x + offset;
		point[0].y = y + h - offset * 2 + 1;
		point[1].x = point[0].x + w - offset * 2;
		point[1].y = y + offset - 1;
		glvGl_drawLines(point,2,4);

		glDisable(GL_BLEND);
	}

	//glDisable(GL_BLEND);
	glPopMatrix();

	return(GLV_OK);
}

static int  wiget_value_cb_checkBox(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_CHECK_BOX_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "check";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_checkBox: key error [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		value->n[0].v.int32 = user_data->check;
	}else{
		// set
		user_data->check = value->n[0].v.int32;
	}
	return(GLV_OK);
}

static int wiget_check_box_init(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	glv_allocUserData(wiget,sizeof(WIGET_CHECK_BOX_USER_DATA_t));
	WIGET_CHECK_BOX_USER_DATA_t *user_data = glv_getUserData(wiget);

	user_data->check				= 0;

	glv_setValue(wiget,"gFocusBkgdColor"	,"C",GLV_SET_RGBA(255,  0,  0,255));
	glv_setValue(wiget,"gPressBkgdColor"	,"C",GLV_SET_RGBA(  0,255,  0,255));
	glv_setValue(wiget,"gReleaseBkgdColor"	,"C",GLV_SET_RGBA(178,178,178,255));
	glv_setValue(wiget,"gBoxColor"			,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"check"				,"iT",user_data->check,wiget_value_cb_checkBox);

	return(GLV_OK);
}

static const struct glv_wiget_listener _wiget_checkBox_listener = {
	.attr		= (GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_FOCUS),
	.init		= wiget_check_box_init,
	.redraw		= wiget_check_box_redraw,
};
const struct glv_wiget_listener *wiget_checkBox_listener = &_wiget_checkBox_listener;
