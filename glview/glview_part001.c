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

typedef struct _wiget_text_input_user_data{
	char	*utf8_string;
	int		*utf32_string;
	int		utf32_length;
	int16_t	*advance_x;
	int		cursorIndex;
	int		cursorPos;

	int		*utf32_preedit_string;
	uint8_t	*utf32_preedit_attr;
	int		utf32_preedit_length;
	int		preedit_cursorIndex;
	int		preedit_cursorPos;

	int		select_mode;
	int		select_start,select_end;
} WIGET_TEXT_INPUT_USER_DATA_t;

static int wiget_text_input_text_focus(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int focus_stat,glvWiget in_wiget)
{
	WIGET_TEXT_INPUT_USER_DATA_t *user_data = glv_getUserData(wiget);

	GLV_IF_DEBUG_IME_INPUT printf("wiget_text_input_text_focus\n");
	if(focus_stat == GLV_STAT_OUT_FOCUS){
		user_data->select_start = user_data->select_end = 0;
		user_data->select_mode = 0;
		user_data->utf32_preedit_length = 0;
		user_data->cursorIndex = -1;
		glvOnReDraw(glv_win);	
	}
	return(GLV_OK);
}

static int wiget_text_input_text_input(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int kind,int state,uint32_t kyesym,int *utf32,uint8_t *attr,int length)
{
	WIGET_TEXT_INPUT_USER_DATA_t *user_data = glv_getUserData(wiget);
	int i,string_changeFlag=0,del_flag=0;
	int	insert_mode=glv_isInsertMode(glv_win);
	int *realloc_ptr;

	GLV_IF_DEBUG_KB_INPUT {
		//printf("wiget_text_input_text_input\n");

		if(kind == GLV_KEY_KIND_ASCII) printf(GLV_DEBUG_KB_INPUT_COLOR"GLV_KEY_KIND_ASCII "GLV_DEBUG_END_COLOR);
		if(kind == GLV_KEY_KIND_CTRL) printf(GLV_DEBUG_KB_INPUT_COLOR"GLV_KEY_KIND_CTRL "GLV_DEBUG_END_COLOR);
		if(kind == GLV_KEY_KIND_IM) printf(GLV_DEBUG_KB_INPUT_COLOR"GLV_KEY_KIND_IM "GLV_DEBUG_END_COLOR);

		if(state == GLV_KEY_STATE_IM_PREEDIT) printf(GLV_DEBUG_KB_INPUT_COLOR"GLV_KEY_STATE_IM_PREEDIT "GLV_DEBUG_END_COLOR);
		if(state == GLV_KEY_STATE_IM_COMMIT) printf(GLV_DEBUG_KB_INPUT_COLOR"GLV_KEY_STATE_IM_COMMIT "GLV_DEBUG_END_COLOR);
		if(state == GLV_KEY_STATE_IM_RESET) printf(GLV_DEBUG_KB_INPUT_COLOR"GLV_KEY_STATE_IM_RESET "GLV_DEBUG_END_COLOR);

		if(kind == GLV_KEY_KIND_ASCII){
			printf(GLV_DEBUG_KB_INPUT_COLOR"[%c]\n"GLV_DEBUG_END_COLOR,utf32[0]);
		}
		if(kind == GLV_KEY_KIND_CTRL){
			printf(GLV_DEBUG_KB_INPUT_COLOR"[%x]\n"GLV_DEBUG_END_COLOR,kyesym);
		}
		if(kind == GLV_KEY_KIND_IM){
			char *utf8;
			utf8 = malloc(length * 4 + 1);
			printf(GLV_DEBUG_KB_INPUT_COLOR"GLV_KEY_KIND_IM length = %d"GLV_DEBUG_END_COLOR,length);
			glvFont_utf32_to_string(utf32,length,utf8,(length * 4));
			printf(GLV_DEBUG_KB_INPUT_COLOR"[%s]\n"GLV_DEBUG_END_COLOR,utf8);
			free(utf8);
		}
	}

	if((kind == GLV_KEY_KIND_IM) && (state == GLV_KEY_STATE_IM_RESET)){
		user_data->select_start = user_data->select_end = 0;
		user_data->select_mode = 0;
		user_data->utf32_preedit_length = 0;
		user_data->cursorIndex = -1;
	}

	if(user_data->select_mode == 1){
		if((kind == GLV_KEY_KIND_ASCII) ||
		((kind == GLV_KEY_KIND_IM) && (state == GLV_KEY_STATE_IM_COMMIT)) ||
		((kind == GLV_KEY_KIND_CTRL) && (kyesym == XKB_KEY_BackSpace))		){
			int start,end,num,i;
			if(user_data->select_start < user_data->select_end){
				start = user_data->select_start;
				end = user_data->select_end;
			}else{
				end = user_data->select_start;
				start = user_data->select_end;
			}
			if(end >= user_data->utf32_length){
				end = user_data->utf32_length - 1;
			}
			GLV_IF_DEBUG_IME_INPUT printf("wiget_text_input_text_input delete %d -> %d\n",start,end);
			num = end - start + 1;
			if(num > 0){
				user_data->cursorIndex = end + 1;
				for(i=0;i<num;i++){
					user_data->cursorIndex = glvFont_deleteCharacter(user_data->utf32_string,&user_data->utf32_length,user_data->cursorIndex);
				}
				insert_mode = 1;
				del_flag = 1;
				string_changeFlag = 1;
				user_data->preedit_cursorIndex = user_data->cursorIndex;
				user_data->preedit_cursorPos   = user_data->cursorPos;
			}
			user_data->select_start = user_data->select_end = 0;
			user_data->select_mode = 0;
		}
	}

	if((kind == GLV_KEY_KIND_IM) && ((state == GLV_KEY_STATE_IM_PREEDIT) || (state == GLV_KEY_STATE_IM_COMMIT))){
		if((state == GLV_KEY_STATE_IM_PREEDIT) && (user_data->utf32_preedit_length == 0)){
			user_data->preedit_cursorIndex = user_data->cursorIndex;
			user_data->preedit_cursorPos   = user_data->cursorPos;
		}
		if(user_data->utf32_preedit_string != NULL){
			free(user_data->utf32_preedit_string);
			user_data->utf32_preedit_string = NULL;
		}
		if(user_data->utf32_preedit_attr != NULL){
			free(user_data->utf32_preedit_attr);
			user_data->utf32_preedit_attr = NULL;
		}
		if(length > 0){
			if(utf32 != NULL){
				user_data->utf32_preedit_string = malloc(sizeof(int) * (length + 1));
				memcpy(user_data->utf32_preedit_string,utf32,length * sizeof(int));
			}
			if(attr  != NULL){
				user_data->utf32_preedit_attr = malloc(sizeof(uint8_t) * (length + 1));
				memcpy(user_data->utf32_preedit_attr,attr,length * sizeof(uint8_t));
			}
		}
		user_data->utf32_preedit_length = length;
	}

	if(kind == GLV_KEY_KIND_ASCII){
		if(insert_mode == 1){
			// insert text
			realloc_ptr = realloc(user_data->utf32_string,sizeof(int) * (user_data->utf32_length+2));
			if(realloc_ptr != NULL){
				user_data->utf32_string = realloc_ptr;
				user_data->cursorIndex = glvFont_insertCharacter(user_data->utf32_string,&user_data->utf32_length,utf32[0],user_data->cursorIndex);
			}
		}else{
			realloc_ptr = realloc(user_data->utf32_string,sizeof(int) * (user_data->utf32_length+2));
			if(realloc_ptr != NULL){
				user_data->utf32_string = realloc_ptr;
				user_data->cursorIndex = glvFont_setCharacter(user_data->utf32_string,&user_data->utf32_length,utf32[0],user_data->cursorIndex);
			}
		}
		string_changeFlag = 1;
	}
	if((kind == GLV_KEY_KIND_IM) && (state == GLV_KEY_STATE_IM_COMMIT)){
		if(insert_mode == 1){
			// insert text
			realloc_ptr = realloc(user_data->utf32_string,sizeof(int) * (user_data->utf32_length+1+length));
			if(realloc_ptr != NULL){
				user_data->utf32_string = realloc_ptr;
				for(i=0;i<length;i++){
					user_data->preedit_cursorIndex = glvFont_insertCharacter(user_data->utf32_string,&user_data->utf32_length,user_data->utf32_preedit_string[i],user_data->preedit_cursorIndex);
				}
			}
		}else{
			if(length > 0){
				realloc_ptr = realloc(user_data->utf32_string,sizeof(int) * (user_data->utf32_length+1+length));
				if(realloc_ptr != NULL){
					user_data->utf32_string = realloc_ptr;
					user_data->preedit_cursorIndex = glvFont_setCharacter(user_data->utf32_string,&user_data->utf32_length,utf32[0],user_data->preedit_cursorIndex);				
					for(i=1;i<length;i++){
						user_data->preedit_cursorIndex = glvFont_insertCharacter(user_data->utf32_string,&user_data->utf32_length,user_data->utf32_preedit_string[i],user_data->preedit_cursorIndex);
					}
				}
			}
		}
		user_data->cursorIndex = user_data->preedit_cursorIndex;
		user_data->utf32_preedit_length = 0;
		string_changeFlag = 1;
	}
	if((kind == GLV_KEY_KIND_CTRL) && (kyesym == XKB_KEY_BackSpace) && (del_flag == 0)){
		// delete char
		user_data->cursorIndex = glvFont_deleteCharacter(user_data->utf32_string,&user_data->utf32_length,user_data->cursorIndex);
		string_changeFlag = 1;
	}
	if((kind == GLV_KEY_KIND_CTRL) && (kyesym == XKB_KEY_Left)){
		// Move left, left arrow
		if(user_data->cursorIndex > 0) user_data->cursorIndex--;
		user_data->select_start = user_data->select_end = 0;
		user_data->select_mode = 0;
		string_changeFlag = 1;
	}
	if((kind == GLV_KEY_KIND_CTRL) && (kyesym == XKB_KEY_Right)){
		// Move right, right arrow
		if(user_data->cursorIndex < user_data->utf32_length) user_data->cursorIndex++;
		user_data->select_start = user_data->select_end = 0;
		user_data->select_mode = 0;
		string_changeFlag = 1;
	}
	if((kind == GLV_KEY_KIND_CTRL) && (kyesym == XKB_KEY_Insert)){
		// Insert, insert here
	}

	if(string_changeFlag == 1){
		if(user_data->utf8_string != NULL){
			free(user_data->utf8_string);
			user_data->utf8_string = NULL;
		}
		user_data->utf8_string = malloc(user_data->utf32_length * 4 + 1);
		glvFont_utf32_to_string(user_data->utf32_string,user_data->utf32_length,user_data->utf8_string,(user_data->utf32_length * 4));
	}

	glvOnReDraw(glv_win);

	return(GLV_OK);
}

static int wiget_text_input_mousePointer(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int glv_mouse_event_type,glvTime glv_mouse_event_time,int glv_mouse_event_x,int glv_mouse_event_y,int pointer_left_stat)
{
	GLV_WIGET_GEOMETRY_t	geometry;
	WIGET_TEXT_INPUT_USER_DATA_t *user_data = glv_getUserData(wiget);
	glvWiget_getWigetGeometry(wiget,&geometry);

	//printf("text_input_mouse (%d,%d) position = %d\n",glv_mouse_event_x,glv_mouse_event_y,user_data->position);
	if(user_data->utf32_preedit_length > 0){
		return(GLV_OK);
	}
	user_data->cursorIndex = glvFont_getCursorPosition(user_data->utf32_string,user_data->utf32_length,user_data->advance_x,glv_mouse_event_x);
	user_data->cursorPos = user_data->advance_x[user_data->cursorIndex];

	// IME候補表示エリア位置設定
	glvWiget_setIMECandidatePotition(wiget,user_data->cursorPos,geometry.height);

	//printf("user_data->cursorIndex (%d) user_data->cursorPos (%d)\n",user_data->cursorIndex,user_data->cursorPos);
	//printf("glv_mouse_event_type %d\n",glv_mouse_event_type);
	if(glv_mouse_event_type == GLV_MOUSE_EVENT_PRESS){
		user_data->select_end = user_data->select_start = user_data->cursorIndex;
		user_data->select_mode = 0;
		user_data->preedit_cursorIndex = user_data->cursorIndex;
		user_data->preedit_cursorPos   = user_data->cursorPos;
	}

	if(glv_mouse_event_type == GLV_MOUSE_EVENT_RELEASE){
		if(user_data->select_mode == 1){
			int start,end,num;
			if(user_data->select_start < user_data->select_end){
				start = user_data->select_start;
				end = user_data->select_end;
			}else{
				end = user_data->select_start;
				start = user_data->select_end;
			}
			if(end >= user_data->utf32_length){
				end = user_data->utf32_length - 1;
			}
			num = end - start + 1;
			if(num < 1){
				user_data->select_end = user_data->select_start = user_data->cursorIndex;
				user_data->select_mode = 0;
			}else{
				user_data->cursorIndex = end + 1;
				GLV_IF_DEBUG_IME_INPUT {
					char *utf8;
					utf8 = malloc(num * 4 + 1);
					glvFont_utf32_to_string(user_data->utf32_string + start,num,utf8,num * 4);
					printf("text_input_mouse selection %d -> %d [%s]\n",start,end,utf8);
					free(utf8);
				}
			}
		}
	}

	if(glv_mouse_event_type == GLV_MOUSE_EVENT_MOTION){
		if(pointer_left_stat == GLV_MOUSE_EVENT_LEFT_PRESS){
			user_data->select_end = user_data->cursorIndex;
			/* if(user_data->select_end != user_data->select_start) */{
				user_data->select_mode = 1;
			}
		}
		glvOnReDraw(glv_win);
	}
	//printf("select (%d)->(%d)\n",user_data->select_start,user_data->select_end);
	return(GLV_OK);
}

static int wiget_text_input_mouseButton(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int glv_mouse_event_type,glvTime glv_mouse_event_time,int glv_mouse_event_x,int glv_mouse_event_y,int pointer_stat)
{
	GLV_IF_DEBUG_IME_INPUT {
		switch(glv_mouse_event_type){
			case GLV_MOUSE_EVENT_RELEASE:
			printf("GLV_MOUSE_EVENT_RELEASE ");
			break;
			case GLV_MOUSE_EVENT_PRESS:
			printf("GLV_MOUSE_EVENT_PRESS ");
			break;
			case GLV_MOUSE_EVENT_MOTION:
			printf("GLV_MOUSE_EVENT_MOTION ");
			break;
		}

		switch(pointer_stat){
		case GLV_MOUSE_EVENT_LEFT_RELEASE:
			printf("GLV_MOUSE_EVENT_LEFT_RELEASE\n");
			break;
			case GLV_MOUSE_EVENT_LEFT_PRESS:
			printf("GLV_MOUSE_EVENT_LEFT_PRESS\n");
			break;
			case GLV_MOUSE_EVENT_MIDDLE_RELEASE:
			printf("GLV_MOUSE_EVENT_MIDDLE_RELEASE\n");
			break;
			case GLV_MOUSE_EVENT_MIDDLE_PRESS:
			printf("GLV_MOUSE_EVENT_MIDDLE_PRESS\n");
			break;
			case GLV_MOUSE_EVENT_RIGHT_RELEASE:
			printf("GLV_MOUSE_EVENT_RIGHT_RELEASE\n");
			break;
			case GLV_MOUSE_EVENT_RIGHT_PRESS:
			printf("GLV_MOUSE_EVENT_RIGHT_PRESS\n");
			break;
			case GLV_MOUSE_EVENT_OTHER_RELEASE:
			printf("GLV_MOUSE_EVENT_OTHER_RELEASE\n");
			break;
			case GLV_MOUSE_EVENT_OTHER_PRESS:
			printf("GLV_MOUSE_EVENT_OTHER_PRESS\n");
			break;
		}
	}
	return(GLV_OK);
}

static int wiget_text_input_redraw(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	WIGET_TEXT_INPUT_USER_DATA_t *user_data = glv_getUserData(wiget);
	GLV_WIGET_GEOMETRY_t	geometry;
	int		kind,i,last;
	int		insert_mode=glv_isInsertMode(glv_win);
	int		select_mode=user_data->select_mode;
	float x,y,w,h;
	GLV_WIGET_STATUS_t	wigetStatus;
	char *str;
	int len;
	int16_t	work_advance_x[2];
	uint8_t	*utf32_string_attr;
	int candidateStat=0;

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
	uint32_t	gFontColor;
	uint32_t	gBkgdColor;
	uint32_t	gInsertModeCursorColor;
	uint32_t	gOverwriteModeCursorFontColor;
	uint32_t	gOverwriteModeCursorBkgdColor;
	uint32_t	gSerectionFontColor;
	uint32_t	gSerectionBkgdColor;
	uint32_t	gImeUnderlineColor;
	uint32_t	gImeUnderlineFontColor;
	uint32_t	gImeUnderlineBkgdColor;
	uint32_t	gImeCandidateFontColor;
	uint32_t	gImeCandidateBkgdColor;
	int			fontSize;

	glv_getValue(wiget,"gFocusBkgdColor"				,"C",&gFocusBkgdColor);
	glv_getValue(wiget,"gPressBkgdColor"				,"C",&gPressBkgdColor);
	glv_getValue(wiget,"gReleaseBkgdColor"				,"C",&gReleaseBkgdColor);
	glv_getValue(wiget,"gFontColor"						,"C",&gFontColor);
	glv_getValue(wiget,"gBkgdColor"						,"C",&gBkgdColor);
	glv_getValue(wiget,"gInsertModeCursorColor"			,"C",&gInsertModeCursorColor);
	glv_getValue(wiget,"gOverwriteModeCursorFontColor"	,"C",&gOverwriteModeCursorFontColor);
	glv_getValue(wiget,"gOverwriteModeCursorBkgdColor"	,"C",&gOverwriteModeCursorBkgdColor);
	glv_getValue(wiget,"gSerectionFontColor"			,"C",&gSerectionFontColor);
	glv_getValue(wiget,"gSerectionBkgdColor"			,"C",&gSerectionBkgdColor);
	glv_getValue(wiget,"gImeUnderlineColor"				,"C",&gImeUnderlineColor);
	glv_getValue(wiget,"gImeUnderlineFontColor"			,"C",&gImeUnderlineFontColor);
	glv_getValue(wiget,"gImeUnderlineBkgdColor"			,"C",&gImeUnderlineBkgdColor);
	glv_getValue(wiget,"gImeCandidateFontColor"			,"C",&gImeCandidateFontColor);
	glv_getValue(wiget,"gImeCandidateBkgdColor"			,"C",&gImeCandidateBkgdColor);
	glv_getValue(wiget,"font size"						,"i",&fontSize);

	glPushMatrix();
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	kind =  glvWiget_kindSelectWigetStatus(wiget,&wigetStatus);

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

	// selection
	utf32_string_attr = malloc(user_data->utf32_length+1);

	for(i=0;i<user_data->utf32_length;i++){
		utf32_string_attr[i] = 0;
	}
	if(user_data->select_mode == 0){
		if(user_data->cursorIndex >= 0){
			utf32_string_attr[user_data->cursorIndex] = 4;
		}
	}else{
		int start,end;
		if(user_data->select_start < user_data->select_end){
			start = user_data->select_start;
			end = user_data->select_end + 1;
		}else{
			end = user_data->select_start + 1;
			start = user_data->select_end;
		}
		if(end > user_data->utf32_length){
			end = user_data->utf32_length;
		}
		GLV_IF_DEBUG_IME_INPUT printf("select (%d)->(%d)\n",start,end);
		for(i=start;i<end;i++){
			utf32_string_attr[i] = 4;
		}
	}

	glvFont_SetStyle(GLV_FONT_NAME_TYPE1,fontSize,0.0f,0,GLV_FONT_NAME | GLV_FONT_NOMAL | GLV_FONT_SIZE | GLV_FONT_LEFT);
	glvFont_SetPosition(x,y);
	glvFont_SetBaseHeight(geometry.height);
	// -----------------------------------------------------------------------------------------------
	str = user_data->utf8_string;
	len = 0;
	if(str != NULL){
		len = strlen(str);
	}
	if(user_data->utf32_string != NULL){
		free(user_data->utf32_string);
		user_data->utf32_string = NULL;
	}
	user_data->utf32_string = malloc(sizeof(int) * (len + 1));
	user_data->utf32_length = glvFont_string_to_utf32(str,len,user_data->utf32_string,len);

	//printf("utf8_string[%s] strlen = %ld , user_data->utf32_length = %d\n",str,strlen(str),user_data->utf32_length);
	if(user_data->advance_x != NULL){
		free(user_data->advance_x);
		user_data->advance_x = NULL;
	}
	user_data->advance_x = malloc(sizeof(int16_t) * (user_data->utf32_length + user_data->utf32_preedit_length + 2));

	if(user_data->cursorIndex < 0){
		glvFont_setColor(gFontColor);
		glvFont_setBkgdColor(gBkgdColor);
		glvFont_DrawUTF32String(user_data->utf32_string,user_data->utf32_length,user_data->advance_x);
	}else{
		last = 0;
		user_data->advance_x[0] = 0;
		for(i=0;i<user_data->cursorIndex;i++){
			if(((select_mode == 1) || (insert_mode == 0)) && ((utf32_string_attr[i] & 4) == 4)){
				if(select_mode == 0){
					glvFont_setColor(gOverwriteModeCursorFontColor);
					glvFont_setBkgdColor(gOverwriteModeCursorBkgdColor);
				}else{
					glvFont_setColor(gSerectionFontColor);
					glvFont_setBkgdColor(gSerectionBkgdColor);
				}
			}else{
				glvFont_setColor(gFontColor);
				glvFont_setBkgdColor(gBkgdColor);
			}
			glvFont_DrawUTF32String(user_data->utf32_string+i,1,work_advance_x);
			user_data->advance_x[i + 1] = last + work_advance_x[1];
			last = user_data->advance_x[i + 1];
		}
		for(i=0;i<user_data->utf32_preedit_length;i++){
			if((user_data->utf32_preedit_attr[i] & 1) == 1){
				// IME候補表示エリア位置設定
				if(candidateStat == 0){
					glvWiget_setIMECandidatePotition(wiget,last,geometry.height);
					candidateStat = 1;
				}
			}
			if((user_data->utf32_preedit_attr[i] & 2) == 2){
				glvFont_setColor(gImeCandidateFontColor);
				glvFont_setBkgdColor(gImeCandidateBkgdColor);

				// IME候補表示エリア位置設定
				if(candidateStat < 2){
					glvWiget_setIMECandidatePotition(wiget,last,geometry.height);
					candidateStat = 2;
				}
			}else{
				glvFont_setColor(gImeUnderlineFontColor);
				glvFont_setBkgdColor(gImeUnderlineBkgdColor);
			}
			glvFont_DrawUTF32String(user_data->utf32_preedit_string+i,1,work_advance_x);
			user_data->advance_x[i + user_data->cursorIndex + 1] = last + work_advance_x[1];
			last = user_data->advance_x[user_data->cursorIndex + i + 1];
			if((user_data->utf32_preedit_attr[i] & 1) == 1){
				// draw under line
				int x1,x2;
				x1 = user_data->advance_x[i + user_data->cursorIndex    ];
				x2 = user_data->advance_x[i + user_data->cursorIndex + 1];
				glColor4f_RGBA(gImeUnderlineColor);
				glvGl_drawRectangle(x+x1,y+geometry.height - 5,(x2 - x1),1);
			}
		}
		for(i=user_data->cursorIndex;i<user_data->utf32_length;i++){
			if(((select_mode == 1) || (insert_mode == 0))  && ((utf32_string_attr[i] & 4) == 4)){
				if(select_mode == 0){
					glvFont_setColor(gOverwriteModeCursorFontColor);
					glvFont_setBkgdColor(gOverwriteModeCursorBkgdColor);
				}else{
					glvFont_setColor(gSerectionFontColor);
					glvFont_setBkgdColor(gSerectionBkgdColor);
				}
			}else{
				glvFont_setColor(gFontColor);
				glvFont_setBkgdColor(gBkgdColor);
			}
			glvFont_DrawUTF32String(user_data->utf32_string+i,1,work_advance_x);
			user_data->advance_x[i + 1 + user_data->utf32_preedit_length] = last + work_advance_x[1];
			last = user_data->advance_x[i + 1 + user_data->utf32_preedit_length];
		}
#if 0
		{
			int length = user_data->utf32_length + user_data->utf32_preedit_length;
			int i;
			printf("preedit advance_x(%d):",length);
			for(i=0;i<length+1;i++){
				printf("%d ",user_data->advance_x[i]);
			}
			printf("\n");
		}
#endif
	}

	GLV_IF_DEBUG_IME_INPUT {
		char *utf8;
		utf8 = malloc(user_data->utf32_length * 4 + 1);
		int length;
		length = glvFont_utf32_to_string(user_data->utf32_string,user_data->utf32_length,utf8,(user_data->utf32_length * 4));
		printf("glvFont_utf32_to_string %d [%s]\n",length,utf8);
	}

	if((select_mode == 0) && (user_data->cursorIndex >= 0)){
		glColor4f_RGBA(gInsertModeCursorColor);
		if(user_data->utf32_preedit_length == 0){
			if((insert_mode == 1) || (user_data->cursorIndex == user_data->utf32_length)){
				user_data->cursorPos = user_data->advance_x[user_data->cursorIndex];
				glvGl_drawRectangle(x+user_data->cursorPos,y+2,2,geometry.height-4);
			}
		}else{
			if((insert_mode == 1) || (user_data->cursorIndex == user_data->utf32_length)){
				user_data->preedit_cursorPos = user_data->advance_x[user_data->cursorIndex + user_data->utf32_preedit_length];
				glvGl_drawRectangle(x+user_data->preedit_cursorPos,y+2,2,geometry.height-4);
			}
		}
	}
	glvFont_SetBaseHeight(0);

	//printf("wiget_text_input_redraw\n");

	//glDisable(GL_BLEND);
	glPopMatrix();

	if(utf32_string_attr != NULL){
		free(utf32_string_attr);
	}

	return(GLV_OK);
}

static int  wiget_value_cb_textInput(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_TEXT_INPUT_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "text";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_textInput: key error. [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		if(value->n[0].v.string != NULL){
			free(value->n[0].v.string);
		}
		if(user_data->utf8_string != NULL){
			value->n[0].v.string = strdup(user_data->utf8_string);
		}else{
			value->n[0].v.string = NULL;
		}
	}else{
		// set
		if(user_data->utf8_string != NULL){
			free(user_data->utf8_string);
		}
		if(value->n[0].v.string != NULL){
			user_data->utf8_string = strdup(value->n[0].v.string);
		}else{
			user_data->utf8_string = NULL;
		}
	}
	return(GLV_OK);
}

static int wiget_text_input_init(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	glv_allocUserData(wiget,sizeof(WIGET_TEXT_INPUT_USER_DATA_t));
	WIGET_TEXT_INPUT_USER_DATA_t *user_data = glv_getUserData(wiget);

	user_data->utf8_string = NULL;
	user_data->cursorIndex = -1;
	user_data->cursorPos = -1;
	user_data->utf32_preedit_length = 0;

	glv_setValue(wiget,"gFocusBkgdColor"				,"C",GLV_SET_RGBA(255,  0,  0,255));
	glv_setValue(wiget,"gPressBkgdColor"				,"C",GLV_SET_RGBA(  0,255,  0,255));
	glv_setValue(wiget,"gReleaseBkgdColor"				,"C",GLV_SET_RGBA(178,178,178,255));
	glv_setValue(wiget,"gFontColor"						,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"gBkgdColor"						,"C",GLV_SET_RGBA(  0,  0,  0,  0));
	glv_setValue(wiget,"gInsertModeCursorColor"			,"C",GLV_SET_RGBA(  0,  0,255,255));
	glv_setValue(wiget,"gOverwriteModeCursorFontColor"	,"C",GLV_SET_RGBA(255,255,255,255));
	glv_setValue(wiget,"gOverwriteModeCursorBkgdColor"	,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"gSerectionFontColor"			,"C",GLV_SET_RGBA(255,255,255,255));
	glv_setValue(wiget,"gSerectionBkgdColor"			,"C",GLV_SET_RGBA(252, 18,132,255));
	glv_setValue(wiget,"gImeUnderlineColor"				,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"gImeUnderlineFontColor"			,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"gImeUnderlineBkgdColor"			,"C",GLV_SET_RGBA(128,128,128,255));
	glv_setValue(wiget,"gImeCandidateFontColor"			,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"gImeCandidateBkgdColor"			,"C",GLV_SET_RGBA(  0,128,128,255));
	glv_setValue(wiget,"font size"						,"i",16);
	glv_setValue(wiget,"text"							,"ST","",wiget_value_cb_textInput);
	return(GLV_OK);
}

static int wiget_text_input_terminate(glvWiget wiget)
{
	WIGET_TEXT_INPUT_USER_DATA_t *user_data = glv_getUserData(wiget);
	if(user_data->utf32_preedit_string != NULL){
		free(user_data->utf32_preedit_string);
		user_data->utf32_preedit_string = NULL;
	}
	if(user_data->utf32_preedit_attr != NULL){
		free(user_data->utf32_preedit_attr);
		user_data->utf32_preedit_attr = NULL;
	}
	if(user_data->utf8_string != NULL){
		free(user_data->utf8_string);
		user_data->utf8_string = NULL;
	}
	if(user_data->utf32_string != NULL){
		free(user_data->utf32_string);
		user_data->utf32_string = NULL;
	}
	if(user_data->advance_x != NULL){
		free(user_data->advance_x);
		user_data->advance_x = NULL;
	}
	//printf("wiget_text_input_terminate\n");
	return(GLV_OK);
}

static const struct glv_wiget_listener _wiget_textInput_listener = {
	.attr			= (GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_FOCUS | GLV_WIGET_ATTR_TEXT_INPUT_FOCUS),
	.init			= wiget_text_input_init,
	.redraw			= wiget_text_input_redraw,
	.mousePointer	= wiget_text_input_mousePointer,
    .mouseButton    = wiget_text_input_mouseButton,
	.input			= wiget_text_input_text_input,
	.focus			= wiget_text_input_text_focus,
	.terminate		= wiget_text_input_terminate,
};
const struct glv_wiget_listener *wiget_textInput_listener = &_wiget_textInput_listener;

// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
typedef struct _wiget_text_output_user_data{
	char		*utf8_string;
	int			lineSpace;
} WIGET_TEXT_OUTPUT_USER_DATA_t;

static int wiget_text_output_redraw(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	WIGET_TEXT_OUTPUT_USER_DATA_t *user_data = glv_getUserData(wiget);
	GLV_WIGET_GEOMETRY_t	geometry;
	int		kind,attr;
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
	uint32_t	gFontColor;
	uint32_t	gBkgdColor;
	int			fontSize;
	int			font;

	glv_getValue(wiget,"gFocusBkgdColor"	,"C",&gFocusBkgdColor);
	glv_getValue(wiget,"gPressBkgdColor"	,"C",&gPressBkgdColor);
	glv_getValue(wiget,"gReleaseBkgdColor"	,"C",&gReleaseBkgdColor);
	glv_getValue(wiget,"gFontColor"			,"C",&gFontColor);
	glv_getValue(wiget,"gBkgdColor"			,"C",&gBkgdColor);
	glv_getValue(wiget,"font size"			,"i",&fontSize);

	int			lineSpace			= user_data->lineSpace;

	glPushMatrix();
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	kind =  glvWiget_kindSelectWigetStatus(wiget,&wigetStatus);

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
	
	if(user_data->utf8_string != NULL){
		//attr = GLV_FONT_LEFT;
		attr = GLV_FONT_CENTER;
		//font = GLV_FONT_NAME_TYPE1;
		font = GLV_FONT_NAME_NORMAL;
		glvFont_SetStyle(font,fontSize,0.0f,0,GLV_FONT_NAME | GLV_FONT_NOMAL | GLV_FONT_SIZE | attr);
		if(attr == GLV_FONT_LEFT){
			glvFont_SetPosition(x,y + 2);
		}else{
			glvFont_SetPosition(x + w / 2,y + (fontSize+6) / 2);
		}
		glvFont_setColor(gFontColor);
		glvFont_setBkgdColor(gBkgdColor);
		glvFont_SetlineSpace(lineSpace);

		glvFont_printf("%s",user_data->utf8_string);
	}

	//glDisable(GL_BLEND);
	glPopMatrix();

	return(GLV_OK);
}

static int  wiget_value_cb_textOutput(int io,struct _glv_r_value *value)
{
	glvWiget wiget = value->instance;
	WIGET_TEXT_OUTPUT_USER_DATA_t *user_data = glv_getUserData(wiget);
	char *key_string = "text";
	if(strcmp(value->key,key_string) != 0){
		printf("wiget_value_cb_textOutput: key error. [%s][%s]\n",value->key,key_string);
		return(GLV_ERROR);
	}
	if(io == GLV_R_VALUE_IO_GET){
		// get
		if(value->n[0].v.string != NULL){
			free(value->n[0].v.string);
		}
		if(user_data->utf8_string != NULL){
			value->n[0].v.string = strdup(user_data->utf8_string);
		}else{
			value->n[0].v.string = NULL;
		}
	}else{
		// set
		if(user_data->utf8_string != NULL){
			free(user_data->utf8_string);
		}
		if(value->n[0].v.string != NULL){
			user_data->utf8_string = strdup(value->n[0].v.string);
		}else{
			user_data->utf8_string = NULL;
		}
	}
	return(GLV_OK);
}
static int wiget_text_output_init(glvWindow glv_win,glvSheet sheet,glvWiget wiget)
{
	glv_allocUserData(wiget,sizeof(WIGET_TEXT_OUTPUT_USER_DATA_t));
	WIGET_TEXT_OUTPUT_USER_DATA_t *user_data = glv_getUserData(wiget);

	user_data->utf8_string = NULL;

	glv_setValue(wiget,"gFocusBkgdColor"	,"C",GLV_SET_RGBA(255,  0,  0,255));
	glv_setValue(wiget,"gPressBkgdColor"	,"C",GLV_SET_RGBA(  0,255,  0,255));
	glv_setValue(wiget,"gReleaseBkgdColor"	,"C",GLV_SET_RGBA(178,178,178,255));
	glv_setValue(wiget,"gFontColor"			,"C",GLV_SET_RGBA(  0,  0,  0,255));
	glv_setValue(wiget,"gBkgdColor"			,"C",GLV_SET_RGBA(  0,  0,  0,  0));
	glv_setValue(wiget,"font size"			,"i",24);
	glv_setValue(wiget,"text"				,"ST","",wiget_value_cb_textOutput);

	user_data->lineSpace = 2;

	return(GLV_OK);
}

static int wiget_text_output_terminate(glvWiget wiget)
{
	WIGET_TEXT_OUTPUT_USER_DATA_t *user_data = glv_getUserData(wiget);

	if(user_data->utf8_string != NULL){
		free(user_data->utf8_string);
		user_data->utf8_string = NULL;
	}
	//printf("wiget_text_output_terminate\n");
	
	return(GLV_OK);
}

static const struct glv_wiget_listener _wiget_textOutput_listener = {
//	.attr		= (GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_FOCUS),
	.attr		= GLV_WIGET_ATTR_NO_OPTIONS,
	.init		= wiget_text_output_init,
	.redraw		= wiget_text_output_redraw,
	.terminate	= wiget_text_output_terminate
};
const struct glv_wiget_listener *wiget_textOutput_listener = &_wiget_textOutput_listener;
