/*
 * Copyright © 2022 T.Aikawa
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

#define _GNU_SOURCE

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <xkbcommon/xkbcommon.h>

#include "wayland-util.h"
#include "glview.h"
#include "weston-client-window.h"
#include "glview_local.h"

/*
	c言語python3用定数値連携処理インターフェース(Python 3.9.7で動作確認済み)
	pythonのctypesを利用してshared libraryを呼び出す場合、通常、
	C言語側で#defineで設定した値をpython側では変数として手入力しているが、
	プリプロセッサー処理が無いため、煩雑であり、間違ったり、
	修正漏れによる不具合が出る可能性が高い。
	その為、同期合わせする仕組みを作成した。

・C言語側条件
	記号定数マクロのみ指定可能
	引数付きマクロの場合は、一度記号定数マクロで定義して使用する
	マクロ展開後に定数に展開できない場合は、使用できない(変数が含まれる場合など)
*/
/*
// 使用例

// C言語側）

// 以下の3つのマクロの値をpython側に渡したい場合
#define HOGE_CONST1	0xff123456
#define HOGE_CONST2	"[test string test]"
#define HOGE_CONST3	(132.6)

// GLV_CONST_DEFINE(マクロ名、型);
// マクロの入れ子（多重定義）も展開される
// 記号定数マクロのみ指定可能なので、引数付きマクロの内容を
// 型の名前は、ctypesの型定義名称から'c_'を外した名称が設定できる
// 			(size_t,int8,uint8,int16),uint16,int32,uint32,int64,uint64,char_p,float,double)
GLV_CONST_DEFINE(HOGE_CONST1,uint32);
GLV_CONST_DEFINE(HOGE_CONST2,char_p);
GLV_CONST_DEFINE(HOGE_CONST3,float);

//例えば、GLV_CONST_DEFINE(HOGE_CONST1,uint32);と記述した場合、以下の処理に展開される
struct c_const_value glv_c__HOGE_CONST1 = {.type = "c_""uint32" , .v.c_uint32 = (0xff123456)};

// (python言語側）

HOGE_CONST1 = glv_linking_value('HOGE_CONST1')
HOGE_CONST2 = glv_linking_value('HOGE_CONST2')
HOGE_CONST3 = glv_linking_value('HOGE_CONST3')

print('HOGE_CONST1:',type(HOGE_CONST1),'{:x}'.format(HOGE_CONST1))
print('HOGE_CONST2:',type(HOGE_CONST2),HOGE_CONST2)
print('HOGE_CONST3:',type(HOGE_CONST3),HOGE_CONST3)

*/

// pythonのインターフェース実装は以下を参照の事
/*
from ctypes import *

class c_Structure(Structure):
    pass

class c_union(Union):
    pass

class   glv_c_interface_v(c_union):
    _fields_ = [("c_size_t", c_size_t),
                ("c_int8", c_int8),
                ("c_uint8", c_uint8),
                ("c_int16", c_int16),
                ("c_uint16", c_uint16),
                ("c_int32", c_int32),
                ("c_uint32", c_uint32),
                ("c_int64", c_int64),
                ("c_uint64", c_uint64),
                ("c_char_p", c_char_p),
                ("c_float", c_float),
                ("c_double", c_double)]

class glv_c_interface(c_Structure):
    _fields_ = [("data_type", c_char_p),
                ("data", glv_c_interface_v)]

def glv_linking_value(name):
    '''
    get consttant data into shared library.
    '''
	address = getattr(glview, 'glv_c__' + name, None)
    if address:
        data = cast(address,POINTER(glv_c_interface)).contents
        value = eval(b'data.data.' + data.data_type)
        #print('data_type:',type(data.data_type),data.data_type)
        #print('value:',type(value),value)
        data_type = data.data_type.decode('utf-8')
        if data_type == b'c_char_p':
            value = value.decode('utf-8')
        #
        if 0:
            if data_type in ['c_int8','c_int16','c_int32','c_int64',
                                    'c_size_t','c_float','c_double','c_char_p']:
                print("loading {} typeof({}):\t{:5}".format(name,type(value),value))
            elif data_type in ['c_uint8','c_uint16','c_uint32','c_uint64']:
                print("loading {} typeof({}):\t{:5}(0x{:x})".format(name,type(value),value,value))    
        #
        return value
    else:
        print('error:',name,' is not found into shared library.')
    return None
*/

struct c_const_value {
   char *type;
   union{
	size_t		c_size_t;
	int8_t		c_int8;
	uint8_t		c_uint8;
	int16_t		c_int16;
	uint16_t	c_uint16;
	int32_t		c_int32;
	uint32_t	c_uint32;
	int64_t		c_int64;
	uint64_t	c_uint64;
	char		*c_char_p;
	float		c_float;
	double  	c_double;
   }v;
};

#define GLV_CONST_DEFINE(a,b)       GLV_CONST_DEFINE2(_##a,b,a)
#define GLV_CONST_DEFINE2(a,b,c)    struct c_const_value glv_c_##a = {.type = "c_"#b , .v.c_##b = (c)}

#if 0
#define AIKAWA_CONST	0xff123456
#define AIKAWA_CONST2	"[test string test]"
#define AIKAWA_CONST3	(132.6)
GLV_CONST_DEFINE(AIKAWA_CONST,uint32);
GLV_CONST_DEFINE(AIKAWA_CONST2,char_p);
GLV_CONST_DEFINE(AIKAWA_CONST3,float);
#endif

// #####################################################################################################
// glview/glview.h
GLV_CONST_DEFINE(GLV_TYPE_THREAD_FRAME,int16);
GLV_CONST_DEFINE(GLV_TYPE_THREAD_WINDOW,int16);
GLV_CONST_DEFINE(GLV_TYPE_CHILD_WINDOW,int16);

GLV_CONST_DEFINE(GLV_INSTANCE_ALIVE,int16);
GLV_CONST_DEFINE(GLV_INSTANCE_DEAD,int16);

GLV_CONST_DEFINE(GLV_KEYBOARD_KEY_STATE_RELEASED,int16);
GLV_CONST_DEFINE(GLV_KEYBOARD_KEY_STATE_PRESSED,int16);

GLV_CONST_DEFINE(GLV_STAT_DRAW_REDRAW,int16);
GLV_CONST_DEFINE(GLV_STAT_DRAW_UPDATE,int16);
GLV_CONST_DEFINE(GLV_STAT_IN_FOCUS,int16);
GLV_CONST_DEFINE(GLV_STAT_OUT_FOCUS,int16);

GLV_CONST_DEFINE(GLV_ACTION_WIGET,int16);
GLV_CONST_DEFINE(GLV_ACTION_PULL_DOWN_MENU,int16);
GLV_CONST_DEFINE(GLV_ACTION_CMD_MENU,int16);

GLV_CONST_DEFINE(GLV_ERROR,int16);
GLV_CONST_DEFINE(GLV_OK,int16);

GLV_CONST_DEFINE(GLV_TIMER_ONLY_ONCE,int16);
GLV_CONST_DEFINE(GLV_TIMER_REPEAT,int16);

GLV_CONST_DEFINE(GLV_GESTURE_EVENT_DOWN,int16);
GLV_CONST_DEFINE(GLV_GESTURE_EVENT_SINGLE_UP,int16);
GLV_CONST_DEFINE(GLV_GESTURE_EVENT_SCROLL,int16);
GLV_CONST_DEFINE(GLV_GESTURE_EVENT_FLING,int16);
GLV_CONST_DEFINE(GLV_GESTURE_EVENT_SCROLL_STOP,int16);

GLV_CONST_DEFINE(GLV_MOUSE_EVENT_RELEASE,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_PRESS,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_MOTION,int16);

GLV_CONST_DEFINE(GLV_WIGET_STATUS_RELEASE,int16);
GLV_CONST_DEFINE(GLV_WIGET_STATUS_PRESS,int16);
GLV_CONST_DEFINE(GLV_WIGET_STATUS_FOCUS,int16);

GLV_CONST_DEFINE(GLV_MOUSE_EVENT_LEFT_RELEASE,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_LEFT_PRESS,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_LEFT_MOTION,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_MIDDLE_RELEASE,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_MIDDLE_PRESS,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_RIGHT_RELEASE,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_RIGHT_PRESS,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_OTHER_RELEASE,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_OTHER_PRESS,int16);

GLV_CONST_DEFINE(GLV_MOUSE_EVENT_AXIS_VERTICAL_SCROLL,int16);
GLV_CONST_DEFINE(GLV_MOUSE_EVENT_AXIS_HORIZONTAL_SCROLL,int16);

GLV_CONST_DEFINE(GLV_FRAME_SHADOW_DRAW_OFF,int16);
GLV_CONST_DEFINE(GLV_FRAME_SHADOW_DRAW_ON,int16);
GLV_CONST_DEFINE(GLV_FRAME_BACK_DRAW_OFF,int16);
GLV_CONST_DEFINE(GLV_FRAME_BACK_DRAW_INNER,int16);
GLV_CONST_DEFINE(GLV_FRAME_BACK_DRAW_FULL,int16);

GLV_CONST_DEFINE(GLV_WINDOW_ATTR_DEFAULT,int16);	// ウィンドウ属性を指定しない
GLV_CONST_DEFINE(GLV_WINDOW_ATTR_NON_TRANSPARENT,int16);	// ウィンドウを不透明にする
GLV_CONST_DEFINE(GLV_WINDOW_ATTR_DISABLE_POINTER_EVENT,int16);	// ポインターイベントを受け取らない
GLV_CONST_DEFINE(GLV_WINDOW_ATTR_POINTER_MOTION	,int16);	// 左マウスボタン押下していない場合でもマウス移動位置を通知する

GLV_CONST_DEFINE(GLV_WIGET_ATTR_NO_OPTIONS,int16);
GLV_CONST_DEFINE(GLV_WIGET_ATTR_PUSH_ACTION	,int16);		// 左マウスボタン押下を通知する
GLV_CONST_DEFINE(GLV_WIGET_ATTR_POINTER_FOCUS,int16);		// 左マウスボタン押下していない場合でもマウス移動でフォーカスを設定する
GLV_CONST_DEFINE(GLV_WIGET_ATTR_POINTER_MOTION,int16);		// 左マウスボタン押下していない場合でもマウス移動位置を通知する
GLV_CONST_DEFINE(GLV_WIGET_ATTR_TEXT_INPUT_FOCUS,int16);		// 左マウスボタン押下時に入力フォーカスを設定する

GLV_CONST_DEFINE(GLV_INVISIBLE,int16);
GLV_CONST_DEFINE(GLV_VISIBLE,int16);

GLV_CONST_DEFINE(GLV_KEY_KIND_ASCII,int16);
GLV_CONST_DEFINE(GLV_KEY_KIND_CTRL,int16);
GLV_CONST_DEFINE(GLV_KEY_KIND_IM,int16);
GLV_CONST_DEFINE(GLV_KEY_KIND_INSERT,int16);

GLV_CONST_DEFINE(GLV_KEY_STATE_IM_OFF,int16);
GLV_CONST_DEFINE(GLV_KEY_STATE_IM_PREEDIT,int16);
GLV_CONST_DEFINE(GLV_KEY_STATE_IM_COMMIT,int16);
GLV_CONST_DEFINE(GLV_KEY_STATE_IM_HIDE,int16);
GLV_CONST_DEFINE(GLV_KEY_STATE_IM_RESET,int16);

GLV_CONST_DEFINE(GLV_SHIFT_MASK,uint32);
GLV_CONST_DEFINE(GLV_LOCK_MASK,uint32);
GLV_CONST_DEFINE(GLV_CONTROL_MASK,uint32);
GLV_CONST_DEFINE(GLV_MOD1_MASK,uint32);
GLV_CONST_DEFINE(GLV_MOD2_MASK,uint32);
GLV_CONST_DEFINE(GLV_MOD3_MASK,uint32);
GLV_CONST_DEFINE(GLV_MOD4_MASK,uint32);
GLV_CONST_DEFINE(GLV_MOD5_MASK,uint32);
GLV_CONST_DEFINE(GLV_SUPER_MASK,uint32);
GLV_CONST_DEFINE(GLV_HYPER_MASK,uint32);
GLV_CONST_DEFINE(GLV_META_MASK,uint32);
GLV_CONST_DEFINE(GLV_MODIFIER_MASK,uint32);

GLV_CONST_DEFINE(GLV_W_MENU_LIST_MAX,int16);

GLV_CONST_DEFINE(CURSOR_BOTTOM_LEFT,int16);
GLV_CONST_DEFINE(CURSOR_BOTTOM_RIGHT,int16);
GLV_CONST_DEFINE(CURSOR_BOTTOM,int16);
GLV_CONST_DEFINE(CURSOR_DRAGGING,int16);
GLV_CONST_DEFINE(CURSOR_LEFT_PTR,int16);
GLV_CONST_DEFINE(CURSOR_LEFT,int16);
GLV_CONST_DEFINE(CURSOR_RIGHT,int16);
GLV_CONST_DEFINE(CURSOR_TOP_LEFT,int16);
GLV_CONST_DEFINE(CURSOR_TOP_RIGHT,int16);
GLV_CONST_DEFINE(CURSOR_TOP,int16);
GLV_CONST_DEFINE(CURSOR_IBEAM,int16);
GLV_CONST_DEFINE(CURSOR_HAND1,int16);
GLV_CONST_DEFINE(CURSOR_WATCH,int16);
GLV_CONST_DEFINE(CURSOR_DND_MOVE,int16);
GLV_CONST_DEFINE(CURSOR_DND_COPY,int16);
GLV_CONST_DEFINE(CURSOR_DND_FORBIDDEN,int16);
GLV_CONST_DEFINE(CURSOR_V_DOUBLE_ARROW,int16);
GLV_CONST_DEFINE(CURSOR_BLANK,int16);
GLV_CONST_DEFINE(CURSOR_NUM,int16);

GLV_CONST_DEFINE(GLV_R_VALUE_IO_SET,int16);
GLV_CONST_DEFINE(GLV_R_VALUE_IO_GET,int16);

GLV_CONST_DEFINE(GLV_R_VALUE_TYPE__NOTHING,int16);	// 未確定
GLV_CONST_DEFINE(GLV_R_VALUE_TYPE__SIZE,int16);		// 8byte: unsigned long , size_t
GLV_CONST_DEFINE(GLV_R_VALUE_TYPE__INT64,int16);	// 8byte: signed long , size_t
GLV_CONST_DEFINE(GLV_R_VALUE_TYPE__INT32,int16);	// 4baye: signed int
GLV_CONST_DEFINE(GLV_R_VALUE_TYPE__UINT32,int16);	// 4baye: unsigned int
GLV_CONST_DEFINE(GLV_R_VALUE_TYPE__STRING,int16);	// 8byte: string(UTF8) pointer
GLV_CONST_DEFINE(GLV_R_VALUE_TYPE__POINTER,int16);	// 8byte: data pointer
GLV_CONST_DEFINE(GLV_R_VALUE_TYPE__DOUBLE,int16);	// 8byte: double
GLV_CONST_DEFINE(GLV_R_VALUE_TYPE__FUNCTION,int16);	// 8byte: function pointer
GLV_CONST_DEFINE(GLV_R_VALUE_TYPE__COLOR,int16);	// 4byte: GLV_SET_RGBA(r,g,b,a)

GLV_CONST_DEFINE(_GLV_R_VALUE_MAX,int16);

// #####################################################################################################
// xkbcommon-keysyms.h
/*
 * TTY function keys, cleverly chosen to map to ASCII, for convenience of
 * programming, but could have been arbitrary (at the cost of lookup
 * tables in client code).
 */
GLV_CONST_DEFINE(XKB_KEY_BackSpace,uint16);  /* Back space, back char */
GLV_CONST_DEFINE(XKB_KEY_Tab,uint16);
GLV_CONST_DEFINE(XKB_KEY_Linefeed,uint16);  /* Linefeed, LF */
GLV_CONST_DEFINE(XKB_KEY_Clear,uint16);
GLV_CONST_DEFINE(XKB_KEY_Return,uint16);  /* Return, enter */
GLV_CONST_DEFINE(XKB_KEY_Pause,uint16);  /* Pause, hold */
GLV_CONST_DEFINE(XKB_KEY_Scroll_Lock,uint16);
GLV_CONST_DEFINE(XKB_KEY_Sys_Req,uint16);
GLV_CONST_DEFINE(XKB_KEY_Escape,uint16);
GLV_CONST_DEFINE(XKB_KEY_Delete,uint16);  /* Delete, rubout */

/* Cursor control & motion */
GLV_CONST_DEFINE(XKB_KEY_Home,uint16);
GLV_CONST_DEFINE(XKB_KEY_Left,uint16);  /* Move left, left arrow */
GLV_CONST_DEFINE(XKB_KEY_Up,uint16);  /* Move up, up arrow */
GLV_CONST_DEFINE(XKB_KEY_Right,uint16);  /* Move right, right arrow */
GLV_CONST_DEFINE(XKB_KEY_Down,uint16);  /* Move down, down arrow */
GLV_CONST_DEFINE(XKB_KEY_Prior,uint16);  /* Prior, previous */
GLV_CONST_DEFINE(XKB_KEY_Page_Up,uint16);
GLV_CONST_DEFINE(XKB_KEY_Next,uint16);  /* Next */
GLV_CONST_DEFINE(XKB_KEY_Page_Down,uint16);
GLV_CONST_DEFINE(XKB_KEY_End,uint16);  /* EOL */
GLV_CONST_DEFINE(XKB_KEY_Begin,uint16);  /* BOL */

/* Misc functions */
GLV_CONST_DEFINE(XKB_KEY_Select,uint16);  /* Select, mark */
GLV_CONST_DEFINE(XKB_KEY_Print,uint16);
GLV_CONST_DEFINE(XKB_KEY_Execute,uint16);  /* Execute, run, do */
GLV_CONST_DEFINE(XKB_KEY_Insert,uint16);  /* Insert, insert here */
GLV_CONST_DEFINE(XKB_KEY_Undo,uint16);
GLV_CONST_DEFINE(XKB_KEY_Redo,uint16);  /* Redo, again */
GLV_CONST_DEFINE(XKB_KEY_Menu,uint16);
GLV_CONST_DEFINE(XKB_KEY_Find,uint16);  /* Find, search */
GLV_CONST_DEFINE(XKB_KEY_Cancel,uint16);  /* Cancel, stop, abort, exit */
GLV_CONST_DEFINE(XKB_KEY_Help,uint16);  /* Help */
GLV_CONST_DEFINE(XKB_KEY_Break,uint16);
GLV_CONST_DEFINE(XKB_KEY_Mode_switch,uint16);  /* Character set switch */
GLV_CONST_DEFINE(XKB_KEY_script_switch,uint16);  /* Alias for mode_switch */
GLV_CONST_DEFINE(XKB_KEY_Num_Lock,uint16);

/*
 * Auxiliary functions; note the duplicate definitions for left and right
 * function keys;  Sun keyboards and a few other manufacturers have such
 * function key groups on the left and/or right sides of the keyboard.
 * We've not found a keyboard with more than 35 function keys total.
 */

GLV_CONST_DEFINE(XKB_KEY_F1,uint16);
GLV_CONST_DEFINE(XKB_KEY_F2,uint16);
GLV_CONST_DEFINE(XKB_KEY_F3,uint16);
GLV_CONST_DEFINE(XKB_KEY_F4,uint16);
GLV_CONST_DEFINE(XKB_KEY_F5,uint16);
GLV_CONST_DEFINE(XKB_KEY_F6,uint16);
GLV_CONST_DEFINE(XKB_KEY_F7,uint16);
GLV_CONST_DEFINE(XKB_KEY_F8,uint16);
GLV_CONST_DEFINE(XKB_KEY_F9,uint16);
GLV_CONST_DEFINE(XKB_KEY_F10,uint16);
GLV_CONST_DEFINE(XKB_KEY_F11,uint16);
GLV_CONST_DEFINE(XKB_KEY_F12,uint16);

// #####################################################################################################
// es1emu/es1emu_emulation.h
GLV_CONST_DEFINE(GL_MODELVIEW,uint16);
GLV_CONST_DEFINE(GL_PROJECTION,uint16);
GLV_CONST_DEFINE(GL_VERTEX_ARRAY,uint16);
GLV_CONST_DEFINE(GL_NORMAL_ARRAY,uint16);
GLV_CONST_DEFINE(GL_COLOR_ARRAY,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_COORD_ARRAY,uint16);
// #####################################################################################################
// GLES2/gl2.h
GLV_CONST_DEFINE(GL_DEPTH_BUFFER_BIT,uint32);
GLV_CONST_DEFINE(GL_STENCIL_BUFFER_BIT,uint32);
GLV_CONST_DEFINE(GL_COLOR_BUFFER_BIT,uint32);
GLV_CONST_DEFINE(GL_FALSE,uint16);
GLV_CONST_DEFINE(GL_TRUE,uint16);
GLV_CONST_DEFINE(GL_POINTS,uint16);
GLV_CONST_DEFINE(GL_LINES,uint16);
GLV_CONST_DEFINE(GL_LINE_LOOP,uint16);
GLV_CONST_DEFINE(GL_LINE_STRIP,uint16);
GLV_CONST_DEFINE(GL_TRIANGLES,uint16);
GLV_CONST_DEFINE(GL_TRIANGLE_STRIP,uint16);
GLV_CONST_DEFINE(GL_TRIANGLE_FAN,uint16);
GLV_CONST_DEFINE(GL_ZERO,uint16);
GLV_CONST_DEFINE(GL_ONE,uint16);
GLV_CONST_DEFINE(GL_SRC_COLOR,uint16);
GLV_CONST_DEFINE(GL_ONE_MINUS_SRC_COLOR,uint16);
GLV_CONST_DEFINE(GL_SRC_ALPHA,uint16);
GLV_CONST_DEFINE(GL_ONE_MINUS_SRC_ALPHA,uint16);
GLV_CONST_DEFINE(GL_DST_ALPHA,uint16);
GLV_CONST_DEFINE(GL_ONE_MINUS_DST_ALPHA,uint16);
GLV_CONST_DEFINE(GL_DST_COLOR,uint16);
GLV_CONST_DEFINE(GL_ONE_MINUS_DST_COLOR,uint16);
GLV_CONST_DEFINE(GL_SRC_ALPHA_SATURATE,uint16);
GLV_CONST_DEFINE(GL_FUNC_ADD,uint16);
GLV_CONST_DEFINE(GL_BLEND_EQUATION,uint16);
GLV_CONST_DEFINE(GL_BLEND_EQUATION_RGB,uint16);
GLV_CONST_DEFINE(GL_BLEND_EQUATION_ALPHA,uint16);
GLV_CONST_DEFINE(GL_FUNC_SUBTRACT,uint16);
GLV_CONST_DEFINE(GL_FUNC_REVERSE_SUBTRACT,uint16);
GLV_CONST_DEFINE(GL_BLEND_DST_RGB,uint16);
GLV_CONST_DEFINE(GL_BLEND_SRC_RGB,uint16);
GLV_CONST_DEFINE(GL_BLEND_DST_ALPHA,uint16);
GLV_CONST_DEFINE(GL_BLEND_SRC_ALPHA,uint16);
GLV_CONST_DEFINE(GL_CONSTANT_COLOR,uint16);
GLV_CONST_DEFINE(GL_ONE_MINUS_CONSTANT_COLOR,uint16);
GLV_CONST_DEFINE(GL_CONSTANT_ALPHA,uint16);
GLV_CONST_DEFINE(GL_ONE_MINUS_CONSTANT_ALPHA,uint16);
GLV_CONST_DEFINE(GL_BLEND_COLOR,uint16);
GLV_CONST_DEFINE(GL_ARRAY_BUFFER,uint16);
GLV_CONST_DEFINE(GL_ELEMENT_ARRAY_BUFFER,uint16);
GLV_CONST_DEFINE(GL_ARRAY_BUFFER_BINDING,uint16);
GLV_CONST_DEFINE(GL_ELEMENT_ARRAY_BUFFER_BINDING,uint16);
GLV_CONST_DEFINE(GL_STREAM_DRAW,uint16);
GLV_CONST_DEFINE(GL_STATIC_DRAW,uint16);
GLV_CONST_DEFINE(GL_DYNAMIC_DRAW,uint16);
GLV_CONST_DEFINE(GL_BUFFER_SIZE,uint16);
GLV_CONST_DEFINE(GL_BUFFER_USAGE,uint16);
GLV_CONST_DEFINE(GL_CURRENT_VERTEX_ATTRIB,uint16);
GLV_CONST_DEFINE(GL_FRONT,uint16);
GLV_CONST_DEFINE(GL_BACK,uint16);
GLV_CONST_DEFINE(GL_FRONT_AND_BACK,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_2D,uint16);
GLV_CONST_DEFINE(GL_CULL_FACE,uint16);
GLV_CONST_DEFINE(GL_BLEND,uint16);
GLV_CONST_DEFINE(GL_DITHER,uint16);
GLV_CONST_DEFINE(GL_STENCIL_TEST,uint16);
GLV_CONST_DEFINE(GL_DEPTH_TEST,uint16);
GLV_CONST_DEFINE(GL_SCISSOR_TEST,uint16);
GLV_CONST_DEFINE(GL_POLYGON_OFFSET_FILL,uint16);
GLV_CONST_DEFINE(GL_SAMPLE_ALPHA_TO_COVERAGE,uint16);
GLV_CONST_DEFINE(GL_SAMPLE_COVERAGE,uint16);
GLV_CONST_DEFINE(GL_NO_ERROR,uint16);
GLV_CONST_DEFINE(GL_INVALID_ENUM,uint16);
GLV_CONST_DEFINE(GL_INVALID_VALUE,uint16);
GLV_CONST_DEFINE(GL_INVALID_OPERATION,uint16);
GLV_CONST_DEFINE(GL_OUT_OF_MEMORY,uint16);
GLV_CONST_DEFINE(GL_CW,uint16);
GLV_CONST_DEFINE(GL_CCW,uint16);
GLV_CONST_DEFINE(GL_LINE_WIDTH,uint16);
GLV_CONST_DEFINE(GL_ALIASED_POINT_SIZE_RANGE,uint16);
GLV_CONST_DEFINE(GL_ALIASED_LINE_WIDTH_RANGE,uint16);
GLV_CONST_DEFINE(GL_CULL_FACE_MODE,uint16);
GLV_CONST_DEFINE(GL_FRONT_FACE,uint16);
GLV_CONST_DEFINE(GL_DEPTH_RANGE,uint16);
GLV_CONST_DEFINE(GL_DEPTH_WRITEMASK,uint16);
GLV_CONST_DEFINE(GL_DEPTH_CLEAR_VALUE,uint16);
GLV_CONST_DEFINE(GL_DEPTH_FUNC,uint16);
GLV_CONST_DEFINE(GL_STENCIL_CLEAR_VALUE,uint16);
GLV_CONST_DEFINE(GL_STENCIL_FUNC,uint16);
GLV_CONST_DEFINE(GL_STENCIL_FAIL,uint16);
GLV_CONST_DEFINE(GL_STENCIL_PASS_DEPTH_FAIL,uint16);
GLV_CONST_DEFINE(GL_STENCIL_PASS_DEPTH_PASS,uint16);
GLV_CONST_DEFINE(GL_STENCIL_REF,uint16);
GLV_CONST_DEFINE(GL_STENCIL_VALUE_MASK,uint16);
GLV_CONST_DEFINE(GL_STENCIL_WRITEMASK,uint16);
GLV_CONST_DEFINE(GL_STENCIL_BACK_FUNC,uint16);
GLV_CONST_DEFINE(GL_STENCIL_BACK_FAIL,uint16);
GLV_CONST_DEFINE(GL_STENCIL_BACK_PASS_DEPTH_FAIL,uint16);
GLV_CONST_DEFINE(GL_STENCIL_BACK_PASS_DEPTH_PASS,uint16);
GLV_CONST_DEFINE(GL_STENCIL_BACK_REF,uint16);
GLV_CONST_DEFINE(GL_STENCIL_BACK_VALUE_MASK,uint16);
GLV_CONST_DEFINE(GL_STENCIL_BACK_WRITEMASK,uint16);
GLV_CONST_DEFINE(GL_VIEWPORT,uint16);
GLV_CONST_DEFINE(GL_SCISSOR_BOX,uint16);
GLV_CONST_DEFINE(GL_COLOR_CLEAR_VALUE,uint16);
GLV_CONST_DEFINE(GL_COLOR_WRITEMASK,uint16);
GLV_CONST_DEFINE(GL_UNPACK_ALIGNMENT,uint16);
GLV_CONST_DEFINE(GL_PACK_ALIGNMENT,uint16);
GLV_CONST_DEFINE(GL_MAX_TEXTURE_SIZE,uint16);
GLV_CONST_DEFINE(GL_MAX_VIEWPORT_DIMS,uint16);
GLV_CONST_DEFINE(GL_SUBPIXEL_BITS,uint16);
GLV_CONST_DEFINE(GL_RED_BITS,uint16);
GLV_CONST_DEFINE(GL_GREEN_BITS,uint16);
GLV_CONST_DEFINE(GL_BLUE_BITS,uint16);
GLV_CONST_DEFINE(GL_ALPHA_BITS,uint16);
GLV_CONST_DEFINE(GL_DEPTH_BITS,uint16);
GLV_CONST_DEFINE(GL_STENCIL_BITS,uint16);
GLV_CONST_DEFINE(GL_POLYGON_OFFSET_UNITS,uint16);
GLV_CONST_DEFINE(GL_POLYGON_OFFSET_FACTOR,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_BINDING_2D,uint16);
GLV_CONST_DEFINE(GL_SAMPLE_BUFFERS,uint16);
GLV_CONST_DEFINE(GL_SAMPLES,uint16);
GLV_CONST_DEFINE(GL_SAMPLE_COVERAGE_VALUE,uint16);
GLV_CONST_DEFINE(GL_SAMPLE_COVERAGE_INVERT,uint16);
GLV_CONST_DEFINE(GL_NUM_COMPRESSED_TEXTURE_FORMATS,uint16);
GLV_CONST_DEFINE(GL_COMPRESSED_TEXTURE_FORMATS,uint16);
GLV_CONST_DEFINE(GL_DONT_CARE,uint16);
GLV_CONST_DEFINE(GL_FASTEST,uint16);
GLV_CONST_DEFINE(GL_NICEST,uint16);
GLV_CONST_DEFINE(GL_GENERATE_MIPMAP_HINT,uint16);
GLV_CONST_DEFINE(GL_BYTE,uint16);
GLV_CONST_DEFINE(GL_UNSIGNED_BYTE,uint16);
GLV_CONST_DEFINE(GL_SHORT,uint16);
GLV_CONST_DEFINE(GL_UNSIGNED_SHORT,uint16);
GLV_CONST_DEFINE(GL_INT,uint16);
GLV_CONST_DEFINE(GL_UNSIGNED_INT,uint16);
GLV_CONST_DEFINE(GL_FLOAT,uint16);
GLV_CONST_DEFINE(GL_FIXED,uint16);
GLV_CONST_DEFINE(GL_DEPTH_COMPONENT,uint16);
GLV_CONST_DEFINE(GL_ALPHA,uint16);
GLV_CONST_DEFINE(GL_RGB,uint16);
GLV_CONST_DEFINE(GL_RGBA,uint16);
GLV_CONST_DEFINE(GL_LUMINANCE,uint16);
GLV_CONST_DEFINE(GL_LUMINANCE_ALPHA,uint16);
GLV_CONST_DEFINE(GL_UNSIGNED_SHORT_4_4_4_4,uint16);
GLV_CONST_DEFINE(GL_UNSIGNED_SHORT_5_5_5_1,uint16);
GLV_CONST_DEFINE(GL_UNSIGNED_SHORT_5_6_5,uint16);
GLV_CONST_DEFINE(GL_FRAGMENT_SHADER,uint16);
GLV_CONST_DEFINE(GL_VERTEX_SHADER,uint16);
GLV_CONST_DEFINE(GL_MAX_VERTEX_ATTRIBS,uint16);
GLV_CONST_DEFINE(GL_MAX_VERTEX_UNIFORM_VECTORS,uint16);
GLV_CONST_DEFINE(GL_MAX_VARYING_VECTORS,uint16);
GLV_CONST_DEFINE(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS,uint16);
GLV_CONST_DEFINE(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS,uint16);
GLV_CONST_DEFINE(GL_MAX_TEXTURE_IMAGE_UNITS,uint16);
GLV_CONST_DEFINE(GL_MAX_FRAGMENT_UNIFORM_VECTORS,uint16);
GLV_CONST_DEFINE(GL_SHADER_TYPE,uint16);
GLV_CONST_DEFINE(GL_DELETE_STATUS,uint16);
GLV_CONST_DEFINE(GL_LINK_STATUS,uint16);
GLV_CONST_DEFINE(GL_VALIDATE_STATUS,uint16);
GLV_CONST_DEFINE(GL_ATTACHED_SHADERS,uint16);
GLV_CONST_DEFINE(GL_ACTIVE_UNIFORMS,uint16);
GLV_CONST_DEFINE(GL_ACTIVE_UNIFORM_MAX_LENGTH,uint16);
GLV_CONST_DEFINE(GL_ACTIVE_ATTRIBUTES,uint16);
GLV_CONST_DEFINE(GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,uint16);
GLV_CONST_DEFINE(GL_SHADING_LANGUAGE_VERSION,uint16);
GLV_CONST_DEFINE(GL_CURRENT_PROGRAM,uint16);
GLV_CONST_DEFINE(GL_NEVER,uint16);
GLV_CONST_DEFINE(GL_LESS,uint16);
GLV_CONST_DEFINE(GL_EQUAL,uint16);
GLV_CONST_DEFINE(GL_LEQUAL,uint16);
GLV_CONST_DEFINE(GL_GREATER,uint16);
GLV_CONST_DEFINE(GL_NOTEQUAL,uint16);
GLV_CONST_DEFINE(GL_GEQUAL,uint16);
GLV_CONST_DEFINE(GL_ALWAYS,uint16);
GLV_CONST_DEFINE(GL_KEEP,uint16);
GLV_CONST_DEFINE(GL_REPLACE,uint16);
GLV_CONST_DEFINE(GL_INCR,uint16);
GLV_CONST_DEFINE(GL_DECR,uint16);
GLV_CONST_DEFINE(GL_INVERT,uint16);
GLV_CONST_DEFINE(GL_INCR_WRAP,uint16);
GLV_CONST_DEFINE(GL_DECR_WRAP,uint16);
GLV_CONST_DEFINE(GL_VENDOR,uint16);
GLV_CONST_DEFINE(GL_RENDERER,uint16);
GLV_CONST_DEFINE(GL_VERSION,uint16);
GLV_CONST_DEFINE(GL_EXTENSIONS,uint16);
GLV_CONST_DEFINE(GL_NEAREST,uint16);
GLV_CONST_DEFINE(GL_LINEAR,uint16);
GLV_CONST_DEFINE(GL_NEAREST_MIPMAP_NEAREST,uint16);
GLV_CONST_DEFINE(GL_LINEAR_MIPMAP_NEAREST,uint16);
GLV_CONST_DEFINE(GL_NEAREST_MIPMAP_LINEAR,uint16);
GLV_CONST_DEFINE(GL_LINEAR_MIPMAP_LINEAR,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_MAG_FILTER,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_MIN_FILTER,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_WRAP_S,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_WRAP_T,uint16);
GLV_CONST_DEFINE(GL_TEXTURE,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_CUBE_MAP,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_BINDING_CUBE_MAP,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_CUBE_MAP_POSITIVE_X,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_CUBE_MAP_NEGATIVE_X,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_CUBE_MAP_POSITIVE_Y,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_CUBE_MAP_POSITIVE_Z,uint16);
GLV_CONST_DEFINE(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,uint16);
GLV_CONST_DEFINE(GL_MAX_CUBE_MAP_TEXTURE_SIZE,uint16);
GLV_CONST_DEFINE(GL_TEXTURE0,uint16);
GLV_CONST_DEFINE(GL_TEXTURE1,uint16);
GLV_CONST_DEFINE(GL_TEXTURE2,uint16);
GLV_CONST_DEFINE(GL_TEXTURE3,uint16);
GLV_CONST_DEFINE(GL_TEXTURE4,uint16);
GLV_CONST_DEFINE(GL_TEXTURE5,uint16);
GLV_CONST_DEFINE(GL_TEXTURE6,uint16);
GLV_CONST_DEFINE(GL_TEXTURE7,uint16);
GLV_CONST_DEFINE(GL_TEXTURE8,uint16);
GLV_CONST_DEFINE(GL_TEXTURE9,uint16);
GLV_CONST_DEFINE(GL_TEXTURE10,uint16);
GLV_CONST_DEFINE(GL_TEXTURE11,uint16);
GLV_CONST_DEFINE(GL_TEXTURE12,uint16);
GLV_CONST_DEFINE(GL_TEXTURE13,uint16);
GLV_CONST_DEFINE(GL_TEXTURE14,uint16);
GLV_CONST_DEFINE(GL_TEXTURE15,uint16);
GLV_CONST_DEFINE(GL_TEXTURE16,uint16);
GLV_CONST_DEFINE(GL_TEXTURE17,uint16);
GLV_CONST_DEFINE(GL_TEXTURE18,uint16);
GLV_CONST_DEFINE(GL_TEXTURE19,uint16);
GLV_CONST_DEFINE(GL_TEXTURE20,uint16);
GLV_CONST_DEFINE(GL_TEXTURE21,uint16);
GLV_CONST_DEFINE(GL_TEXTURE22,uint16);
GLV_CONST_DEFINE(GL_TEXTURE23,uint16);
GLV_CONST_DEFINE(GL_TEXTURE24,uint16);
GLV_CONST_DEFINE(GL_TEXTURE25,uint16);
GLV_CONST_DEFINE(GL_TEXTURE26,uint16);
GLV_CONST_DEFINE(GL_TEXTURE27,uint16);
GLV_CONST_DEFINE(GL_TEXTURE28,uint16);
GLV_CONST_DEFINE(GL_TEXTURE29,uint16);
GLV_CONST_DEFINE(GL_TEXTURE30,uint16);
GLV_CONST_DEFINE(GL_TEXTURE31,uint16);
GLV_CONST_DEFINE(GL_ACTIVE_TEXTURE,uint16);
GLV_CONST_DEFINE(GL_REPEAT,uint16);
GLV_CONST_DEFINE(GL_CLAMP_TO_EDGE,uint16);
GLV_CONST_DEFINE(GL_MIRRORED_REPEAT,uint16);
GLV_CONST_DEFINE(GL_FLOAT_VEC2,uint16);
GLV_CONST_DEFINE(GL_FLOAT_VEC3,uint16);
GLV_CONST_DEFINE(GL_FLOAT_VEC4,uint16);
GLV_CONST_DEFINE(GL_INT_VEC2,uint16);
GLV_CONST_DEFINE(GL_INT_VEC3,uint16);
GLV_CONST_DEFINE(GL_INT_VEC4,uint16);
GLV_CONST_DEFINE(GL_BOOL,uint16);
GLV_CONST_DEFINE(GL_BOOL_VEC2,uint16);
GLV_CONST_DEFINE(GL_BOOL_VEC3,uint16);
GLV_CONST_DEFINE(GL_BOOL_VEC4,uint16);
GLV_CONST_DEFINE(GL_FLOAT_MAT2,uint16);
GLV_CONST_DEFINE(GL_FLOAT_MAT3,uint16);
GLV_CONST_DEFINE(GL_FLOAT_MAT4,uint16);
GLV_CONST_DEFINE(GL_SAMPLER_2D,uint16);
GLV_CONST_DEFINE(GL_SAMPLER_CUBE,uint16);
GLV_CONST_DEFINE(GL_VERTEX_ATTRIB_ARRAY_ENABLED,uint16);
GLV_CONST_DEFINE(GL_VERTEX_ATTRIB_ARRAY_SIZE,uint16);
GLV_CONST_DEFINE(GL_VERTEX_ATTRIB_ARRAY_STRIDE,uint16);
GLV_CONST_DEFINE(GL_VERTEX_ATTRIB_ARRAY_TYPE,uint16);
GLV_CONST_DEFINE(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,uint16);
GLV_CONST_DEFINE(GL_VERTEX_ATTRIB_ARRAY_POINTER,uint16);
GLV_CONST_DEFINE(GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,uint16);
GLV_CONST_DEFINE(GL_IMPLEMENTATION_COLOR_READ_TYPE,uint16);
GLV_CONST_DEFINE(GL_IMPLEMENTATION_COLOR_READ_FORMAT,uint16);
GLV_CONST_DEFINE(GL_COMPILE_STATUS,uint16);
GLV_CONST_DEFINE(GL_INFO_LOG_LENGTH,uint16);
GLV_CONST_DEFINE(GL_SHADER_SOURCE_LENGTH,uint16);
GLV_CONST_DEFINE(GL_SHADER_COMPILER,uint16);
GLV_CONST_DEFINE(GL_SHADER_BINARY_FORMATS,uint16);
GLV_CONST_DEFINE(GL_NUM_SHADER_BINARY_FORMATS,uint16);
GLV_CONST_DEFINE(GL_LOW_FLOAT,uint16);
GLV_CONST_DEFINE(GL_MEDIUM_FLOAT,uint16);
GLV_CONST_DEFINE(GL_HIGH_FLOAT,uint16);
GLV_CONST_DEFINE(GL_LOW_INT,uint16);
GLV_CONST_DEFINE(GL_MEDIUM_INT,uint16);
GLV_CONST_DEFINE(GL_HIGH_INT,uint16);
GLV_CONST_DEFINE(GL_FRAMEBUFFER,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER,uint16);
GLV_CONST_DEFINE(GL_RGBA4,uint16);
GLV_CONST_DEFINE(GL_RGB5_A1,uint16);
GLV_CONST_DEFINE(GL_RGB565,uint16);
GLV_CONST_DEFINE(GL_DEPTH_COMPONENT16,uint16);
GLV_CONST_DEFINE(GL_STENCIL_INDEX8,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER_WIDTH,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER_HEIGHT,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER_INTERNAL_FORMAT,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER_RED_SIZE,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER_GREEN_SIZE,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER_BLUE_SIZE,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER_ALPHA_SIZE,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER_DEPTH_SIZE,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER_STENCIL_SIZE,uint16);
GLV_CONST_DEFINE(GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,uint16);
GLV_CONST_DEFINE(GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,uint16);
GLV_CONST_DEFINE(GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,uint16);
GLV_CONST_DEFINE(GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE,uint16);
GLV_CONST_DEFINE(GL_COLOR_ATTACHMENT0,uint16);
GLV_CONST_DEFINE(GL_DEPTH_ATTACHMENT,uint16);
GLV_CONST_DEFINE(GL_STENCIL_ATTACHMENT,uint16);
GLV_CONST_DEFINE(GL_NONE,uint16);
GLV_CONST_DEFINE(GL_FRAMEBUFFER_COMPLETE,uint16);
GLV_CONST_DEFINE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,uint16);
GLV_CONST_DEFINE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,uint16);
#ifdef GLV_OPENGL_ES_SERIES
GLV_CONST_DEFINE(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS,uint16);
#endif
GLV_CONST_DEFINE(GL_FRAMEBUFFER_UNSUPPORTED,uint16);
GLV_CONST_DEFINE(GL_FRAMEBUFFER_BINDING,uint16);
GLV_CONST_DEFINE(GL_RENDERBUFFER_BINDING,uint16);
GLV_CONST_DEFINE(GL_MAX_RENDERBUFFER_SIZE,uint16);
GLV_CONST_DEFINE(GL_INVALID_FRAMEBUFFER_OPERATION,uint16);
// #####################################################################################################
// EGL/egl.h
GLV_CONST_DEFINE(EGL_OPENGL_ES_API,uint16);
GLV_CONST_DEFINE(EGL_OPENGL_API   ,uint16);
// #####################################################################################################

void glv__py_print_array(float *array,int row,int col,char *text)
{
    int i,k,n;
    n = 0;
    if(col == 0) col = 1;
    if(text == NULL){
        text = "";
    }
    printf("array[%s]:\n",text);
    for(i=0;i<row;i++){
        printf("{");
        for(k=0;k<col;k++){
            printf(" %f",array[n]);
            n++;
        }
        printf("}\n");
    }
}

void glv__py_print_window_address(GLV_DISPLAY_t *glv_dpy,void *address)
{
	GLV_WINDOW_t *glv_window;
	if(address == NULL){
		printf("python_print_address: address = NULL\n");
		return;
	}
	pthread_mutex_lock(&glv_dpy->display_mutex);			// display

	wl_list_for_each(glv_window, &((GLV_DISPLAY_t*)glv_dpy)->window_list, link){
		if((glv_window->instance.alive == GLV_INSTANCE_ALIVE) && (glv_window == address)){
			pthread_mutex_unlock(&glv_dpy->display_mutex);	// display
			printf("python_print_address: address(%p) = window[%s]\n",address,glv_window->name);
			return;
		}
	}
	pthread_mutex_unlock(&glv_dpy->display_mutex);			// display
	printf("python_print_address: address(%p) = unnokwn\n",address);
}

char *glv__py_charPP_to_charP(void **text)
{
	return(*text);
}

void *glv__py_voidPP_to_voidP(void **ptr)
{
	return(*ptr);
}
