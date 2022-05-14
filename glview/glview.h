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

#ifndef GLVIEW_H
#define GLVIEW_H

#include "glview_opengl.h"

#include <xkbcommon/xkbcommon.h>

#include "pthread_msq.h"
#include "pthread_timer.h"

#include "glview_gl.h"
#include "glview_png.h"

#ifndef UNUSED
#define UNUSED(x) (void)x
#endif

#ifdef __cplusplus
extern "C" {
#endif

// コンソール出力用
#define GLV_C_BLACK		"\033[30m"
#define GLV_C_RED		"\033[31m"
#define GLV_C_GREEN		"\033[32m"
#define GLV_C_YELLOW	"\033[33m"
#define GLV_C_BLUE		"\033[34m"
#define GLV_C_PURPLE	"\033[35m"
#define GLV_C_CYAN		"\033[36m"
#define GLV_C_WHITE		"\033[37m"
#define GLV_C_UNDERLINE	"\033[4m"
#define GLV_C_REVERCE	"\033[07m"
#define GLV_C_END		"\033[0m"

// デバック用
#define GLV_DEBUG_OFF			(0)
#define GLV_DEBUG_VERSION		(2)
#define GLV_DEBUG_MSG			(4)
#define GLV_DEBUG_INSTANCE		(8)
#define GLV_DEBUG_KB_INPUT  	(16)
#define GLV_DEBUG_IME_INPUT 	(32)
#define GLV_DEBUG_WIGET			(64)
#define GLV_DEBUG_DATA_DEVICE	(128)

#define GLV_DEBUG_ON		(GLV_DEBUG_VERSION | GLV_DEBUG_MSG | GLV_DEBUG_INSTANCE | GLV_DEBUG_KB_INPUT | GLV_DEBUG_IME_INPUT | GLV_DEBUG_WIGET | GLV_DEBUG_DATA_DEVICE)

#define GLV_IF_DEBUG_VERSION		if(glvCheckDebugFlag(GLV_DEBUG_VERSION))
#define GLV_IF_DEBUG_MSG			if(glvCheckDebugFlag(GLV_DEBUG_MSG))
#define GLV_IF_DEBUG_INSTANCE		if(glvCheckDebugFlag(GLV_DEBUG_INSTANCE))
#define GLV_IF_DEBUG_KB_INPUT		if(glvCheckDebugFlag(GLV_DEBUG_KB_INPUT))
#define GLV_IF_DEBUG_IME_INPUT		if(glvCheckDebugFlag(GLV_DEBUG_IME_INPUT))
#define GLV_IF_DEBUG_WIGET			if(glvCheckDebugFlag(GLV_DEBUG_WIGET))
#define GLV_IF_DEBUG_DATA_DEVICE	if(glvCheckDebugFlag(GLV_DEBUG_DATA_DEVICE))

#define GLV_DEBUG_MSG_COLOR			GLV_C_RED
#define GLV_DEBUG_INSTANCE_COLOR	GLV_C_YELLOW
#define GLV_DEBUG_KB_INPUT_COLOR	GLV_C_GREEN
#define GLV_DEBUG_END_COLOR			GLV_C_END

//-------------------------------------------------------------------------------------

#define GLV_TYPE_THREAD_FRAME		(1)
#define GLV_TYPE_THREAD_WINDOW		(2)
#define GLV_TYPE_CHILD_WINDOW		(3)

#define GLV_INSTANCE_ALIVE			(1)
#define GLV_INSTANCE_DEAD			(0)

#define GLV_KEYBOARD_KEY_STATE_RELEASED		(0)
#define GLV_KEYBOARD_KEY_STATE_PRESSED		(1)

#define GLV_STAT_DRAW_REDRAW	(0)
#define GLV_STAT_DRAW_UPDATE	(1)
#define GLV_STAT_IN_FOCUS		(2)
#define GLV_STAT_OUT_FOCUS		(3)

#define GLV_ACTION_WIGET			(1)
#define GLV_ACTION_PULL_DOWN_MENU	(2)
#define GLV_ACTION_CMD_MENU			(3)

#define GLV_ERROR	PTHREAD_TIMER_ERROR
#define GLV_OK		PTHREAD_TIMER_OK

#define GLV_TIMER_ONLY_ONCE		PTHREAD_TIMER_ONLY_ONCE
#define GLV_TIMER_REPEAT		PTHREAD_TIMER_REPEAT

//#define GLV_GESTURE_EVENT_MOTION		(0)
#define GLV_GESTURE_EVENT_DOWN			(1)
#define GLV_GESTURE_EVENT_SINGLE_UP		(2)
#define GLV_GESTURE_EVENT_SCROLL		(3)
#define GLV_GESTURE_EVENT_FLING			(4)
#define GLV_GESTURE_EVENT_SCROLL_STOP	(5)

#define GLV_MOUSE_EVENT_RELEASE			(0)
#define GLV_MOUSE_EVENT_PRESS			(1)
#define GLV_MOUSE_EVENT_MOTION			(2)

#define GLV_WIGET_STATUS_RELEASE		GLV_MOUSE_EVENT_RELEASE
#define GLV_WIGET_STATUS_PRESS			GLV_MOUSE_EVENT_PRESS
#define GLV_WIGET_STATUS_FOCUS			GLV_MOUSE_EVENT_MOTION

#define GLV_MOUSE_EVENT_LEFT_RELEASE	GLV_MOUSE_EVENT_RELEASE
#define GLV_MOUSE_EVENT_LEFT_PRESS		GLV_MOUSE_EVENT_PRESS
#define GLV_MOUSE_EVENT_LEFT_MOTION		GLV_MOUSE_EVENT_MOTION
#define GLV_MOUSE_EVENT_MIDDLE_RELEASE	(4)
#define GLV_MOUSE_EVENT_MIDDLE_PRESS	(8)
#define GLV_MOUSE_EVENT_RIGHT_RELEASE	(16)
#define GLV_MOUSE_EVENT_RIGHT_PRESS		(32)
#define GLV_MOUSE_EVENT_OTHER_RELEASE	(64)
#define GLV_MOUSE_EVENT_OTHER_PRESS		(128)

#define GLV_MOUSE_EVENT_AXIS_VERTICAL_SCROLL	(0)
#define GLV_MOUSE_EVENT_AXIS_HORIZONTAL_SCROLL	(1)

#define GLV_FRAME_SHADOW_DRAW_OFF		(1)
#define GLV_FRAME_SHADOW_DRAW_ON		(2)
#define GLV_FRAME_BACK_DRAW_OFF			(1)
#define GLV_FRAME_BACK_DRAW_INNER		(2)
#define GLV_FRAME_BACK_DRAW_FULL		(3)

#define GLV_WINDOW_ATTR_DEFAULT					(0)	// ウィンドウ属性を指定しない
#define GLV_WINDOW_ATTR_NON_TRANSPARENT			(1)	// ウィンドウを不透明にする
#define GLV_WINDOW_ATTR_DISABLE_POINTER_EVENT	(2)	// ポインターイベントを受け取らない
#define GLV_WINDOW_ATTR_POINTER_MOTION			(4)	// 左マウスボタン押下していない場合でもマウス移動位置を通知する

#define GLV_WIGET_ATTR_NO_OPTIONS		(0)
#define GLV_WIGET_ATTR_PUSH_ACTION		(1)		// 左マウスボタン押下を通知する
#define GLV_WIGET_ATTR_POINTER_FOCUS	(2)		// 左マウスボタン押下していない場合でもマウス移動でフォーカスを設定する
#define GLV_WIGET_ATTR_POINTER_MOTION	(4)		// 左マウスボタン押下していない場合でもマウス移動位置を通知する
#define GLV_WIGET_ATTR_TEXT_INPUT_FOCUS	(8)		// 左マウスボタン押下時に入力フォーカスを設定する

#define GLV_INVISIBLE				(0)
#define GLV_VISIBLE					(1)

#define GLV_KEY_KIND_ASCII	(1)
#define GLV_KEY_KIND_CTRL	(2)
#define GLV_KEY_KIND_IM		(3)
#define GLV_KEY_KIND_INSERT	(4)

#define GLV_KEY_STATE_IM_OFF		(0)
#define GLV_KEY_STATE_IM_PREEDIT	(1)
#define GLV_KEY_STATE_IM_COMMIT		(2)
#define GLV_KEY_STATE_IM_HIDE		(3)
#define GLV_KEY_STATE_IM_RESET		(4)

#define GLV_SHIFT_MASK		(1 << 0)
#define GLV_LOCK_MASK		(1 << 1)
#define GLV_CONTROL_MASK	(1 << 2)
#define GLV_MOD1_MASK		(1 << 3)
#define GLV_MOD2_MASK		(1 << 4)
#define GLV_MOD3_MASK		(1 << 5)
#define GLV_MOD4_MASK		(1 << 6)
#define GLV_MOD5_MASK		(1 << 7)
#define GLV_SUPER_MASK		(1 << 26)
#define GLV_HYPER_MASK		(1 << 27)
#define GLV_META_MASK		(1 << 28)
#define GLV_MODIFIER_MASK	(0x5f001fff)

//-----------------------------------
// 型定義
//-----------------------------------
typedef	void	*glvDisplay;		//
typedef	void	*glvWindow;			//
typedef	void	*glvSheet;			//
typedef	void	*glvWiget;			//
typedef	void	*glvResource;		//

typedef uint32_t	glvTime;
typedef size_t		glvInstanceId;

typedef struct glv_wiget_geometry_t {
	int			x;
	int			y;
	int			width;
	int			height;
	float		scale;
	glvInstanceId		wigetId;	// get only
	glvInstanceId		sheetId;	// get only
	glvInstanceId		windowId;	// get only
} GLV_WIGET_GEOMETRY_t;

typedef struct glv_wiget_status_t {
	glvInstanceId		focusId;
	glvInstanceId		selectId;
	int					selectStatus;
} GLV_WIGET_STATUS_t;

typedef struct glv_frame_info_t {
	short top_shadow_size;
	short bottom_shadow_size;
	short left_shadow_size;
	short right_shadow_size;

    short top_name_size;
    short top_cmd_menu_size;
    short top_pulldown_menu_size;
    short top_user_area_size;		// = top_name_size + top_cmd_menu_size + top_pulldown_menu_size
	short bottom_status_bar_size;
    short bottom_user_area_size;	// = bottom_status_bar_size

	short top_edge_size;
	short bottom_edge_size;
	short left_edge_size;
	short right_edge_size;

	short top_size;					// = top_shadow_size    + top_edge_size    + top_user_area_size
	short bottom_size;				// = bottom_shadow_size + bottom_edge_size + bottom_user_area_size
	short left_size;				// = left_shadow_size   + left_edge_size
	short right_size;				// = right_shadow_size  + right_edge_size

	short inner_width;
	short inner_height;
	short frame_width;				// = inner_width  + left_size + right_size
	short frame_height;				// = inner_height + top_size + bottom_size
}GLV_FRAME_INFO_t;

#define GLV_W_MENU_LIST_MAX     (25)

struct glv_w_menu_item_t {
	char	*text;
	short	length;
	short	attrib;
	short	next;
	int		functionId;
};

typedef struct glv_w_menu_t {
	short	id;
	short	num;
	short	width;
	short	height;
	struct glv_w_menu_item_t	item[GLV_W_MENU_LIST_MAX];
}GLV_W_MENU_t;

enum cursor_type {
	CURSOR_BOTTOM_LEFT,
	CURSOR_BOTTOM_RIGHT,
	CURSOR_BOTTOM,
	CURSOR_DRAGGING,
	CURSOR_LEFT_PTR,
	CURSOR_LEFT,
	CURSOR_RIGHT,
	CURSOR_TOP_LEFT,
	CURSOR_TOP_RIGHT,
	CURSOR_TOP,
	CURSOR_IBEAM,
	CURSOR_HAND1,
	CURSOR_WATCH,
	CURSOR_DND_MOVE,
	CURSOR_DND_COPY,
	CURSOR_DND_FORBIDDEN,
	CURSOR_V_DOUBLE_ARROW,

	CURSOR_BLANK,
	CURSOR_NUM
};

//-----------------------------------
// 関数ポインタ定義
//-----------------------------------
typedef int (*GLV_WINDOW_EVENT_FUNC_new_t)(glvWindow glv_win);
typedef int (*GLV_WINDOW_EVENT_FUNC_init_t)(glvWindow glv_win,int width, int height);
typedef int (*GLV_WINDOW_EVENT_FUNC_start_t)(glvWindow glv_win,int width, int height);
typedef int (*GLV_WINDOW_EVENT_FUNC_configure_t)(glvWindow glv_win,int width, int height);
typedef int (*GLV_WINDOW_EVENT_FUNC_reshape_t)(glvWindow glv_win,int width, int height);
typedef int (*GLV_WINDOW_EVENT_FUNC_redraw_t)(glvWindow glv_win,int drawStat);
typedef int (*GLV_WINDOW_EVENT_FUNC_update_t)(glvWindow glv_win,int drawStat);
typedef int (*GLV_WINDOW_EVENT_FUNC_timer_t)(glvWindow glv_win,int group,int id);
typedef int (*GLV_WINDOW_EVENT_FUNC_mousePointer_t)(glvWindow glv_win,int type,glvTime time,int x,int y,int pointer_stat);
typedef int (*GLV_WINDOW_EVENT_FUNC_mouseButton_t)(glvWindow glv_win,int type,glvTime time,int x,int y,int pointer_stat);
typedef int (*GLV_WINDOW_EVENT_FUNC_mouseAxis_t)(glvWindow glv_win,int type,glvTime time,int value);
typedef int (*GLV_WINDOW_EVENT_FUNC_gesture_t)(glvWindow glv_win,int eventType,int x,int y,int distance_x,int distance_y,int velocity_x,int velocity_y);
typedef int (*GLV_WINDOW_EVENT_FUNC_cursor_t)(glvWindow glv_win,int width, int height,int pos_x,int pos_y);
typedef int (*GLV_WINDOW_EVENT_FUNC_userMsg_t)(glvWindow glv_win,int kind,void *data);
typedef int (*GLV_WINDOW_EVENT_FUNC_action_t)(glvWindow glv_win,int action,glvInstanceId functionId);
typedef int (*GLV_WINDOW_EVENT_FUNC_key_t)(glvWindow glv_win,unsigned int key,unsigned int modifiers,unsigned int state);
typedef int (*GLV_WINDOW_EVENT_FUNC_endDraw_t)(glvWindow glv_win,glvTime time);
typedef int (*GLV_WINDOW_EVENT_FUNC_terminate_t)(glvWindow glv_win);

typedef int (*GLV_SHEET_EVENT_FUNC_new_t)(glvWindow glv_win,glvSheet sheet);
typedef int (*GLV_SHEET_EVENT_FUNC_init_t)(glvWindow glv_win,glvSheet sheet,int window_width, int window_height);
typedef int (*GLV_SHEET_EVENT_FUNC_reshape_t)(glvWindow glv_win,glvSheet sheet,int window_width, int window_height);
typedef int (*GLV_SHEET_EVENT_FUNC_redraw_t)(glvWindow glv_win,glvSheet sheet,int drawStat);
typedef int (*GLV_SHEET_EVENT_FUNC_update_t)(glvWindow glv_win,glvSheet sheet,int drawStat);
typedef int (*GLV_SHEET_EVENT_FUNC_timer_t)(glvWindow glv_win,glvSheet sheet,int group,int id);
typedef int (*GLV_SHEET_EVENT_FUNC_mousePointer_t)(glvWindow glv_win,glvSheet sheet,int type,glvTime time,int x,int y,int pointer_stat);
typedef int (*GLV_SHEET_EVENT_FUNC_mouseButton_t)(glvWindow glv_win,glvSheet sheet,int type,glvTime time,int x,int y,int pointer_stat);
typedef int (*GLV_SHEET_EVENT_FUNC_mouseAxis_t)(glvWindow glv_win,glvSheet sheet,int type,glvTime time,int value);
typedef int (*GLV_SHEET_EVENT_FUNC_action_t)(glvWindow glv_win,glvSheet sheet,int action,glvInstanceId selectId);
typedef int (*GLV_SHEET_EVENT_FUNC_userMsg_t)(glvWindow glv_win,glvSheet sheet,int kind,void *data);
typedef int (*GLV_SHEET_EVENT_FUNC_terminate_t)(glvSheet sheet);

typedef int (*GLV_WIGET_EVENT_FUNC_new_t)(glvWindow glv_win,glvSheet sheet,glvWiget wiget);
typedef int (*GLV_WIGET_EVENT_FUNC_init_t)(glvWindow glv_win,glvSheet sheet,glvWiget wiget);
typedef int (*GLV_WIGET_EVENT_FUNC_redraw_t)(glvWindow glv_win,glvSheet sheet,glvWiget wiget);
typedef int (*GLV_WIGET_EVENT_FUNC_mousePointer_t)(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int type,glvTime time,int x,int y,int pointer_left_stat);
typedef int (*GLV_WIGET_EVENT_FUNC_mouseButton_t)(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int type,glvTime time,int x,int y,int pointer_stat);
typedef int (*GLV_WIGET_EVENT_FUNC_mouseAxis_t)(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int type,glvTime time,int value);
typedef int (*GLV_WIGET_EVENT_FUNC_input_t)(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int kind,int state,uint32_t kyeSym,int *text,uint8_t *attr,int length);
typedef int (*GLV_WIGET_EVENT_FUNC_focus_t)(glvWindow glv_win,glvSheet sheet,glvWiget wiget,int focus_stat,glvWiget in_wigwt);
typedef int (*GLV_WIGET_EVENT_FUNC_terminate_t)(glvWiget wiget);

struct glv_frame_listener {
	GLV_WINDOW_EVENT_FUNC_start_t		start;
	GLV_WINDOW_EVENT_FUNC_configure_t	configure;
	GLV_WINDOW_EVENT_FUNC_terminate_t	terminate;
	int									cmdMenu;
	int									pullDownMenu;
	int									statusBar;
	int									back;
	int									shadow;
	GLV_WINDOW_EVENT_FUNC_action_t		action;
	GLV_WINDOW_EVENT_FUNC_key_t			key;
};

struct glv_window_listener {
	struct glv_window_listener				*_class;
	GLV_WINDOW_EVENT_FUNC_new_t				_new;
	int										attr;
	int										beauty;
	GLV_WINDOW_EVENT_FUNC_init_t			init;
	GLV_WINDOW_EVENT_FUNC_reshape_t			reshape;
	GLV_WINDOW_EVENT_FUNC_redraw_t			redraw;
	GLV_WINDOW_EVENT_FUNC_update_t			update;
	GLV_WINDOW_EVENT_FUNC_timer_t			timer;
	GLV_WINDOW_EVENT_FUNC_mousePointer_t	mousePointer;
	GLV_WINDOW_EVENT_FUNC_mouseButton_t		mouseButton;
	GLV_WINDOW_EVENT_FUNC_mouseAxis_t		mouseAxis;
	GLV_WINDOW_EVENT_FUNC_gesture_t			gesture;
	GLV_WINDOW_EVENT_FUNC_cursor_t			cursor;
	GLV_WINDOW_EVENT_FUNC_userMsg_t			userMsg;
	GLV_WINDOW_EVENT_FUNC_endDraw_t			endDraw;
	GLV_WINDOW_EVENT_FUNC_terminate_t		terminate;
};

struct glv_sheet_listener {
	struct glv_sheet_listener			*_class;
	GLV_SHEET_EVENT_FUNC_new_t			_new;
	GLV_SHEET_EVENT_FUNC_init_t			init;
	GLV_SHEET_EVENT_FUNC_reshape_t		reshape;
	GLV_SHEET_EVENT_FUNC_redraw_t		redraw;
	GLV_SHEET_EVENT_FUNC_update_t		update;
	GLV_SHEET_EVENT_FUNC_timer_t		timer;
	GLV_SHEET_EVENT_FUNC_mousePointer_t	mousePointer;
	GLV_SHEET_EVENT_FUNC_mouseButton_t	mouseButton;
	GLV_SHEET_EVENT_FUNC_mouseAxis_t	mouseAxis;
	GLV_SHEET_EVENT_FUNC_action_t		action;
	GLV_SHEET_EVENT_FUNC_userMsg_t		userMsg;
	GLV_SHEET_EVENT_FUNC_terminate_t	terminate;
};

struct glv_wiget_listener {
	struct glv_wiget_listener			*_class;
	GLV_WIGET_EVENT_FUNC_new_t			_new;
	int									attr;
	GLV_WIGET_EVENT_FUNC_init_t			init;
	GLV_WIGET_EVENT_FUNC_redraw_t		redraw;
	GLV_WIGET_EVENT_FUNC_mousePointer_t	mousePointer;
    GLV_WIGET_EVENT_FUNC_mouseButton_t	mouseButton;
	GLV_WIGET_EVENT_FUNC_mouseAxis_t	mouseAxis;
	GLV_WIGET_EVENT_FUNC_input_t		input;
	GLV_WIGET_EVENT_FUNC_focus_t		focus;
	GLV_WIGET_EVENT_FUNC_terminate_t	terminate;
};

typedef struct _glvinputfunc {
	int (*keyboard_function_key)(unsigned int key,unsigned int modifiers,unsigned int state);
	int (*touch_down)(int pointer_sx,int pointer_sy);
	int (*touch_up)(int pointer_sx,int pointer_sy);
} GLVINPUTFUNC_t;

#define GLV_R_VALUE_IO_SET			(1)	
#define GLV_R_VALUE_IO_GET			(2)

/*
Zz size_t(uint64_t)	: GLV_R_VALUE_TYPE__SIZE
Ll int64_t			: GLV_R_VALUE_TYPE__INT64
Ii int32_t			: GLV_R_VALUE_TYPE__INT32
Uu uint32_t			: GLV_R_VALUE_TYPE__UINT32
Cc uint32_t(color)	: GLV_R_VALUE_TYPE__COLOR
Ss string			: GLV_R_VALUE_TYPE__STRING
Pp pointer			: GLV_R_VALUE_TYPE__POINTER
Rr double			: GLV_R_VALUE_TYPE__DOUBLE
Tt function			: GLV_R_VALUE_TYPE__FUNCTION
*/
#define GLV_R_VALUE_TYPE__NOTHING		(0)		// 未確定
#define GLV_R_VALUE_TYPE__SIZE			(1)		// 8byte: unsigned long , size_t
#define GLV_R_VALUE_TYPE__INT64			(2)		// 8byte: signed long
#define GLV_R_VALUE_TYPE__INT32			(3)		// 4baye: signed int
#define GLV_R_VALUE_TYPE__UINT32		(4)		// 4baye: unsigned int
#define GLV_R_VALUE_TYPE__COLOR			(5)		// 4byte: GLV_SET_RGBA(r,g,b,a)
#define GLV_R_VALUE_TYPE__STRING		(6)		// 8byte: string(UTF8) pointer
#define GLV_R_VALUE_TYPE__POINTER		(7)		// 8byte: data pointer
#define GLV_R_VALUE_TYPE__DOUBLE		(8)		// 8byte: double
#define GLV_R_VALUE_TYPE__FUNCTION		(9)		// 8byte: function pointer

#define _GLV_R_VALUE_MAX		(7)

extern struct _glv_r_value *_glv_r_value;

typedef int (*GLV_R_TRIGGER_t)(int io,struct _glv_r_value *value);

typedef struct _glv_r_value {
    struct _glv_r_value  *link;
	char	*key;
	void	*instance;
	GLV_R_TRIGGER_t func;
	char	*abstract;
	char	*type_string;
	int		valueNum;
	struct {
		int		type;
		char	*abstract;
		/*
		Zz size_t(uint64_t)	: GLV_R_VALUE_TYPE__SIZE
		Ll int64_t			: GLV_R_VALUE_TYPE__INT64
		Ii int32_t			: GLV_R_VALUE_TYPE__INT32
		Uu uint32_t			: GLV_R_VALUE_TYPE__UINT32
		Cc uint32_t(color)	: GLV_R_VALUE_TYPE__COLOR
		Ss string			: GLV_R_VALUE_TYPE__STRING
		Pp pointer			: GLV_R_VALUE_TYPE__POINTER
		Rr double			: GLV_R_VALUE_TYPE__DOUBLE
		*/
		union value {
			size_t	size;
			int64_t	int64;
			int		int32;
			uint32_t uint32;
			uint32_t color;
			void	*string;
			void	*pointer;
			double	real;
		}v;
	}n[_GLV_R_VALUE_MAX+1];
}GLV_R_VALUE_t;

extern GLVINPUTFUNC_t	glv_input_func;

//-----------------------------------
void	glvGetVersion(int *major,int *minor,int *patch);
void	glvSetDebugFlag(int flag);
int		glvCheckDebugFlag(int flag);

glvResource glvCreateResource(void);
void glvDestroyResource(glvResource res);

glvDisplay	glvOpenDisplay(char *dpyName);
int			glvCloseDisplay(glvDisplay glv_dpy);

void glvEnterEventLoop(glvDisplay glv_dpy);
void glvEscapeEventLoop(void *glv_instance);

glvWindow glvCreateFrameWindow(void *glv_instance,const struct glv_frame_listener *listener,char *name,char *title,int width, int height,glvInstanceId *id);
glvWindow glvCreateWindow(glvWindow parent,const struct glv_window_listener *listener,char *name,int x, int y, int width, int height,int attr,glvInstanceId *id);
glvWindow glvCreateThreadWindow(glvWindow parent,const struct glv_window_listener *listener,char *name,int x, int y, int width, int height,int attr,glvInstanceId *id);
glvWindow glvCreateChildWindow(glvWindow parent,const struct glv_window_listener *listener,char *name,int x, int y, int width, int height,int attr,glvInstanceId *id);

void glvDestroyWindow(glvWindow *glv_win);

glvWindow glvGetWindowFromId(glvDisplay glv_dpy,glvInstanceId windowId);
int glvWindow_isAliveWindow(void *glv_instance,glvInstanceId windowId);
glvTime glvWindow_getLastTime(glvWindow glv_win);
char *glvWindow_getWindowName(glvWindow glv_win);
int glvReqSwapBuffers(glvWindow glv_win);

int glvWindow_setTitle(glvWindow glv_win,const char *title);
int glvWindow_setInnerSize(glvWindow glv_win,int width, int height);
int glvOnReShape(glvWindow glv_win,int x, int y,int width, int height);
int glvOnReDraw(glvWindow glv_win);
int glvOnUpdate(glvWindow glv_win);
int glvOnGesture(glvWindow glv_win,int eventType,int x,int y,int distance_x,int distance_y,int velocity_x,int velocity_y);
int glvOnAction(void *glv_instance,int action,glvInstanceId selectId);
int glvOnUserMsg(glvWindow glv_win,int kind,void *data,size_t size);

int glvCreate_mTimer(glvWindow glv_win,int group,int id,int type,int mTime);
int glvCreate_uTimer(glvWindow glv_win,int group,int id,int type,int64_t tv_sec,int64_t tv_nsec);
int glvStartTimer(glvWindow glv_win,int id);
int glvStopTimer(glvWindow glv_win,int id);

void glvSheet_setHandler_init(glvSheet sheet,GLV_SHEET_EVENT_FUNC_init_t init);
void glvSheet_setHandler_reshape(glvSheet sheet,GLV_SHEET_EVENT_FUNC_reshape_t reshape);
void glvSheet_setHandler_redraw(glvSheet sheet,GLV_SHEET_EVENT_FUNC_redraw_t redraw);
void glvSheet_setHandler_update(glvSheet sheet,GLV_SHEET_EVENT_FUNC_update_t update);
void glvSheet_setHandler_timer(glvSheet sheet,GLV_SHEET_EVENT_FUNC_timer_t timer);
void glvSheet_setHandler_mousePointer(glvSheet sheet,GLV_SHEET_EVENT_FUNC_mousePointer_t mousePointer);
void glvSheet_setHandler_mouseButton(glvSheet sheet,GLV_SHEET_EVENT_FUNC_mouseButton_t mouseButton);
void glvSheet_setHandler_mouseAxis(glvSheet sheet,GLV_SHEET_EVENT_FUNC_mouseAxis_t mouseAxis);
void glvSheet_setHandler_action(glvSheet sheet,GLV_SHEET_EVENT_FUNC_action_t action);
void glvSheet_setHandler_userMsg(glvSheet sheet,GLV_SHEET_EVENT_FUNC_userMsg_t userMsg);
void glvSheet_setHandler_terminate(glvSheet sheet,GLV_SHEET_EVENT_FUNC_terminate_t terminate);

void glvWiget_setHandler_init(glvWiget wiget,GLV_WIGET_EVENT_FUNC_init_t init);
void glvWiget_setHandler_redraw(glvWiget wiget,GLV_WIGET_EVENT_FUNC_redraw_t redraw);
void glvWiget_setHandler_mousePointer(glvWiget wiget,GLV_WIGET_EVENT_FUNC_mousePointer_t mousePointer);
void glvWiget_setHandler_mouseButton(glvWiget wiget,GLV_WIGET_EVENT_FUNC_mouseButton_t mouseButton);
void glvWiget_setHandler_mouseAxis(glvWiget wiget,GLV_WIGET_EVENT_FUNC_mouseAxis_t mouseAxis);
void glvWiget_setHandler_input(glvWiget wiget,GLV_WIGET_EVENT_FUNC_input_t input);
void glvWiget_setHandler_focus(glvWiget wiget,GLV_WIGET_EVENT_FUNC_focus_t focus);
void glvWiget_setHandler_terminate(glvWiget wiget,GLV_WIGET_EVENT_FUNC_terminate_t terminate);

glvSheet glvCreateSheet(glvWindow glv_win,const struct glv_sheet_listener *listener,char *name);
void glvDestroySheet(glvSheet glv_sheet);
glvWiget glvCreateWiget(glvSheet glv_sheet,const struct glv_wiget_listener *listener,int attr);
void glvDestroyWiget(glvWiget glv_wiget);

int glvWiget_setWigetGeometry(glvWiget wiget,GLV_WIGET_GEOMETRY_t *geometry);
int glvWiget_getWigetGeometry(glvWiget wiget,GLV_WIGET_GEOMETRY_t *geometry);
int glvWiget_setWigetVisible(glvWiget wiget,int visible);
int glvWiget_getWigetVisible(glvWiget wiget);
int glvWiget_setIMECandidatePotition(glvWiget wiget,int candidate_pos_x,int candidate_pos_y);

int glvSheet_reqSwapBuffers(glvSheet sheet);
int glvSheet_reqDrawWigets(glvSheet sheet);
int glvSheet_getSelectWigetStatus(glvSheet sheet,GLV_WIGET_STATUS_t *wigetStatus);
int glvWiget_kindSelectWigetStatus(glvWiget wiget,GLV_WIGET_STATUS_t *wigetStatus);

void glvWindow_setViewport(glvWindow glv_win,int width,int height);

int glv_isInsertMode(void *glv_instance);
int glv_isPullDownMenu(void *glv_instance);
int glv_isCmdMenu(void *glv_instance);
glvDisplay glv_getDisplay(void *glv_instance);
glvWindow glv_getWindow(void *glv_instance);
glvWindow glv_getFrameWindow(void *glv_instance);
int glv_getWindowType(void *glv_instance);
int glv_getFrameInfo(void *glv_instance,GLV_FRAME_INFO_t *frameInfo);
glvInstanceId glv_getInstanceId(void *glv_instance);
int glvWiget_setFocus(glvWiget wiget);

int glvWindow_activeSheet(glvWindow glv_win,glvSheet sheet);
int glvWindow_inactiveSheet(glvWindow glv_win);

int glv_menu_init(GLV_W_MENU_t *menu);
int glv_menu_free(GLV_W_MENU_t *menu);
int glv_menu_getFunctionId(GLV_W_MENU_t *menu,int select);
int glv_menu_getNext(GLV_W_MENU_t *menu,int select);
int glv_menu_searchFunctionId(GLV_W_MENU_t *menu,int functionId);
int glv_menu_setItem(GLV_W_MENU_t *menu,int n,char *text,int attr,int next,int functionId);

int glv_setValue(void *glv_instance,char *key,char *type_string,...);
int glv_getValue(void *glv_instance,char *key,char *type_string,...);
int glv_setAbstract(void *glv_instance,char *key,char *abstract,char *type_string,...);
void glv_printValue(void *glv_instance,char *note);

int glv_allocUserData(void *glv_instance,size_t size);
void *glv_getUserData(void *glv_instance);

#ifdef __cplusplus
}
#endif

#include "glview_font.h"
#include "glview_part.h"

#endif /* GLVIEW_H */

