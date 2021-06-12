/*
 * Copyright © 2008 Kristian Høgsberg
 * Copyright © 2012-2013 Collabora, Ltd.
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

#include <stdio.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <xkbcommon/xkbcommon.h>
#include "xdg-shell-client-protocol.h"
#include "xdg-shell-unstable-v6-client-protocol.h"
#include "ivi-application-client-protocol.h"
#include "glview.h"
#include "weston-client-window.h"
#include "glview_local.h"

static int wayland_roundtrip(struct wl_display *display);

GLV_GARBAGE_BOX_t	_glv_garbage_box;

GLVINPUTFUNC_t	glv_input_func = {};
static int running = 1;

typedef void (*data_func_t)(void *data, size_t len,
			    int32_t x, int32_t y, void *user_data);

struct data_offer {
	struct wl_data_offer	*offer;
	struct _glvinput		*input;
	struct wl_array			types;

	int refcount;

	struct task io_task;
	int fd;
	data_func_t func;
	int32_t x, y;
	uint32_t dnd_action;
	uint32_t source_actions;
	void *user_data;
};

enum{
	AREA_MOVE = 0,
	AREA_RESIZE_TOP = 1,
	AREA_RESIZE_BOTTOM = 2,
	AREA_RESIZE_LEFT = 4,
	AREA_RESIZE_RIGHT = 8,
	AREA_RESIZE_TOP_LEFT = AREA_RESIZE_TOP | AREA_RESIZE_LEFT, //5
	AREA_RESIZE_BOTTOM_LEFT = AREA_RESIZE_BOTTOM | AREA_RESIZE_LEFT, //6
	AREA_RESIZE_TOP_RIGHT = AREA_RESIZE_TOP | AREA_RESIZE_RIGHT, //9
	AREA_RESIZE_BOTTOM_RIGHT = AREA_RESIZE_BOTTOM | AREA_RESIZE_RIGHT //10
};

static uint8_t area_to_cursor[] = {
	CURSOR_LEFT_PTR, CURSOR_TOP, CURSOR_BOTTOM, 0,
	CURSOR_LEFT, CURSOR_TOP_LEFT,
	CURSOR_BOTTOM_LEFT, 0, CURSOR_RIGHT,
	CURSOR_TOP_RIGHT, CURSOR_BOTTOM_RIGHT
};

static int _get_frame_edges(GLV_WINDOW_t *glv_window,int x,int y)
{
	int edge = 0;
	int cursor_image_margin_width = 5;
	int cursor_image_margin_height = 2;

	if((x >= glv_window->frameInfo.left_shadow_size) &&
	   (x <= (glv_window->frameInfo.left_shadow_size + glv_window->frameInfo.left_edge_size))){
		edge |= AREA_RESIZE_LEFT;
	} else if((x >= (glv_window->frameInfo.frame_width - glv_window->frameInfo.right_edge_size - glv_window->frameInfo.right_shadow_size + cursor_image_margin_width)) &&
			  (x <= (glv_window->frameInfo.frame_width - glv_window->frameInfo.right_shadow_size + cursor_image_margin_width))){
		edge |= AREA_RESIZE_RIGHT;
	}
	if((y >= glv_window->frameInfo.top_shadow_size) &&
	   (y <= (glv_window->frameInfo.top_shadow_size + glv_window->frameInfo.top_edge_size))){
		edge |= AREA_RESIZE_TOP;
	} else if((y >= (glv_window->frameInfo.frame_height - glv_window->frameInfo.bottom_edge_size - glv_window->frameInfo.bottom_shadow_size + cursor_image_margin_height)) &&
			  (y <= (glv_window->frameInfo.frame_height - glv_window->frameInfo.bottom_shadow_size + cursor_image_margin_height))){
		edge |= AREA_RESIZE_BOTTOM;
	}

	return edge;
}

static GLV_WINDOW_t *_glv_window_list_is_surface(GLV_DISPLAY_t *glv_dpy,struct wl_surface *surface)
{
	GLV_WINDOW_t *glv_window;

	pthread_mutex_lock(&glv_dpy->display_mutex);			// display

	wl_list_for_each(glv_window, &glv_dpy->window_list, link){
		if(glv_window->wl_window.surface == surface){
			if(glv_window->instance.alive != GLV_INSTANCE_ALIVE) glv_window = NULL;
			pthread_mutex_unlock(&glv_dpy->display_mutex);	// display
			return(glv_window);
		}
	}

	pthread_mutex_unlock(&glv_dpy->display_mutex);			// display
	return(NULL);
}

void _glvResizeWindow(GLV_WINDOW_t *glv_window,int x,int y,int width,int height)
{
	glv_window->x  = x;
	glv_window->y  = y;
	glv_window->width  = width;
	glv_window->height = height;

	glv_window->frameInfo.frame_width  = width;
	glv_window->frameInfo.frame_height = height;
	glv_window->frameInfo.inner_width  = width  - glv_window->frameInfo.left_size - glv_window->frameInfo.right_size;
	glv_window->frameInfo.inner_height = height - glv_window->frameInfo.top_size  - glv_window->frameInfo.bottom_size;

	wl_egl_window_resize(glv_window->egl_window,width,height,0,0);
	if(glv_window->windowType != GLV_TYPE_THREAD_FRAME){
		wl_subsurface_set_position(glv_window->wl_window.subsurface,(x + glv_window->parent->frameInfo.left_size),(y + glv_window->parent->frameInfo.top_size));
		wl_surface_commit(glv_window->wl_window.surface);
	}
}

static void _glv_window_list_on_reshape(GLV_DISPLAY_t *glv_dpy,GLV_WINDOW_t *frame_window,int width,int height)
{
	GLV_WINDOW_t *glv_window;
	pthread_mutex_lock(&glv_dpy->display_mutex);			// display

	wl_list_for_each(glv_window, &glv_dpy->window_list, link){
		if((glv_window->windowType != GLV_TYPE_THREAD_FRAME) && (glv_window->parent == frame_window)){
			if(glv_window->instance.alive == GLV_INSTANCE_ALIVE){
				glvOnReShape((glvWindow)glv_window,glv_window->x,glv_window->y,width,height);
			}
		}
	}

	pthread_mutex_unlock(&glv_dpy->display_mutex);			// display
}

// ------------------------------------------------------------------------------------
// keyboard

/* XKB キーマップ作成 */

static void _xkb_keymap(struct _glvinput *glv_input,char *mapstr)
{
    struct xkb_context *context;
    struct xkb_keymap *keymap;

    //コンテキスト作成
    context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if(!context) return;

    //キーマップ作成
    keymap = xkb_keymap_new_from_string(context,
            mapstr, XKB_KEYMAP_FORMAT_TEXT_V1, 0);

    if(!keymap)
    {
        xkb_context_unref(context);
        return;
    }

    //
    xkb_keymap_unref(glv_input->xkb_keymap);
    xkb_state_unref(glv_input->xkb_state);

    glv_input->xkb_keymap = keymap;
    glv_input->xkb_state = xkb_state_new(keymap);

    glv_input->shift_mask   = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, XKB_MOD_NAME_SHIFT);
    glv_input->lock_mask    = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, XKB_MOD_NAME_CAPS);
    glv_input->control_mask = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, XKB_MOD_NAME_CTRL);
    glv_input->mod1_mask    = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, XKB_MOD_NAME_ALT);
    glv_input->mod2_mask    = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, XKB_MOD_NAME_NUM);
    glv_input->mod3_mask    = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, "Mod3");
    glv_input->mod4_mask    = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, XKB_MOD_NAME_LOGO);
    glv_input->mod5_mask    = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, "Mod5");
    glv_input->super_mask   = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, "Super");
    glv_input->hyper_mask   = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, "Hyper");
    glv_input->meta_mask    = 1 << xkb_map_mod_get_index (glv_input->xkb_keymap, "Meta");	

    //
    xkb_context_unref(context);
}

static void keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard,
                       uint32_t format, int fd, uint32_t size)
{
	struct _glvinput *glv_input = data;
    char *mapstr;

    //printf("keymap | format:%u, fd:%d, size:%u\n", format, fd, size);

    //XKB ver 1 以外は対応しない
    if(format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
    {
        close(fd);
        return;
    }

    //文字列としてメモリにマッピング
    mapstr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);

    if(mapstr == MAP_FAILED)
    {
        close(fd);
        return;
    }

    //キーマップ作成
    _xkb_keymap(glv_input, mapstr);

    //
    munmap(mapstr, size);
    close(fd);

	glv_ime_startIbus(glv_input);
}

static void keyboard_handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
	struct _glvinput *glv_input = data;
	glv_input->wl_dpy->serial = serial;
	//printf("Keyboard gained focus\n");
}

static void keyboard_handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
	struct _glvinput *glv_input = data;
	glv_input->wl_dpy->serial = serial;
    //printf("Keyboard lost focus\n");
}

static void keyboard_handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key,
                    uint32_t state)
{
	int functionFlag = 0;
	xkb_keysym_t sym;
	int text[2];
	struct _glvinput *glv_input = data;
	int rc;
	glv_input->wl_dpy->serial = serial;

    //printf("key | key:%u, %s\n", key,(state == WL_KEYBOARD_KEY_STATE_PRESSED)? "press": "release");

    //現在の状態とキーコードから keysym (XKB_KEY_*) を取得

    sym = xkb_state_key_get_one_sym(glv_input->xkb_state, key + 8);

	if(state == WL_KEYBOARD_KEY_STATE_PRESSED){
		GLV_IF_DEBUG_KB_INPUT printf("-- xkb keysym (0x%X) , %s\n", sym,(state == WL_KEYBOARD_KEY_STATE_PRESSED)? "press": "release");
	}

	if((sym >= XKB_KEY_F1) && (sym <= XKB_KEY_F12)){
		functionFlag = 1;
	}

	// F01-F12 key
	rc = GLV_OK;
	if(functionFlag == 1){
		GLV_WIGET_t *glv_wiget=NULL;
		int			key_status;
		if(state == WL_KEYBOARD_KEY_STATE_PRESSED){
			key_status = GLV_KEYBOARD_KEY_STATE_PRESSED;
		}else{
			key_status = GLV_KEYBOARD_KEY_STATE_RELEASED;
		}
		glv_getValue(glv_input->glv_dpy,"cmdMenu wiget","P",&glv_wiget);
		if(glv_wiget != NULL){
			rc = glv_setValue(glv_input->glv_dpy,"cmdMenu key action","Pii",glv_wiget,sym,key_status);
			if(rc == GLV_OK){
				return;
			}
		}

		if(state == WL_KEYBOARD_KEY_STATE_PRESSED){
			if(glv_input_func.keyboard_function_key != 0) (*glv_input_func.keyboard_function_key)(sym,GLV_KEYBOARD_KEY_STATE_PRESSED);
		}else if(state == WL_KEYBOARD_KEY_STATE_RELEASED){
			if(glv_input_func.keyboard_function_key != 0) (*glv_input_func.keyboard_function_key)(sym,GLV_KEYBOARD_KEY_STATE_RELEASED);
		}
		return;
	}

	glv_input->im_state = GLV_KEY_STATE_IM_OFF;
	if(glv_input->im != NULL){
		glv_input->im_state = GLV_KEY_STATE_IM_OFF;
		if(state == WL_KEYBOARD_KEY_STATE_PRESSED){
			glv_ime_key_event(glv_input,key + 8,sym,0,GLV_KeyPress);
		}else if(state == WL_KEYBOARD_KEY_STATE_RELEASED){
			glv_ime_key_event(glv_input,key + 8,sym,0,GLV_KeyRelease);
		}
	}
	if(glv_input->im_state != GLV_KEY_STATE_IM_OFF){
		return;
	}

	switch (sym) {
	case XKB_KEY_KP_Space:
		sym = XKB_KEY_space;
		break;
	case XKB_KEY_KP_Tab:
		sym = XKB_KEY_Tab;
		break;
	case XKB_KEY_KP_Enter:
		sym = XKB_KEY_Return;
		break;
	case XKB_KEY_KP_Left:
		sym = XKB_KEY_Left;
		break;
	case XKB_KEY_KP_Up:
		sym = XKB_KEY_Up;
		break;
	case XKB_KEY_KP_Right:
		sym = XKB_KEY_Right;
		break;
	case XKB_KEY_KP_Down:
		sym = XKB_KEY_Down;
		break;
	case XKB_KEY_KP_Equal:
		sym = XKB_KEY_equal;
		break;
	case XKB_KEY_KP_Multiply:
		sym = XKB_KEY_asterisk;
		break;
	case XKB_KEY_KP_Add:
		sym = XKB_KEY_plus;
		break;
	case XKB_KEY_KP_Separator:
		sym = XKB_KEY_period;
		break;
	case XKB_KEY_KP_Subtract:
		sym = XKB_KEY_minus;
		break;
	case XKB_KEY_KP_Decimal:
		sym = XKB_KEY_period;
		break;
	case XKB_KEY_KP_Divide:
		sym = XKB_KEY_slash;
		break;
	case XKB_KEY_KP_0:
	case XKB_KEY_KP_1:
	case XKB_KEY_KP_2:
	case XKB_KEY_KP_3:
	case XKB_KEY_KP_4:
	case XKB_KEY_KP_5:
	case XKB_KEY_KP_6:
	case XKB_KEY_KP_7:
	case XKB_KEY_KP_8:
	case XKB_KEY_KP_9:
		sym = (sym - XKB_KEY_KP_0) + XKB_KEY_0;
		break;
	default:
		break;
	}

	if(state == WL_KEYBOARD_KEY_STATE_PRESSED){
		if(sym < 0xff){
			text[0] = (int)sym;
			text[1] = 0;
  			_glvOnTextInput((glvDisplay)glv_input->glv_dpy,GLV_KEY_KIND_ASCII,GLV_KEY_STATE_IM_OFF,sym,text,NULL,1);
		}else{
			if(sym == XKB_KEY_Insert){
				glv_input->glv_dpy->ins_mode ^= 1;
				_glvOnTextInput((glvDisplay)glv_input->glv_dpy,GLV_KEY_KIND_INSERT,GLV_KEY_STATE_IM_OFF,sym,NULL,NULL,0);
			}else{
  				_glvOnTextInput((glvDisplay)glv_input->glv_dpy,GLV_KEY_KIND_CTRL,GLV_KEY_STATE_IM_OFF,sym,NULL,NULL,0);
			}
		}
	}
}

static void keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
	struct _glvinput *glv_input = data;
    xkb_mod_mask_t mods;

    //printf("modifiers | depressed:%u, latched:%u, locaked:%u, group:%u\n",mods_depressed, mods_latched, mods_locked, group);

    //状態をセット

    xkb_state_update_mask(glv_input->xkb_state,mods_depressed, mods_latched, mods_locked, group, 0, 0);

    //現在の状態のフラグ取得

#if 1
    mods = xkb_state_serialize_mods(glv_input->xkb_state, XKB_STATE_MODS_EFFECTIVE);
#else
    mods = xkb_state_serialize_mods(glv_input->xkb_state,(XKB_STATE_DEPRESSED | XKB_STATE_LATCHED));
#endif

    //状態表示
	GLV_IF_DEBUG_KB_INPUT {
		printf("-- mods: ");

		if(mods & (1 << xkb_keymap_mod_get_index(glv_input->xkb_keymap, XKB_MOD_NAME_CTRL)))
			printf("Ctrl ");

		if(mods & (1 << xkb_keymap_mod_get_index(glv_input->xkb_keymap, XKB_MOD_NAME_SHIFT)))
			printf("Shift ");

		if(mods & (1 << xkb_keymap_mod_get_index(glv_input->xkb_keymap, XKB_MOD_NAME_ALT)))
			printf("Alt ");

		if(mods & (1 << xkb_keymap_mod_get_index(glv_input->xkb_keymap, XKB_MOD_NAME_LOGO)))
			printf("Logo ");

		if(mods & (1 << xkb_keymap_mod_get_index(glv_input->xkb_keymap, XKB_MOD_NAME_NUM)))
			printf("NumLock ");

		if(mods & (1 << xkb_keymap_mod_get_index(glv_input->xkb_keymap, XKB_MOD_NAME_CAPS)))
			printf("CapsLock ");

		printf("\n");
	}

	glv_ime_key_modifiers(glv_input,mods);
}

/* キーリピートの情報 */

static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard,
    int32_t rate, int32_t delay)
{
    printf("repeat_info | rate:%d, delay:%d\n", rate, delay);
}

static const struct wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap,
    keyboard_handle_enter,
    keyboard_handle_leave,
    keyboard_handle_key,
    keyboard_handle_modifiers,
	keyboard_repeat_info,
};
// ------------------------------------------------------------------------------------
// pointer
static uint32_t pointer_left_stat=0;
static uint32_t pointer_button_stat=0;
static uint32_t pointer_stat=0;

static void pointer_handle_enter(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface,
                     wl_fixed_t sx, wl_fixed_t sy)
{
	struct _glvinput *glv_input = data;
	GLV_WINDOW_t *glv_window;
	int cursor;

	glv_input->wl_dpy->serial = serial;

	pthread_mutex_lock(&glv_input->glv_dpy->display_mutex);		// display

	glv_window = _glv_window_list_is_surface(glv_input->wl_dpy->glv_dpy,surface);
	if(glv_window != NULL){
		glv_input->pointer_enter_windowId = glv_window->instance.Id;
	}else{
		glv_input->pointer_enter_windowId = 0;
	}

	glv_input->pointer_enter_serial = serial;

	glv_input->pointer_sx = wl_fixed_to_int(sx);
    glv_input->pointer_sy = wl_fixed_to_int(sy);

	if(glv_window != NULL){
		if(glv_window->windowType == GLV_TYPE_THREAD_FRAME){
			glv_window->edges = _get_frame_edges(glv_window, glv_input->pointer_sx, glv_input->pointer_sy);
			weston_client_window__input_set_pointer_image(glv_input,area_to_cursor[glv_window->edges]);
		}else{
			cursor = CURSOR_LEFT_PTR;
			if(glv_window->eventFunc.cursor != NULL){
				cursor = (glv_window->eventFunc.cursor)(glv_window,glv_window->width,glv_window->height,glv_input->pointer_sx, glv_input->pointer_sy);
			}
			weston_client_window__input_set_pointer_image(glv_input,cursor);
		}
	}
	pthread_mutex_unlock(&glv_input->glv_dpy->display_mutex);		// display
}

static void pointer_handle_leave(void *data, struct wl_pointer *pointer,
                     uint32_t serial, struct wl_surface *surface)
{
	struct _glvinput *glv_input = data;
	glv_input->wl_dpy->serial = serial;
	glv_input->pointer_enter_windowId = 0;
}

static void pointer_handle_motion(void *data, struct wl_pointer *pointer,
                      uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
	struct _glvinput *glv_input = data;
	GLV_WINDOW_t	*glv_window;
	GLV_WINDOW_t	*enter_window = NULL;
	GLV_WINDOW_t	*leave_window = NULL;
	int cursor;

    glv_input->pointer_sx = wl_fixed_to_int(sx);
    glv_input->pointer_sy = wl_fixed_to_int(sy);

	//printf("Pointer moved at %d %d\n", glv_input->pointer_sx, glv_input->pointer_sy);
	pthread_mutex_lock(&glv_input->glv_dpy->display_mutex);		// display

	glv_window = _glvGetWindow(glv_input->glv_dpy,glv_input->pointer_enter_windowId);
	if(glv_window != NULL){
		if(glv_window->windowType == GLV_TYPE_THREAD_FRAME){
			glv_window->edges = _get_frame_edges(glv_window, glv_input->pointer_sx, glv_input->pointer_sy);
			weston_client_window__input_set_pointer_image(glv_input,area_to_cursor[glv_window->edges]);
			//printf("_get_frame_edges %d\n",glv_window->edges);
		}else{
			cursor = CURSOR_LEFT_PTR;
			if(glv_window->eventFunc.cursor != NULL){
				cursor = (glv_window->eventFunc.cursor)(glv_window,glv_window->width,glv_window->height,glv_input->pointer_sx, glv_input->pointer_sy);
			}
			weston_client_window__input_set_pointer_image(glv_input,cursor);
		}
	}
	if(glv_window != NULL){
		glvInstanceId	wigetId;
		wigetId = _glv_wiget_check_wiget_area(glv_window,glv_input->pointer_sx,glv_input->pointer_sy,GLV_MOUSE_EVENT_MOTION);
		//printf("wigetId = %ld\n",wigetId);
		if(glv_input->pointer_focus_wigetId != wigetId){
			if(glv_input->pointer_focus_wigetId != 0){
				leave_window = _glvGetWindow(glv_input->glv_dpy,glv_input->pointer_focus_windowId);
				GLV_IF_DEBUG_WIGET printf("wigetId = %ld leave\n",glv_input->pointer_focus_wigetId);
				if(leave_window != NULL){
					if(leave_window->active_sheet != NULL){
						if(wigetId == 0){
							leave_window->active_sheet->pointer_focus_wiget_Id = 0;
						}
					}
				}
			}
			if(wigetId != 0){
				enter_window = glv_window;
				GLV_IF_DEBUG_WIGET printf("wigetId = %ld enter\n",wigetId);
			}
			// --------------------------------------------------------
			if(leave_window == enter_window){
				leave_window = NULL;
			}
			glv_input->pointer_focus_wigetId = wigetId;
			if(glv_input->pointer_focus_wigetId != 0){
				glv_input->pointer_focus_windowId = glv_window->instance.Id;
			}else{
				glv_input->pointer_focus_windowId = 0;
			}
		}
	}

	if((pointer_button_stat != GLV_MOUSE_EVENT_PRESS) || (pointer_left_stat == GLV_MOUSE_EVENT_LEFT_PRESS)){
		// マウスのボタンが何も押されていないか、またはレフトボタンのみ押されている場合
		_glv_wiget_mousePointer_front(glv_input,glv_window,GLV_MOUSE_EVENT_MOTION,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_left_stat);
#ifndef GLV_GESTURE_EVENT_MOTION
		if((pointer_left_stat == GLV_MOUSE_EVENT_LEFT_PRESS) || ((glv_window->attr & GLV_WINDOW_ATTR_POINTER_MOTION) == GLV_WINDOW_ATTR_POINTER_MOTION))
#endif /* GLV_GESTURE_EVENT_MOTION */
		{
			_glv_window_and_sheet_mousePointer_front(glv_window,GLV_MOUSE_EVENT_MOTION,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_left_stat);
		}
	}
	if(pointer_button_stat == GLV_MOUSE_EVENT_PRESS){
		// mouse press + motion
		_glv_wiget_mouseButton_front(glv_input,glv_window,GLV_MOUSE_EVENT_MOTION,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_stat);
		_glv_window_and_sheet_mouseButton_front(glv_window,GLV_MOUSE_EVENT_MOTION,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_stat);
	}
	if(leave_window != NULL) glvOnUpdate(leave_window);
	if(enter_window != NULL) glvOnUpdate(enter_window);

	pthread_mutex_unlock(&glv_input->glv_dpy->display_mutex);		// display
}

static void pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
                      uint32_t serial, uint32_t time, uint32_t button,
                      uint32_t state)
{
	int rc;
	struct _glvinput *glv_input = data;
	GLV_WINDOW_t	*glv_window;
	WL_WINDOW_t	*w;

	glv_input->wl_dpy->serial = serial;
    //printf("Pointer button\n");

	pthread_mutex_lock(&glv_input->glv_dpy->display_mutex);			// display

	glv_window = _glvGetWindow(glv_input->glv_dpy,glv_input->pointer_enter_windowId);
	if(state == WL_POINTER_BUTTON_STATE_PRESSED){
		if(glv_window != NULL){
			glv_window->wl_window.last_time = time;
		}
	}
    if(button == BTN_LEFT){
    	if(state == WL_POINTER_BUTTON_STATE_PRESSED){
			pointer_left_stat = GLV_MOUSE_EVENT_LEFT_PRESS;
    		rc = 0;
    		if(glv_input_func.touch_down != NULL) rc = (*glv_input_func.touch_down)(glv_input->pointer_sx,glv_input->pointer_sy);
			if(glv_window != NULL){
				glvInstanceId	wigetId;
				wigetId = _glv_wiget_check_wiget_area(glv_window,glv_input->pointer_sx,glv_input->pointer_sy,GLV_MOUSE_EVENT_PRESS);
				glv_input->select_wigetId = wigetId;
				// printf("wigetId = %ld\n",wigetId);
				if(wigetId != 0){
					_glv_wiget_mousePointer_front(glv_input,glv_window,GLV_MOUSE_EVENT_PRESS,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_left_stat);
					glvOnUpdate(glv_window);
					rc = 1;
				}else{
					//printf("wiget以外をクリックした\n");
					_glvOnFocus((glvDisplay)glv_input->glv_dpy,GLV_STAT_OUT_FOCUS,NULL);
					//_glvOnTextInput((glvDisplay)glv_input->glv_dpy,GLV_KEY_KIND_IM,GLV_KEY_STATE_IM_RESET,0,NULL,NULL,0);
					glv_input->glv_dpy->kb_input_windowId = 0;
					glv_input->glv_dpy->kb_input_sheetId  = 0;
					glv_input->glv_dpy->kb_input_wigetId  = 0;					
				}
			}
    		if(rc == 1){
    		}else{
				_glv_window_and_sheet_mousePointer_front(glv_window,GLV_MOUSE_EVENT_PRESS,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_left_stat);
    		}
    	}else{
			pointer_left_stat =GLV_MOUSE_EVENT_LEFT_RELEASE;
    		rc = 0;
    		if(glv_input_func.touch_up != NULL) rc = (*glv_input_func.touch_up)(glv_input->pointer_sx,glv_input->pointer_sy);
			if(glv_window != NULL){
				glvInstanceId	wigetId;
				wigetId = _glv_wiget_check_wiget_area(glv_window,glv_input->pointer_sx,glv_input->pointer_sy,GLV_MOUSE_EVENT_RELEASE);
				//printf("wigetId = %ld\n",wigetId);
				if((glv_input->select_wigetId != 0) || (wigetId != 0)){
					glvOnUpdate(glv_window);
				}

				if((glv_input->select_wigetId == wigetId) && (wigetId != 0)){
					_glv_wiget_mousePointer_front(glv_input,glv_window,GLV_MOUSE_EVENT_RELEASE,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_left_stat);
					_glv_sheet_action_front(glv_window);
				}
				if(wigetId != 0) rc = 1;
				glv_input->select_wigetId = wigetId;
			}
    		if(rc == 1){
    		}else{
				_glv_window_and_sheet_mousePointer_front(glv_window,GLV_MOUSE_EVENT_RELEASE,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_left_stat);
    		}
    	}
    }

#define	DEBUG_MOUSE_STATE_PRINT		if(0)

    if(state == WL_POINTER_BUTTON_STATE_PRESSED){
        pointer_button_stat = GLV_MOUSE_EVENT_PRESS;
    }else{
        pointer_button_stat = GLV_MOUSE_EVENT_RELEASE;
    }
    if(button == BTN_LEFT){
        if(state == WL_POINTER_BUTTON_STATE_PRESSED){
            DEBUG_MOUSE_STATE_PRINT printf("BTN_LEFT PRESS\n");
            pointer_stat = GLV_MOUSE_EVENT_LEFT_PRESS;
        }else{
            DEBUG_MOUSE_STATE_PRINT printf("BTN_LEFT RELEASE\n");
            pointer_stat = GLV_MOUSE_EVENT_LEFT_RELEASE;
        }
    }else if(button == BTN_RIGHT){
        if(state == WL_POINTER_BUTTON_STATE_PRESSED){
            DEBUG_MOUSE_STATE_PRINT printf("BTN_RIGHT PRESS\n");
            pointer_stat = GLV_MOUSE_EVENT_RIGHT_PRESS;
        }else{
            DEBUG_MOUSE_STATE_PRINT printf("BTN_RIGHT RELEASE\n");
            pointer_stat = GLV_MOUSE_EVENT_RIGHT_RELEASE;
        }
    }else if(button == BTN_MIDDLE){
        if(state == WL_POINTER_BUTTON_STATE_PRESSED){
            DEBUG_MOUSE_STATE_PRINT printf("BTN_MIDDLE PRESS\n");
            pointer_stat = GLV_MOUSE_EVENT_MIDDLE_PRESS;
        }else{
            DEBUG_MOUSE_STATE_PRINT printf("BTN_MIDDLE RELEASE\n");
            pointer_stat = GLV_MOUSE_EVENT_MIDDLE_RELEASE;
        }
    }else{
        if(state == WL_POINTER_BUTTON_STATE_PRESSED){
            DEBUG_MOUSE_STATE_PRINT printf("MOUSE BTN PRESS (%x)\n",button);
			pointer_stat = GLV_MOUSE_EVENT_OTHER_PRESS;
        }else{
            DEBUG_MOUSE_STATE_PRINT printf("MOUSE BTN RELEASE (%x)\n",button);
			pointer_stat = GLV_MOUSE_EVENT_OTHER_RELEASE;
        }        
    }
	_glv_wiget_mouseButton_front(glv_input,glv_window,pointer_button_stat,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_stat);
	_glv_window_and_sheet_mouseButton_front(glv_window,pointer_button_stat,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_stat);

	if(glv_window == NULL){
			pthread_mutex_unlock(&glv_input->glv_dpy->display_mutex);		// display
        return;
    }
    if(glv_window->windowType == GLV_TYPE_THREAD_FRAME){
        if(button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED){
            w = &glv_window->wl_window;
            if((glv_input->pointer_sy > (glv_window->myFrame->frameInfo.top_shadow_size + glv_window->myFrame->frameInfo.top_edge_size)) &&
                (glv_input->pointer_sy < (glv_window->myFrame->frameInfo.top_name_size + glv_window->myFrame->frameInfo.top_shadow_size + glv_window->myFrame->frameInfo.top_edge_size - 2)) &&
                (glv_input->pointer_sx > glv_window->myFrame->frameInfo.left_size ) &&
                (glv_input->pointer_sx < (glv_window->myFrame->frameInfo.left_size + glv_window->myFrame->frameInfo.inner_width - 20))){
                if(w->xdg_wm_toplevel){
                    xdg_toplevel_move(w->xdg_wm_toplevel,glv_input->seat, serial);
                }else if(w->zxdgV6_toplevel){
                    zxdg_toplevel_v6_move(w->zxdgV6_toplevel,glv_input->seat, serial);
                }else if(w->wl_shell_surface){
                    wl_shell_surface_move(w->wl_shell_surface,glv_input->seat, serial);
                }
                pointer_left_stat =GLV_MOUSE_EVENT_LEFT_RELEASE;
				pointer_button_stat = GLV_MOUSE_EVENT_RELEASE;
            }else{
                if(w->xdg_wm_toplevel){
                    xdg_toplevel_resize(w->xdg_wm_toplevel,glv_input->seat, serial,glv_window->edges);
                }else if(w->zxdgV6_toplevel){
                    zxdg_toplevel_v6_resize(w->zxdgV6_toplevel,glv_input->seat, serial,glv_window->edges);
                }else if(w->wl_shell_surface){
                    wl_shell_surface_resize(w->wl_shell_surface, glv_input->seat, serial,glv_window->edges);
                }
                pointer_left_stat =GLV_MOUSE_EVENT_LEFT_RELEASE;
				pointer_button_stat = GLV_MOUSE_EVENT_RELEASE;
            }
        }
    }
	pthread_mutex_unlock(&glv_input->glv_dpy->display_mutex);		// display
}

static void pointer_handle_axis(void *data, struct wl_pointer *wl_pointer,
                    uint32_t time, uint32_t axis, wl_fixed_t value)
{
	struct _glvinput *glv_input = data;
	GLV_WINDOW_t	*glv_window;
	int				type;
	int				v;

	pthread_mutex_lock(&glv_input->glv_dpy->display_mutex);			// display

	glv_window = _glvGetWindow(glv_input->glv_dpy,glv_input->pointer_enter_windowId);

#if 0
	if(glv_window != NULL){
		printf("[%s] Pointer handle axis: %s , value:%.2f , time:%u\n",glv_window->name,(axis == WL_POINTER_AXIS_VERTICAL_SCROLL)? "vert": "horz",(value / 256.0), time);
	}
#endif

	if(axis == WL_POINTER_AXIS_VERTICAL_SCROLL){
		type = GLV_MOUSE_EVENT_AXIS_VERTICAL_SCROLL;
	}else{
		type = GLV_MOUSE_EVENT_AXIS_HORIZONTAL_SCROLL;
	}
	v = value / 256.0;

	_glv_wiget_mouseAxis_front(glv_window,type,time,v);
	_glv_window_and_sheet_mouseAxis_front(glv_window,type,time,v);

	pthread_mutex_unlock(&glv_input->glv_dpy->display_mutex);		// display
}

static const struct wl_pointer_listener pointer_listener = {
    pointer_handle_enter,
    pointer_handle_leave,
    pointer_handle_motion,
    pointer_handle_button,
    pointer_handle_axis,
};

static void touch_handle_down(void *data, struct wl_touch *touch, uint32_t serial,
				  uint32_t time, struct wl_surface *surface, int32_t id,
				  wl_fixed_t x_w, wl_fixed_t y_w)
{
	int rc;
	struct _glvinput *glv_input = data;
	GLV_WINDOW_t	*glv_window;

	pointer_left_stat = GLV_MOUSE_EVENT_LEFT_PRESS;
	/* This does not support multi-touch, since implementation is for testing */
	if (id > 0) {
		//printf("Receive touch down event with touch id(%d), "
		//	   "but this is not handled\n", id);
		return;
	}
	glv_input->wl_dpy->serial = serial;

	pthread_mutex_lock(&glv_input->glv_dpy->display_mutex);		// display

	glv_window = _glv_window_list_is_surface(glv_input->wl_dpy->glv_dpy,surface);
	if(glv_window != NULL){
		glv_window->wl_window.last_time = time;
		glv_input->pointer_enter_windowId = glv_window->instance.Id;
	}else{
		glv_input->pointer_enter_windowId = 0;
	}
	glv_input->pointer_enter_serial = serial;

	glv_input->pointer_sx = wl_fixed_to_int(x_w);
	glv_input->pointer_sy = wl_fixed_to_int(y_w);

    rc = 0;
    if(glv_input_func.touch_down != 0) rc = (*glv_input_func.touch_down)(glv_input->pointer_sx,glv_input->pointer_sy);
	if(glv_window != NULL){
		glvInstanceId	wigetId;
		wigetId = _glv_wiget_check_wiget_area(glv_window,glv_input->pointer_sx,glv_input->pointer_sy,GLV_MOUSE_EVENT_PRESS);
		glv_input->select_wigetId = wigetId;
		// printf("wigetId = %ld\n",wigetId);
		if(wigetId != 0){
			glvOnUpdate(glv_window);
			rc = 1;
		}
	}
    if(rc == 1){
		glv_input->find_wiget = 1; /* wigetを見つけている */
	}else{
		glv_input->find_wiget = 0; /* wigetを見つけていない */
		_glv_window_and_sheet_mousePointer_front(glv_window,GLV_MOUSE_EVENT_PRESS,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_left_stat);
	}
	pthread_mutex_unlock(&glv_input->glv_dpy->display_mutex);		// display
}

static void touch_handle_up(void *data, struct wl_touch *touch, uint32_t serial,
				uint32_t time, int32_t id)
{
	struct _glvinput *glv_input = data;
	GLV_WINDOW_t	*glv_window;

	pointer_left_stat =GLV_MOUSE_EVENT_LEFT_RELEASE;
	if (id > 0) {
		return;
	}
	glv_input->wl_dpy->serial = serial;

	pthread_mutex_lock(&glv_input->glv_dpy->display_mutex);		// display

	glv_window = _glvGetWindow(glv_input->glv_dpy,glv_input->pointer_enter_windowId);
	if (glv_input->find_wiget == 1){
		 /* wigetを見つけている */
		if(glv_input_func.touch_up != 0) (*glv_input_func.touch_up)(glv_input->pointer_sx,glv_input->pointer_sy);
		if(glv_window != NULL){
			glvInstanceId	wigetId;
			wigetId = _glv_wiget_check_wiget_area(glv_window,glv_input->pointer_sx,glv_input->pointer_sy,GLV_MOUSE_EVENT_RELEASE);
			//printf("wigetId = %ld\n",wigetId);
			if((glv_input->select_wigetId != 0) || (wigetId != 0)){
				glvOnUpdate(glv_window);
			}
			if((glv_input->select_wigetId == wigetId) && (wigetId != 0)){
				_glv_wiget_mousePointer_front(glv_input,glv_window,GLV_MOUSE_EVENT_RELEASE,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_left_stat);
				_glv_sheet_action_front(glv_window);
			}
			glv_input->select_wigetId = wigetId;
		}
	}else if (glv_input->find_wiget == 0){
		/* wigetを見つけていない */
		_glv_window_and_sheet_mousePointer_front(glv_window,GLV_MOUSE_EVENT_RELEASE,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_left_stat);
	}

	/* reset */
	glv_input->find_wiget = 0;	/* wigetを見つけていない */
	pthread_mutex_unlock(&glv_input->glv_dpy->display_mutex);		// display
}

static void touch_handle_motion(void *data, struct wl_touch *touch, uint32_t time,
					int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
	GLV_WINDOW_t	*glv_window;
	struct _glvinput *glv_input = data;

	if (id > 0) {
		return;
	}

	glv_input->pointer_sx = wl_fixed_to_int(x_w);
	glv_input->pointer_sy = wl_fixed_to_int(y_w);

	if (glv_input->find_wiget != 0) {
		return; /* wigetを見つけている */
	}
    /* wigetを見つけていない */

	pthread_mutex_lock(&glv_input->glv_dpy->display_mutex);		// display

	glv_window = _glvGetWindow(glv_input->glv_dpy,glv_input->pointer_enter_windowId);
	_glv_window_and_sheet_mousePointer_front(glv_window,GLV_MOUSE_EVENT_MOTION,time,glv_input->pointer_sx,glv_input->pointer_sy,pointer_left_stat);

	pthread_mutex_unlock(&glv_input->glv_dpy->display_mutex);		// display
}

static void touch_handle_frame(void *data, struct wl_touch *touch)
{
	/* no op */
}

static void touch_handle_cancel(void *data, struct wl_touch *touch)
{
	/* no op */
}

static const struct wl_touch_listener touch_listener = {
	touch_handle_down,
	touch_handle_up,
	touch_handle_motion,
	touch_handle_frame,
	touch_handle_cancel
};

static void seat_handle_capabilities(void *data, struct wl_seat *seat,
                         enum wl_seat_capability caps)
{
	struct _glvinput *glv_input = data;
    if(caps & WL_SEAT_CAPABILITY_POINTER) {
		//printf("Display has a pointer\n");
    }

    if(caps & WL_SEAT_CAPABILITY_KEYBOARD) {
		//printf("Display has a keyboard\n");
    }

    if(caps & WL_SEAT_CAPABILITY_TOUCH) {
		//printf("Display has a touch screen\n");
    }

    if((caps & WL_SEAT_CAPABILITY_POINTER) && !glv_input->pointer) {
		glv_input->pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(glv_input->pointer, &pointer_listener, glv_input);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && glv_input->pointer) {
		if (glv_input->seat_version >= WL_POINTER_RELEASE_SINCE_VERSION)
			wl_pointer_release(glv_input->pointer);
		else
			wl_pointer_destroy(glv_input->pointer);
		glv_input->pointer = NULL;
    }

    if(caps & WL_SEAT_CAPABILITY_KEYBOARD) {
		glv_input->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(glv_input->keyboard, &keyboard_listener, glv_input);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {

		//XKB 解放
		xkb_keymap_unref(glv_input->xkb_keymap);
		xkb_state_unref(glv_input->xkb_state);
		glv_input->xkb_keymap = NULL;
		glv_input->xkb_state = NULL;

		if (glv_input->seat_version >= WL_KEYBOARD_RELEASE_SINCE_VERSION)
			wl_keyboard_release(glv_input->keyboard);
		else
			wl_keyboard_destroy(glv_input->keyboard);
		glv_input->keyboard = NULL;
    }

	if((caps & WL_SEAT_CAPABILITY_TOUCH) && !glv_input->touch) {
		glv_input->touch = wl_seat_get_touch(seat);
		wl_touch_add_listener(glv_input->touch, &touch_listener, glv_input);
	} else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && glv_input->touch) {
		if (glv_input->seat_version >= WL_TOUCH_RELEASE_SINCE_VERSION)
			wl_touch_release(glv_input->touch);
		else
			wl_touch_destroy(glv_input->touch);
		glv_input->touch = NULL;
	}
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
	NULL	//
};

static void handle_ping(void *data, struct wl_shell_surface *shell_surface,
	    uint32_t serial)
{
	wl_shell_surface_pong(shell_surface, serial);
	//printf("Pinged and ponged\n");
}
// ------------------------------------------------------------------------------------
static void xdg_wm_surface_handle_configure(void *data,
			     struct xdg_surface *surface,
			     uint32_t serial)
{
	xdg_surface_ack_configure(surface, serial);
}

static const struct xdg_surface_listener xdg_wm_surface_listener = {
	xdg_wm_surface_handle_configure
};

static void _Window_configure(GLV_WINDOW_t *glv_frame,int width,int height)
{
	if((width == 0) || (height == 0)){
		return;
	}
#define MIN_WINDOW_SIZE		(200)
	if(width  < MIN_WINDOW_SIZE) width  = MIN_WINDOW_SIZE;
	if(height < MIN_WINDOW_SIZE) height = MIN_WINDOW_SIZE;

	//サイズが変わったら更新

	if((width != glv_frame->frameInfo.frame_width) || (height != glv_frame->frameInfo.frame_height)){
		glv_frame->frameInfo.frame_width  = width;
		glv_frame->frameInfo.frame_height = height;
		glv_frame->frameInfo.inner_width  = width  - glv_frame->frameInfo.left_size - glv_frame->frameInfo.right_size;
		glv_frame->frameInfo.inner_height = height - glv_frame->frameInfo.top_size  - glv_frame->frameInfo.bottom_size;
		//printf("_Window_configure frameInfo(%d,%d) inner(%d,%d)\n",glv_frame->frameInfo.frame_width,glv_frame->frameInfo.frame_height,glv_frame->frameInfo.inner_width,glv_frame->frameInfo.inner_height);
		_glvOnConfigure(glv_frame,width, height);
		if(glv_frame->eventFunc.configure == NULL){
			// フレームにconfigureの処理が記述されていない場合フレーム直下のウインドウは自動リサイズする
			_glv_window_list_on_reshape(glv_frame->glv_dpy,glv_frame,glv_frame->frameInfo.inner_width,glv_frame->frameInfo.inner_height);
		}
	}
}

static void xdg_wm_toplevel_handle_configure(void *data, struct xdg_toplevel *toplevel,
			      int32_t width, int32_t height,
			      struct wl_array *states)
{
	GLV_WINDOW_t *glv_window = data;
    int old_top = glv_window->top;
    uint32_t *p;

    glv_window->top = 0;

    GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"xdg_wm_toplevel_handle_configure %s\n"GLV_DEBUG_END_COLOR,glv_window->name);

    wl_array_for_each(p, states) {
        uint32_t state = *p;
        switch (state) {
        case XDG_TOPLEVEL_STATE_MAXIMIZED:
            GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"XDG_TOPLEVEL_STATE_MAXIMIZED %s\n"GLV_DEBUG_END_COLOR,glv_window->name);
            break;
        case XDG_TOPLEVEL_STATE_FULLSCREEN:
            GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"XDG_TOPLEVEL_STATE_FULLSCREEN %s\n"GLV_DEBUG_END_COLOR,glv_window->name);
            break;
        case XDG_TOPLEVEL_STATE_RESIZING:
            GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"XDG_TOPLEVEL_STATE_RESIZING %s\n"GLV_DEBUG_END_COLOR,glv_window->name);
            break;
        case XDG_TOPLEVEL_STATE_ACTIVATED:
            glv_window->top = 1;
            GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"XDG_TOPLEVEL_STATE_ACTIVATED %s\n"GLV_DEBUG_END_COLOR,glv_window->name);
            break;
        default:
            /* Unknown state */
            GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"xdg_wm_toplevel_handle_configure:Unknown state %s\n"GLV_DEBUG_END_COLOR,glv_window->name);
            break;
        }
    }
    //printf(GLV_DEBUG_MSG_COLOR"xdg_wm_toplevel_handle_configure [%s] id = %ld , width = %d , height = %d\n"GLV_DEBUG_END_COLOR,glv_window->name,glv_window->instance.Id,width,height);
	
	_Window_configure(glv_window,width,height);
    if(old_top != glv_window->top){
        //printf("xdg_wm_toplevel_handle_configure: glvOnUpdate %s\n",glv_window->name);
        glvOnUpdate(glv_window);
    }
}

static void xdg_wm_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_surface)
{
	running = 0;
}

static const struct xdg_toplevel_listener xdg_wm_toplevel_listener = {
	xdg_wm_toplevel_handle_configure,
	xdg_wm_toplevel_handle_close,
};
// ------------------------------------------------------------------------------------
static void handle_zxdgV6_surface_configure(void *data, struct zxdg_surface_v6 *surface,
			 uint32_t serial)
{
	zxdg_surface_v6_ack_configure(surface, serial);
}

static const struct zxdg_surface_v6_listener xdg_zxdgV6_surface_listener = {
	handle_zxdgV6_surface_configure
};

static void handle_zxdgV6_toplevel_configure(void *data, struct zxdg_toplevel_v6 *toplevel,
			  int32_t width, int32_t height,
			  struct wl_array *states)
{
	GLV_WINDOW_t *glv_window = data;
	//printf("handle_zxdgV6_toplevel_configure , width = %d , height = %d\n",width,height);
	_Window_configure(glv_window,width,height);
}

static void handle_zxdgV6_toplevel_close(void *data, struct zxdg_toplevel_v6 *zxdgV6_toplevel)
{
	running = 0;
}

static const struct zxdg_toplevel_v6_listener xdg_zxdgV6_toplevel_listener = {
	handle_zxdgV6_toplevel_configure,
	handle_zxdgV6_toplevel_close,
};
// -----------------------------------------------------------------------
static void handle_configure(void *data, struct wl_shell_surface *shell_surface,
		 uint32_t edges, int32_t width, int32_t height)
{
	// resize
}

static void handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
	handle_ping,
	handle_configure,
	handle_popup_done
};
// ----------------------------------------------------------------------------

GLV_DISPLAY_t * _glvInitNativeDisplay(GLV_DISPLAY_t *glv_dpy)
{
	// non
	return(glv_dpy);
}

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *shell, uint32_t serial)
{
	xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
	xdg_wm_base_ping,
};

static void xdg_shell_ping(void *data, struct zxdg_shell_v6 *shell, uint32_t serial)
{
	zxdg_shell_v6_pong(shell, serial);
}

static const struct zxdg_shell_v6_listener zxdgV6_shell_listener = {
	xdg_shell_ping,
};

static void data_offer_offer(void *data, struct wl_data_offer *wl_data_offer, const char *type)
{
	struct data_offer *offer = data;
	char **p;

	GLV_IF_DEBUG_DATA_DEVICE printf("wl_data_offer # offer | type:\"%s\"\n", type);

	p = wl_array_add(&offer->types, sizeof *p);
	*p = strdup(type);
}

static void data_offer_source_actions(void *data, struct wl_data_offer *wl_data_offer, uint32_t source_actions)
{
    //D&D
	struct data_offer *offer = data;

	offer->source_actions = source_actions;
}

static void data_offer_action(void *data, struct wl_data_offer *wl_data_offer, uint32_t dnd_action)
{
    //D&D
	struct data_offer *offer = data;

	offer->dnd_action = dnd_action;
}

static const struct wl_data_offer_listener data_offer_listener = {
	data_offer_offer,
	data_offer_source_actions,
	data_offer_action
};

static void data_offer_destroy(struct data_offer *offer)
{
	char **p;

	offer->refcount--;
	if (offer->refcount == 0) {
		wl_data_offer_destroy(offer->offer);
		for (p = offer->types.data; *p; p++)
			free(*p);
		wl_array_release(&offer->types);
		free(offer);
	}
}

static void data_device_data_offer(void *data,
		       struct wl_data_device *data_device,
		       struct wl_data_offer *_offer)
{
	struct data_offer *offer;

    GLV_IF_DEBUG_DATA_DEVICE printf("wl_data_device # data_offer | offer:%p\n", _offer);

	offer = malloc(sizeof *offer);

	wl_array_init(&offer->types);
	offer->refcount = 1;
	offer->input = data;
	offer->offer = _offer;
	wl_data_offer_add_listener(offer->offer,
				   &data_offer_listener, offer);
}

static void data_device_enter(void *data, struct wl_data_device *data_device,
		  uint32_t serial, struct wl_surface *surface,
		  wl_fixed_t x_w, wl_fixed_t y_w,
		  struct wl_data_offer *offer)
{
    //D&D
	//struct _glvinput *input = data;
	//void *types_data;
	//float x = wl_fixed_to_double(x_w);
	//float y = wl_fixed_to_double(y_w);
	//char **p;

	if (!surface) {
		/* enter event for a window we've just destroyed */
		return;
	}
#ifdef DnD_AIKAWA
	struct window *window;
	void *types_data;
	float x = wl_fixed_to_double(x_w);
	float y = wl_fixed_to_double(y_w);
	char **p;

	if (!surface) {
		/* enter event for a window we've just destroyed */
		return;
	}

	window = wl_surface_get_user_data(surface);
	input->drag_enter_serial = serial;
	input->drag_focus = window,
	input->drag_x = x;
	input->drag_y = y;

	if (!input->touch_grab)
		input->pointer_enter_serial = serial;

	if (offer) {
		input->drag_offer = wl_data_offer_get_user_data(offer);

		p = wl_array_add(&input->drag_offer->types, sizeof *p);
		*p = NULL;

		types_data = input->drag_offer->types.data;

		if (input->display->data_device_manager_version >=
		    WL_DATA_OFFER_SET_ACTIONS_SINCE_VERSION) {
			wl_data_offer_set_actions(offer,
						  WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY |
						  WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE,
						  WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE);
		}
	} else {
		input->drag_offer = NULL;
		types_data = NULL;
	}

	if (window->data_handler)
		window->data_handler(window, input, x, y, types_data,
				     window->instance.user_data);
#endif // DnD_AIKAWA
}

static void data_device_leave(void *data, struct wl_data_device *data_device)
{
    //D&D
	struct _glvinput *input = data;

	if (input->drag_offer) {
		data_offer_destroy(input->drag_offer);
		input->drag_offer = NULL;
	}
}

static void data_device_motion(void *data, struct wl_data_device *data_device,
		   uint32_t time, wl_fixed_t x_w, wl_fixed_t y_w)
{
    //D&D
	//struct _glvinput *input = data;
#ifdef DnD_AIKAWA
	struct window *window = input->drag_focus;
	float x = wl_fixed_to_double(x_w);
	float y = wl_fixed_to_double(y_w);
	void *types_data;

	input->drag_x = x;
	input->drag_y = y;

	if (input->drag_offer)
		types_data = input->drag_offer->types.data;
	else
		types_data = NULL;

	if (window->data_handler)
		window->data_handler(window, input, x, y, types_data,
				     window->instance.user_data);
#endif // DnD_AIKAWA
}

static void data_device_drop(void *data, struct wl_data_device *data_device)
{
    //D&D
	//struct _glvinput *input = data;
#ifdef DnD_AIKAWA
	struct window *window = input->drag_focus;
	float x, y;

	x = input->drag_x;
	y = input->drag_y;

	if (window->drop_handler)
		window->drop_handler(window, input,
				     x, y, window->instance.user_data);

	if (input->touch_grab)
		touch_ungrab(input);
#endif // DnD_AIKAWA
}

static void data_device_selection(void *data,
		      struct wl_data_device *wl_data_device,
		      struct wl_data_offer *offer)
{
	struct _glvinput *input = data;
	char **p;

    GLV_IF_DEBUG_DATA_DEVICE printf("wl_data_device # selection | offer:%p\n", offer);

    //前回の data_offer は破棄する
	if (input->selection_offer)
		data_offer_destroy(input->selection_offer);

	if (offer) {
		input->selection_offer = wl_data_offer_get_user_data(offer);
		p = wl_array_add(&input->selection_offer->types, sizeof *p);
		*p = NULL;
	} else {
		input->selection_offer = NULL;
	}
}

static const struct wl_data_device_listener data_device_listener = {
	data_device_data_offer,
	data_device_enter,
	data_device_leave,
	data_device_motion,
	data_device_drop,
	data_device_selection
};

static void display_add_data_device(GLV_DISPLAY_t *glv_display, uint32_t id, uint32_t ddm_version)
{
	struct _glvinput *input;
	WL_DISPLAY_t *d = &glv_display->wl_dpy;

    if(ddm_version >= 3) ddm_version = 3;
	d->data_device_manager_version = ddm_version;
	d->data_device_manager =wl_registry_bind(d->registry, id,&wl_data_device_manager_interface,d->data_device_manager_version);

	wl_list_for_each(input, &d->input_list, link) {
		if (!input->data_device) {
			input->data_device = wl_data_device_manager_get_data_device(d->data_device_manager,input->seat);
			wl_data_device_add_listener(input->data_device,&data_device_listener,input);
		}
	}
}

static void display_add_input(GLV_DISPLAY_t *glv_display, struct wl_registry *registry,uint32_t id, int version)
{
	struct _glvinput *input = calloc(1, sizeof *input);
	WL_DISPLAY_t *d = &glv_display->wl_dpy;

	input->glv_dpy = glv_display;
	input->wl_dpy = d;
	if(version > 7) version = 7;
	input->seat_version = version;
	input->seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
	wl_list_insert(d->input_list.prev, &input->link);
	wl_seat_add_listener(input->seat, &seat_listener, input);
	wl_seat_set_user_data(input->seat, input);

	if (d->data_device_manager) {
		input->data_device = wl_data_device_manager_get_data_device(d->data_device_manager,input->seat);
		wl_data_device_add_listener(input->data_device,&data_device_listener,input);
	}
	input->pointer_surface = wl_compositor_create_surface(d->compositor);

	wayland_roundtrip(d->display);

	glv_display->ins_mode = 1;
}

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t id,
                       const char *interface, uint32_t version)
{
	GLV_DISPLAY_t *glv_display = data;
	WL_DISPLAY_t *d = &glv_display->wl_dpy;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	} else if(strcmp(interface, "wl_subcompositor") == 0){
   		d->subcompositor = wl_registry_bind(registry, id, &wl_subcompositor_interface, 1);
	} else if(strcmp(interface, "xdg_wm_base") == 0) {
		printf("glview:registry-interface-xdg_wm_base , version = %d\n",version);
		d->xdg_wm_shell = wl_registry_bind(registry, id,&xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(d->xdg_wm_shell, &wm_base_listener, d);
	} else if(strcmp(interface, "zxdg_shell_v6") == 0) {
		printf("glview:registry-interface-zxdg_shell_v6 , version = %d\n",version);
		d->zxdgV6_shell = wl_registry_bind(registry, id,&zxdg_shell_v6_interface, 1);
		zxdg_shell_v6_add_listener(d->zxdgV6_shell, &zxdgV6_shell_listener, d);
	} else if(strcmp(interface, "ivi_application") == 0) {
		printf("glview:registry-interface-wl_shell , version = %d\n",version);
		d->ivi_application = wl_registry_bind(registry, id,&ivi_application_interface, 1);
	} else if(strcmp(interface, "wl_shell") == 0) {
		printf("glview:registry-interface-wl_shell , version = %d\n",version);
		if((0 == d->xdg_wm_shell) && (0 == d->zxdgV6_shell) && (0 == d->ivi_application) ){
			d->wl_shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
		}
	} else if(strcmp(interface, "wl_shm") == 0) {
		printf("glview:registry-interface-wl_shm , version = %d\n",version);
		d->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
	} else if (strcmp(interface, "wl_seat") == 0) {
		printf("glview:registry-interface-wl_seat , version = %d\n",version);
		display_add_input(glv_display,registry,id,version);
	} else if (strcmp(interface, "wl_data_device_manager") == 0) {
		printf("glview:registry-interface-wl_data_device_manager , version = %d\n",version);
		display_add_data_device(glv_display, id, version);
	}
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
   registry_handle_global,
   registry_handle_global_remove
};

static void sync_callback(void *data, struct wl_callback *callback, uint32_t serial)
{
   int *done = data;

   *done = 1;
   wl_callback_destroy(callback);
}

static const struct wl_callback_listener sync_listener = {
   sync_callback
};

// -----------------------------------------------------------------------
static void surface_enter(void *data,
	      struct wl_surface *wl_surface, struct wl_output *wl_output)
{
}

static void surface_leave(void *data,
	      struct wl_surface *wl_surface, struct wl_output *output)
{
}

static const struct wl_surface_listener surface_listener = {
	surface_enter,
	surface_leave
};
// -----------------------------------------------------------------------

static int wayland_roundtrip(struct wl_display *display)
{
   struct wl_callback *callback;
   int done = 0, ret = 0;

   callback = wl_display_sync(display);
   wl_callback_add_listener(callback, &sync_listener, &done);
   while (ret != -1 && !done)
      ret = wl_display_dispatch(display);

   if (!done)
      wl_callback_destroy(callback);

   return ret;
}

GLV_DISPLAY_t * _glvOpenNativeDisplay(GLV_DISPLAY_t *glv_dpy)
{
	glv_dpy->native_dpy = wl_display_connect(NULL);
	glv_dpy->wl_dpy.display = glv_dpy->native_dpy;

	if(!glv_dpy->native_dpy){
		fprintf(stderr,"failed to initialize native display\n");
		return(NULL);
	}
	wl_list_init(&glv_dpy->wl_dpy.input_list);

	glv_dpy->wl_dpy.registry = wl_display_get_registry(glv_dpy->native_dpy);
	wl_registry_add_listener(glv_dpy->wl_dpy.registry, &registry_listener, glv_dpy);
	wayland_roundtrip(glv_dpy->native_dpy);

	weston_client_window__create_cursors(&glv_dpy->wl_dpy);

#ifdef GLV_PTHREAD_MUTEX_RECURSIVE
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&glv_dpy->display_mutex,&attr);	// display
#else
	pthread_mutex_init(&glv_dpy->display_mutex,NULL);	// display
#endif

	return(glv_dpy);
}

static void input_destroy(struct _glvinput *glv_input)
{

	if (glv_input->drag_offer)
		data_offer_destroy(glv_input->drag_offer);

	if (glv_input->selection_offer)
		data_offer_destroy(glv_input->selection_offer);

    if(glv_input->glv_dpy->wl_dpy.data_device_manager_version >= WL_DATA_DEVICE_RELEASE_SINCE_VERSION){
		if (glv_input->data_device)
    		wl_data_device_release(glv_input->data_device);
	} else {
		if (glv_input->data_device)
    		wl_data_device_destroy(glv_input->data_device);
	}

	if(glv_input->seat_version >= WL_POINTER_RELEASE_SINCE_VERSION){
		if (glv_input->touch)
			wl_touch_release(glv_input->touch);
		if (glv_input->pointer)
			wl_pointer_release(glv_input->pointer);
		if (glv_input->keyboard)
			wl_keyboard_release(glv_input->keyboard);
	} else {
		if (glv_input->touch)
			wl_touch_destroy(glv_input->touch);
		if (glv_input->pointer)
			wl_pointer_destroy(glv_input->pointer);
		if (glv_input->keyboard)
			wl_keyboard_destroy(glv_input->keyboard);
	}

	wl_surface_destroy(glv_input->pointer_surface);

	wl_list_remove(&glv_input->link);
	wl_seat_destroy(glv_input->seat);

	free(glv_input);
}

static void display_destroy_inputs(WL_DISPLAY_t *wl_dpy)
{
	struct _glvinput *tmp;
	struct _glvinput *glv_input;

	wl_list_for_each_safe(glv_input, tmp, &wl_dpy->input_list, link)
		input_destroy(glv_input);
}

void _glvCloseNativeDisplay(GLV_DISPLAY_t *glv_dpy)
{
	{
		struct _glv_window *tmp;
		struct _glv_window *glv_window;

		pthread_mutex_lock(&glv_dpy->display_mutex);				// display
		wl_list_for_each_safe(glv_window, tmp, &glv_dpy->window_list, link){
			_glvDestroyWindow(glv_window);
			free(glv_window);
		}
		pthread_mutex_unlock(&glv_dpy->display_mutex);				// display
	}

	display_destroy_inputs(&glv_dpy->wl_dpy);

	if(glv_dpy->wl_dpy.cursor_theme)
		wl_cursor_theme_destroy(glv_dpy->wl_dpy.cursor_theme);
	if(glv_dpy->wl_dpy.cursors)
		free(glv_dpy->wl_dpy.cursors);
	if(glv_dpy->wl_dpy.subcompositor)
		wl_subcompositor_destroy(glv_dpy->wl_dpy.subcompositor);
	if(glv_dpy->wl_dpy.xdg_wm_shell)
		xdg_wm_base_destroy(glv_dpy->wl_dpy.xdg_wm_shell);
	if(glv_dpy->wl_dpy.zxdgV6_shell)
		zxdg_shell_v6_destroy(glv_dpy->wl_dpy.zxdgV6_shell);
	if(glv_dpy->wl_dpy.shm)
		wl_shm_destroy(glv_dpy->wl_dpy.shm);
	if(glv_dpy->wl_dpy.data_device_manager)
		wl_data_device_manager_destroy(glv_dpy->wl_dpy.data_device_manager);

	wl_compositor_destroy(glv_dpy->wl_dpy.compositor);
	wl_registry_destroy(glv_dpy->wl_dpy.registry);

	weston_client_window__display_destroy(glv_dpy);

	wl_display_disconnect(glv_dpy->native_dpy);
}

GLV_WINDOW_t *_glvAllocWindowResource(GLV_DISPLAY_t *glv_dpy,char *name)
{
	GLV_WINDOW_t *glv_window;

	glv_window = (GLV_WINDOW_t *)malloc(sizeof(GLV_WINDOW_t));
	if(!glv_window){
		return(NULL);
	}
	memset(glv_window,0,sizeof(GLV_WINDOW_t));

	_glvInitInstance(&glv_window->instance,GLV_INSTANCE_TYPE_WINDOW);

	wl_list_init(&glv_window->sheet_list);
	glv_window->glv_dpy		= glv_dpy;
	glv_window->name		= name;

#ifdef GLV_PTHREAD_MUTEX_RECURSIVE
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&glv_window->window_mutex,&attr);	// window
#else
	pthread_mutex_init(&glv_window->window_mutex,NULL);		// window
#endif

	sem_init(&glv_window->initSync, 0, 0);

	return(glv_window);
}

int _glvCreateWindow(GLV_WINDOW_t *glv_window,char *name,
			int windowType,char *title,int x, int y, int width, int height,
			  glvWindow parent,int attr)
{
	WL_WINDOW_t	*w;
	struct wl_egl_window	*native;
	struct wl_region		*region;
	WL_DISPLAY_t	*wl_dpy;
	GLV_DISPLAY_t	*glv_dpy;
	GLV_WINDOW_t *glv_parent_window = (GLV_WINDOW_t*)parent;

	if((windowType != GLV_TYPE_THREAD_FRAME) && (glv_parent_window == NULL)){
		return(GLV_ERROR);
	}

	glv_dpy = glv_window->glv_dpy;
	wl_dpy = &glv_dpy->wl_dpy;
	glv_window->attr |= attr;
	w = &glv_window->wl_window;

	if(glv_parent_window != NULL){
		glv_window->parent = glv_parent_window;
		w->parent = glv_parent_window->wl_window.surface;
	}
	if(windowType == GLV_TYPE_THREAD_FRAME){
		glv_window->myFrame = glv_window;
	}else{
		glv_window->myFrame = glv_parent_window->myFrame;
	}

	w->surface = wl_compositor_create_surface(wl_dpy->compositor);

	if((w->parent == NULL) || (windowType == GLV_TYPE_THREAD_FRAME)){
		// --------------------------------------------------------------------------------------
		/* frame */
#if 0
		region = wl_compositor_create_region(wl_dpy->compositor);
		wl_region_add(region, 0, 0, width, height);
		wl_surface_set_opaque_region(w->surface, region);
		wl_region_destroy(region);
#endif
		native = wl_egl_window_create(w->surface, width, height);
		if(wl_dpy->xdg_wm_shell){
			// interface:xdg_wm_base
			w->xdg_wm_surface = xdg_wm_base_get_xdg_surface(wl_dpy->xdg_wm_shell,w->surface);
			xdg_surface_add_listener(w->xdg_wm_surface,&xdg_wm_surface_listener, glv_window);
			w->xdg_wm_toplevel = xdg_surface_get_toplevel(w->xdg_wm_surface);
			xdg_toplevel_add_listener(w->xdg_wm_toplevel,&xdg_wm_toplevel_listener, glv_window);
			xdg_toplevel_set_title(w->xdg_wm_toplevel, title);
			xdg_toplevel_set_max_size(w->xdg_wm_toplevel, 2000, 2000);
			if(glv_parent_window != NULL){
				xdg_toplevel_set_parent(w->xdg_wm_toplevel,glv_parent_window->wl_window.xdg_wm_toplevel);
			}
		} else if(wl_dpy->zxdgV6_shell){
			// interface:zxdg_shell_v6
			w->zxdgV6_surface = zxdg_shell_v6_get_xdg_surface(wl_dpy->zxdgV6_shell,w->surface);
			zxdg_surface_v6_add_listener(w->zxdgV6_surface,&xdg_zxdgV6_surface_listener, glv_window);
			w->zxdgV6_toplevel = zxdg_surface_v6_get_toplevel(w->zxdgV6_surface);
			zxdg_toplevel_v6_add_listener(w->zxdgV6_toplevel,&xdg_zxdgV6_toplevel_listener, glv_window);
			zxdg_toplevel_v6_set_title(w->zxdgV6_toplevel, title);
			if(glv_parent_window != NULL){
				zxdg_toplevel_v6_set_parent(w->zxdgV6_toplevel,glv_parent_window->wl_window.zxdgV6_toplevel);
			}
		}else if(wl_dpy->ivi_application){
			// interface:ivi_application
			uint32_t id_ivisurf = 0x1302;
			w->ivi_surface = ivi_application_surface_create(wl_dpy->ivi_application, id_ivisurf, w->surface);
		}else if(wl_dpy->wl_shell){
			// interface:wl_shell
			w->wl_shell_surface = wl_shell_get_shell_surface(wl_dpy->wl_shell,w->surface);
			wl_shell_surface_set_toplevel(w->wl_shell_surface);
			wl_shell_surface_add_listener(w->wl_shell_surface,&shell_surface_listener, glv_window);
		}else{

		}
		// --------------------------------------------------------------------------------------
		glv_window->absolute_x = 0;
		glv_window->absolute_y = 0;
	}else{
		// --------------------------------------------------------------------------------------
		// リファレンス実装であるweston以外のコンポジターで不透過を設定すると
		// 描画が壊れるため、処理を削除する 2021.05.01
#if 0
		// ウィンドウを不透明にする
		if(glv_window->attr &  GLV_WINDOW_ATTR_NON_TRANSPARENT){
			region = wl_compositor_create_region(wl_dpy->compositor);
			wl_region_add(region, 0, 0, width, height);
			wl_surface_set_opaque_region(w->surface, region);
			wl_region_destroy(region);
		}
#endif

		wl_surface_add_listener(w->surface,&surface_listener, NULL);

		w->subsurface = wl_subcompositor_get_subsurface(wl_dpy->subcompositor,w->surface,w->parent);
		wl_subsurface_set_desync(w->subsurface);

		native = wl_egl_window_create(w->surface, width, height);

		// サーフェイスへのポインターイベント通知を無効
		if(glv_window->attr &  GLV_WINDOW_ATTR_DISABLE_POINTER_EVENT){
			region = wl_compositor_create_region(wl_dpy->compositor);
			wl_surface_set_input_region(w->surface, region);
			wl_region_destroy(region);
		}

		wl_subsurface_set_position(w->subsurface,(x + glv_parent_window->frameInfo.left_size),(y + glv_parent_window->frameInfo.top_size));
		glv_window->absolute_x = glv_parent_window->absolute_x + x + glv_parent_window->frameInfo.left_size;
		glv_window->absolute_y = glv_parent_window->absolute_y + y + glv_parent_window->frameInfo.top_size;
		// --------------------------------------------------------------------------------------
	}

	wl_surface_commit(w->surface);

	glv_window->egl_window	= native;
	glv_window->windowType	= windowType;
	glv_window->name		= name;
	glv_window->title		= title;
	glv_window->x			= x;
	glv_window->y			= y;
	glv_window->width		= width;
	glv_window->height		= height;

	// input 
	glv_window->frameInfo.top_shadow_size		= 0;
	glv_window->frameInfo.bottom_shadow_size	= 0;
	glv_window->frameInfo.left_shadow_size		= 0;
	glv_window->frameInfo.right_shadow_size		= 0;

    glv_window->frameInfo.top_name_size				= 0;
    glv_window->frameInfo.top_cmd_menu_size			= 0;
    glv_window->frameInfo.top_pulldown_menu_size	= 0;
    glv_window->frameInfo.bottom_status_bar_size	= 0;

	glv_window->frameInfo.top_edge_size			= 0;
	glv_window->frameInfo.bottom_edge_size		= 0;
	glv_window->frameInfo.left_edge_size		= 0;
	glv_window->frameInfo.right_edge_size		= 0;

    // output
    glv_window->frameInfo.top_user_area_size    = glv_window->frameInfo.top_name_size + glv_window->frameInfo.top_cmd_menu_size + glv_window->frameInfo.top_pulldown_menu_size;
    glv_window->frameInfo.bottom_user_area_size = glv_window->frameInfo.bottom_status_bar_size;

	glv_window->frameInfo.top_size				= glv_window->frameInfo.top_user_area_size + glv_window->frameInfo.top_shadow_size + glv_window->frameInfo.top_edge_size;
	glv_window->frameInfo.bottom_size			= glv_window->frameInfo.bottom_status_bar_size + glv_window->frameInfo.bottom_shadow_size + glv_window->frameInfo.bottom_edge_size;
	glv_window->frameInfo.left_size				= glv_window->frameInfo.left_shadow_size   + glv_window->frameInfo.left_edge_size;
	glv_window->frameInfo.right_size			= glv_window->frameInfo.right_shadow_size  + glv_window->frameInfo.right_edge_size;

	glv_window->frameInfo.frame_width  = width;
	glv_window->frameInfo.frame_height = height;
	glv_window->frameInfo.inner_width  = width  - glv_window->frameInfo.left_size - glv_window->frameInfo.right_size;
	glv_window->frameInfo.inner_height = height - glv_window->frameInfo.top_size  - glv_window->frameInfo.bottom_size;

	// メッセージの届け先(teamLeader)及び、EGLConfigを設定する。
	switch(windowType){
		case GLV_TYPE_THREAD_FRAME:
			// 自分がフレーム、メッセージは自分で受け取る
			glv_window->teamLeader = glv_window;
			// フレームは、標準のEGLConfig設定を使用する
			glv_window->ctx.egl_config = glv_dpy->egl_config_normal;
			break;
		case GLV_TYPE_THREAD_WINDOW:
			// 自分がglvCreateThreadWindowで生成されたウインドウ、メッセージは自分で受け取る
			glv_window->teamLeader = glv_window;
			// EGLConfig設定
			if(glv_window->beauty == 1){
				glv_window->ctx.egl_config = glv_dpy->egl_config_beauty;
			}else{
				glv_window->ctx.egl_config = glv_dpy->egl_config_normal;
			}
			break;
		case GLV_TYPE_CHILD_WINDOW:
			// チームリーダーにメッセージを送信する
			glv_window->teamLeader = glv_window->parent->teamLeader;
			// チームリーダーのEGLConfig設定を引き継ぐ
			glv_window->ctx.egl_config = glv_window->teamLeader->ctx.egl_config;
			break;
		default:
			printf("_glvCreateWindow unnon window type %d.\n",windowType);
			break;
	}

	glv_window->ctx.egl_surf = eglCreateWindowSurface(glv_dpy->egl_dpy, glv_window->ctx.egl_config, glv_window->egl_window, NULL);
	if(!glv_window->ctx.egl_surf){
    	fprintf(stderr,"_glvCreateWindow:Error: eglCreateWindowSurface failed\n");
     	return(GLV_ERROR);
	}
	glv_window->ctx.threadId = pthread_self();
	glv_window->ctx.runThread = 0;

	pthread_mutex_lock(&glv_dpy->display_mutex);				// display
	wl_list_insert(glv_dpy->window_list.prev, &glv_window->link);
	pthread_mutex_unlock(&glv_dpy->display_mutex);				// display

	GLV_IF_DEBUG_INSTANCE {
		printf(GLV_DEBUG_INSTANCE_COLOR"glvCreateWindow "GLV_DEBUG_END_COLOR);
		switch(windowType){
			case GLV_TYPE_THREAD_FRAME:
				printf(GLV_DEBUG_INSTANCE_COLOR"freame "GLV_DEBUG_END_COLOR);
				break;
			case GLV_TYPE_THREAD_WINDOW:
				printf(GLV_DEBUG_INSTANCE_COLOR"thrad "GLV_DEBUG_END_COLOR);
				break;
			case GLV_TYPE_CHILD_WINDOW:
				printf(GLV_DEBUG_INSTANCE_COLOR"child "GLV_DEBUG_END_COLOR);
				break;
			default:
				printf(GLV_DEBUG_INSTANCE_COLOR"unknown "GLV_DEBUG_END_COLOR);
				break;
		}
		printf(GLV_DEBUG_INSTANCE_COLOR" %ld [%s]"GLV_DEBUG_END_COLOR,glv_window->instance.Id,glv_window->name);
		if(glv_window->parent == NULL){
			printf(GLV_DEBUG_INSTANCE_COLOR" parent NULL"GLV_DEBUG_END_COLOR);
		}else{
			printf(GLV_DEBUG_INSTANCE_COLOR" parent %ld [%s]"GLV_DEBUG_END_COLOR,glv_window->parent->instance.Id,glv_window->parent->name);
		}
		if(glv_window->teamLeader == NULL){
			printf(GLV_DEBUG_INSTANCE_COLOR" teamLeader NULL"GLV_DEBUG_END_COLOR);
		}else{
			printf(GLV_DEBUG_INSTANCE_COLOR" teamLeader %ld [%s]"GLV_DEBUG_END_COLOR,glv_window->teamLeader->instance.Id,glv_window->teamLeader->name);
		}
		printf("\n");
	}
	return(GLV_OK);
}

void _glvGcDestroyWindow(GLV_WINDOW_t *glv_window)
{
	//wl_list_remove(&glv_window->link);

	glvDestroyResource(&glv_window->instance);

	eglDestroySurface(glv_window->glv_dpy->egl_dpy, glv_window->ctx.egl_surf);

	wl_egl_window_destroy(glv_window->egl_window);

	// ---------------------------------------------------------------------
	if(glv_window->wl_window.xdg_wm_toplevel)
		xdg_toplevel_destroy(glv_window->wl_window.xdg_wm_toplevel);
	if(glv_window->wl_window.xdg_wm_surface)
		xdg_surface_destroy(glv_window->wl_window.xdg_wm_surface);
	if(glv_window->wl_window.xdg_wm_popup)
		xdg_popup_destroy(glv_window->wl_window.xdg_wm_popup);

	// ---------------------------------------------------------------------
	if(glv_window->wl_window.zxdgV6_surface)
		zxdg_surface_v6_destroy(glv_window->wl_window.zxdgV6_surface);
	if(glv_window->wl_window.zxdgV6_toplevel)
		zxdg_toplevel_v6_destroy(glv_window->wl_window.zxdgV6_toplevel);
	if(glv_window->wl_window.zxdgV6_popup)
		zxdg_popup_v6_destroy(glv_window->wl_window.zxdgV6_popup);
	// ---------------------------------------------------------------------
	if(glv_window->wl_window.wl_shell_surface)
		wl_shell_surface_destroy(glv_window->wl_window.wl_shell_surface);
	// ---------------------------------------------------------------------
	if(glv_window->wl_window.ivi_surface)
		ivi_surface_destroy(glv_window->wl_window.ivi_surface);
	// ---------------------------------------------------------------------

	if(glv_window->wl_window.subsurface)
		wl_subsurface_destroy(glv_window->wl_window.subsurface);
		
	wl_surface_destroy(glv_window->wl_window.surface);

	if(glv_window->wl_window.frame_cb)
		wl_callback_destroy(glv_window->wl_window.frame_cb);

	GLV_IF_DEBUG_INSTANCE printf(GLV_DEBUG_INSTANCE_COLOR"_glvGcDestroyWindow id = %ld [%s]\n"GLV_DEBUG_END_COLOR,glv_window->instance.Id,glv_window->name);

	sem_destroy(&glv_window->initSync);

	//free(glv_window);
}

void _glvDestroyWindow(GLV_WINDOW_t *glv_window)
{
	/* 終了 */
	if((glv_window->teamLeader == NULL) || (glv_window->windowType == GLV_TYPE_CHILD_WINDOW)){
		// メッセージを送信する先がいないので、ここで終了する
		if(glv_window->eventFunc.terminate != NULL){
			int rc;
			rc = (glv_window->eventFunc.terminate)(glv_window);
			if(rc != GLV_OK){
				fprintf(stderr,"glv_window->eventFunc.terminate error\n");
			}
		}
	}

	glv_window->instance.alive = GLV_INSTANCE_DEAD;
	glv_window->drawCount = 0;

	wl_list_remove(&glv_window->link);

	{
		struct _glv_sheet *tmp;
		struct _glv_sheet *glv_sheet;
		pthread_mutex_lock(&glv_window->window_mutex);					// window
		wl_list_for_each_safe(glv_sheet, tmp, &glv_window->sheet_list, link)
			glvDestroySheet((glvSheet)glv_sheet);
		pthread_mutex_unlock(&glv_window->window_mutex);				// window
	}

	_glvGcDestroyWindow(glv_window);
}

int _glvCreateGarbageBox(void)
{
	wl_list_init(&_glv_garbage_box.wiget_list);
	wl_list_init(&_glv_garbage_box.sheet_list);	
	//wl_list_init(&_glv_garbage_box.window_list);

#ifdef GLV_PTHREAD_MUTEX_RECURSIVE
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&_glv_garbage_box.pthread_mutex,&attr);	// garbage box
#else
	pthread_mutex_init(&_glv_garbage_box.pthread_mutex,NULL);	// garbage box
#endif

	return(1);
}

void _glvDestroyGarbageBox(void)
{
	pthread_mutex_destroy(&_glv_garbage_box.pthread_mutex);
}

int _glvGcGarbageBox(void)
{
	_glv_garbage_box.gc_run = 1;
	pthread_mutex_lock(&_glv_garbage_box.pthread_mutex);			// garbage box

	if(	(wl_list_empty(&_glv_garbage_box.wiget_list) == 1) &&
		(wl_list_empty(&_glv_garbage_box.sheet_list) == 1) ){
			pthread_mutex_unlock(&_glv_garbage_box.pthread_mutex);	// garbage box
			return(0);
	}

	if(wl_list_empty(&_glv_garbage_box.wiget_list) == 0)
	{
		struct _glv_wiget *tmp;
		struct _glv_wiget *glv_wiget;

		wl_list_for_each_safe(glv_wiget, tmp, &_glv_garbage_box.wiget_list, link)
			_glvGcDestroyWiget(glv_wiget);
	}
	if(wl_list_empty(&_glv_garbage_box.sheet_list) == 0)
	{
		struct _glv_sheet *tmp;
		struct _glv_sheet *glv_sheet;

		wl_list_for_each_safe(glv_sheet, tmp, &_glv_garbage_box.sheet_list, link)
			_glvGcDestroySheet(glv_sheet);
	}

	pthread_mutex_unlock(&_glv_garbage_box.pthread_mutex);			// garbage box
	_glv_garbage_box.gc_run = 0;
	return(1);
}

#if 1
void glvEnterEventLoop(glvDisplay glv_dpy)
{
	weston_client_window__display_run(glv_dpy);
}

#else
//#define AIKAWA_EVENT_TEST
void glvEnterEventLoop(glvDisplay glv_dpy)
{
	struct pollfd pollfd;
	struct wl_display		*display;
	int ret,rc_poll;

	display = ((GLV_DISPLAY_t*)glv_dpy)->wl_dpy.display;

	pollfd.fd = wl_display_get_fd(display);
	pollfd.events = POLLIN;
	pollfd.revents = 0;

	while(running){
	  int redraw = 0;
#if 0
{
	static int count=0;
	  printf("@@@(%d)\n",count++);
}
#endif	  
		_glvWindowMsgHandler_dispatch(glv_dpy);
		_glvGcGarbageBox();
#ifdef AIKAWA_EVENT_TEST
		while(wl_display_prepare_read(display) != 0)
			wl_display_dispatch_pending(display);

		ret = wl_display_flush(display);
    	if (ret < 0 && errno == EAGAIN)
    		pollfd.events |= POLLOUT;
    	else if (ret < 0){
#ifdef AIKAWA_EVENT_TEST
			wl_display_cancel_read(display);
#endif /* AIKAWA_EVENT_TEST */
        	break;
		}
#else /* AIKAWA_EVENT_TEST */
    	wl_display_dispatch_pending(display);

     	ret = wl_display_flush(display);
    	if (ret < 0 && errno == EAGAIN)
        	pollfd.events |= POLLOUT;
      	else if (ret < 0){
        	break;
		}
#endif /* AIKAWA_EVENT_TEST */

		rc_poll = poll(&pollfd, 1, (redraw ? 0 : -1));
#ifdef AIKAWA_EVENT_TEST
		if(rc_poll == -1){
			wl_display_cancel_read(display);
		}else{
			wl_display_read_events(display);
		}
		//wl_display_dispatch_pending(display);
#endif /* AIKAWA_EVENT_TEST */
		if(rc_poll == -1)
			break;

     	if(pollfd.revents & (POLLERR | POLLHUP))
        	break;

    	if(pollfd.revents & POLLIN){
        	ret = wl_display_dispatch(display);
        	if (ret == -1)
            	break;
      	}

    	if(pollfd.revents & POLLOUT){
        	ret = wl_display_flush(display);
        	if(ret == 0)
            	pollfd.events &= ~POLLOUT;
        	else if(ret == -1 && errno != EAGAIN)
            	break;
    	}

    	if (redraw){
        	//glvOnReDraw(glv_win);
      	}
 	}
}
#endif

void glvEscapeEventLoop(glvDisplay glv_dpy)
{
	GLV_DISPLAY_t *display = (GLV_DISPLAY_t*)glv_dpy;
	running = 0;
	display->running = 0;
}

void glvCommitWindow(glvWindow glv_win)
{
	GLV_WINDOW_t	*glv_window = (GLV_WINDOW_t*)glv_win;
	wl_surface_commit(glv_window->wl_window.surface);
}

//#define DEBUG_frame_callback
static void frame_callback(void *data, struct wl_callback *callback, uint32_t time)
{
	GLV_WINDOW_t *glv_window = data;

	//assert(callback == glv_window->wl_window.frame_cb);

	//printf("-------------------------- frame_callback [%s]\n",glv_window->name);

	wl_callback_destroy(callback);
		
	glv_window->wl_window.frame_cb = NULL;

	glv_window->wl_window.last_time = time;

	pthread_mutex_lock(&glv_window->window_mutex);			// window
	glv_window->draw__run = 0;
	glv_window->draw__run_count--;
#ifdef DEBUG_frame_callback
	if(glv_window->draw__run_count == 0){
		printf("frame_callback [%s]  all end\n",glv_window->name);
	}else{
		printf("frame_callback [%s]  remain %d\n",glv_window->name,glv_window->draw__run_count);
	}
#endif /* DEBUG_frame_callback */
	pthread_mutex_unlock(&glv_window->window_mutex);		// window

	if(glv_window->eventFunc.endDraw != NULL){
		_glvOnEndDraw(glv_window,time);
	}
}

static const struct wl_callback_listener freame_listener = {
	frame_callback
};

void glvSwapBuffers(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;

	// [Wayland] Crashes with src/wayland-client.c:230: wl_proxy_unref: Assertion `proxy->refcount > 0' failed.
	// wl_callback_destroyを呼ばなくすると落ちなくなった。 2021.05.11
#if 0
	if (glv_window->wl_window.frame_cb) {
		wl_callback_destroy(glv_window->wl_window.frame_cb);
	}
#endif

#ifdef DEBUG_frame_callback
	if(glv_window->draw__run == 1){
		// 描画が終了していないのに次の描画を行った。
		// 現状の処理では、正常ケースです。
		printf("glvSwapBuffers:glv_window->draw__run = 1 [%s]\n",glv_window->name);
	}
#endif /* DEBUG_frame_callback */

	glv_window->wl_window.frame_cb = wl_surface_frame(glv_window->wl_window.surface);
	wl_callback_add_listener(glv_window->wl_window.frame_cb, &freame_listener, glv_window);

	glv_window->drawCount++;

	pthread_mutex_lock(&glv_window->window_mutex);			// window
	glv_window->draw__run = 1;
	glv_window->draw__run_count++;
	pthread_mutex_unlock(&glv_window->window_mutex);		// window

	eglSwapBuffers(glv_window->glv_dpy->egl_dpy, glv_window->ctx.egl_surf);
	//printf("glvSwapBuffers: eglSwapBuffers %s\n",glv_window->name);
	// -------------------------------------------------------------------------
	// surfaceを作成した最初の描画では、そのsurfaceが表示されない場合がある為、
	// surfaceの最初の描画(glv_window->drawCount == 1))後に、
	// 親であるfreameのsurfaceを描画する 2021.01.24
#if 0
	if((glv_window->windowType  != GLV_TYPE_THREAD_FRAME) && (glv_window->drawCount == 1)){
		glvOnUpdate(glv_window->myFrame);
		//printf("glvSwapBuffers: Freame req Update %s\n",glv_window->myFrame->name);
	}
#else
	// surface生成時のwl_subsurface_set_position設定がすぐに表示に反映されない為、
	// 親であるsurfaceの描画に変更 2021.05.01
	if((glv_window->windowType  != GLV_TYPE_THREAD_FRAME) && (glv_window->drawCount == 1)){
		glvOnReDraw(glv_window->parent);
		//printf("glvSwapBuffers: parent req Update %s\n",glv_window->parent->name);
	}
#endif
}

GLV_WINDOW_t *_glvGetWindow(GLV_DISPLAY_t *glv_dpy,glvInstanceId windowId)
{
	GLV_WINDOW_t *glv_window;

	pthread_mutex_lock(&glv_dpy->display_mutex);			// display

	wl_list_for_each(glv_window, &((GLV_DISPLAY_t*)glv_dpy)->window_list, link){
		if((glv_window->instance.alive == GLV_INSTANCE_ALIVE) && (glv_window->instance.Id == windowId)){
			pthread_mutex_unlock(&glv_dpy->display_mutex);	// display
			return(glv_window);
		}
	}
	pthread_mutex_unlock(&glv_dpy->display_mutex);			// display
	return(NULL);
}

int _glv_destroyAllWindow(GLV_DISPLAY_t *glv_dpy)
{
	struct _glv_window *tmp;
	struct _glv_window *glv_all_window;

	pthread_mutex_lock(&glv_dpy->display_mutex);				// display
	wl_list_for_each_safe(glv_all_window, tmp, &glv_dpy->window_list, link){
		glvTerminateThreadSurfaceView(glv_all_window);
		_glvDestroyWindow(glv_all_window);
		pthread_mutex_destroy(&glv_all_window->window_mutex);
		GLV_IF_DEBUG_INSTANCE printf(GLV_DEBUG_INSTANCE_COLOR"_glv_destroyAllWindow: destroy [%s]\n"GLV_DEBUG_END_COLOR,glv_all_window->name);
		free(glv_all_window);
	}
	pthread_mutex_unlock(&glv_dpy->display_mutex);				// display
	
	return(GLV_OK);
}
