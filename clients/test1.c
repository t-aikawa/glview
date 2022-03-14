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

extern void simple_egl_window_test(glvWindow glv_frame_window);
extern void smoke_window_test(glvWindow glv_frame_window);
extern glvWindow	glv_frame_window;
extern glvWindow	glv_hmi_window;

static glvWindow	glv_frame_window2 = NULL;
static glvInstanceId glv_frame_window2_id=0;
static glvWindow	glv_hmi_window2 = NULL;
static glvInstanceId	glv_hmi_window2_id= 0;
static glvWindow	glv_hmi_window4 = NULL;
// ==============================================================================================
// ==============================================================================================
// ==============================================================================================

typedef struct _sheet_user_data{
	glvWiget wiget_slider_bar;
	glvWiget wiget_text_input;
	glvWiget wiget_text_output_test;
	glvWiget wiget_check_box_test;
	glvWiget wiget_list_box_test;
	glvWiget wiget_close_button;
	glvWiget wiget_pullDownMenu_test;
} SHEET_USER_DATA_t;

static int hmi_sheet_action2(glvWindow glv_win,glvSheet sheet,int action,glvInstanceId selectId)
{
	SHEET_USER_DATA_t *sheet_user_data = glv_getUserData(sheet);

	if(selectId == glv_getInstanceId(sheet_user_data->wiget_slider_bar)){
		// select slider bar
		int position;
		glv_getValue(sheet_user_data->wiget_slider_bar,"position","i",&position);
		printf("hmi_sheet_action2:wiget_slider_bar slider bar position = %d\n",position);
	}

	if(selectId == glv_getInstanceId(sheet_user_data->wiget_text_input)){
		//printf("hmi_sheet_action2:wiget_text_input\n");
	}

	if(selectId == glv_getInstanceId(sheet_user_data->wiget_text_output_test)){
		printf("hmi_sheet_action2:wiget_text_output_test\n");
	}

	if((selectId == glv_getInstanceId(sheet_user_data->wiget_check_box_test  )) ||
	   (selectId == glv_getInstanceId(sheet_user_data->wiget_text_output_test))   ){
		int check;
		glv_getValue(sheet_user_data->wiget_check_box_test,"check","i",&check);
		if(check == 0){
			check = 1;
		}else{
			check = 0;
		}
		glv_setValue(sheet_user_data->wiget_check_box_test,"check","i",check);
		printf("hmi_sheet_action2:wiget_check_box_test check = %d\n",check);
		glvOnReDraw(glv_win);
	}

	if(selectId == glv_getInstanceId(sheet_user_data->wiget_list_box_test)){
		int functionId;
		glv_getValue(sheet_user_data->wiget_list_box_test,"select function id","i",&functionId);
		printf("hmi_sheet_action2:wiget_list_box_test functionId = %d\n",functionId);
	}

	if(selectId == glv_getInstanceId(sheet_user_data->wiget_close_button)){
		//char string[256]={};
		char *string;
		int check;
		int position;
		int functionId;
		printf("hmi_sheet_action2:wiget_close_button\n");

		glv_getValue(sheet_user_data->wiget_text_input,"text","S",&string);
		printf("hmi_sheet_action2:input [%s]\n",string);
		
		glv_getValue(sheet_user_data->wiget_check_box_test,"check","i",&check);
		printf("hmi_sheet_action2:check [%d]\n",check);
		
		glv_getValue(sheet_user_data->wiget_slider_bar,"position","i",&position);
		printf("hmi_sheet_action2:position [%d]\n",position);
		
		glv_getValue(sheet_user_data->wiget_list_box_test,"select function id","i",&functionId);
		printf("hmi_sheet_action2:functionId [%d]\n",functionId);

		// close
		glvWindow frame = NULL;
		int windowType;
		windowType = glv_getWindowType(glv_win);
		if(windowType == GLV_TYPE_THREAD_WINDOW){
			frame = glv_getFrameWindow(glv_win);
		}
		glvDestroyWindow(&glv_win);
		if(frame != NULL){
			glvDestroyWindow(&frame);
		}
	}

	if(selectId == glv_getInstanceId(sheet_user_data->wiget_pullDownMenu_test)){
		int functionId;
		glv_getValue(sheet_user_data->wiget_pullDownMenu_test,"select function id","i",&functionId);
		printf("hmi_sheet_action2:wiget_pullDownMenu_test functionId = %d\n",functionId);
	}

	return(GLV_OK);
}

static void hmi_sheet_init_params(glvSheet sheet,int WinWidth,int WinHeight)
{
	GLV_WIGET_GEOMETRY_t	geometry;
	GLV_WIGET_GEOMETRY_t	geometry2;
	SHEET_USER_DATA_t *user_data = glv_getUserData(sheet);

	geometry.scale	= 1.0;
	geometry.width	= 500;
	geometry.height	= 30;
	geometry.x	= 50;
	geometry.y	= WinHeight / 5;
	glvWiget_setWigetGeometry(user_data->wiget_text_input,&geometry);

	geometry.scale	= 1.0;
	geometry.width	= 200;
	geometry.height	= 30;
	geometry.x	= 50;
	geometry.y	= WinHeight * 2 / 5;
	glvWiget_setWigetGeometry(user_data->wiget_slider_bar,&geometry);

	geometry.scale	= 1.0;
	geometry.width	= 20;
	geometry.height	= 20;
	geometry.x	= 20;
	geometry.y	= WinHeight * 3 / 5;
	glvWiget_setWigetGeometry(user_data->wiget_check_box_test,&geometry);

	geometry.scale	= 1.0;
	geometry.width	= 300;
	geometry.height	= 60;
	geometry.x	= 100;
	geometry.y	= WinHeight * 3 / 5;
	glvWiget_setWigetGeometry(user_data->wiget_text_output_test,&geometry);

	geometry.scale	= 1.0;
	geometry.width	= 300;
	geometry.height	= 30;
	geometry.x	= 50;
	geometry.y	= WinHeight * 4 / 5;
	glvWiget_setWigetGeometry(user_data->wiget_list_box_test,&geometry);

	geometry2.scale = 1.0;
	geometry2.width	= 100;
	geometry2.height	= 30;
	geometry2.x	= geometry.x + geometry.width + 100;
	geometry2.y	= WinHeight * 4 / 5;
	glvWiget_setWigetGeometry(user_data->wiget_close_button,&geometry2);

	geometry.scale 	= 1.0;
	geometry.width	= WinWidth - 5 * 2;
	geometry.height	= 26;
	geometry.x	= 5;
	geometry.y	= 5;
	glvWiget_setWigetGeometry(user_data->wiget_pullDownMenu_test,&geometry);
}

static int hmi_sheet_reshape2(glvWindow glv_win,glvSheet sheet,int window_width, int window_height)
{
	//printf("hmi_sheet_reshape2\n");
	hmi_sheet_init_params(sheet,window_width,window_height);
	glvSheet_reqDrawWigets(sheet);
	glvSheet_reqSwapBuffers(sheet);
	return(GLV_OK);
}

static int hmi_sheet_update2(glvWindow glv_win,glvSheet sheet,int drawStat)
{
	//printf("hmi_sheet_update2\n");

	glvSheet_reqDrawWigets(sheet);
	glvSheet_reqSwapBuffers(sheet);

	return(GLV_OK);
}

static int hmi_sheet_init2(glvWindow glv_win,glvSheet sheet,int window_width, int window_height)
{
	char *string;
	int check;
	glv_allocUserData(sheet,sizeof(SHEET_USER_DATA_t));
	SHEET_USER_DATA_t *user_data = glv_getUserData(sheet);

	// -------------------------------------------------------------------------
	user_data->wiget_slider_bar	= glvCreateWiget(sheet,wiget_sliderBar_listener,GLV_WIGET_ATTR_NO_OPTIONS);
	glv_setValue(user_data->wiget_slider_bar,"params","ii",1,99);
	glv_setValue(user_data->wiget_slider_bar,"position","i",18);
	glvWiget_setWigetVisible(user_data->wiget_slider_bar,GLV_VISIBLE);

	// -------------------------------------------------------------------------
	user_data->wiget_text_input	= glvCreateWiget(sheet,wiget_textInput_listener,GLV_WIGET_ATTR_NO_OPTIONS);
	string = "日本語入力テスト ime input test";
	glv_setValue(user_data->wiget_text_input,"text","S",string);
	glvWiget_setWigetVisible(user_data->wiget_text_input,GLV_VISIBLE);

	// -------------------------------------------------------------------------
	user_data->wiget_check_box_test	= glvCreateWiget(sheet,wiget_checkBox_listener,GLV_WIGET_ATTR_NO_OPTIONS);
	glv_setValue(user_data->wiget_check_box_test,"gFocusBkgdColor"	,"C",GLV_SET_RGBA(  0,255,  0,255));
	glv_setValue(user_data->wiget_check_box_test,"gPressBkgdColor"	,"C",GLV_SET_RGBA(255,  0,  0,255));
	glv_setValue(user_data->wiget_check_box_test,"gReleaseBkgdColor","C",GLV_SET_RGBA(255,178,178,255));
	glv_setValue(user_data->wiget_check_box_test,"gBoxColor"		,"C",GLV_SET_RGBA(  0,  0,255,255));
	check = 1;
	glv_setValue(user_data->wiget_check_box_test,"check","i",check);
	glvWiget_setWigetVisible(user_data->wiget_check_box_test,GLV_VISIBLE);

	// -------------------------------------------------------------------------
	user_data->wiget_text_output_test = glvCreateWiget(sheet,wiget_textOutput_listener,(GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_FOCUS));
	string = "check:チェックボックス\nのテストです";
	glv_setValue(user_data->wiget_text_output_test,"text","S",string);
	glv_setValue(user_data->wiget_text_output_test,"font size","i",22);
	glvWiget_setWigetVisible(user_data->wiget_text_output_test,GLV_VISIBLE);

	// -------------------------------------------------------------------------
	user_data->wiget_list_box_test = glvCreateWiget(sheet,wiget_listBox_listener,GLV_WIGET_ATTR_NO_OPTIONS);

	glv_setValue(user_data->wiget_list_box_test,"list"					,"iiSiii",0,1,"カタカナひらがな"	,0,0,100);
	glv_setValue(user_data->wiget_list_box_test,"list"					,"iiSiii",0,2,"行末まで削除"		,0,0,101);
	glv_setValue(user_data->wiget_list_box_test,"list"					,"iiSiii",0,3,"一行挿入"			,0,0,102);
	glv_setValue(user_data->wiget_list_box_test,"list"					,"iiSiii",0,4,"削除(くず箱)"		,0,0,103);
	glv_setValue(user_data->wiget_list_box_test,"select function id"	,"i",101);
	glv_setValue(user_data->wiget_list_box_test,"item height"			,"i",30);
	glvWiget_setWigetVisible(user_data->wiget_list_box_test,GLV_VISIBLE);

	//glv_printValue(user_data->wiget_list_box_test,"user_data->wiget_list_box_test");

	// -------------------------------------------------------------------------
	user_data->wiget_close_button = glvCreateWiget(sheet,wiget_textOutput_listener,(GLV_WIGET_ATTR_PUSH_ACTION | GLV_WIGET_ATTR_POINTER_FOCUS));
	glv_setValue(user_data->wiget_close_button,"text","S","close");
	glv_setValue(user_data->wiget_close_button,"font size","i",22);
	glvWiget_setWigetVisible(user_data->wiget_close_button,GLV_VISIBLE);

	// -------------------------------------------------------------------------
	user_data->wiget_pullDownMenu_test = glvCreateWiget(sheet,wiget_pullDownMenu_listener,GLV_WIGET_ATTR_NO_OPTIONS);
#if 1
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",0,1,"ファイル(F)"	,0,0,100);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",0,2,"編集(E)"		,0,0,101);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",0,3,"選択(S)"		,0,0,102);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",0,4,"表示(V)"		,0,0,103);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",0,5,"移動(G)"		,0,0,104);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",0,6,"実行(R)"		,0,0,105);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",0,7,"ターミナル(R)"	,0,0,106);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",0,8,"ヘルプ(R)"		,0,0,107);

	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",1,1,"リスト１−１"	,0,0,111);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",1,2,"リスト１−２"	,0,0,112);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",1,3,"リスト１−３"	,0,0,113);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",1,4,"リスト１−４"	,0,0,114);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",1,5,"リスト１−５"	,0,0,115);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",1,6,"リスト１−６"	,0,0,116);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",1,7,"リスト１−７"	,0,0,117);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",1,8,"リスト１−８"	,0,0,118);

	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",2,1,"リスト２−１"	,0,0,121);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",2,2,"リスト２−２"	,0,0,122);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",2,3,"リスト２−３"	,0,0,123);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",2,4,"リスト２−４-abcdefg"	,0,0,124);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",2,5,"リスト２−５"	,0,0,125);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",2,6,"リスト２−６"	,0,0,126);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",2,7,"リスト２−７"	,0,0,127);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",2,8,"リスト２−８"	,0,0,128);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",2,9,"リスト２−９"	,0,0,129);

	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",3,1,"リスト３−１"	,0,0,131);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",3,2,"リスト３−２"	,0,0,132);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",3,3,"リスト３−３"	,0,0,133);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",3,4,"リスト３−４"	,0,0,134);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",3,5,"リスト３−５"	,0,0,135);
//	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",3,6,"リスト３−６"	,0,0,136);
//	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",3,7,"リスト３−７"	,0,0,137);
//	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",3,8,"リスト３−８"	,0,0,138);

	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",4,1,"リスト４−１"	,0,0,141);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",4,2,"リスト４−２"	,0,0,142);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",4,3,"リスト４−３"	,0,0,143);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",4,4,"リスト４−４"	,0,0,144);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",4,5,"リスト４−５"	,0,0,145);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",4,6,"リスト４−６"	,0,0,146);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",4,7,"リスト４−７"	,0,0,147);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",4,8,"リスト４−８"	,0,0,148);

	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",5,1,"リスト５−１"	,0,0,151);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",5,2,"リスト５−２"	,0,0,152);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",5,3,"リスト５−３"	,0,0,153);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",5,4,"リスト５−４"	,0,0,154);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",5,5,"リスト５−５"	,0,0,155);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",5,6,"リスト５−６"	,0,0,156);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",5,7,"リスト５−７"	,0,0,157);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",5,8,"リスト５−８"	,0,0,158);

	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",6,1,"リスト６−１"	,0,0,161);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",6,2,"リスト６−２"	,0,0,162);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",6,3,"リスト６−３"	,0,0,163);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",6,4,"リスト６−４"	,0,0,164);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",6,5,"リスト６−５"	,0,0,165);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",6,6,"リスト６−６"	,0,0,166);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",6,7,"リスト６−７"	,0,0,167);
//	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",6,8,"リスト６−８"	,0,0,168);

	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",7,1,"リスト７−１"	,0,0,171);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",7,2,"リスト７−２"	,0,0,172);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",7,3,"リスト７−３"	,0,0,173);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",7,4,"リスト７−４"	,0,0,174);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",7,5,"リスト７−５"	,0,0,175);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",7,6,"リスト７−６"	,0,0,176);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",7,7,"リスト７−７"	,0,0,177);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",7,8,"リスト７−８"	,0,0,178);

	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",8,1,"リスト８−１"	,0,0,181);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",8,2,"リスト８−２"	,0,0,182);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",8,3,"リスト８−３"	,0,0,183);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",8,4,"リスト８−４"	,0,0,184);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",8,5,"リスト８−５"	,0,0,185);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",8,6,"リスト８−６"	,0,0,186);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",8,7,"リスト８−７"	,0,0,187);
	glv_setValue(user_data->wiget_pullDownMenu_test,"list","iiSiii",8,8,"リスト８−８"	,0,0,188);

	glv_setValue(user_data->wiget_pullDownMenu_test,"item height"			,"i",22);
#endif
	glvWiget_setWigetVisible(user_data->wiget_pullDownMenu_test,GLV_VISIBLE);

	// -------------------------------------------------------------------------
	hmi_sheet_init_params(sheet,window_width,window_height);

	return(GLV_OK);
}

static const struct glv_sheet_listener _hmi2_sheet_listener = {
	.init			= hmi_sheet_init2,
	.reshape		= hmi_sheet_reshape2,
	.redraw			= hmi_sheet_update2,
	.update 		= hmi_sheet_update2,
	.timer			= NULL,
	.mousePointer	= NULL,
	.action			= hmi_sheet_action2,
	.terminate		= NULL,
};

static const struct glv_sheet_listener *hmi2_sheet_listener = &_hmi2_sheet_listener;

int hmi_window_init2(glvWindow glv_win,int width, int height)
{
	glvGl_init();
	glvWindow_setViewport(glv_win,width,height);

	glvSheet glv_sheet;
	glv_sheet = glvCreateSheet(glv_win,hmi2_sheet_listener,"hmi 2 sheet");
	glvWindow_activeSheet(glv_win,glv_sheet);

	return(GLV_OK);
}

int hmi_window_update2(glvWindow glv_win,int drawStat)
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glvReqSwapBuffers(glv_win);
	return(GLV_OK);
}

int hmi_window_reshape2(glvWindow glv_win,int width, int height)
{
	glvWindow_setViewport(glv_win,width,height);
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glvReqSwapBuffers(glv_win);
	return(GLV_OK);
}

int hmi_window_terminate4(glvWindow glv_win)
{
	printf("hmi_window_terminate4: glv_hmi_window4\n");
	return(GLV_OK);
}

int hmi_window_terminate2(glvWindow glv_win)
{
	printf("hmi_window_terminate2: glv_hmi_window2\n");
	return(GLV_OK);
}

static const struct glv_window_listener _sub_window_listener = {
	.init		= hmi_window_init2,
	.reshape	= hmi_window_reshape2,
	.redraw		= hmi_window_update2,
	.update 	= hmi_window_update2,
	.timer		= NULL,
	.gesture	= NULL,
	.userMsg	= NULL,
	.terminate	= hmi_window_terminate4,
};
static const struct glv_window_listener *sub_window_listener = &_sub_window_listener;

static const struct glv_window_listener _sub_window_menu_listener = {
	.init		= hmi_window_init2,
	.reshape	= hmi_window_reshape2,
	.redraw		= hmi_window_update2,
	.update 	= hmi_window_update2,
	.timer		= NULL,
	.gesture	= NULL,
	.userMsg	= NULL,
	.terminate	= hmi_window_terminate2,
};
static const struct glv_window_listener *sub_window_menu_listener = &_sub_window_menu_listener;

int sub_frame_start(glvWindow glv_frame_window,int width, int height)
{
	printf("sub_frame_start [%s] width = %d , height = %d\n",glvWindow_getWindowName(glv_frame_window),width,height);
	glvCreateWindow(glv_frame_window,sub_window_listener,&glv_hmi_window4,"glv_hmi_window4",
			0, 0, width, height,GLV_WINDOW_ATTR_DEFAULT);
	glvOnReDraw(glv_hmi_window4);
	return(GLV_OK);
}

int sub_frame_configure(glvWindow glv_win,int width, int height)
{
	//printf("sub_frame_configure width = %d , height = %d\n",width,height);
	glvOnReShape(glv_hmi_window4,0,0,width,height);
	return(GLV_OK);
}

int sub_frame_terminate(glvWindow glv_win)
{
	printf("sub_frame_terminate\n");
	return(GLV_OK);
}

static const struct glv_frame_listener _sub_frame_window_listener = {
	.start		= sub_frame_start,
//	.configure	= sub_frame_configure,
	.terminate	= sub_frame_terminate
};
static const struct glv_frame_listener *sub_frame_window_listener = &_sub_frame_window_listener;

void part_window_test(glvWindow glv_frame_window)
{
	int window_width  = 600;
	int winodw_height = 400;

	if(glvWindow_isAliveWindow(glv_frame_window,glv_frame_window2_id) == GLV_INSTANCE_DEAD){
		glv_frame_window2_id = glvCreateFrameWindow(glv_frame_window,sub_frame_window_listener,&glv_frame_window2,"glv_frame_window2","sub window",window_width, winodw_height);
		printf("glv_frame_window2 create\n");
	}else{
		glvDestroyWindow(&glv_hmi_window4);
		glvDestroyWindow(&glv_frame_window2);
		printf("glv_frame_window2 delete\n");
	}
}

int test_keyboard_handle_key(unsigned int key,unsigned int modifiers,unsigned int state)
{
    //printf("Key is %d state is %d\n", key, state);
    if(state == GLV_KEYBOARD_KEY_STATE_PRESSED){
    	switch(key){
		case XKB_KEY_F5:		// XKB_KEY_F5
				part_window_test(glv_frame_window);
			break;
		case XKB_KEY_F6:		// XKB_KEY_F6
				simple_egl_window_test(glv_frame_window);
			break;			
		case XKB_KEY_F7:		// XKB_KEY_F7
			{
				int window_width = 600;
				int window_height = 400;
				int offset_x = 25;
				int offset_y = 25;
				if(glvWindow_isAliveWindow(glv_frame_window,glv_hmi_window2_id) == GLV_INSTANCE_DEAD){
					glv_hmi_window2_id = glvCreateChildWindow(glv_frame_window,sub_window_menu_listener,&glv_hmi_window2,"glv_hmi_window2",
									offset_x, offset_y, window_width, window_height,GLV_WINDOW_ATTR_DEFAULT);
					glvOnReDraw(glv_hmi_window2);
				}
			}
			break;
		case XKB_KEY_F8:		// XKB_KEY_F8
			{
				if(glvWindow_isAliveWindow(glv_frame_window,glv_hmi_window2_id) == GLV_INSTANCE_ALIVE){
					glvDestroyWindow(&glv_hmi_window2);
				}
			}
			break;
		case XKB_KEY_F9:		// XKB_KEY_F9
			smoke_window_test(glv_frame_window);
			break;
    	}
    }
    return(GLV_OK);
}

void test_set_pulldownMenu(glvWindow frame)
{
	if(glv_isPullDownMenu(frame) == 1){
		glv_setValue(frame,"pullDownMenu","iiSiii",0,1,"ファイル(F)"	,0,0,100);
		glv_setValue(frame,"pullDownMenu","iiSiii",0,2,"編集(E)"		,0,0,101);
		glv_setValue(frame,"pullDownMenu","iiSiii",0,3,"選択(S)"		,0,0,102);
		glv_setValue(frame,"pullDownMenu","iiSiii",0,4,"表示(V)"		,0,0,103);
		glv_setValue(frame,"pullDownMenu","iiSiii",0,5,"移動(G)"		,0,0,104);
		glv_setValue(frame,"pullDownMenu","iiSiii",0,6,"実行(R)"		,0,0,105);
		glv_setValue(frame,"pullDownMenu","iiSiii",0,7,"ターミナル(R)"	,0,0,106);
		glv_setValue(frame,"pullDownMenu","iiSiii",0,8,"ヘルプ(R)"		,0,0,107);

		glv_setValue(frame,"pullDownMenu","iiSiii",1,1,"リスト１−１"	,0,0,111);
		glv_setValue(frame,"pullDownMenu","iiSiii",1,2,"リスト１−２"	,0,0,112);
		glv_setValue(frame,"pullDownMenu","iiSiii",1,3,"リスト１−３"	,0,0,113);
		glv_setValue(frame,"pullDownMenu","iiSiii",1,4,"リスト１−４"	,0,0,114);
		glv_setValue(frame,"pullDownMenu","iiSiii",1,5,"リスト１−５"	,0,0,115);
		glv_setValue(frame,"pullDownMenu","iiSiii",1,6,"リスト１−６"	,0,0,116);
		glv_setValue(frame,"pullDownMenu","iiSiii",1,7,"リスト１−７"	,0,0,117);
		glv_setValue(frame,"pullDownMenu","iiSiii",1,8,"リスト１−８"	,0,0,118);

		glv_setValue(frame,"pullDownMenu","iiSiii",2,1,"リスト２−１"	,0,0,121);
		glv_setValue(frame,"pullDownMenu","iiSiii",2,2,"リスト２−２"	,0,0,122);
		glv_setValue(frame,"pullDownMenu","iiSiii",2,3,"リスト２−３"	,0,0,123);
		glv_setValue(frame,"pullDownMenu","iiSiii",2,4,"リスト２−４"	,0,0,124);
		glv_setValue(frame,"pullDownMenu","iiSiii",2,5,"リスト２−５"	,0,0,125);
		glv_setValue(frame,"pullDownMenu","iiSiii",2,6,"リスト２−６"	,0,0,126);
		glv_setValue(frame,"pullDownMenu","iiSiii",2,7,"リスト２−７"	,0,0,127);
		glv_setValue(frame,"pullDownMenu","iiSiii",2,8,"リスト２−８"	,0,0,128);
		glv_setValue(frame,"pullDownMenu","iiSiii",2,9,"リスト２−９"	,0,0,129);

		glv_setValue(frame,"pullDownMenu","iiSiii",3,1,"リスト３−１"	,0,0,131);
		glv_setValue(frame,"pullDownMenu","iiSiii",3,2,"リスト３−２"	,0,0,132);
		glv_setValue(frame,"pullDownMenu","iiSiii",3,3,"リスト３−３"	,0,0,133);
		glv_setValue(frame,"pullDownMenu","iiSiii",3,4,"リスト３−４"	,0,0,134);
		glv_setValue(frame,"pullDownMenu","iiSiii",3,5,"リスト３−５"	,0,0,135);
		glv_setValue(frame,"pullDownMenu","iiSiii",3,6,"リスト３−６"	,0,0,136);
		glv_setValue(frame,"pullDownMenu","iiSiii",3,7,"リスト３−７"	,0,0,137);
		glv_setValue(frame,"pullDownMenu","iiSiii",3,8,"リスト３−８"	,0,0,138);

		glv_setValue(frame,"pullDownMenu","iiSiii",4,1,"リスト４−１"	,0,0,141);
		glv_setValue(frame,"pullDownMenu","iiSiii",4,2,"リスト４−２"	,0,0,142);
		glv_setValue(frame,"pullDownMenu","iiSiii",4,3,"リスト４−３"	,0,0,143);
		glv_setValue(frame,"pullDownMenu","iiSiii",4,4,"リスト４−４"	,0,0,144);
		glv_setValue(frame,"pullDownMenu","iiSiii",4,5,"リスト４−５"	,0,0,145);
		glv_setValue(frame,"pullDownMenu","iiSiii",4,6,"リスト４−６"	,0,0,146);
		glv_setValue(frame,"pullDownMenu","iiSiii",4,7,"リスト４−７"	,0,0,147);
		glv_setValue(frame,"pullDownMenu","iiSiii",4,8,"リスト４−８"	,0,0,148);

		glv_setValue(frame,"pullDownMenu","iiSiii",5,1,"リスト５−１"	,0,0,151);
		glv_setValue(frame,"pullDownMenu","iiSiii",5,2,"リスト５−２"	,0,0,152);
		glv_setValue(frame,"pullDownMenu","iiSiii",5,3,"リスト５−３"	,0,0,153);
		glv_setValue(frame,"pullDownMenu","iiSiii",5,4,"リスト５−４"	,0,0,154);
		glv_setValue(frame,"pullDownMenu","iiSiii",5,5,"リスト５−５"	,0,0,155);
		glv_setValue(frame,"pullDownMenu","iiSiii",5,6,"リスト５−６"	,0,0,156);
		glv_setValue(frame,"pullDownMenu","iiSiii",5,7,"リスト５−７"	,0,0,157);
		glv_setValue(frame,"pullDownMenu","iiSiii",5,8,"リスト５−８"	,0,0,158);

		glv_setValue(frame,"pullDownMenu","iiSiii",6,1,"リスト６−１"	,0,0,161);
		glv_setValue(frame,"pullDownMenu","iiSiii",6,2,"リスト６−２"	,0,0,162);
		glv_setValue(frame,"pullDownMenu","iiSiii",6,3,"リスト６−３"	,0,0,163);
		glv_setValue(frame,"pullDownMenu","iiSiii",6,4,"リスト６−４"	,0,0,164);
		glv_setValue(frame,"pullDownMenu","iiSiii",6,5,"リスト６−５"	,0,0,165);
		glv_setValue(frame,"pullDownMenu","iiSiii",6,6,"リスト６−６"	,0,0,166);
		glv_setValue(frame,"pullDownMenu","iiSiii",6,7,"リスト６−７"	,0,0,167);
	//	glv_setValue(frame,"pullDownMenu","iiSiii",6,8,"リスト６−８"	,0,0,168);

		glv_setValue(frame,"pullDownMenu","iiSiii",7,1,"リスト７−１"	,0,0,171);
		glv_setValue(frame,"pullDownMenu","iiSiii",7,2,"リスト７−２"	,0,0,172);
		glv_setValue(frame,"pullDownMenu","iiSiii",7,3,"リスト７−３"	,0,0,173);
		glv_setValue(frame,"pullDownMenu","iiSiii",7,4,"リスト７−４"	,0,0,174);
		glv_setValue(frame,"pullDownMenu","iiSiii",7,5,"リスト７−５"	,0,0,175);
		glv_setValue(frame,"pullDownMenu","iiSiii",7,6,"リスト７−６"	,0,0,176);
		glv_setValue(frame,"pullDownMenu","iiSiii",7,7,"リスト７−７"	,0,0,177);
		glv_setValue(frame,"pullDownMenu","iiSiii",7,8,"リスト７−８"	,0,0,178);

		glv_setValue(frame,"pullDownMenu","iiSiii",8,1,"リスト８−１"	,0,0,181);
		glv_setValue(frame,"pullDownMenu","iiSiii",8,2,"リスト８−２"	,0,0,182);
		glv_setValue(frame,"pullDownMenu","iiSiii",8,3,"リスト８−３"	,0,0,183);
		glv_setValue(frame,"pullDownMenu","iiSiii",8,4,"リスト８−４"	,0,0,184);
		glv_setValue(frame,"pullDownMenu","iiSiii",8,5,"リスト８−５"	,0,0,185);
		glv_setValue(frame,"pullDownMenu","iiSiii",8,6,"リスト８−６"	,0,0,186);
		glv_setValue(frame,"pullDownMenu","iiSiii",8,7,"リスト８−７"	,0,0,187);
		glv_setValue(frame,"pullDownMenu","iiSiii",8,8,"リスト８−８"	,0,0,188);
	}
	if(glv_isCmdMenu(frame) == 1){
		glv_setValue(frame,"cmdMenu","iiSiii",0, 1,"閉じる"	,0,0,1001);
		glv_setValue(frame,"cmdMenu","iiSiii",0, 2,"　♪　"	,0,1,1002);
		glv_setValue(frame,"cmdMenu","iiSiii",0, 3,"　　　"	,0,0,1003);
		glv_setValue(frame,"cmdMenu","iiSiii",0, 4,"　　　"	,0,0,1004);
		glv_setValue(frame,"cmdMenu","iiSiii",0, 5,"sub-win",0,0,1005);
		glv_setValue(frame,"cmdMenu","iiSiii",0, 6," gles "	,0,0,1006);
		glv_setValue(frame,"cmdMenu","iiSiii",0, 7,"menu-open"	,0,0,1007);
		glv_setValue(frame,"cmdMenu","iiSiii",0, 8,"menu-close"	,0,0,1008);
		glv_setValue(frame,"cmdMenu","iiSiii",0, 9," smoke "	,0,0,1009);
		glv_setValue(frame,"cmdMenu","iiSiii",0,10,"　　　"	,0,0,1010);
		glv_setValue(frame,"cmdMenu","iiSiii",0,11,"　　　"	,0,0,1011);
		glv_setValue(frame,"cmdMenu","iiSiii",0,12,"　　　"	,0,0,1012);

		glv_setValue(frame,"cmdMenu","iiSiii",1, 1,"　　　"	,0,0,1101);
		glv_setValue(frame,"cmdMenu","iiSiii",1, 2,"　♫　"	,0,0,1102);
		glv_setValue(frame,"cmdMenu","iiSiii",1, 3,"　　　"	,0,0,1103);
		glv_setValue(frame,"cmdMenu","iiSiii",1, 4,"１−４"	,0,0,1104);
		glv_setValue(frame,"cmdMenu","iiSiii",1, 5,"１−５"	,0,0,1105);
		glv_setValue(frame,"cmdMenu","iiSiii",1, 6,"１−６"	,0,0,1106);
		glv_setValue(frame,"cmdMenu","iiSiii",1, 7,"　　　"	,0,0,1107);
		glv_setValue(frame,"cmdMenu","iiSiii",1, 8,"　　　"	,0,0,1108);
		glv_setValue(frame,"cmdMenu","iiSiii",1, 9,"　　　"	,0,0,1109);
		glv_setValue(frame,"cmdMenu","iiSiii",1,10,"　　　"	,0,0,1110);
		glv_setValue(frame,"cmdMenu","iiSiii",1,11,"　　　"	,0,0,1111);
		glv_setValue(frame,"cmdMenu","iiSiii",1,12,"　　　"	,0,0,1112);

		glv_setValue(frame,"cmdMenu number","i",0);
	}
}
