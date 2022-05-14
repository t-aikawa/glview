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

#ifndef GLVIEW_LOCAL_H
#define GLVIEW_LOCAL_H

#ifdef __cplusplus
extern "C" {
#endif

#define GLV_TEST__THIN_OUT_DRAWING		// 描画要求を連続に要求されている場合、その描画を間引く
#define GLV_PTHREAD_MUTEX_RECURSIVE		// リソースの排他制御のリカーシブコールを有効とする

#define GLV_INSTANCE_TYPE_DISPLAY	(0x10102020)
#define GLV_INSTANCE_TYPE_WINDOW	(0x30304040)
#define GLV_INSTANCE_TYPE_SHEET		(0x50506060)
#define GLV_INSTANCE_TYPE_WIGET		(0x70708080)
#define GLV_INSTANCE_TYPE_RESOURCE	(0x80809090)

#define GLV_END_REASON__INTERNAL	(1)
#define GLV_END_REASON__EXTERNAL	(2)

#define GLV_ON_ACTIVATE		    (0)
#define GLV_ON_INIT			    (1)
#define GLV_ON_CONFIGURE	    (2)
#define GLV_ON_RESHAPE		    (3)
#define GLV_ON_REDRAW		    (4)
#define GLV_ON_UPDATE		    (5)
#define GLV_ON_TIMER		    (6)
#define GLV_ON_MOUSE_POINTER	(7)
#define GLV_ON_MOUSE_BUTTON 	(8)
#define GLV_ON_MOUSE_AXIS	 	(9)
#define GLV_ON_ACTION		    (10)
#define GLV_ON_GESTURE		    (11)
#define GLV_ON_USER_MSG		    (12)
#define GLV_ON_KEY_INPUT	    (13)
#define GLV_ON_FOCUS		    (14)
#define GLV_ON_END_DRAW		    (15)
#define GLV_ON_KEY			    (16)
#define GLV_ON_TERMINATE	    (99)

#define GLV_KeyPress		(2)
#define GLV_KeyRelease		(3)

typedef struct _glv_window_event_func {
	struct glv_window_listener				*_class;
	GLV_WINDOW_EVENT_FUNC_new_t				_new;
	GLV_WINDOW_EVENT_FUNC_init_t			init;
	GLV_WINDOW_EVENT_FUNC_start_t			start;
	GLV_WINDOW_EVENT_FUNC_configure_t		configure;
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
	GLV_WINDOW_EVENT_FUNC_action_t			action;
	GLV_WINDOW_EVENT_FUNC_key_t				key;
	GLV_WINDOW_EVENT_FUNC_endDraw_t			endDraw;
	GLV_WINDOW_EVENT_FUNC_terminate_t		terminate;
} GLV_WINDOW_EVENT_FUNC_t;

typedef struct _glv_sheet_event_func {
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
} GLV_SHEET_EVENT_FUNC_t;

typedef struct _glv_wiget_event_func {
	struct glv_wiget_listener			*_class;
	GLV_WIGET_EVENT_FUNC_new_t			_new;
	GLV_WIGET_EVENT_FUNC_init_t			init;
	GLV_WIGET_EVENT_FUNC_redraw_t		redraw;
	GLV_WIGET_EVENT_FUNC_mousePointer_t	mousePointer;
	GLV_WIGET_EVENT_FUNC_mouseButton_t	mouseButton;
	GLV_WIGET_EVENT_FUNC_mouseAxis_t	mouseAxis;
	GLV_WIGET_EVENT_FUNC_input_t		input;
	GLV_WIGET_EVENT_FUNC_focus_t		focus;
	GLV_WIGET_EVENT_FUNC_terminate_t	terminate;
} GLV_WIGET_EVENT_FUNC_t;

typedef struct _glv_instance {
	void 				*oneself;
	glvInstanceId		Id;
	int					instanceType;
	int					alive;
	void				*user_data;
	struct _glv_r_value *resource;
	int					resource_run;
	pthread_mutex_t		resource_mutex;
} _GLV_INSTANCE_t;

typedef struct _wldisplay {
	struct _glv_display		*glv_dpy;
	struct wl_display		*display;
	struct wl_registry		*registry;
	struct wl_compositor	*compositor;
	struct wl_shm			*shm;
	struct wl_subcompositor	*subcompositor;
	struct xdg_wm_base		*xdg_wm_shell;		// interface:xdg_wm_base
	struct zxdg_shell_v6	*zxdgV6_shell;		// interface:zxdg_shell_v6
	struct wl_shell			*wl_shell;			// interface:wl_shell
	struct ivi_application	*ivi_application;	// interface:ivi_application
	struct wl_data_device_manager *data_device_manager;
	struct wl_cursor_theme	*cursor_theme;
	struct wl_cursor		**cursors;
	struct wl_list			input_list;
	struct wl_list			output_list;
	uint32_t				serial;
	uint32_t				data_device_manager_version;
} WL_DISPLAY_t;

typedef struct _glv_display {
	struct _glv_instance	instance;
	const char				*display_name;
	pthread_t				threadId;
	EGLNativeDisplayType	native_dpy;
	EGLDisplay				egl_dpy;
	EGLint					egl_major;
	EGLint					egl_minor;
	EGLConfig				egl_config_normal;
	EGLConfig				egl_config_beauty;
	EGLint					vid;
	WL_DISPLAY_t			wl_dpy;
	struct wl_list			window_list;
	struct _glv_window		*rootWindow;
	/* ------------------------------------- */
	pthread_mutex_t			display_mutex;
	/* ------------------------------------- */
	int						display_fd;
	uint32_t				display_fd_events;
	struct task				display_task;
	int						epoll_fd;
	int						running;
	// ------------------------------------
	glvInstanceId			kb_input_windowId;
	glvInstanceId			kb_input_sheetId;
	glvInstanceId			kb_input_wigetId;
	int						ins_mode;
	glvInstanceId			toplevel_active_frameId;
	int						(*ime_setCandidatePotition)(int candidate_pos_x,int candidate_pos_y);
} GLV_DISPLAY_t;

typedef struct _wlwindow {
	struct wl_surface		*parent;
	struct wl_surface		*surface;
	struct wl_subsurface	*subsurface;
	struct xdg_surface		*xdg_wm_surface;	// interface:xdg_wm_base
	struct xdg_toplevel		*xdg_wm_toplevel;	// interface:xdg_wm_base
	struct xdg_popup		*xdg_wm_popup;		// interface:xdg_wm_base
	struct zxdg_surface_v6	*zxdgV6_surface;	// interface:zxdg_shell_v6
	struct zxdg_toplevel_v6	*zxdgV6_toplevel;	// interface:zxdg_shell_v6
	struct zxdg_popup_v6	*zxdgV6_popup;		// interface:zxdg_shell_v6
	struct wl_shell_surface	*wl_shell_surface;	// interface:wl_shell
	struct ivi_surface		*ivi_surface;		// interface:ivi_application
	struct wl_callback		*frame_cb;
	int32_t					buffer_scale;
	uint32_t				last_time;
} WL_WINDOW_t;

typedef struct _glvContext {
	pthread_t			threadId;
	int					runThread;
	int					endReason;
	EGLConfig			egl_config;
	EGLSurface			egl_surf;
	pthread_msq_id_t	queue;
} _GLVCONTEXT_t;

typedef struct _glv_window {
	struct _glv_instance	instance;
	int			windowType;
	int			attr;
	int			drawCount;
    int         toplevel_activated;			// XDG_TOPLEVEL_STATE_ACTIVATED 状態
	int			toplevel_maximized;			// XDG_TOPLEVEL_STATE_MAXIMIZED 状態
	int			toplevel_unset_maximized_width;
	int			toplevel_unset_maximized_height;
	int			flag_surface_configure;		// 最初に surface の configure イベントが呼ばれた
	int			flag_toplevel_configure;	// 最初に toplevel の configure イベントが呼ばれた
	int			req_toplevel_resize;
	int			req_toplevel_inner_width;
	int			req_toplevel_inner_height;
	GLV_FRAME_INFO_t	frameInfo;
	/* --------------------------- */
	// for debug
	int draw__run;
	int draw__run_count;
	/* --------------------------- */
	GLV_DISPLAY_t			*glv_dpy;
	EGLNativeWindowType		egl_window;
	WL_WINDOW_t				wl_window;
	struct _glv_window		*parent;	// window作成時の上位window
	struct _glv_window		*myFrame;	// windowが表示されているframeのwindow
	struct _glv_window		*teamLeader;// このウインドウがメッセージを受信するための送信先ウインドウ
	GLV_WINDOW_EVENT_FUNC_t	eventFunc;
	/* --------------------------- */
	char 	*title;
	char 	*name;
	int 	x, y;
	int 	width, height;
	int		absolute_x,absolute_y;
	/* --------------------------- */
	int		cmdMenu;
	int		pullDownMenu;
	int		statusBar;
	int		beauty;
	/* --------------------------- */
	_GLVCONTEXT_t		ctx;
	struct _glv_sheet	*active_sheet;
	struct wl_list		sheet_list;
	int					reqSwapBuffersFlag;
	uint32_t			configure_serial;
	uint32_t			resharp_serial;
	uint32_t			draw_serial;
	int					edges;
	/* --------------------------- */
	sem_t				initSync;
	pthread_mutex_t		window_mutex;
	pthread_mutex_t		serialize_mutex;
	struct wl_list link;
}GLV_WINDOW_t;

typedef struct _glv_sheet {
	struct _glv_instance	instance;
	GLV_DISPLAY_t		*glv_dpy;
	GLV_WINDOW_t		*glv_window;
	glvInstanceId		pointer_focus_wiget_Id;
//	int					pointer_focus_wiget_x;
//	int					pointer_focus_wiget_y;
	glvInstanceId		select_wiget_Id;
	int					select_wiget_x;
	int					select_wiget_y;
	int					select_wiget_button_status;
	int					initialized;
	int					reqDrawWigetsFlag;
	GLV_SHEET_EVENT_FUNC_t	eventFunc;
	/* --------------------------- */
    char 	*name;
	/* --------------------------- */
	struct wl_list		wiget_list;
	/* --------------------------- */
	pthread_mutex_t		sheet_mutex;
	struct wl_list		link;
} GLV_SHEET_t;

typedef struct _glv_wiget {
	struct _glv_instance	instance;
	int				attr;
	GLV_DISPLAY_t		*glv_dpy;
	GLV_SHEET_t			*glv_sheet;
	int			sheet_x;
	int			sheet_y;
	int			visible;
	int			width,height;
	float		scale;
	// ------------------------------------
	//int			ibus_candidate_x;
	//int			ibus_candidate_y;
	// ------------------------------------
	GLV_WIGET_EVENT_FUNC_t	eventFunc;
	struct wl_list link;
} GLV_WIGET_t;

struct _glvinput
{
	GLV_DISPLAY_t		*glv_dpy;
	WL_DISPLAY_t		*wl_dpy;
	struct wl_seat		*seat;
	struct wl_pointer	*pointer;
	struct wl_keyboard	*keyboard;
	struct wl_touch		*touch;
	// ------------------------------------
	struct wl_data_device *data_device;
	struct data_offer	*drag_offer;
	struct data_offer	*selection_offer;
	uint32_t drag_enter_serial;
	// ------------------------------------
	int					seat_version;
	// ------------------------------------
	struct xkb_keymap	*xkb_keymap;
	struct xkb_state	*xkb_state;
	xkb_mod_mask_t		modifiers;
	xkb_mod_mask_t		shift_mask;
	xkb_mod_mask_t		lock_mask;
	xkb_mod_mask_t		control_mask;
	xkb_mod_mask_t		mod1_mask;
	xkb_mod_mask_t		mod2_mask;
	xkb_mod_mask_t		mod3_mask;
	xkb_mod_mask_t		mod4_mask;
	xkb_mod_mask_t		mod5_mask;
	xkb_mod_mask_t		super_mask;
	xkb_mod_mask_t		hyper_mask;
	xkb_mod_mask_t		meta_mask;
	// ------------------------------------
	void				*im;
	int					im_state;
	int					(*ime_key_event)(struct _glvinput *glv_input,int keycode, int ksym, int state_,int type);
	void				(*ime_key_modifiers)(struct _glvinput *glv_input,xkb_mod_mask_t mask);
	//int					(*ime_setCandidatePotition)(int candidate_pos_x,int candidate_pos_y);
	// ------------------------------------
	struct wl_surface	*pointer_surface;
	glvInstanceId		pointer_enter_windowId;
	uint32_t			pointer_enter_serial;	// enter 時の serial
	uint32_t			cursor_serial;
	int					current_cursor;
	// ------------------------------------
	int find_wiget;	/* 0: wigetを見つけていない */
					/* 1: wigetを見つけている   */
	int pointer_sx;
	int pointer_sy;	
	glvInstanceId		select_wigetId;
	glvInstanceId		pointer_focus_wigetId;
	glvInstanceId		pointer_focus_windowId;
	struct wl_list link;
//}GLV_INPUT_t;
};

struct _glvoutput {
	GLV_DISPLAY_t	*glv_dpy;
	struct wl_output *output;
	uint32_t id;
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	struct wl_list link;
	int transform;
	int scale;
};

typedef struct _glv_garbage_box {
	struct wl_list		wiget_list;
	struct wl_list		sheet_list;
	//struct wl_list		window_list;
	int					gc_run;
	/* --------------------------- */
	pthread_mutex_t		pthread_mutex;
} GLV_GARBAGE_BOX_t;

int _glvInitInstance(_GLV_INSTANCE_t *instance,int instanceType);
GLV_DISPLAY_t *_glvInitNativeDisplay(GLV_DISPLAY_t *glv_dpy);
GLV_DISPLAY_t *_glvOpenNativeDisplay(GLV_DISPLAY_t *glv_dpy);
void _glvCloseNativeDisplay(GLV_DISPLAY_t *glv_dpy);
int glvTerminateThreadSurfaceView(glvWindow glv_win);
EGLNativeDisplayType glvGetNativeDisplay(glvDisplay glv_dpy);
EGLDisplay	glvGetEGLDisplay(glvDisplay glv_dpy);
EGLConfig glvGetEGLConfig_normal(glvDisplay glv_dpy);
EGLConfig glvGetEGLConfig_beauty(glvDisplay glv_dpy);
EGLint		glvGetEGLVisualid(glvDisplay glv_dpy);
int glvSelectDrawingWindow(glvWindow glv_win);
void glvSwapBuffers(glvWindow glv_win);
int glv_getInstanceType(void *glv_instance);

int glvAllocWindowResource(glvDisplay glv_dpy,glvWindow *glv_win,char *name,const struct glv_window_listener *listener);
GLV_WINDOW_t *_glvAllocWindowResource(GLV_DISPLAY_t *glv_dpy,char *name);
int _glvCreateWindow(GLV_WINDOW_t *glv_window,char *name,int windowType,char *title,int x, int y, int width, int height,glvWindow glv_win_parent,int attr);
void _glvDestroyWindow(GLV_WINDOW_t *glv_window);
void _glvResizeWindow(GLV_WINDOW_t *glv_window,int x,int y,int width,int height);
glvWindow	glvCreateThreadSurfaceView(glvWindow glv_win);
void glvCommitWindow(glvWindow glv_win);

int _glvOnInit_for_childWindow(glvWindow glv_win,int width, int height);
int _glvOnConfigure(glvWindow glv_win,int width, int height);
int glvOnActivate(glvWindow glv_win);
int _glvOnMousePointer(void *glv_instance,int type,glvTime time,int x,int y,int pointer_left_stat);
int _glvOnMouseButton(void *glv_instance,int type,glvTime time,int x,int y,int pointer_stat);
int _glvOnMouseAxis(void *glv_instance,int type,glvTime time,int value);
int _glvOnKey(glvWindow glv_win,unsigned int key,unsigned int modifiers,unsigned int state);
int _glvOnTextInput(glvDisplay glv_dpy,int kind,int state,uint32_t kyesym,int *utf32,uint8_t *attr,int length);
int _glvOnFocus(glvDisplay glv_dpy,int focus_stat,glvWiget in_Wiget);
int _glvOnEndDraw(glvWindow glv_win,glvTime time);

int glvCheckTimer(glvWindow glv_win,int id,int count);
void glvInitializeTimer(void);
void glvTerminateTimer(void);

void _glv_sheet_initialize_cb(GLV_WINDOW_t *glv_window);
void _glv_sheet_with_wiget_reshape_cb(GLV_WINDOW_t *glv_window);
void _glv_sheet_with_wiget_redraw_cb(GLV_WINDOW_t *glv_window);
void _glv_sheet_with_wiget_update_cb(GLV_WINDOW_t *glv_window);
void _glv_sheet_userMsg_cb(GLV_WINDOW_t *glv_window,int kind,void *data);
void _glv_sheet_action_front(GLV_WINDOW_t *glv_window);
void _glv_sheet_action_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int action,glvInstanceId selectId);
void _glv_window_and_sheet_mousePointer_front(GLV_WINDOW_t *glv_window,int type,glvTime time,int x,int y,int pointer_left_stat);
void _glv_sheet_mousePointer_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,int type,glvTime time,int x,int y,int pointer_left_stat);
void _glv_wiget_mousePointer_front(struct _glvinput *glv_input,GLV_WINDOW_t *glv_window,int type,glvTime time,int x,int y,int pointer_left_stat);
void _glv_wiget_mousePointer_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int type,glvTime time,int x,int y,int pointer_left_stat);
void _glv_window_and_sheet_mouseButton_front(GLV_WINDOW_t *glv_window,int type,glvTime time,int x,int y,int pointer_stat);
void _glv_sheet_mouseButton_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,int type,glvTime time,int x,int y,int pointer_left_stat);
void _glv_wiget_mouseButton_front(struct _glvinput *glv_input,GLV_WINDOW_t *glv_window,int type,glvTime time,int x,int y,int pointer_stat);
void _glv_wiget_mouseButton_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int type,glvTime time,int x,int y,int pointer_stat);
void _glv_window_and_sheet_mouseAxis_front(GLV_WINDOW_t *glv_window,int type,glvTime time,int value);
void _glv_sheet_mouseAxis_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,int type,glvTime time,int value);
void _glv_wiget_mouseAxis_front(GLV_WINDOW_t *glv_window,int type,glvTime time,int value);
void _glv_wiget_mouseAxis_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int type,glvTime time,int value);
void _glv_wiget_key_input_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int type,int state,uint32_t kyesym,int *in_text,uint8_t *attr,int length);
void _glv_wiget_focus_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int focus_stat,glvInstanceId in_windowId,glvInstanceId in_sheetId,glvInstanceId in_wigetId);
glvInstanceId _glv_wiget_check_wiget_area(GLV_WINDOW_t *glv_window,int x,int y,int button_status);

void _glvGcDestroyWiget(GLV_WIGET_t *glv_wiget);
void _glvGcDestroySheet(GLV_SHEET_t *glv_sheet);
void _glvGcDestroyWindow(GLV_WINDOW_t *glv_window);

int _glvWindowMsgHandler_dispatch(glvDisplay glv_dpy);
int _glvCreateGarbageBox(void);
void _glvDestroyGarbageBox(void);
int _glvGcGarbageBox(void);

GLV_WINDOW_t *_glvGetWindowFromId(GLV_DISPLAY_t *glv_dpy,glvInstanceId windowId);
void _glv_window_list_on_reshape(GLV_WINDOW_t *frame_window,int width,int height);

// ibus
int glv_ime_startIbus(struct _glvinput *glv_input);
//void glv_ime_key_modifiers(struct _glvinput *glv_input,xkb_mod_mask_t mask);
//int glv_ime_key_event(struct _glvinput *glv_input,int keycode, int ksym, int state_,int type);

// fcitx
int glv_ime_startFcitx(struct _glvinput *glv_input);

extern GLV_GARBAGE_BOX_t	_glv_garbage_box;
extern glvInstanceId _glv_instance_Id;

void weston_client_window__input_set_pointer_image(struct _glvinput *input, int cursor);
void weston_client_window__create_cursors(WL_DISPLAY_t *display);
void weston_client_window__display_watch_fd(struct _glv_display *display,int fd, uint32_t events, struct task *task);
void weston_client_window__display_unwatch_fd(struct _glv_display *display, int fd);
void weston_client_window__display_exit(struct _glv_display *display);
void weston_client_window__display_create(struct _glv_display *display);
void weston_client_window__display_destroy(struct _glv_display *display);
void weston_client_window__display_run(struct _glv_display *display);

int glv_r_set_value(struct _glv_r_value **link,void *instance,char *key,char *type_string,va_list args);
int glv_r_get_value(struct _glv_r_value **link,void *instance,char *key,char *type_string,va_list args);
int glv_r_free_value__list(struct _glv_r_value **link);

int _glv_destroyAllWindow(GLV_DISPLAY_t *glv_dpy);

void glvWindow_setHandler_class(glvWindow glv_win,struct glv_window_listener *_class);
void glvWindow_setHandler_new(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_new_t _new);
void glvWindow_setHandler_init(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_init_t init);
void glvWindow_setHandler_start(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_start_t start);
void glvWindow_setHandler_configure(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_configure_t configure);
void glvWindow_setHandler_reshape(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_reshape_t reshape);
void glvWindow_setHandler_redraw(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_redraw_t redraw);
void glvWindow_setHandler_update(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_update_t update);
void glvWindow_setHandler_timer(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_timer_t timer);
void glvWindow_setHandler_mousePointer(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_mousePointer_t mousePointer);
void glvWindow_setHandler_mouseButton(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_mouseButton_t mouseButton);
void glvWindow_setHandler_mouseAxis(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_mouseAxis_t mouseAxis);
void glvWindow_setHandler_gesture(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_gesture_t gesture);
void glvWindow_setHandler_cursor(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_cursor_t cursor);
void glvWindow_setHandler_userMsg(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_userMsg_t userMsg);
void glvWindow_setHandler_terminate(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_terminate_t terminate);
void glvWindow_setHandler_action(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_action_t action);
void glvWindow_setHandler_key(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_key_t key);
void glvWindow_setHandler_endDraw(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_endDraw_t endDraw);

void glvSheet_setHandler_class(glvSheet sheet,struct glv_sheet_listener	*_class);
void glvSheet_setHandler_new(glvSheet sheet,GLV_SHEET_EVENT_FUNC_new_t _new);
void glvWiget_setHandler_class(glvWiget wiget,struct glv_wiget_listener *_class);
void glvWiget_setHandler_new(glvWiget wiget,GLV_WIGET_EVENT_FUNC_new_t _new);

int glv_printOpenGLRuntimeEnvironment(glvDisplay glv_dpy);

#ifdef __cplusplus
}
#endif

#endif /* GLVIEW_LOCAL_H */

