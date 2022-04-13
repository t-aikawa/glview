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

// ----------------------------------------------------
#define GLV_VERSION_MAJOR	(0)
#define GLV_VERSION_MINOR	(1)
#define GLV_VERSION_PATCH	(15)
// ----------------------------------------------------

#define GLV_OPENGL_ES1_API	(1)
#define GLV_OPENGL_ES2_API	(2)

static int glv_system_onetime_counter = 0;

static int _usr_msg_ok_receive_count=0;
static int _usr_msg_ok_send_count=0;
static int _usr_msg_ng_receive_count=0;
static int _usr_msg_ng_send_count=0;

static int _glv_debug_flag = GLV_DEBUG_OFF;

static int _glv_debug_flag_validity_list[] = {
	GLV_DEBUG_VERSION,
	//GLV_DEBUG_MSG,
	GLV_DEBUG_INSTANCE,
	//GLV_DEBUG_KB_INPUT,
	//GLV_DEBUG_IME_INPUT,
	GLV_DEBUG_WIGET,
	//GLV_DEBUG_DATA_DEVICE,
};

void glvGetVersion(int *major,int *minor,int *patch)
{
	if(major != NULL){
		*major = GLV_VERSION_MAJOR;
	}
	if(minor != NULL){
		*minor = GLV_VERSION_MINOR;
	}
	if(patch != NULL){
		*patch = GLV_VERSION_PATCH;
	}
}

void glvSetDebugFlag(int flag){
	int i,n;
	int new_flag=0;
	n = sizeof(_glv_debug_flag_validity_list) / sizeof(int);
	for(i=0;i<n;i++){
		if(flag & _glv_debug_flag_validity_list[i]){
			new_flag |= _glv_debug_flag_validity_list[i];
		}
	}
	_glv_debug_flag = new_flag;
}

int glvCheckDebugFlag(int flag){
	return(_glv_debug_flag & flag);
}

int _glvInitInstance(_GLV_INSTANCE_t *instance,int instanceType)
{
	instance->oneself		= instance;
	instance->Id			= ++_glv_instance_Id;
	instance->instanceType	= instanceType;
	instance->alive			= GLV_INSTANCE_ALIVE;
	instance->user_data		= NULL;
	instance->resource		= NULL;
	instance->resource_run	= 0;
	pthread_mutex_init(&instance->resource_mutex,NULL);
	return(GLV_OK);
}

glvResource glvCreateResource(void)
{
	int rc;
	_GLV_INSTANCE_t *instance = calloc(sizeof(_GLV_INSTANCE_t),1);

	rc = _glvInitInstance(instance,GLV_INSTANCE_TYPE_RESOURCE);

	UNUSED(rc);
	
	return(instance);
}

void glvDestroyResource(glvResource res)
{
	_GLV_INSTANCE_t *instance = (_GLV_INSTANCE_t*)res;
	int instanceType = instance->instanceType;

	instance->alive = GLV_INSTANCE_DEAD;

	if(instance->user_data != NULL){
		free(instance->user_data);
		instance->user_data = NULL;
	}

	if(instance->resource != NULL){
		glv_r_free_value__list(&instance->resource);
	}

	instance->oneself = NULL;
	pthread_mutex_destroy(&instance->resource_mutex);
	if(instanceType == GLV_INSTANCE_TYPE_RESOURCE){
		free(instance);
	}
}

/* ---------------------------------------------------------- */
glvDisplay glvOpenDisplay(char *dpyName)
{
	EGLDisplay egl_dpy;
	GLV_DISPLAY_t *glv_dpy,*dpy;
	EGLint egl_major, egl_minor;
	EGLBoolean rc;
	EGLint num_configs;
	EGLenum api;
	EGLContext	egl_ctx;

	static EGLint attribs_normal[] = {
#ifdef GLV_OPENGL_ES2
		EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
#endif
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
	static EGLint attribs_beauty[] = {
#ifdef GLV_OPENGL_ES2
		EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
#endif
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 8,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_SAMPLE_BUFFERS, 1, //マルチサンプリングを有効
		EGL_SAMPLES, 4, //サンプリング数を指定
		EGL_NONE
	};

	// 環境変数によるデバック設定
	// ex.
	// GLVIEW_DEBUG=1 ./a.out		_glv_debug_flag_validity_list に設定されているglview内のデバック情報を出力する
	// GLVIEW_DEBUG=2 ./a.out		glview内の全てのデバック情報を出力する
	// GLVIEW_WAYLAND=1 ./a.out		waylandのデバック情報(WAYLAND_DEBUG=1)を出力する
	// GLVIEW_WAYLAND=1 GLVIEW_DEBUG=2 ./a.out		全てのデバック情報を出力する	
	{
		char *env;
		env = getenv("GLVIEW_DEBUG");
		if(env != NULL){
			if(strcmp(env, "1") == 0) {
				glvSetDebugFlag(GLV_DEBUG_ON);
			}else if(strcmp(env, "2") == 0) {
				_glv_debug_flag = 0x7fffffff;
			}
		}
		env = getenv("GLVIEW_WAYLAND");
		if(env != NULL){
			if(strcmp(env, "1") == 0) {
				setenv("WAYLAND_DEBUG","1",0);
				_glv_debug_flag |= GLV_DEBUG_VERSION;
			}
		}
	}

	GLV_IF_DEBUG_VERSION printf("--------------------------------------------------------------------------\n");
	GLV_IF_DEBUG_VERSION printf("glview:version %d.%d.%d " "(" __DATE__ ")\n",GLV_VERSION_MAJOR,GLV_VERSION_MINOR,GLV_VERSION_PATCH);

	glvGl_thread_safe_init();
	glvFont_thread_safe_init();

	glv_dpy =(GLV_DISPLAY_t *)malloc(sizeof(GLV_DISPLAY_t));
	if(!glv_dpy){
		return(0);
	}
	memset(glv_dpy,0,sizeof(GLV_DISPLAY_t));

	_glvInitInstance(&glv_dpy->instance,GLV_INSTANCE_TYPE_DISPLAY);
	
	wl_list_init(&glv_dpy->window_list);

	glv_dpy->threadId = pthread_self();
	glv_dpy->display_name = dpyName;
	glv_dpy->wl_dpy.glv_dpy = glv_dpy;

	dpy = _glvInitNativeDisplay(glv_dpy);
	if(!dpy){
		free(glv_dpy);
		return(0);
	}
	dpy = _glvOpenNativeDisplay(glv_dpy);
	if(!dpy){
		free(glv_dpy);
		return(0);
	}
	egl_dpy = eglGetDisplay(glv_dpy->native_dpy);
	if(!egl_dpy){
		free(glv_dpy);
		return(0);
	}

	rc = eglInitialize(egl_dpy, &egl_major, &egl_minor);
	if(!rc){
		eglTerminate(egl_dpy);
		free(glv_dpy);
		return(0);
	}

	if(!eglChooseConfig(egl_dpy,attribs_normal,&glv_dpy->egl_config_normal,1,&num_configs)){
		fprintf(stderr,"glvOpenDisplay:eglChooseConfig error (egl_config_normal,eglError: %d)\n", eglGetError());
		eglTerminate(egl_dpy);
		free(glv_dpy);
		return(0);
	}

	if(!eglChooseConfig(egl_dpy,attribs_beauty,&glv_dpy->egl_config_beauty,1,&num_configs)){
#if 1
		fprintf(stderr,"glvOpenDisplay:eglChooseConfig error (egl_config_beauty,eglError: %d)\n", eglGetError());
		fprintf(stderr,"glvOpenDisplay:Continue processing using normal settings.\n");
		glv_dpy->egl_config_beauty = glv_dpy->egl_config_normal;
#else
		eglTerminate(egl_dpy);
		free(glv_dpy);
		return(0);
#endif
	}	
#if 0
	if (!eglGetConfigAttrib(egl_dpy,glv_dpy->egl_config_normal,EGL_NATIVE_VISUAL_ID,&glv_dpy->vid)) {
		eglTerminate(egl_dpy);
		free(glv_dpy);
		return(0);
	}
#endif

#ifdef GLV_OPENGL_ES2
   api = GLV_OPENGL_ES2_API;
#else
   api = GLV_OPENGL_ES1_API;
#endif
	//// egl-contexts collect all state descriptions needed required for operation
	EGLint ctxattr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,	/* 1:opengles1.1 2:opengles2.0 */
		EGL_NONE
	};
	ctxattr[1] = api;
	egl_ctx = eglCreateContext(egl_dpy,glv_dpy->egl_config_normal,EGL_NO_CONTEXT,ctxattr);
	if(egl_ctx == EGL_NO_CONTEXT ) {
		fprintf(stderr,"glvOpenDisplay:Unable to create EGL context (eglError: %d)\n", eglGetError());
	   	eglTerminate(egl_dpy);
		free(glv_dpy);
		return(NULL);
	}
	glvGl_setEglContextInfo(egl_dpy,egl_ctx);
	eglSwapInterval(egl_dpy, 0);

	glv_dpy->egl_dpy = egl_dpy;

	// ---------------------------------------------------------------------------
	{
		pthread_msq_id_t queue = PTHREAD_MSQ_ID_INITIALIZER;

		glv_dpy->rootWindow = _glvAllocWindowResource(glv_dpy,"root");
		memcpy(&glv_dpy->rootWindow->ctx.queue,&queue,sizeof(pthread_msq_id_t));

		// メッセージキュー生成
		if(0 != pthread_msq_create(&glv_dpy->rootWindow->ctx.queue, 100)) {
			fprintf(stderr,"glvOpenDisplay:Error: pthread_msq_create() failed\n");
	   		eglTerminate(egl_dpy);
			free(glv_dpy);
		}
	}
	// ---------------------------------------------------------------------------

	// one time only
	if(glv_system_onetime_counter <= 0){
		glvInitializeTimer();
		_glvCreateGarbageBox();
	}
	glv_system_onetime_counter++;

	if (!eglMakeCurrent(glvGl_GetEglDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE, glvGl_GetEglContext())) {
		fprintf(stderr,"glvOpenDisplay:Error: eglMakeCurrent() failed\n");
	   	eglTerminate(egl_dpy);
		free(glv_dpy);
		return(NULL);
	}

#ifdef GLV_OPENGL_ES2
	es1emu_Init();
#endif

	GLV_IF_DEBUG_VERSION printf("glview:GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
	GLV_IF_DEBUG_VERSION printf("glview:GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
	//printf("glview:GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
	//printf("glview:GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));

	weston_client_window__display_create(glv_dpy);

	glv_setValue(glv_dpy,"cmdMenu wiget","P",NULL);

	return ((glvDisplay)glv_dpy);
}

int glvCloseDisplay(glvDisplay display)
{
	GLV_DISPLAY_t *glv_dpy = (GLV_DISPLAY_t*)display;

	_glv_destroyAllWindow(glv_dpy);

	// one time only
	--glv_system_onetime_counter;
	if(glv_system_onetime_counter <= 0){
		glvTerminateTimer();
		while(_glvGcGarbageBox());
		_glvDestroyGarbageBox();
	}
	glv_dpy->instance.alive	= GLV_INSTANCE_DEAD;

	pthread_msq_destroy(&glv_dpy->rootWindow->ctx.queue);
	pthread_mutex_destroy(&glv_dpy->rootWindow->window_mutex);
	free(glv_dpy->rootWindow);

	glvDestroyResource(&glv_dpy->instance);

	eglTerminate(glv_dpy->egl_dpy);
	_glvCloseNativeDisplay(glv_dpy);
	pthread_mutex_destroy(&glv_dpy->display_mutex);

#ifdef GLV_OPENGL_ES2
   es1emu_Finish();
#endif

	free(glv_dpy);

	return(GLV_OK);
}

// -----------------------------------------------------------------------------------------
void glvWindow_setHandler_class(glvWindow glv_win,struct glv_window_listener *class)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.class = class;
}

void glvWindow_setHandler_init(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_init_t init)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.init = init;
}

void glvWindow_setHandler_start(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_start_t start)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.start = start;
}

void glvWindow_setHandler_configure(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_configure_t configure)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.configure = configure;
}

void glvWindow_setHandler_reshape(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_reshape_t reshape)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.reshape = reshape;
}

void glvWindow_setHandler_redraw(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_redraw_t redraw)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.redraw = redraw;
}

void glvWindow_setHandler_update(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_update_t update)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.update = update;
}

void glvWindow_setHandler_timer(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_timer_t timer)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.timer = timer;
}

void glvWindow_setHandler_mousePointer(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_mousePointer_t mousePointer)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.mousePointer = mousePointer;
}

void glvWindow_setHandler_mouseButton(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_mouseButton_t mouseButton)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.mouseButton = mouseButton;
}

void glvWindow_setHandler_mouseAxis(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_mouseAxis_t mouseAxis)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.mouseAxis = mouseAxis;
}

void glvWindow_setHandler_gesture(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_gesture_t gesture)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.gesture = gesture;
}

void glvWindow_setHandler_cursor(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_cursor_t cursor)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.cursor = cursor;
}

void glvWindow_setHandler_userMsg(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_userMsg_t userMsg)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.userMsg = userMsg;
}

void glvWindow_setHandler_action(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_action_t action)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.action = action;
}

void glvWindow_setHandler_key(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_key_t key)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.key = key;
}

void glvWindow_setHandler_endDraw(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_endDraw_t endDraw)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.endDraw = endDraw;
}

void glvWindow_setHandler_terminate(glvWindow glv_win,GLV_WINDOW_EVENT_FUNC_terminate_t terminate)
{
	GLV_WINDOW_t *glv_window=(GLV_WINDOW_t *)glv_win;
	if(glv_window == NULL) return;
	glv_window->eventFunc.terminate = terminate;
}

// -----------------------------------------------------------------------------------------
void glvSheet_setHandler_class(glvSheet sheet,struct glv_sheet_listener	*class)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.class = class;
}

void glvSheet_setHandler_init(glvSheet sheet,GLV_SHEET_EVENT_FUNC_init_t init)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	GLV_WINDOW_t *glv_window;

	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.init = init;

	glv_window = glv_sheet->glv_window;

	if(glv_sheet->eventFunc.init){
		int rc;
		rc = (glv_sheet->eventFunc.init)((glvWindow)glv_window,(glvSheet)glv_sheet,glv_window->width,glv_window->height);
		if(rc != GLV_OK){
			fprintf(stderr,"glvSheet_setHandler_init:glv_sheet->eventFunc.init error\n");
		}
		glv_sheet->initialized = 1;
	}
}

void glvSheet_setHandler_reshape(glvSheet sheet,GLV_SHEET_EVENT_FUNC_reshape_t reshape)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.reshape = reshape;
}

void glvSheet_setHandler_redraw(glvSheet sheet,GLV_SHEET_EVENT_FUNC_redraw_t redraw)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.redraw = redraw;
}

void glvSheet_setHandler_update(glvSheet sheet,GLV_SHEET_EVENT_FUNC_update_t update)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.update = update;
}

void glvSheet_setHandler_timer(glvSheet sheet,GLV_SHEET_EVENT_FUNC_timer_t timer)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.timer = timer;
}

void glvSheet_setHandler_mousePointer(glvSheet sheet,GLV_SHEET_EVENT_FUNC_mousePointer_t mousePointer)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.mousePointer = mousePointer;
}

void glvSheet_setHandler_mouseButton(glvSheet sheet,GLV_SHEET_EVENT_FUNC_mouseButton_t mouseButton)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.mouseButton = mouseButton;
}

void glvSheet_setHandler_mouseAxis(glvSheet sheet,GLV_SHEET_EVENT_FUNC_mouseAxis_t mouseAxis)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.mouseAxis = mouseAxis;
}

void glvSheet_setHandler_action(glvSheet sheet,GLV_SHEET_EVENT_FUNC_action_t action)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.action = action;
}

void glvSheet_setHandler_userMsg(glvSheet sheet,GLV_SHEET_EVENT_FUNC_userMsg_t userMsg)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.userMsg = userMsg;
}

void glvSheet_setHandler_terminate(glvSheet sheet,GLV_SHEET_EVENT_FUNC_terminate_t terminate)
{
	GLV_SHEET_t *glv_sheet=(GLV_SHEET_t *)sheet;
	if(glv_sheet == NULL) return;
	glv_sheet->eventFunc.terminate = terminate;
}

// -----------------------------------------------------------------------------------------
void glvWiget_setHandler_class(glvWiget wiget,struct glv_wiget_listener *class)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t *)wiget;
	if(glv_wiget == NULL) return;
	glv_wiget->eventFunc.class = class;
}

void glvWiget_setHandler_init(glvWiget wiget,GLV_WIGET_EVENT_FUNC_init_t init)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t *)wiget;
	if(glv_wiget == NULL) return;
	glv_wiget->eventFunc.init = init;

	if(glv_wiget->eventFunc.init){
		int rc;
		rc = (glv_wiget->eventFunc.init)((glvWindow)glv_wiget->glv_sheet->glv_window,(glvWiget)glv_wiget->glv_sheet,(glvSheet)glv_wiget);
		if(rc != GLV_OK){
			fprintf(stderr,"glvWiget_setHandler_init:glv_wiget->eventFunc.init error\n");
		}
	}
}

void glvWiget_setHandler_redraw(glvWiget wiget,GLV_WIGET_EVENT_FUNC_redraw_t redraw)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t *)wiget;
	if(glv_wiget == NULL) return;
	glv_wiget->eventFunc.redraw = redraw;
}

void glvWiget_setHandler_mousePointer(glvWiget wiget,GLV_WIGET_EVENT_FUNC_mousePointer_t mousePointer)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t *)wiget;
	if(glv_wiget == NULL) return;
	glv_wiget->eventFunc.mousePointer = mousePointer;
}

void glvWiget_setHandler_mouseButton(glvWiget wiget,GLV_WIGET_EVENT_FUNC_mouseButton_t mouseButton)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t *)wiget;
	if(glv_wiget == NULL) return;
	glv_wiget->eventFunc.mouseButton = mouseButton;
}

void glvWiget_setHandler_mouseAxis(glvWiget wiget,GLV_WIGET_EVENT_FUNC_mouseAxis_t mouseAxis)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t *)wiget;
	if(glv_wiget == NULL) return;
	glv_wiget->eventFunc.mouseAxis = mouseAxis;
}

void glvWiget_setHandler_input(glvWiget wiget,GLV_WIGET_EVENT_FUNC_input_t input)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t *)wiget;
	if(glv_wiget == NULL) return;
	glv_wiget->eventFunc.input = input;
}

void glvWiget_setHandler_focus(glvWiget wiget,GLV_WIGET_EVENT_FUNC_focus_t focus)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t *)wiget;
	if(glv_wiget == NULL) return;
	glv_wiget->eventFunc.focus = focus;
}

void glvWiget_setHandler_terminate(glvWiget wiget,GLV_WIGET_EVENT_FUNC_terminate_t terminate)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t *)wiget;
	if(glv_wiget == NULL) return;
	glv_wiget->eventFunc.terminate = terminate;
}
// -----------------------------------------------------------------------------------------
int glvAllocWindowResource(glvDisplay glv_dpy,glvWindow *glv_win,char *name,const struct glv_window_listener *listener)
{
	GLV_WINDOW_t *glv_window;

	glv_window = *glv_win;

	glv_window = _glvAllocWindowResource((GLV_DISPLAY_t*)glv_dpy,name);
	if(glv_window == NULL){
		return(GLV_ERROR);
	}
	*glv_win = glv_window;

	if(listener != NULL){
		if(listener->beauty != 0){
			glv_window->beauty = 1;
		}
		glvWindow_setHandler_class(*glv_win,listener->class);
		((GLV_WINDOW_t*)glv_window)->attr |= listener->attr;
		glvWindow_setHandler_init(*glv_win,listener->init);
		glvWindow_setHandler_reshape(*glv_win,listener->reshape);
		glvWindow_setHandler_redraw(*glv_win,listener->redraw);
		glvWindow_setHandler_update(*glv_win,listener->update);
		glvWindow_setHandler_timer(*glv_win,listener->timer);
		glvWindow_setHandler_mousePointer(*glv_win,listener->mousePointer);
		glvWindow_setHandler_mouseButton(*glv_win,listener->mouseButton);
		glvWindow_setHandler_mouseAxis(*glv_win,listener->mouseAxis);
		glvWindow_setHandler_gesture(*glv_win,listener->gesture);
		glvWindow_setHandler_cursor(*glv_win,listener->cursor);
		glvWindow_setHandler_userMsg(*glv_win,listener->userMsg);
		glvWindow_setHandler_endDraw(*glv_win,listener->endDraw);
		glvWindow_setHandler_terminate(*glv_win,listener->terminate);
	}

	return(GLV_OK);
}

glvWindow glvCreateThreadWindow(glvWindow parent,const struct glv_window_listener *listener,char *name,int x, int y, int width, int height,int attr,glvInstanceId *id)
{
	glvWindow glv_win;
	GLV_WINDOW_t *glv_window;
	glvDisplay glv_dpy = glv_getDisplay(parent);
	int rc;

	glvAllocWindowResource(glv_dpy,&glv_win,name,listener);
	glv_window = (GLV_WINDOW_t*)glv_win;
	rc = _glvCreateWindow(glv_window,name,GLV_TYPE_THREAD_WINDOW,NULL, x, y, width, height,parent,attr);
	glvCreateThreadSurfaceView(glv_window);

	if(id != NULL){
		*id = glv_window->instance.Id;
	}
	return(glv_window);
}

glvWindow glvCreateChildWindow(glvWindow parent,const struct glv_window_listener *listener,char *name,int x, int y, int width, int height,int attr,glvInstanceId *id)
{
	glvWindow glv_win;
	GLV_WINDOW_t *glv_window;
	glvDisplay glv_dpy = glv_getDisplay(parent);
	int rc;

	glvAllocWindowResource(glv_dpy,&glv_win,name,listener);
	glv_window = (GLV_WINDOW_t*)glv_win;
	rc = _glvCreateWindow(glv_window,name,GLV_TYPE_CHILD_WINDOW,NULL, x, y, width, height,parent,attr);
	_glvOnInit_for_childWindow((glvWindow)glv_window,glv_window->width,glv_window->height);

	if(id != NULL){
		*id = glv_window->instance.Id;
	}
	return(glv_window);
}

glvWindow glvCreateWindow(glvWindow parent,const struct glv_window_listener *listener,char *name,int x, int y, int width, int height,int attr,glvInstanceId *id)
{
	glvWindow glv_win;
	GLV_WINDOW_t *glv_window;
	GLV_WINDOW_t *parent_window = (GLV_WINDOW_t*)parent;
	glvDisplay glv_dpy = glv_getDisplay(parent);
	int rc;

	glvAllocWindowResource(glv_dpy,&glv_win,name,listener);
	glv_window = (GLV_WINDOW_t*)glv_win;

	if(parent_window->windowType == GLV_TYPE_THREAD_FRAME){
		rc = _glvCreateWindow(glv_window,name,GLV_TYPE_THREAD_WINDOW,NULL, x, y, width, height,parent,attr);
		glvCreateThreadSurfaceView(glv_window);
	}else{
		rc = _glvCreateWindow(glv_window,name,GLV_TYPE_CHILD_WINDOW,NULL, x, y, width, height,parent,attr);
		_glvOnInit_for_childWindow((glvWindow)glv_window,glv_window->width,glv_window->height);
	}

	if(id != NULL){
		*id = glv_window->instance.Id;
	}
	return(glv_window);
}

void glvDestroyWindow(glvWindow *glv_win)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)*glv_win;

	if(glv_window == NULL){
		return;
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return;
	}

	{
		// 自分の子供をすべて削除する
		struct _glv_window *tmp;
		struct _glv_window *glv_all_window;
		struct _glv_display *glv_dpy;
		int loop=1;
		glv_dpy = glv_window->glv_dpy;
		pthread_mutex_lock(&glv_dpy->display_mutex);							// display

		// wl_list_for_each_safe 内で自分以外を削除するとおかしくなるので、1件削除後もう一度入り直すようにする
		while(loop){
			loop = 0;
			wl_list_for_each_safe(glv_all_window, tmp, &glv_window->glv_dpy->window_list, link){
				if(glv_all_window->instance.Id != glv_window->instance.Id){
					if(glv_all_window->parent != NULL){
						if(glv_all_window->parent->instance.Id == glv_window->instance.Id){
							glvDestroyWindow((glvWindow*)&glv_all_window);
							loop = 1;
							break;
						}
					}
				}
			}
		}
		pthread_mutex_unlock(&glv_dpy->display_mutex);							// display
	}

	glvTerminateThreadSurfaceView(*glv_win);

	pthread_mutex_lock(&glv_window->glv_dpy->display_mutex);			// display
	_glvDestroyWindow(glv_window);
	pthread_mutex_unlock(&glv_window->glv_dpy->display_mutex);			// display

	glv_window->instance.oneself = NULL;

	if(glv_window->ctx.endReason == GLV_END_REASON__EXTERNAL){
		pthread_mutex_destroy(&glv_window->window_mutex);
		pthread_mutex_destroy(&glv_window->serialize_mutex);
		free(glv_window);
	}
	*glv_win = NULL;
}

EGLNativeDisplayType glvGetNativeDisplay(glvDisplay glv_dpy)
{
	return (((GLV_DISPLAY_t*)glv_dpy)->native_dpy);
}

EGLDisplay glvGetEGLDisplay(glvDisplay glv_dpy)
{
	return ((EGLDisplay)((GLV_DISPLAY_t*)glv_dpy)->egl_dpy);
}

EGLConfig glvGetEGLConfig_normal(glvDisplay glv_dpy)
{
	return ((EGLDisplay)((GLV_DISPLAY_t*)glv_dpy)->egl_config_normal);
}

EGLConfig glvGetEGLConfig_beauty(glvDisplay glv_dpy)
{
	return ((EGLDisplay)((GLV_DISPLAY_t*)glv_dpy)->egl_config_beauty);
}

EGLint glvGetEGLVisualid(glvDisplay glv_dpy)
{
	return (((GLV_DISPLAY_t*)glv_dpy)->vid);
}

void *glvExecMsg(GLV_WINDOW_t *glv_window,pthread_msq_msg_t *rmsg)
{
	int event = rmsg->data[0];

	switch(event){
		case GLV_ON_INIT:
			/* 初期化 */
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_INIT(%d,%d)\n"GLV_DEBUG_END_COLOR,glv_window->name,(int)rmsg->data[4],(int)rmsg->data[5]);
			if(glv_window->eventFunc.init != NULL){
				int rc;
				rc = (glv_window->eventFunc.init)(glv_window,rmsg->data[4],rmsg->data[5]);
				if(rc != GLV_OK){
					fprintf(stderr,"glv_window->eventFunc.init error\n");
				}
			}
			break;		
		case GLV_ON_CONFIGURE:
			if(glv_window->configure_serial > (uint32_t)rmsg->data[6]){
				GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_CONFIGURE(%d,%d) sirial(%d > %d)\n"GLV_DEBUG_END_COLOR,glv_window->name,(int)rmsg->data[4],(int)rmsg->data[5],glv_window->configure_serial,(uint32_t)rmsg->data[6]);
				break;
			}
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_CONFIGURE(%d,%d)\n"GLV_DEBUG_END_COLOR,glv_window->name,(int)rmsg->data[4],(int)rmsg->data[5]);
			/* 描画サイズ変更 */
			_glvResizeWindow(glv_window,rmsg->data[2],rmsg->data[3],rmsg->data[4],rmsg->data[5]);
			//wl_egl_window_resize(glv_window->egl_window,rmsg->data[4],rmsg->data[5],0,0);
			if(glv_window->eventFunc.configure != NULL){
				int rc;
				//rc = (glv_window->eventFunc.reshape)(glv_window,rmsg->data[4],rmsg->data[3]);
				rc = (glv_window->eventFunc.configure)(glv_window,glv_window->frameInfo.inner_width,glv_window->frameInfo.inner_height);
				if(rc != GLV_OK){
					fprintf(stderr,"[%s] glv_window->eventFunc.configure error\n",glv_window->name);
				}
			}
			if(glv_window->eventFunc.reshape != NULL){
				int rc;
				rc = (glv_window->eventFunc.reshape)(glv_window,glv_window->width,glv_window->height);
				if(rc != GLV_OK){
					fprintf(stderr,"[%s] glv_window->eventFunc.reshape error\n",glv_window->name);
				}
			}
			_glv_sheet_with_wiget_reshape_cb(glv_window);
			break;
		case GLV_ON_RESHAPE:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_RESHAPE(%d,%d)\n"GLV_DEBUG_END_COLOR,glv_window->name,(int)rmsg->data[4],(int)rmsg->data[5]);
			/* 描画サイズ変更 */
			_glvResizeWindow(glv_window,rmsg->data[2],rmsg->data[3],rmsg->data[4],rmsg->data[5]);
			//wl_egl_window_resize(glv_window->egl_window,rmsg->data[4],rmsg->data[5],0,0);
			if(glv_window->eventFunc.reshape != NULL){
				int rc;
				rc = (glv_window->eventFunc.reshape)(glv_window,glv_window->width,glv_window->height);
				if(rc != GLV_OK){
					fprintf(stderr,"[%s] glv_window->eventFunc.reshape error\n",glv_window->name);
				}
			}
			_glv_sheet_with_wiget_reshape_cb(glv_window);
			break;
		case GLV_ON_REDRAW:
#ifdef GLV_TEST__THIN_OUT_DRAWING
			if(glv_window->draw_serial > (uint32_t)rmsg->data[6]){
				GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_REDRAW cancel sirial(%d > %d)\n"GLV_DEBUG_END_COLOR,glv_window->name,glv_window->draw_serial,(uint32_t)rmsg->data[6]);
				break;
			}
#endif /* GLV_TEST__THIN_OUT_DRAWING */
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_REDRAW   count = %d\n"GLV_DEBUG_END_COLOR,glv_window->name,glv_window->drawCount);
			/* 描画 */
			if(glv_window->eventFunc.redraw != NULL){
				int rc;
				glv_window->reqSwapBuffersFlag = 1;	// REDRAWは、必ず描画するので強制SwapBuffersを実行する
				rc = (glv_window->eventFunc.redraw)(glv_window,GLV_STAT_DRAW_REDRAW);
				if(rc != GLV_OK){
					fprintf(stderr,"[%s] glv_window->eventFunc.redraw error\n",glv_window->name);
				}
			}else
			if(glv_window->eventFunc.update != NULL){
				int rc;
				rc = (glv_window->eventFunc.update)(glv_window,GLV_STAT_DRAW_REDRAW);
				if(rc != GLV_OK){
					fprintf(stderr,"[%s] glv_window->eventFunc.update error\n",glv_window->name);
				}
			}
			_glv_sheet_with_wiget_redraw_cb(glv_window);
			break;
		case GLV_ON_UPDATE:
#ifdef GLV_TEST__THIN_OUT_DRAWING
			if(glv_window->draw_serial > (uint32_t)rmsg->data[6]){
				GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_UPDATE cancel sirial(%d > %d)\n"GLV_DEBUG_END_COLOR,glv_window->name,glv_window->draw_serial,(uint32_t)rmsg->data[6]);
				break;
			}
#endif /* GLV_TEST__THIN_OUT_DRAWING */
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_UPDATE   count = %d\n"GLV_DEBUG_END_COLOR,glv_window->name,glv_window->drawCount);
			/* 描画 */
			if(glv_window->eventFunc.update != NULL){
				int rc;
				rc = (glv_window->eventFunc.update)(glv_window,GLV_STAT_DRAW_UPDATE);
				if(rc != GLV_OK){
					fprintf(stderr,"[%s] glv_window->eventFunc.update error\n",glv_window->name);
				}
			}else
			if(glv_window->eventFunc.redraw != NULL){
				int rc;
				glv_window->reqSwapBuffersFlag = 1;	// REDRAWは、必ず描画するので強制SwapBuffersを実行する
				rc = (glv_window->eventFunc.redraw)(glv_window,GLV_STAT_DRAW_REDRAW);
				if(rc != GLV_OK){
					fprintf(stderr,"[%s] glv_window->eventFunc.redraw error\n",glv_window->name);
				}
			}
			_glv_sheet_with_wiget_update_cb(glv_window);
			break;
		case GLV_ON_TIMER:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_TIMER\n"GLV_DEBUG_END_COLOR,glv_window->name);
			if(glvCheckTimer(glv_window,rmsg->data[3],rmsg->data[4]) == GLV_OK){
				//fprintf(stdout,"[%s] GLV_ON_TIMER id = %d OK\n",rmsg->data[3],glv_window->name);
				if(glv_window->eventFunc.timer != NULL){
					int rc;
					rc = (glv_window->eventFunc.timer)(glv_window,rmsg->data[2],rmsg->data[3]);
					if(rc != GLV_OK){
						fprintf(stderr,"[%s] glv_window->eventFunc.timer error\n",glv_window->name);
					}
				}
			}else{
				//fprintf(stderr,"[%s] GLV_ON_TIMER id = %d IGNORE\n",rmsg->data[3],glv_window->name);
			}
			break;
		case GLV_ON_GESTURE:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_GESTURE\n"GLV_DEBUG_END_COLOR,glv_window->name);
			if(glv_window->eventFunc.gesture != NULL){
				int rc;
				rc = (glv_window->eventFunc.gesture)(glv_window,
						rmsg->data[2],rmsg->data[3],rmsg->data[4],rmsg->data[5],rmsg->data[6],rmsg->data[7],rmsg->data[8]);
				if(rc != GLV_OK){
					fprintf(stderr,"[%s] glv_window->eventFunc.gesture error\n",glv_window->name);
				}
			}
			break;
		case GLV_ON_ACTIVATE:
			// GLV_ON_ACTIVATE: unimplemented msg
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_ACTIVATE\n"GLV_DEBUG_END_COLOR,glv_window->name);
			break;
		case GLV_ON_TERMINATE:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_TERMINATE %ld\n"GLV_DEBUG_END_COLOR,glv_window->name,glv_window->instance.Id);
			break;
		case GLV_ON_USER_MSG:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_USER_MSG\n"GLV_DEBUG_END_COLOR,glv_window->name);
			{
				int kind;
				void *data;
				kind = (int)rmsg->data[2];
				data = (void *)rmsg->data[3];
				if(glv_window->eventFunc.userMsg != NULL){
					int rc;
					rc = (glv_window->eventFunc.userMsg)(glv_window,kind,data);
					if(rc != GLV_OK){
						fprintf(stderr,"[%s] glv_window->eventFunc.userMsg error\n",glv_window->name);
					}
				}
				_glv_sheet_userMsg_cb(glv_window,kind,data);
				//if(data != NULL) free(data);
				//_usr_msg_ok_receive_count++;
			}
			break;
		case GLV_ON_MOUSE_POINTER:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_MOUSE_POINTER\n"GLV_DEBUG_END_COLOR,glv_window->name);
			//printf("GLV_ON_MOUSE_POINTER\n");
			if((rmsg->data[2] == 0) && (rmsg->data[3] == 0)){
				if(glv_window->eventFunc.mousePointer != NULL){
					int rc;
					rc = (glv_window->eventFunc.mousePointer)(glv_window,rmsg->data[4],rmsg->data[5],rmsg->data[6],rmsg->data[7],rmsg->data[8]);
					if(rc != GLV_OK){
						fprintf(stderr,"[%s] glv_window->eventFunc.mousePointer error\n",glv_window->name);
					}
				}
			}else if(rmsg->data[3] == 0){
				_glv_sheet_mousePointer_cb(glv_window,rmsg->data[2],rmsg->data[4],rmsg->data[5],rmsg->data[6],rmsg->data[7],rmsg->data[8]);
			}else{
				_glv_wiget_mousePointer_cb(glv_window,rmsg->data[2],rmsg->data[3],rmsg->data[4],rmsg->data[5],rmsg->data[6],rmsg->data[7],rmsg->data[8]);	
			}
			break;
		case GLV_ON_MOUSE_BUTTON:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_MOUSE_BUTTON\n"GLV_DEBUG_END_COLOR,glv_window->name);
			//printf("GLV_ON_MOUSE_BUTTON\n");
			if((rmsg->data[2] == 0) && (rmsg->data[3] == 0)){
				if(glv_window->eventFunc.mouseButton != NULL){
					int rc;
					rc = (glv_window->eventFunc.mouseButton)(glv_window,rmsg->data[4],rmsg->data[5],rmsg->data[6],rmsg->data[7],rmsg->data[8]);
					if(rc != GLV_OK){
						fprintf(stderr,"[%s] glv_window->eventFunc.mouseButton error\n",glv_window->name);
					}
				}
			}else if(rmsg->data[3] == 0){
				_glv_sheet_mouseButton_cb(glv_window,rmsg->data[2],rmsg->data[4],rmsg->data[5],rmsg->data[6],rmsg->data[7],rmsg->data[8]);
			}else{
				_glv_wiget_mouseButton_cb(glv_window,rmsg->data[2],rmsg->data[3],rmsg->data[4],rmsg->data[5],rmsg->data[6],rmsg->data[7],rmsg->data[8]);	
			}
			break;
		case GLV_ON_MOUSE_AXIS:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_MOUSE_AXIS\n"GLV_DEBUG_END_COLOR,glv_window->name);
			//printf("GLV_ON_MOUSE_AXIS\n");
			if((rmsg->data[2] == 0) && (rmsg->data[3] == 0)){
				if(glv_window->eventFunc.mouseAxis != NULL){
					int rc;
					rc = (glv_window->eventFunc.mouseAxis)(glv_window,rmsg->data[4],rmsg->data[5],rmsg->data[6]);
					if(rc != GLV_OK){
						fprintf(stderr,"[%s] glv_window->eventFunc.mouseAxis error\n",glv_window->name);
					}
				}
			}else if(rmsg->data[3] == 0){
				_glv_sheet_mouseAxis_cb(glv_window,rmsg->data[2],rmsg->data[4],rmsg->data[5],rmsg->data[6]);
			}else{
				_glv_wiget_mouseAxis_cb(glv_window,rmsg->data[2],rmsg->data[3],rmsg->data[4],rmsg->data[5],rmsg->data[6]);	
			}
			break;
		case GLV_ON_ACTION:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_ACTION\n"GLV_DEBUG_END_COLOR,glv_window->name);
			//printf("GLV_ON_ACTION\n");
			if((rmsg->data[2] != 0) && (rmsg->data[3] == 0)){
				_glv_sheet_action_cb(glv_window,rmsg->data[2],rmsg->data[3],rmsg->data[4],rmsg->data[5]);
			}else if((rmsg->data[2] == 0) && (rmsg->data[3] == 0)){
				if(glv_window->eventFunc.action != NULL){
					int rc;
					rc = (glv_window->eventFunc.action)(glv_window,rmsg->data[4],rmsg->data[5]);
					if(rc != GLV_OK){
						fprintf(stderr,"[%s] glv_window->eventFunc.action error\n",glv_window->name);
					}
				}
			}else{

			}
			break;
		case GLV_ON_KEY:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_KEY\n"GLV_DEBUG_END_COLOR,glv_window->name);
			//printf("GLV_ON_KEY\n");
			if(glv_window->eventFunc.key != NULL){
				int rc;
				rc = (glv_window->eventFunc.key)(glv_window,rmsg->data[2],rmsg->data[3],rmsg->data[4]);
				if(rc != GLV_OK){
					fprintf(stderr,"[%s] glv_window->eventFunc.key error\n",glv_window->name);
				}
			}
			break;
		case GLV_ON_KEY_INPUT:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_KEY_INPUT\n"GLV_DEBUG_END_COLOR,glv_window->name);
			//printf("[%s] GLV_ON_KEY_INPUT\n",glv_window->name);
			{
				_glv_wiget_key_input_cb(glv_window,rmsg->data[2],rmsg->data[3],rmsg->data[4],rmsg->data[5],rmsg->data[6],(int *)rmsg->data[7],(uint8_t*)rmsg->data[8],rmsg->data[9]);
				//if(rmsg->data[7] != 0) free((void *)rmsg->data[7]);
				//if(rmsg->data[8] != 0) free((void *)rmsg->data[8]);
			}
			break;
		case GLV_ON_FOCUS:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_FOCUS\n"GLV_DEBUG_END_COLOR,glv_window->name);
			//printf("[%s] GLV_ON_FOCUS\n",glv_window->name);
			{
				_glv_wiget_focus_cb(glv_window,rmsg->data[2],rmsg->data[3],rmsg->data[4],rmsg->data[5],rmsg->data[6],rmsg->data[7]);
			}
			break;
		case GLV_ON_END_DRAW:
			GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"[%s] GLV_ON_END_DRAW\n"GLV_DEBUG_END_COLOR,glv_window->name);
			//printf("[%s] GLV_ON_END_DRAW\n",glv_window->name);
			if(glv_window->eventFunc.endDraw != NULL){
				int rc;
				rc = (glv_window->eventFunc.endDraw)(glv_window,rmsg->data[2]);
				if(rc != GLV_OK){
					fprintf(stderr,"[%s] glv_window->eventFunc.endDraw error\n",glv_window->name);
				}
			}
			break;			
		default:
			break;
	}
	return NULL;
}

int glvMsgHandler(GLV_WINDOW_t *glv_window,pthread_msq_msg_t *rmsg)
{
	GLV_WINDOW_t *target_window;
	int	rc = 1;

	target_window = glv_window;

	if(glv_window->instance.Id != rmsg->data[1]){
		target_window = _glvGetWindowFromId(glv_window->glv_dpy,rmsg->data[1]);
		//printf("target_window %s\n",target_window->name);
		//printf("glvSurfaceViewMsgHandler:子供のウインドウの処理を受け付けた WindowId = %ld -> %ld\n",glv_window->instance.Id,rmsg->data[1]);
		if(target_window == NULL){
			if(rmsg->data[0] == GLV_ON_FOCUS){
				// GLV_ON_FOCUSはエラーとしない
			}else{
				printf("==========================================================================================\n");
				printf("glvMsgHandler:window is not found. msg = %ld , data[1-3] = %ld,%ld,%ld\n",rmsg->data[0],rmsg->data[1],rmsg->data[2],rmsg->data[3]);
				printf("==========================================================================================\n");
			}
			return(rc);
		}
	}

	if(target_window->instance.alive == GLV_INSTANCE_ALIVE){
		glvSelectDrawingWindow((glvWindow)target_window);
		target_window->reqSwapBuffersFlag = 0;

		glvExecMsg(target_window,rmsg);
		if(target_window->reqSwapBuffersFlag == 1){
			target_window->reqSwapBuffersFlag = 0;
			glvSwapBuffers(target_window);
		}
	}

	switch(rmsg->data[0]){
		case GLV_ON_REDRAW:
			target_window->reqSwapBuffersFlag = 1;	// REDRAWは、必ず描画するので強制SwapBuffersを実行する
			break;
		case GLV_ON_USER_MSG:
			if(rmsg->data[3] != 0) free((void *)rmsg->data[3]);
			_usr_msg_ok_receive_count++;
			break;
		case GLV_ON_KEY_INPUT:
			if(rmsg->data[7] != 0) free((void *)rmsg->data[7]);
			if(rmsg->data[8] != 0) free((void *)rmsg->data[8]);
			break;
		case GLV_ON_TERMINATE:
			rc = 0;
			break;
		default:
			break;
	}
	return(rc);
}

void *glvSurfaceViewMsgHandler(GLV_WINDOW_t *glv_window)
{
	int	loop = 1;
	int rc;
	pthread_msq_msg_t	rmsg = {};

	while (loop) {
		// メッセージ初期化
		memset(&rmsg, 0, sizeof(pthread_msq_msg_t));

		// メッセージ受信
		rc = pthread_msq_msg_receive(&glv_window->ctx.queue, &rmsg);
		if (PTHREAD_MSQ_OK != rc) {
			continue;
		}
		loop = glvMsgHandler(glv_window,&rmsg);
	}
	return(NULL);
}

int _glvWindowMsgHandler_dispatch(glvDisplay glv_dpy)
{
	GLV_DISPLAY_t	*display = (GLV_DISPLAY_t*)glv_dpy;
	GLV_WINDOW_t *glv_window = display->rootWindow;
	int	loop = 1;
	int rc;
	pthread_msq_msg_t	rmsg = {};

	while (loop) {
		// メッセージ初期化
		memset(&rmsg, 0, sizeof(pthread_msq_msg_t));

		// メッセージ受信
		rc = pthread_msq_msg_receive_try(&glv_window->ctx.queue, &rmsg);
		if(PTHREAD_MSQ_OK == rc) {
			loop = glvMsgHandler(glv_window,&rmsg);
		}else{
			loop = 0;
		}
	}
	return(0);
}


void *glvSurfaceViewProc(void *param)
{
	GLV_WINDOW_t *glv_window;
	EGLDisplay egl_dpy;
	EGLenum api;
	EGLContext egl_ctx;
	static int instanceCount=0;
	pthread_msq_msg_t	rmsg = {};

	glvGl_thread_safe_init();
	glvFont_thread_safe_init();

	glv_window = (GLV_WINDOW_t*)param;

	// threadId,runThreadの値が不定な状態とならない様に、thread作成側と、thread側の両方で設定する。
	glv_window->ctx.threadId = pthread_self();
	glv_window->ctx.runThread = 1;

	egl_dpy = glv_window->glv_dpy->egl_dpy;

	switch(glv_window->ctx.api){
		case GLV_OPENGL_ES1_API:
			api = 1;
			break;
		case GLV_OPENGL_ES2_API:
			api = 2;
			break;
		default:
			api = 2;
			break;
	}
	//// egl-contexts collect all state descriptions needed required for operation
	EGLint ctxattr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,	/* 1:opengles1.1 2:opengles2.0 */
		EGL_NONE
	};
	ctxattr[1] = api;
	egl_ctx = eglCreateContext(egl_dpy,glv_window->ctx.egl_config,EGL_NO_CONTEXT,ctxattr);
	if(egl_ctx == EGL_NO_CONTEXT ) {
	   fprintf(stderr,"glvSurfaceViewProc:Unable to create EGL context (eglError: %d)\n", eglGetError());
	   exit(-1);
	}
	glvGl_setEglContextInfo(egl_dpy,egl_ctx);

   if (!eglMakeCurrent(glvGl_GetEglDisplay(), glv_window->ctx.egl_surf, glv_window->ctx.egl_surf,glvGl_GetEglContext())) {
      fprintf(stderr,"glvSurfaceViewProc:Error: eglMakeCurrent() failed\n");
      exit(-1);
   }
   if(instanceCount == 0){
		//fprintf(stdout,"GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
		//fprintf(stdout,"GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
		//fprintf(stdout,"GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
		//fprintf(stdout,"GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }
   instanceCount++;

#ifdef GLV_OPENGL_ES2
	es1emu_Init();
#endif

	/* 初期化 */
	if(glv_window->eventFunc.init != NULL){
		int rc;
		rc = (glv_window->eventFunc.init)(glv_window,glv_window->width,glv_window->height);
		if(rc != GLV_OK){
			fprintf(stderr,"glv_window->eventFunc.init error\n");
		}
	}

	if(glv_window->glv_dpy->wl_dpy.ivi_application){
		// ivi アプリの場合は、surface の configureが呼び出されないので、ここで起動処理を実行する
		int rc;
		rc = (glv_window->eventFunc.start)(glv_window,glv_window->frameInfo.inner_width,glv_window->frameInfo.inner_height);
		if(rc != GLV_OK){
			fprintf(stderr,"glv_window->eventFunc.start error\n");
		}
	}

	//printf("sem_post(&glv_window->initSync); post [%s]\n",glv_window->name);
	// glvCreateThreadSurfaceViewとの待ち合わせ
	sem_post(&glv_window->initSync);

	glvSurfaceViewMsgHandler(glv_window);

	/* 終了 */
	if(glv_window->eventFunc.terminate != NULL){
		int rc;
		rc = (glv_window->eventFunc.terminate)(glv_window);
		if(rc != GLV_OK){
			fprintf(stderr,"glv_window->eventFunc.terminate error\n");
		}
	}

	pthread_msq_stop(&glv_window->ctx.queue);	// メッセージ受信を停止する

	// 受信済みの未処理メッセージの後処理
	// freeが必要なメッセージを処理する
	while(1){
		int rc;
		// メッセージ初期化
		memset(&rmsg, 0, sizeof(pthread_msq_msg_t));
		// メッセージ受信
		rc = pthread_msq_msg_receive_try(&glv_window->ctx.queue, &rmsg);
		if (PTHREAD_MSQ_OK != rc) {
			// メッセージ無し
			break;
		}
		switch(rmsg.data[0]){
			case GLV_ON_USER_MSG:
				if(rmsg.data[3] != 0) free((void *)rmsg.data[3]);
				_usr_msg_ng_receive_count++;
				break;
			case GLV_ON_KEY_INPUT:
				if(rmsg.data[7] != 0) free((void *)rmsg.data[7]);
				if(rmsg.data[8] != 0) free((void *)rmsg.data[8]);
				break;
			default:
				break;
		}
	}
	//printf("glvSurfaceViewProc:msg queue enpty %s\n",glv_window->name);

	pthread_msq_destroy(&glv_window->ctx.queue);
	glvSelectDrawingWindow(NULL);
	eglDestroyContext(egl_dpy, egl_ctx);

#ifdef GLV_OPENGL_ES2
	es1emu_Finish();
#endif

	if(glv_window->ctx.endReason == GLV_END_REASON__INTERNAL){
		pthread_mutex_destroy(&glv_window->window_mutex);
		pthread_mutex_destroy(&glv_window->serialize_mutex);
		free(glv_window);
	}
	return(NULL);
}

glvWindow glvCreateThreadSurfaceView(glvWindow glv_win)
{
	int			pret = 0;
	int			api;
	pthread_t 	threadId;
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t *)glv_win;
	pthread_msq_id_t queue = PTHREAD_MSQ_ID_INITIALIZER;

	if(!glv_window){
		return(NULL);
	}

#ifdef GLV_OPENGL_ES2
   api = GLV_OPENGL_ES2_API;
#else
   api = GLV_OPENGL_ES1_API;
#endif

	glv_window->ctx.api = api;

	memcpy(&glv_window->ctx.queue,&queue,sizeof(pthread_msq_id_t));

	// メッセージキュー生成
	if (0 != pthread_msq_create(&glv_window->ctx.queue, 100)) {
		fprintf(stderr,"glvSurfaceViewProc:Error: pthread_msq_create() failed\n");
		exit(-1);
	}
	// スレッド生成
	pret = pthread_create(&threadId, NULL, glvSurfaceViewProc, (void *)glv_window);

	if (0 != pret) {
		return(NULL);
	}

	if(glv_window->windowType == GLV_TYPE_THREAD_FRAME){
		pthread_setname_np(threadId,"glv_frame");
	}else{
		pthread_setname_np(threadId,"glv_window");
	}

	//printf("sem_wait(&glv_window->initSync); wait [%s]\n",glv_window->name);
	// glvSurfaceViewProcの起動を待つ
	sem_wait(&glv_window->initSync);
	//printf("sem_wait(&glv_window->initSync); sync [%s]\n",glv_window->name);

	// threadId,runThreadの値が不定な状態とならない様に、thread作成側と、thread側の両方で設定する。
	glv_window->ctx.threadId = threadId;
	glv_window->ctx.runThread = 1;

	return (glv_window);
}

int glvWindow_setTitle(glvWindow glv_win,const char *title)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	int instanceType;
	int	rc;

	if(glv_window == NULL){
		return(GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return(GLV_ERROR);
	}

	instanceType = glv_getInstanceType(glv_window);
	if(instanceType != GLV_INSTANCE_TYPE_WINDOW){
		return(GLV_ERROR);
	}

	if(glv_window->windowType != GLV_TYPE_THREAD_FRAME){
		return(GLV_ERROR);
	}
	pthread_mutex_lock(&glv_window->window_mutex);			// window
	if(glv_window->title != NULL){
		free(glv_window->title);
		glv_window->title = NULL;
	}
	if(title != NULL){
		glv_window->title = strdup(title);
	}
	if(glv_window->flag_surface_configure != 0){
		glvOnReDraw(glv_window);
	}
	pthread_mutex_unlock(&glv_window->window_mutex);		// window
	return(GLV_OK);
}

int glvWindow_setInnerSize(glvWindow glv_win,int width, int height)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	int instanceType;
	int	rc;

	if(glv_window == NULL){
		return(GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return(GLV_ERROR);
	}

	instanceType = glv_getInstanceType(glv_window);
	if(instanceType != GLV_INSTANCE_TYPE_WINDOW){
		return(GLV_ERROR);
	}

	pthread_mutex_lock(&glv_window->serialize_mutex);				// window serialize_mutex
	if(glv_window->windowType == GLV_TYPE_THREAD_FRAME){
		//printf("glvWindow_setInnerSize:flag_toplevel_configure = %d\n",glv_window->flag_toplevel_configure);
		if(glv_window->flag_toplevel_configure == 0){
			glv_window->req_toplevel_resize = 1;
			// 内部の描画エリア
			glv_window->req_toplevel_inner_width  = width;
			glv_window->req_toplevel_inner_height = height;
			pthread_mutex_unlock(&glv_window->serialize_mutex);		// window serialize_mutex
			return(GLV_OK);
		}else{
			// 影を除いたエリア
			width  += (glv_window->frameInfo.left_size + glv_window->frameInfo.right_size  - glv_window->frameInfo.left_shadow_size - glv_window->frameInfo.right_shadow_size);
			height += (glv_window->frameInfo.top_size  + glv_window->frameInfo.bottom_size - glv_window->frameInfo.top_shadow_size  - glv_window->frameInfo.bottom_shadow_size);			
		}
	}
	pthread_mutex_unlock(&glv_window->serialize_mutex);				// window serialize_mutex

	rc = glvOnReShape(glv_win,0,0,width,height);

	return(rc);
}

int _glvOnInit_for_childWindow(glvWindow glv_win,int width, int height)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	smsg.data[0] = GLV_ON_INIT;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = 0;
	smsg.data[3] = 0;
	smsg.data[4] = width;
	smsg.data[5] = height;
	GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"_glvOnInit_for_childWindow  INIT %d,%d\n"GLV_DEBUG_END_COLOR,width,height);

	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int _glvOnConfigure(glvWindow glv_win,int width, int height)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	glv_window->configure_serial++;

	smsg.data[0] = GLV_ON_CONFIGURE;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = 0;
	smsg.data[3] = 0;
	smsg.data[4] = width;
	smsg.data[5] = height;
	smsg.data[6] = glv_window->configure_serial;
	GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"_glvOnConfigure  RESHAPE %d,%d\n"GLV_DEBUG_END_COLOR,width,height);

	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int glvOnReShape(glvWindow glv_win,int x, int y,int width, int height)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	if(x == -1){
		x = glv_window->x;
	}
	if(y == -1){
		y = glv_window->y;
	}
	if(width == -1){
		width = glv_window->width;
	}
	if(height == -1){
		height = glv_window->height;
	}

	smsg.data[0] = GLV_ON_RESHAPE;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = x;
	smsg.data[3] = y;
	smsg.data[4] = width;
	smsg.data[5] = height;
	GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"glvOnReShape  RESHAPE %d,%d\n"GLV_DEBUG_END_COLOR,width,height);

	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int glvOnReDraw(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	glv_window->draw_serial++;

	smsg.data[0] = GLV_ON_REDRAW;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = 0;
	smsg.data[3] = 0;
	smsg.data[6] = glv_window->draw_serial;

	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"glvOnReDraw \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int glvOnUpdate(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	glv_window->draw_serial++;

	smsg.data[0] = GLV_ON_UPDATE;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = 0;
	smsg.data[3] = 0;
	smsg.data[6] = glv_window->draw_serial;

	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"glvOnUpdate \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int glvOnGesture(glvWindow glv_win,int eventType,int x,int y,int distance_x,int distance_y,int velocity_x,int velocity_y)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	smsg.data[0] = GLV_ON_GESTURE;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = eventType;
	smsg.data[3] = x;
	smsg.data[4] = y;
	smsg.data[5] = distance_x;
	smsg.data[6] = distance_y;
	smsg.data[7] = velocity_x;
	smsg.data[8] = velocity_y;
	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"glvOnGesture \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int glvOnActivate(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	smsg.data[0] = GLV_ON_ACTIVATE;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = 0;
	smsg.data[3] = 0;
	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"GLV_ON_ACTIVE \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int glvOnUserMsg(glvWindow glv_win,int kind,void *data,size_t size)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;
	void *memory=NULL;
	int rc;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		_usr_msg_ng_send_count++;
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	if(size > 0){
		memory = malloc(size);
		if(memory == NULL){
			_usr_msg_ng_send_count++;
			return(GLV_ERROR);
		}
		memcpy(memory,data,size);
	}

	smsg.data[0] = GLV_ON_USER_MSG;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = kind;
	smsg.data[3] = (size_t)memory;
	smsg.data[4] = 0;
	smsg.data[5] = 0;
	smsg.data[6] = 0;
	smsg.data[7] = 0;

	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"glvOnUserMsg \n"GLV_DEBUG_END_COLOR);
	//printf("glvOnUserMsg\n");
	rc = pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	if(rc == PTHREAD_MSQ_ERROR){
		//printf("glvOnUserMsg:pthread_msq_msg_send error\n");
		if(memory != NULL) free(memory);
		_usr_msg_ng_send_count++;
		return(GLV_ERROR);
	}
	_usr_msg_ok_send_count++;
	return (GLV_OK);
}

int _glvOnMousePointer(void *glv_instance,int type,glvTime time,int x,int y,int pointer_left_stat)
{
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t *glv_sheet;
	GLV_WIGET_t *glv_wiget;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;
	glvInstanceId	sheetId = 0;
	glvInstanceId	wigetId = 0;

	int instanceType;

	if(glv_instance == NULL){
		return (GLV_ERROR);
	}
	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == GLV_INSTANCE_TYPE_WINDOW){
		glv_wiget = NULL;
		glv_sheet = NULL;
		glv_window = glv_instance;
	}else if(instanceType == GLV_INSTANCE_TYPE_SHEET){
		glv_wiget = NULL;
		glv_sheet = glv_instance;
		glv_window = glv_sheet->glv_window;
	}else if(instanceType == GLV_INSTANCE_TYPE_WIGET){
		glv_wiget = glv_instance;
		glv_sheet = glv_wiget->glv_sheet;
		glv_window = glv_sheet->glv_window;
	}else{
		printf("_glvOnMousePointer:bad instance type %08x\n",instanceType);
		return(GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}
	if(glv_sheet != NULL){
		sheetId = glv_sheet->instance.Id;
	}
	if(glv_wiget != NULL){
		wigetId = glv_wiget->instance.Id;
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	smsg.data[0] = GLV_ON_MOUSE_POINTER;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = sheetId;
	smsg.data[3] = wigetId;
	smsg.data[4] = type;
	smsg.data[5] = time;
	smsg.data[6] = x;
	smsg.data[7] = y;
	smsg.data[8] = pointer_left_stat;
	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"GLV_ON_MOUSE_POINTER \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int _glvOnMouseButton(void *glv_instance,int type,glvTime time,int x,int y,int pointer_stat)
{
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t *glv_sheet;
	GLV_WIGET_t *glv_wiget;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;
	glvInstanceId	sheetId = 0;
	glvInstanceId	wigetId = 0;

	int instanceType;

	if(glv_instance == NULL){
		return (GLV_ERROR);
	}
	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == GLV_INSTANCE_TYPE_WINDOW){
		glv_wiget = NULL;
		glv_sheet = NULL;
		glv_window = glv_instance;
	}else if(instanceType == GLV_INSTANCE_TYPE_SHEET){
		glv_wiget = NULL;
		glv_sheet = glv_instance;
		glv_window = glv_sheet->glv_window;
	}else if(instanceType == GLV_INSTANCE_TYPE_WIGET){
		glv_wiget = glv_instance;
		glv_sheet = glv_wiget->glv_sheet;
		glv_window = glv_sheet->glv_window;
	}else{
		printf("_glvOnMouseButton:bad instance type %08x\n",instanceType);
		return(GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_sheet != NULL){
		sheetId = glv_sheet->instance.Id;
	}
	if(glv_wiget != NULL){
		wigetId = glv_wiget->instance.Id;
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	smsg.data[0] = GLV_ON_MOUSE_BUTTON;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = sheetId;
	smsg.data[3] = wigetId;
	smsg.data[4] = type;
	smsg.data[5] = time;
	smsg.data[6] = x;
	smsg.data[7] = y;
	smsg.data[8] = pointer_stat;
	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"GLV_ON_MOUSE_BUTTON \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int _glvOnMouseAxis(void *glv_instance,int type,glvTime time,int value)
{
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t *glv_sheet;
	GLV_WIGET_t *glv_wiget;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;
	glvInstanceId	sheetId = 0;
	glvInstanceId	wigetId = 0;

	int instanceType;

	if(glv_instance == NULL){
		return (GLV_ERROR);
	}
	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == GLV_INSTANCE_TYPE_WINDOW){
		glv_wiget = NULL;
		glv_sheet = NULL;
		glv_window = glv_instance;
	}else if(instanceType == GLV_INSTANCE_TYPE_SHEET){
		glv_wiget = NULL;
		glv_sheet = glv_instance;
		glv_window = glv_sheet->glv_window;
	}else if(instanceType == GLV_INSTANCE_TYPE_WIGET){
		glv_wiget = glv_instance;
		glv_sheet = glv_wiget->glv_sheet;
		glv_window = glv_sheet->glv_window;
	}else{
		printf("_glvOnMouseAxis:bad instance type %08x\n",instanceType);
		return(GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_sheet != NULL){
		sheetId = glv_sheet->instance.Id;
	}
	if(glv_wiget != NULL){
		wigetId = glv_wiget->instance.Id;
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	smsg.data[0] = GLV_ON_MOUSE_AXIS;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = sheetId;
	smsg.data[3] = wigetId;
	smsg.data[4] = type;
	smsg.data[5] = time;
	smsg.data[6] = value;
	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"GLV_ON_MOUSE_AXIS \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int glvOnAction(void *glv_instance,int action,glvInstanceId selectId)
{
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t *glv_sheet;
	GLV_WIGET_t *glv_wiget;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;
	glvInstanceId	sheetId = 0;
	glvInstanceId	wigetId = 0;

	int instanceType;

	if(glv_instance == NULL){
		return (GLV_ERROR);
	}
	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == GLV_INSTANCE_TYPE_WINDOW){
		glv_wiget = NULL;
		glv_sheet = NULL;
		glv_window = glv_instance;
	}else if(instanceType == GLV_INSTANCE_TYPE_SHEET){
		glv_wiget = NULL;
		glv_sheet = glv_instance;
		glv_window = glv_sheet->glv_window;
	}else if(instanceType == GLV_INSTANCE_TYPE_WIGET){
		glv_wiget = glv_instance;
		glv_sheet = glv_wiget->glv_sheet;
		glv_window = glv_sheet->glv_window;
	}else{
		printf("glvOnAction:bad instance type %08x\n",instanceType);
		return(GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}
	if(glv_sheet != NULL){
		sheetId = glv_sheet->instance.Id;
	}
	if(glv_wiget != NULL){
		wigetId = glv_wiget->instance.Id;
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	smsg.data[0] = GLV_ON_ACTION;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = sheetId;
	smsg.data[3] = wigetId;
	smsg.data[4] = action;
	smsg.data[5] = selectId;
	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"GLV_ON_ACTION \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int _glvOnKey(glvWindow glv_win,unsigned int key,unsigned int modifiers,unsigned int state)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	smsg.data[0] = GLV_ON_KEY;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = key;
	smsg.data[3] = modifiers;
	smsg.data[4] = state;
	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"GLV_ON_KEY \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int _glvOnTextInput(glvDisplay glv_dpy,int kind,int state,uint32_t kyesym,int *utf32,uint8_t *attr,int length)
{
	GLV_DISPLAY_t *glv_display = (GLV_DISPLAY_t*)glv_dpy;
	GLV_WINDOW_t *glv_window;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;
	glvInstanceId	windowId = 0;
	glvInstanceId	sheetId = 0;
	glvInstanceId	wigetId = 0;
	void *utf32_text=NULL;
	void *utf32_attr=NULL;
	int rc,size;

	windowId = glv_display->kb_input_windowId;
	sheetId  = glv_display->kb_input_sheetId;
	wigetId  = glv_display->kb_input_wigetId;

	glv_window = _glvGetWindowFromId(glv_display,windowId);

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	if(utf32 != NULL){
		size = length * sizeof(int);
		if(size > 0){
			utf32_text = malloc(size);
			if(utf32_text == NULL){
				return(GLV_ERROR);
			}
			memcpy(utf32_text,utf32,size);
		}
	}
	if(attr != NULL){
		size = length * sizeof(uint8_t);
		if(size > 0){
			utf32_attr = malloc(size);
			if(utf32_attr == NULL){
				if(utf32_text != NULL) free(utf32_text);
				return(GLV_ERROR);
			}
			memcpy(utf32_attr,attr,size);
		}
	}

	smsg.data[0] = GLV_ON_KEY_INPUT;
	smsg.data[1] = windowId;
	smsg.data[2] = sheetId;
	smsg.data[3] = wigetId;
	smsg.data[4] = kind;
	smsg.data[5] = state;
	smsg.data[6] = kyesym;
	smsg.data[7] = (size_t)utf32_text;
	smsg.data[8] = (size_t)utf32_attr;
	smsg.data[9] = length;

	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"_glvOnTextInput\n"GLV_DEBUG_END_COLOR);
	//printf("_glvOnTextInput\n");
	rc = pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	if(rc == PTHREAD_MSQ_ERROR){
		printf("_glvOnTextInput:pthread_msq_msg_send error\n");
		if(utf32_text != NULL) free(utf32_text);
		if(utf32_attr != NULL) free(utf32_attr);
		return(GLV_ERROR);
	}
	return (GLV_OK);
}

int _glvOnFocus(glvDisplay glv_dpy,int focus_stat,glvWiget in_Wiget)
{
	GLV_DISPLAY_t *glv_display = (GLV_DISPLAY_t*)glv_dpy;
	GLV_WIGET_t *glv_in_wiget = (GLV_WIGET_t*)in_Wiget;
	GLV_WINDOW_t *glv_window;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;
	glvInstanceId	windowId = 0;
	glvInstanceId	sheetId = 0;
	glvInstanceId	wigetId = 0;
	glvInstanceId	in_windowId = 0;
	glvInstanceId	in_sheetId = 0;
	glvInstanceId	in_wigetId = 0;

	int rc;

	windowId = glv_display->kb_input_windowId;
	sheetId  = glv_display->kb_input_sheetId;
	wigetId  = glv_display->kb_input_wigetId;

	glv_window = _glvGetWindowFromId(glv_display,windowId);

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	if(glv_in_wiget != NULL){
		in_windowId = glv_in_wiget->glv_sheet->glv_window->instance.Id;;
		in_sheetId = glv_in_wiget->glv_sheet->instance.Id;;
		in_wigetId = glv_in_wiget->instance.Id;
	}

	smsg.data[0] = GLV_ON_FOCUS;
	smsg.data[1] = windowId;
	smsg.data[2] = sheetId;
	smsg.data[3] = wigetId;
	smsg.data[4] = focus_stat;
	smsg.data[5] = in_windowId;
	smsg.data[6] = in_sheetId;
	smsg.data[7] = in_wigetId;

	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"_glvOnFocus\n"GLV_DEBUG_END_COLOR);
	//printf("_glvOnFocus\n");
	rc = pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	if(rc == PTHREAD_MSQ_ERROR){
		printf("_glvOnFocus:pthread_msq_msg_send error\n");
		return(GLV_ERROR);
	}
	return (GLV_OK);
}

int _glvOnEndDraw(glvWindow glv_win,glvTime time)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	GLV_WINDOW_t *teamLeader;
	pthread_msq_msg_t smsg;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->teamLeader == NULL){
		// 関数コール
		return (GLV_OK);
	}
	teamLeader = glv_window->teamLeader;

	smsg.data[0] = GLV_ON_END_DRAW;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = time;

	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"_glvOnEndDraw \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&teamLeader->ctx.queue,&smsg,0);
	return (GLV_OK);
}

int glvTerminateThreadSurfaceView(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window;
	pthread_msq_msg_t smsg;
	int rc;

	glv_window = (GLV_WINDOW_t*)glv_win;

	if(glv_window == NULL){
		return (GLV_ERROR);
	}

	if(glv_window->instance.alive != GLV_INSTANCE_ALIVE){
		return (GLV_ERROR);
	}

	if(glv_window->windowType == GLV_TYPE_CHILD_WINDOW){
		glv_window->ctx.endReason = GLV_END_REASON__EXTERNAL;
		return (GLV_OK);
	}

	if(glv_window->ctx.runThread != 1){
		printf("glvTerminateThreadSurfaceView: thread is not runninng. %s\n",glv_window->name);
		return (GLV_ERROR);
	}
	smsg.data[0] = GLV_ON_TERMINATE;
	smsg.data[1] = glv_window->instance.Id;
	smsg.data[2] = 0;
	smsg.data[3] = 0;
	//GLV_IF_DEBUG_MSG printf(GLV_DEBUG_MSG_COLOR"glvTerminateThreadSurfaceView \n"GLV_DEBUG_END_COLOR);
	pthread_msq_msg_send(&glv_window->ctx.queue,&smsg,0);

	if(pthread_equal(glv_window->ctx.threadId,pthread_self())){
		// 自分自身で終了させようとしている
		glv_window->ctx.endReason = GLV_END_REASON__INTERNAL;
	}else{
		glv_window->ctx.endReason = GLV_END_REASON__EXTERNAL;
		rc = pthread_join(glv_window->ctx.threadId,NULL);
		if(rc != 0){
			printf("glvTerminateThreadSurfaceView:pthread_join error %d\n",rc);
		}
	}

	glv_window->ctx.runThread = 0;

	return (GLV_OK);
}

int glvCheckTimer(glvWindow glv_win,int id,int count)
{
	GLV_WINDOW_t *glv_window;
	glv_window = (GLV_WINDOW_t*)glv_win;
	int rc;

	rc = pthreadCheckTimer(glv_window->ctx.threadId,id,count);
	return(rc);
}

void glvInitializeTimer(void)
{
	pthreadInitializeTimer();
}

void glvTerminateTimer(void)
{
	pthreadTerminateTimer();
}

int glvCreate_mTimer(glvWindow glv_win,int group,int id,int type,int mTime)
{
	GLV_WINDOW_t *glv_window;
	glv_window = (GLV_WINDOW_t*)glv_win;
	int rc;

	rc = pthreadCreate_mTimer(glv_window->ctx.threadId,&glv_window->ctx.queue,GLV_ON_TIMER,glv_window->instance.Id,group,id,type,mTime);

	return(rc);
}

int glvCreate_uTimer(glvWindow glv_win,int group,int id,int type,int64_t tv_sec,int64_t tv_nsec)
{
	GLV_WINDOW_t *glv_window;
	glv_window = (GLV_WINDOW_t*)glv_win;
	struct timespec reqWaitTime;
	int rc;

	reqWaitTime.tv_sec  = tv_sec;
	reqWaitTime.tv_nsec = tv_nsec;

	rc = pthreadCreate_uTimer(glv_window->ctx.threadId,&glv_window->ctx.queue,GLV_ON_TIMER,glv_window->instance.Id,group,id,type,&reqWaitTime);

	return(rc);
}

int glvStartTimer(glvWindow glv_win,int id)
{
	GLV_WINDOW_t *glv_window;
	glv_window = (GLV_WINDOW_t*)glv_win;
	int rc;
	rc = pthreadStartTimer(glv_window->ctx.threadId,id);
	return(rc);
}

int glvStopTimer(glvWindow glv_win,int id)
{
	GLV_WINDOW_t *glv_window;
	glv_window = (GLV_WINDOW_t*)glv_win;
	int rc;
	rc = pthreadStopTimer(glv_window->ctx.threadId,id);
	return(rc);
}

int glvReqSwapBuffers(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;

	glv_window->reqSwapBuffersFlag = 1;
	return(GLV_OK);
}

int glvSelectDrawingWindow(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	EGLBoolean	rc;

	if(glv_window != NULL){
		rc = eglMakeCurrent(glvGl_GetEglDisplay(), glv_window->ctx.egl_surf, glv_window->ctx.egl_surf,glvGl_GetEglContext());
		if(rc == 0) {
    		fprintf(stderr,"glvSelectDrawingWindow:Error: eglMakeCurrent() failed %s\n",glv_window->name);
    		return(GLV_ERROR);
   		}
		glvWindow_setViewport(glv_win,glv_window->width,glv_window->height);
	}else{
		rc = eglMakeCurrent(glvGl_GetEglDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE,glvGl_GetEglContext());
		if(rc == 0) {
    		fprintf(stderr,"glvSelectDrawingWindow:Error: eglMakeCurrent() failed\n");
    		return(GLV_ERROR);
   		}
	}

   return(GLV_OK);
}

glvWindow glvGetWindowFromId(glvDisplay glv_dpy,glvInstanceId windowId)
{
	GLV_WINDOW_t *window;
	window = _glvGetWindowFromId((GLV_DISPLAY_t*)glv_dpy,windowId);
	return(window);
}

int glvWindow_isAliveWindow(void *glv_instance,glvInstanceId windowId)
{
	int instanceType;
	GLV_DISPLAY_t *glv_dpy=NULL;
	GLV_WINDOW_t *window;
	GLV_SHEET_t *sheet;
	GLV_WIGET_t *wiget;

	instanceType = glv_getInstanceType(glv_instance);

	switch(instanceType){
		case GLV_INSTANCE_TYPE_DISPLAY:
			glv_dpy = (GLV_DISPLAY_t*)glv_instance;
			break;
		case GLV_INSTANCE_TYPE_WINDOW:
			window = (GLV_WINDOW_t*)glv_instance;
			glv_dpy = window->glv_dpy;
			break;
		case GLV_INSTANCE_TYPE_SHEET:
			sheet = (GLV_SHEET_t*)glv_instance;
			glv_dpy = sheet->glv_dpy;
			break;
		case GLV_INSTANCE_TYPE_WIGET:
			wiget = (GLV_WIGET_t*)glv_instance;
			glv_dpy = wiget->glv_dpy;
			break;
		default:
			break;
	}

	if(glv_dpy == NULL){
		return(GLV_INSTANCE_DEAD);
	}

	window = _glvGetWindowFromId(glv_dpy,windowId);

	if(window == NULL){
		return(GLV_INSTANCE_DEAD);
	}
	return(window->instance.alive);
}

glvTime glvWindow_getLastTime(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	if(glv_win == NULL){
		return(0);
	}
	return(glv_window->wl_window.last_time);
}

char *glvWindow_getWindowName(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	if(glv_win == NULL){
		return(0);
	}
	return(glv_window->name);
}

#if 0
void glv_debug_send_test(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window = (GLV_WINDOW_t*)glv_win;
	int i;
	printf("send_test start\n");
	for(i=0;i<10000000;i++){
		if(glv_window != NULL){
			glvOnUserMsg(glv_window,101,0,0);
		}
	}
	printf("send_test end\n");

#if 0
	sleep(2);
#endif

	printf("_usr_msg_ok_receive_count = %8d\n",_usr_msg_ok_receive_count);
	printf("_usr_msg_ng_receive_count = %8d\n",_usr_msg_ng_receive_count);
	printf("_usr_msg_ok_send_count    = %8d\n",_usr_msg_ok_send_count);
	printf("_usr_msg_ng_send_count    = %8d\n",_usr_msg_ng_send_count);

	printf("_usr_msg_ok_ng_receive_count = %8d\n",_usr_msg_ok_receive_count+_usr_msg_ng_receive_count);
	printf("_usr_msg_ok_ng_send_count    = %8d\n",_usr_msg_ok_send_count+_usr_msg_ng_send_count);

	if(_usr_msg_ok_send_count == (_usr_msg_ok_receive_count+_usr_msg_ng_receive_count)){
		printf("send receive ok\n");
	}else{
		printf("send receive ng\n");
	}

	_usr_msg_ok_receive_count = 0;
	_usr_msg_ok_send_count    = 0;
	_usr_msg_ng_receive_count = 0;
	_usr_msg_ng_send_count    = 0;
}
#endif

void glvWindow_setViewport(glvWindow glv_win,int width,int height)
{
	glViewport(0, 0, width, height);

	// プロジェクション行列の設定
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// 座標体系設定
	glOrthof(0, width, height, 0, -1, 1);

	// モデルビュー行列の設定
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

int glv_isInsertMode(void *glv_instance)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	GLV_DISPLAY_t	*glv_display=NULL;
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t	*sheet;
	GLV_WIGET_t	*wiget;
	int ins_mode=1;
	switch(instance->instanceType){
		case GLV_INSTANCE_TYPE_DISPLAY:
			glv_display = (GLV_DISPLAY_t*)instance;
			ins_mode = glv_display->ins_mode;
			break;
		case GLV_INSTANCE_TYPE_WINDOW:
			glv_window = (GLV_WINDOW_t*)instance;
			glv_display = glv_window->glv_dpy;
			ins_mode = glv_display->ins_mode;
			break;
		case GLV_INSTANCE_TYPE_SHEET:
			sheet = (GLV_SHEET_t*)instance;
			glv_display = sheet->glv_dpy;
			ins_mode = glv_display->ins_mode;
			break;
		case GLV_INSTANCE_TYPE_WIGET:
			wiget = (GLV_WIGET_t*)instance;
			glv_display = wiget->glv_dpy;
			ins_mode = glv_display->ins_mode;
		default:
			break;
	}
	return(ins_mode);
}

int glv_isPullDownMenu(void *glv_instance)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t	*sheet;
	GLV_WIGET_t	*wiget;
	GLV_WINDOW_t *myFrame=NULL;
	int menu=0;
	switch(instance->instanceType){
		case GLV_INSTANCE_TYPE_WINDOW:
			glv_window = (GLV_WINDOW_t*)instance;
			myFrame = glv_window->myFrame;
			menu = myFrame->pullDownMenu;
			break;
		case GLV_INSTANCE_TYPE_SHEET:
			sheet = (GLV_SHEET_t*)instance;
			glv_window = sheet->glv_window;
			myFrame = glv_window->myFrame;
			menu = myFrame->pullDownMenu;
			break;
		case GLV_INSTANCE_TYPE_WIGET:
			wiget = (GLV_WIGET_t*)instance;
			sheet = wiget->glv_sheet;
			glv_window = sheet->glv_window;
			myFrame = glv_window->myFrame;
			menu = myFrame->pullDownMenu;
		default:
			break;
	}
	return(menu);
}

int glv_isCmdMenu(void *glv_instance)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t	*sheet;
	GLV_WIGET_t	*wiget;
	GLV_WINDOW_t *myFrame=NULL;
	int menu=0;
	switch(instance->instanceType){
		case GLV_INSTANCE_TYPE_WINDOW:
			glv_window = (GLV_WINDOW_t*)instance;
			myFrame = glv_window->myFrame;
			menu = myFrame->cmdMenu;
			break;
		case GLV_INSTANCE_TYPE_SHEET:
			sheet = (GLV_SHEET_t*)instance;
			glv_window = sheet->glv_window;
			myFrame = glv_window->myFrame;
			menu = myFrame->cmdMenu;
			break;
		case GLV_INSTANCE_TYPE_WIGET:
			wiget = (GLV_WIGET_t*)instance;
			sheet = wiget->glv_sheet;
			glv_window = sheet->glv_window;
			myFrame = glv_window->myFrame;
			menu = myFrame->cmdMenu;
		default:
			break;
	}
	return(menu);
}

glvDisplay glv_getDisplay(void *glv_instance)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	GLV_DISPLAY_t	*glv_display=NULL;
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t	*sheet;
	GLV_WIGET_t	*wiget;
	switch(instance->instanceType){
		case GLV_INSTANCE_TYPE_DISPLAY:
			glv_display = (GLV_DISPLAY_t*)instance;
			break;
		case GLV_INSTANCE_TYPE_WINDOW:
			glv_window = (GLV_WINDOW_t*)instance;
			glv_display = glv_window->glv_dpy;
			break;
		case GLV_INSTANCE_TYPE_SHEET:
			sheet = (GLV_SHEET_t*)instance;
			glv_window = sheet->glv_window;
			glv_display = glv_window->glv_dpy;
			break;
		case GLV_INSTANCE_TYPE_WIGET:
			wiget = (GLV_WIGET_t*)instance;
			sheet = wiget->glv_sheet;
			glv_window = sheet->glv_window;
			glv_display = glv_window->glv_dpy;
		default:
			break;
	}
	return(glv_display);
}

glvWindow glv_getWindow(void *glv_instance)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	GLV_WINDOW_t *glv_window=NULL;
	GLV_SHEET_t	*sheet;
	GLV_WIGET_t	*wiget;
	switch(instance->instanceType){
		case GLV_INSTANCE_TYPE_WINDOW:
			glv_window = (GLV_WINDOW_t*)instance;
			break;
		case GLV_INSTANCE_TYPE_SHEET:
			sheet = (GLV_SHEET_t*)instance;
			glv_window = sheet->glv_window;
			break;
		case GLV_INSTANCE_TYPE_WIGET:
			wiget = (GLV_WIGET_t*)instance;
			sheet = wiget->glv_sheet;
			glv_window = sheet->glv_window;
		default:
			break;
	}
	return(glv_window);
}

glvWindow glv_getFrameWindow(void *glv_instance)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t	*sheet;
	GLV_WIGET_t	*wiget;
	GLV_WINDOW_t *myFrame=NULL;
	switch(instance->instanceType){
		case GLV_INSTANCE_TYPE_WINDOW:
			glv_window = (GLV_WINDOW_t*)instance;
			myFrame = glv_window->myFrame;
			break;
		case GLV_INSTANCE_TYPE_SHEET:
			sheet = (GLV_SHEET_t*)instance;
			glv_window = sheet->glv_window;
			myFrame = glv_window->myFrame;
			break;
		case GLV_INSTANCE_TYPE_WIGET:
			wiget = (GLV_WIGET_t*)instance;
			sheet = wiget->glv_sheet;
			glv_window = sheet->glv_window;
			myFrame = glv_window->myFrame;
		default:
			break;
	}
	return(myFrame);
}

int glv_getWindowType(void *glv_instance)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t	*sheet;
	GLV_WIGET_t	*wiget;
	int windowType=0;
	switch(instance->instanceType){
		case GLV_INSTANCE_TYPE_WINDOW:
			glv_window = (GLV_WINDOW_t*)instance;
			windowType = glv_window->windowType;
			break;
		case GLV_INSTANCE_TYPE_SHEET:
			sheet = (GLV_SHEET_t*)instance;
			glv_window = sheet->glv_window;
			windowType = glv_window->windowType;
			break;
		case GLV_INSTANCE_TYPE_WIGET:
			wiget = (GLV_WIGET_t*)instance;
			sheet = wiget->glv_sheet;
			glv_window = sheet->glv_window;
			windowType = glv_window->windowType;
		default:
			break;
	}
	return(windowType);
}

int glv_getFrameInfo(void *glv_instance,GLV_FRAME_INFO_t *frameInfo)
{
	int rc = GLV_ERROR;
	_GLV_INSTANCE_t *instance = glv_instance;
	GLV_WINDOW_t *glv_window;
	GLV_SHEET_t	*sheet;
	GLV_WIGET_t	*wiget;
	switch(instance->instanceType){
		case GLV_INSTANCE_TYPE_WINDOW:
			glv_window = (GLV_WINDOW_t*)instance;
			memcpy(frameInfo,&glv_window->frameInfo,sizeof(GLV_FRAME_INFO_t));
			rc = GLV_OK;
			break;
		case GLV_INSTANCE_TYPE_SHEET:
			sheet = (GLV_SHEET_t*)instance;
			glv_window = sheet->glv_window;
			memcpy(frameInfo,&glv_window->frameInfo,sizeof(GLV_FRAME_INFO_t));
			rc = GLV_OK;
			break;
		case GLV_INSTANCE_TYPE_WIGET:
			wiget = (GLV_WIGET_t*)instance;
			sheet = wiget->glv_sheet;
			glv_window = sheet->glv_window;
			memcpy(frameInfo,&glv_window->frameInfo,sizeof(GLV_FRAME_INFO_t));
			rc = GLV_OK;
		default:
			break;
	}
	return(rc);
}

int glv_getInstanceType(void *glv_instance)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	switch(instance->instanceType){
		case GLV_INSTANCE_TYPE_DISPLAY:
		case GLV_INSTANCE_TYPE_WINDOW:
		case GLV_INSTANCE_TYPE_SHEET:
		case GLV_INSTANCE_TYPE_WIGET:
		case GLV_INSTANCE_TYPE_RESOURCE:
			return(instance->instanceType);
	}
	return(0);
}

int glv_menu_init(GLV_W_MENU_t *menu)
{
	memset(menu,0,sizeof(GLV_W_MENU_t));
	return(GLV_OK);
}

int glv_menu_free(GLV_W_MENU_t *menu)
{
	int n;
	for(n=0;n<GLV_W_MENU_LIST_MAX;n++){
		if(menu->item[n].text != NULL){
			free(menu->item[n].text);
		}
	}
	return(GLV_OK);
}

int glv_menu_getFunctionId(GLV_W_MENU_t *menu,int select)
{
	if((select < 0) && (select >= GLV_W_MENU_LIST_MAX)){
		return(0);
	}
	return(menu->item[select].functionId);
}

int glv_menu_getNext(GLV_W_MENU_t *menu,int select)
{
	if((select < 0) && (select >= GLV_W_MENU_LIST_MAX)){
		return(0);
	}
	return(menu->item[select].next);
}

int glv_menu_searchFunctionId(GLV_W_MENU_t *menu,int functionId)
{
	int n;
	for(n=0;n<GLV_W_MENU_LIST_MAX;n++){
		if(menu->item[n].functionId == functionId){
			return(n);
		}
	}
  return(0);
}

int glv_menu_setItem(GLV_W_MENU_t *menu,int n,char *text,int attr,int next,int functionId)
{
	if((n < 1) || (n > GLV_W_MENU_LIST_MAX)){
		return(GLV_ERROR);
	}
	--n;
	menu->item[n].length	= strlen(text);
	if(menu->item[n].text != NULL){
		free(menu->item[n].text);
	}
	menu->item[n].text			= strdup(text);
	menu->item[n].attrib		= attr;
	menu->item[n].next			= next;
	menu->item[n].functionId	= functionId;
	if(menu->num < (n+1)){
		menu->num = n + 1;
	}

  return(GLV_OK);
}

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
int glv_r_is_typeChar2typeNo(char type_char,int *type)
{
	int rc = 1;
	switch(type_char){
		case 'Z':
		case 'z':
			*type = GLV_R_VALUE_TYPE__SIZE;	// 8byte: unsigned long , size_t
			break;
		case 'L':
		case 'l':
			*type = GLV_R_VALUE_TYPE__INT64;	// 8byte: signed long
			break;
		case 'I':
		case 'i':
			*type = GLV_R_VALUE_TYPE__INT32;	// 4baye: signed int
			break;
		case 'U':
		case 'u':
			*type = GLV_R_VALUE_TYPE__UINT32;	// 4baye: unsigned int
			break;
		case 'C':
		case 'c':
			*type = GLV_R_VALUE_TYPE__COLOR;	// 8byte: function pointer
			break;		
		case 'S':
		case 's':
			*type = GLV_R_VALUE_TYPE__STRING;	// 8byte: string(UTF8)
			break;
		case 'P':
		case 'p':
			*type = GLV_R_VALUE_TYPE__POINTER;	// 8byte: data pointer
			break;
		case 'R':
		case 'r':
			*type = GLV_R_VALUE_TYPE__DOUBLE;	// 8byte: double
			break;
		case 'T':
		case 't':
			*type = GLV_R_VALUE_TYPE__FUNCTION;	// 8byte: function pointer
			break;
		default:
			rc = 0;
			*type = GLV_R_VALUE_TYPE__NOTHING;	// unknown type
			break;
	}
	return(rc);
}

int glv_r_set_value(struct _glv_r_value **link,void *instance,char *key,char *type_string,va_list args)
{
	struct _glv_r_value *fp;
	struct _glv_r_value **last;
	int index;
	int type;
	int findFlag=0;
	int rc;
	void *string;

	int length = strlen(type_string);
	if(length > _GLV_R_VALUE_MAX){
		rc = 1;
		if(length == (_GLV_R_VALUE_MAX+1)){
			int last_type;
			glv_r_is_typeChar2typeNo(type_string[length-1],&last_type);
			if(last_type == GLV_R_VALUE_TYPE__FUNCTION ){
				rc = 0;		
			}
		}
		if(rc == 1){
			printf("glv_r_set_value:too many arguments (%d). [%s] (\"%s\"). up to %d arguments can be set.\n",length,key,type_string,_GLV_R_VALUE_MAX);
			length = _GLV_R_VALUE_MAX;
		}
	}

	fp = *link;
	last = link;
	while(fp){
		if(strcmp(fp->key,key) == 0){
			findFlag = 1;
			break;
		}
		last = &fp->link;
		fp = fp->link;
	}
	if(fp == NULL){
		fp = calloc(sizeof(struct _glv_r_value),1);
		if(fp == NULL){
			return(GLV_ERROR);
		}
	}
	for(index=0;index<length;index++){
		rc = glv_r_is_typeChar2typeNo(type_string[index],&type);
		if(rc == 0){
			printf("glv_r_set_value:type not found. [%s] arg %d (\"%c\")\n",key,index+1,type_string[index]);
		}
		if((findFlag == 1) && (fp->n[index].type != GLV_R_VALUE_TYPE__NOTHING) && (fp->n[index].type != type)){
			printf("glv_r_set_value:type unmach. [%s] arg %d (%d != %d)\n",key,index+1,fp->n[index].type,type);
			return(GLV_ERROR);
		}
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
		switch(type){
			case GLV_R_VALUE_TYPE__SIZE:
				fp->n[index].v.size = va_arg(args, size_t);
				break;
			case GLV_R_VALUE_TYPE__INT64:
				fp->n[index].v.int64 = va_arg(args, int64_t);
				break;
			case GLV_R_VALUE_TYPE__INT32:
				fp->n[index].v.int32 = va_arg(args, int);
				break;
			case GLV_R_VALUE_TYPE__UINT32:
				fp->n[index].v.uint32 = va_arg(args, uint32_t);
				break;
			case GLV_R_VALUE_TYPE__COLOR:
				fp->n[index].v.int32 = va_arg(args, uint32_t);
				break;
			case GLV_R_VALUE_TYPE__STRING:
				if(fp->n[index].v.string != NULL){
					free(fp->n[index].v.string);
				}
				string = va_arg(args, void*);
				if(string != NULL){
					fp->n[index].v.string = strdup(string);
				}else{
					fp->n[index].v.string = NULL;
				}
				break;
			case GLV_R_VALUE_TYPE__POINTER:
				fp->n[index].v.pointer = va_arg(args, void*);
				break;
			case GLV_R_VALUE_TYPE__DOUBLE:
				fp->n[index].v.real = va_arg(args, double);
				break;
			case GLV_R_VALUE_TYPE__FUNCTION:
				//fp->n[index].v.func = va_arg(args, void*);
				fp->func = va_arg(args, void*);
				break;
		}
		fp->n[index].type = type;
	}
	if(index > fp->valueNum){
		fp->valueNum = index;
	}
	if(findFlag == 0){
		fp->key = strdup(key);
		fp->instance = instance;
#if 0
		fp->link = *link;
		*link = fp;
#else
		fp->link = NULL;
		*last = fp;
#endif
	}
	if(fp->func != NULL){
		rc = (fp->func)(GLV_R_VALUE_IO_SET,fp);
		if(rc == GLV_ERROR){
			printf("glv_r_set_value:triger function call error return [%s].\n",key);
			return(GLV_ERROR);
		}
	}
	return(GLV_OK);
}

int glv_r_get_value(struct _glv_r_value **link,void *instance,char *key,char *type_string,va_list args)
{
	struct _glv_r_value *fp;
	size_t *size;
	int *int32;
	uint32_t *uint32,*color;
	int64_t  *int64;
	void **string;
	void **pointer;
	double *real;
	int index;
	int type;
	int rc;

	int length = strlen(type_string);
	if(length > _GLV_R_VALUE_MAX){
		printf("glv_r_get_value:too many arguments (%d). [%s] (\"%s\"). up to %d arguments can be set.\n",length,key,type_string,_GLV_R_VALUE_MAX);
		length = _GLV_R_VALUE_MAX;
	}

	rc = GLV_OK;
	fp = *link;
	while(fp){
		if(strcmp(fp->key,key) == 0){
			if(fp->func != NULL){
				rc = (fp->func)(GLV_R_VALUE_IO_GET,fp);
			}
			break;
		}
		fp = fp->link;
	}
	if(rc == GLV_ERROR){
		printf("glv_r_get_value:triger function call error return [%s].\n",key);
		return(GLV_ERROR);
	}
	if(fp == NULL){
		printf("glv_r_get_value:kye[%s] not found.\n",key);
		return(GLV_ERROR);
	}
	for(index=0;index<length;index++){
		rc = glv_r_is_typeChar2typeNo(type_string[index],&type);
		if(rc == 0){
			printf("glv_r_get_value:type not found. [%s] arg %d (\"%c\")\n",key,index+1,type_string[index]);
		}
		if(type == GLV_R_VALUE_TYPE__FUNCTION){
			printf("glv_r_get_value:bad type. [%s] arg %d (%d != %d)\n",key,index+1,fp->n[index].type,type);
		}
		if(fp->n[index].type != type){
			printf("glv_r_get_value:type unmach. [%s] arg %d (%d != %d)\n",key,index+1,fp->n[index].type,type);
			//return(GLV_ERROR);
		}
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
		switch(type){
			case GLV_R_VALUE_TYPE__SIZE:
				size = va_arg(args, size_t*);
				*size = fp->n[index].v.size;
				break;
			case GLV_R_VALUE_TYPE__INT64:
				int64 = va_arg(args, int64_t*);
				*int64 = fp->n[index].v.int64;
				break;
			case GLV_R_VALUE_TYPE__INT32:
				int32 = va_arg(args, int*);
				*int32 = fp->n[index].v.int32;
				break;
			case GLV_R_VALUE_TYPE__UINT32:
				uint32 = va_arg(args, uint32_t*);
				*uint32 = fp->n[index].v.uint32;
				break;
			case GLV_R_VALUE_TYPE__COLOR:
				color = va_arg(args, uint32_t*);
				*color = fp->n[index].v.color;
				break;
			case GLV_R_VALUE_TYPE__STRING:
				string = va_arg(args, void**);
				*string = fp->n[index].v.string;
				break;
			case GLV_R_VALUE_TYPE__POINTER:
				pointer = va_arg(args, void**);
				*pointer = fp->n[index].v.pointer;
				break;
			case GLV_R_VALUE_TYPE__DOUBLE:
				real = va_arg(args, double*);
				*real = fp->n[index].v.real;
				break;
			case GLV_R_VALUE_TYPE__FUNCTION:
				// functionは、読み出せない
				break;
		}
	}
	return(GLV_OK);
}

int glv_r_free_value__list(struct _glv_r_value **link)
{
	struct _glv_r_value *fp,*next;
	int index;

	fp = *link;
	while(fp){
		next = fp->link;
		free(fp->key);
		free(fp->abstract);
		free(fp->type_string);
		for(index=0;index<_GLV_R_VALUE_MAX;index++){
			free(fp->n[index].abstract);
			if(fp->n[index].type == GLV_R_VALUE_TYPE__STRING){
				free(fp->n[index].v.string);
			}
		}
		free(fp);
		fp = next;
	}
	*link = NULL;
	return(GLV_OK);
}

int glv_setValue(void *glv_instance,char *key,char *type_string,...)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	int rc;
	int instanceType;

	if(glv_instance == NULL){
		printf("glv_setValue:instance is NULL\n");
		return(GLV_ERROR);
	}

	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == 0){
		printf("glv_setValue:bad instance\n");
		return(GLV_ERROR);
	}
	if(instance->resource_run == 1){
		printf("glv_setValue:cannot call glv_setValue inside a callback function.\n");
		return(GLV_ERROR);
	}

	va_list args;
    va_start( args, type_string );

	pthread_mutex_lock(&instance->resource_mutex);				// resource
	instance->resource_run = 1;
	rc = glv_r_set_value(&instance->resource,glv_instance,key,type_string,args);
	instance->resource_run = 0;
	pthread_mutex_unlock(&instance->resource_mutex);			// resource

	va_end( args );
	return(rc);
}

int glv_getValue(void *glv_instance,char *key,char *type_string,...)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	int rc;
	int instanceType;

	if(glv_instance == NULL){
		printf("glv_getValue:instance is NULL\n");
		return(GLV_ERROR);
	}

	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == 0){
		printf("glv_getValue:bad instance\n");
		return(GLV_ERROR);
	}
	if(instance->resource_run == 1){
		printf("glv_getValue:cannot call glv_getValue inside a callback function.\n");
		return(GLV_ERROR);
	}
	
	va_list args;
    va_start( args, type_string );

	pthread_mutex_lock(&instance->resource_mutex);				// resource
	instance->resource_run = 1;
	rc = glv_r_get_value(&instance->resource,glv_instance,key,type_string,args);
	instance->resource_run = 0;
	pthread_mutex_unlock(&instance->resource_mutex);			// resource

	va_end( args );
	return(rc);
}

int glv_r_set_abstract(struct _glv_r_value **link,void *instance,char *key,char *abstract,char *type_string,va_list args)
{
	struct _glv_r_value *fp;
	struct _glv_r_value **last;
	int index;
	int type;
	int findFlag=0;
	int	rc;
	char *string;

	int length = strlen(type_string);
	if(length > _GLV_R_VALUE_MAX){
		rc = 1;
		if(length == (_GLV_R_VALUE_MAX+1)){
			int last_type;
			glv_r_is_typeChar2typeNo(type_string[length-1],&last_type);
			if(last_type == GLV_R_VALUE_TYPE__FUNCTION ){
				rc = 0;		
			}
		}
		if(rc == 1){
			printf("glv_r_set_abstract:too many arguments (%d). [%s] (\"%s\"). up to %d arguments can be set.\n",length,key,type_string,_GLV_R_VALUE_MAX);
			length = _GLV_R_VALUE_MAX;
		}
	}

	fp = *link;
	last = link;
	while(fp){
		if(strcmp(fp->key,key) == 0){
			findFlag = 1;
			break;
		}
		last = &fp->link;
		fp = fp->link;
	}
	if(fp == NULL){
		fp = calloc(sizeof(struct _glv_r_value),1);
		if(fp == NULL){
			return(GLV_ERROR);
		}
	}

// ----------------------------------------------------

	for(index=0;index<length;index++){
		rc = glv_r_is_typeChar2typeNo(type_string[index],&type);
		if(rc == 0){
			printf("glv_r_set_abstract:type not found. [%s] arg %d (\"%c\")\n",key,index+1,type_string[index]);
		}
		if((findFlag == 1) && (fp->n[index].type != GLV_R_VALUE_TYPE__NOTHING) && (fp->n[index].type != type)){
			printf("glv_r_set_abstract:type unmach. [%s] arg %d (%d != %d)\n",key,index+1,fp->n[index].type,type);
			return(GLV_ERROR);
		}
		if(fp->n[index].abstract != NULL){
			free(fp->n[index].abstract);
		}
		string = va_arg(args, char*);
		if(string != NULL){
			fp->n[index].abstract = strdup(string);
		}else{
			fp->n[index].abstract = NULL;
		}
		fp->n[index].type = type;
	}
	if(index > fp->valueNum){
		fp->valueNum = index;
	}
	if(findFlag == 0){
		fp->key = strdup(key);
		fp->abstract = strdup(abstract);
		fp->type_string = strdup(type_string);
		fp->instance = instance;
#if 0
		fp->link = *link;
		*link = fp;
#else
		fp->link = NULL;
		*last = fp;
#endif
	}else{
		if(fp->abstract != NULL){
			free(fp->abstract);
		}
		fp->abstract = strdup(abstract);
		if(fp->type_string != NULL){
			free(fp->type_string);
		}
		fp->type_string = strdup(type_string);
	}
	return(GLV_OK);
}

int glv_setAbstract(void *glv_instance,char *key,char *abstract,char *type_string,...)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	int rc;
	int instanceType;

	if(glv_instance == NULL){
		printf("glv_setAbstract:instance is NULL\n");
		return(GLV_ERROR);
	}

	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == 0){
		printf("glv_setAbstract:bad instance\n");
		return(GLV_ERROR);
	}

	va_list args;
    va_start( args, type_string );

	pthread_mutex_lock(&instance->resource_mutex);				// resource
	rc = glv_r_set_abstract(&instance->resource,glv_instance,key,abstract,type_string,args);
	pthread_mutex_unlock(&instance->resource_mutex);			// resource

	va_end( args );
	return(rc);
}

void glv_r_print_value(struct _glv_r_value **link,void *instance)
{
	struct _glv_r_value *fp;
	size_t size;
	int int32;
	uint32_t uint32,color;
	int64_t int64;
	void *string;
	void *pointer;
	double real;
	int index;
	int type;
	int length;

	fp = *link;
	if(fp == NULL){
		printf("glv_r_print_value:nothing is set.\n");
		return;
	}

	while(fp){
		if(fp->key != NULL){
			printf("key:\"%s\" , ",fp->key);
			if(fp->abstract != NULL){
				printf("%s , ",fp->abstract);
			}
			if(fp->type_string != NULL){
				printf("\"%s\" , ",fp->type_string);
			}
		}
		if(fp->func != NULL){
			printf("trigger: [have]\n");
		}else{
			printf("trigger: [not have]\n");
		}
		length = fp->valueNum;
		for(index=0;index<length;index++){
			type = fp->n[index].type;
			if(type != GLV_R_VALUE_TYPE__FUNCTION){
				printf("    arg%d ",index+1);
				if(fp->n[index].abstract != NULL){
					printf("[%s]",fp->n[index].abstract);
				}
			}
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
			switch(type){
				case GLV_R_VALUE_TYPE__SIZE:
					size = fp->n[index].v.size;
					printf(" type = size_t , value = %lu , %#lx\n",size,size);
					break;
				case GLV_R_VALUE_TYPE__INT64:
					int64 = fp->n[index].v.int64;
					printf(" type = int64_t , value = %ld , %#lx\n",int64,int64);
					break;
				case GLV_R_VALUE_TYPE__INT32:
					int32 = fp->n[index].v.int32;
					printf(" type = int32_t , value = %d , %#x\n",int32,int32);
					break;
				case GLV_R_VALUE_TYPE__UINT32:
					uint32 = fp->n[index].v.uint32;
					printf(" type = uint32_t , value = %u , %#x\n",uint32,uint32);
					break;
				case GLV_R_VALUE_TYPE__STRING:
					string = fp->n[index].v.string;
					printf(" type = char*   , value = \"%s\" , %p\n",(char*)string,string);
					break;
				case GLV_R_VALUE_TYPE__POINTER:
					pointer = fp->n[index].v.pointer;
					printf(" type = void*   , value = %p\n",pointer);
					break;
				case GLV_R_VALUE_TYPE__DOUBLE:
					real = fp->n[index].v.real;
					printf(" type = double  , value = %lf\n",real);
					break;
				case GLV_R_VALUE_TYPE__COLOR:
					color = fp->n[index].v.color;
					printf(" type = GLV_RGBACOLOR , value = GLV_SET_RGBA(%3d,%3d,%3d,%3d)\n",GLV_GET_R(color),GLV_GET_G(color),GLV_GET_B(color),GLV_GET_A(color));
					break;
				case GLV_R_VALUE_TYPE__FUNCTION:
					// functionは、読み出せない
					break;
			}
		}
		fp = fp->link;
	}
}

void glv_r_print_template(struct _glv_r_value **link,void *instance,char *instance_string)
{
	struct _glv_r_value *fp;
	size_t size;
	int int32;
	uint32_t uint32,color;
	int64_t int64;
	void *string;
	void *pointer;
	double real;
	int index;
	int type;
	int length;

	fp = *link;
	if(fp == NULL){
		return;
	}

	printf("------ template -------\n");
	while(fp){
		if(fp->key != NULL){
			printf("glv_setValue(%s,\"%s\",\"",instance_string,fp->key);
		}
		length = fp->valueNum;
		// -----------------------------------------------------------------------------------------
		for(index=0;index<length;index++){
			type = fp->n[index].type;
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
			switch(type){
				case GLV_R_VALUE_TYPE__SIZE:
					printf("Z");
					break;
				case GLV_R_VALUE_TYPE__INT64:
					printf("L");
					break;
				case GLV_R_VALUE_TYPE__INT32:
					printf("i");
					break;
				case GLV_R_VALUE_TYPE__UINT32:
					printf("U");
					break;
				case GLV_R_VALUE_TYPE__COLOR:
					printf("C");
					break;
				case GLV_R_VALUE_TYPE__STRING:
					printf("S");
					break;
				case GLV_R_VALUE_TYPE__POINTER:
					printf("P");
					break;
				case GLV_R_VALUE_TYPE__DOUBLE:
					printf("R");
					break;
				case GLV_R_VALUE_TYPE__FUNCTION:
					// functionは、読み出せない
					break;
			}
		}
		printf("\"");
		// -----------------------------------------------------------------------------------------
		for(index=0;index<length;index++){
			type = fp->n[index].type;
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
			switch(type){
				case GLV_R_VALUE_TYPE__SIZE:
					size = fp->n[index].v.size;
					printf(", (size_t) %lu",size);
					break;
				case GLV_R_VALUE_TYPE__INT64:
					int64 = fp->n[index].v.int64;
					printf(", (int64_t) %ld",int64);
					break;
				case GLV_R_VALUE_TYPE__INT32:
					int32 = fp->n[index].v.int32;
					printf(", (int32_t) %d",int32);
					break;
				case GLV_R_VALUE_TYPE__UINT32:
					uint32 = fp->n[index].v.uint32;
					printf(", (uint32_t) %u",uint32);
					break;
				case GLV_R_VALUE_TYPE__STRING:
					string = fp->n[index].v.string;
					printf(", \"%s\"",(char*)string);
					break;
				case GLV_R_VALUE_TYPE__POINTER:
					pointer = fp->n[index].v.pointer;
					printf(", (void*) pointer");
					break;
				case GLV_R_VALUE_TYPE__DOUBLE:
					real = fp->n[index].v.real;
					printf(", (double)%lf",real);
					break;
				case GLV_R_VALUE_TYPE__COLOR:
					color = fp->n[index].v.color;
					printf(", GLV_SET_RGBA(%3d,%3d,%3d,%3d)",GLV_GET_R(color),GLV_GET_G(color),GLV_GET_B(color),GLV_GET_A(color));
					break;
				case GLV_R_VALUE_TYPE__FUNCTION:
					// functionは、読み出せない
					break;
			}
		}
		printf(");\n");
		// -----------------------------------------------------------------------------------------
		fp = fp->link;
	}
}

void glv_printValue(void *glv_instance,char *note)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	int rc;
	int instanceType;
	GLV_WINDOW_t *window;
	GLV_SHEET_t *sheet;
	GLV_WIGET_t *wiget;
	glvResource *resource;
	char	*instance_string;

	if(glv_instance == NULL){
		printf("glv_printValue:note[%s] instance is NULL\n",note);
		return;
	}

	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == 0){
		printf("glv_printValue:note[%s] bad instance\n",note);
		return;
	}

	if(note != NULL){
		printf("glv_printValue: note[%s]",note);
	}

	switch(instanceType){
		case GLV_INSTANCE_TYPE_DISPLAY:
			printf(" [GLV_INSTANCE_TYPE_DISPLAY]\n");
			instance_string = "glv_dpy";
			break;
		case GLV_INSTANCE_TYPE_WINDOW:
			window = (GLV_WINDOW_t*)glv_instance;
			printf(" [GLV_INSTANCE_TYPE_WINDOW]\n");
			instance_string = "window";
			break;
		case GLV_INSTANCE_TYPE_SHEET:
			sheet = (GLV_SHEET_t*)glv_instance;
			printf(" [GLV_INSTANCE_TYPE_SHEET]\n");
			instance_string = "sheet";
			break;
		case GLV_INSTANCE_TYPE_WIGET:
			wiget = (GLV_WIGET_t*)glv_instance;
			printf(" [GLV_INSTANCE_TYPE_WIGET]\n");
			instance_string = "wiget";
			break;
		case GLV_INSTANCE_TYPE_RESOURCE:
			resource = (glvResource*)glv_instance;
			printf(" [GLV_INSTANCE_TYPE_RESOURCE]\n");
			instance_string = "resource";
			break;
		default:
			printf(" [GLV_INSTANCE_TYPE_UNKNOWN]\n");
			instance_string = "?";
			break;
	}
	
	pthread_mutex_lock(&instance->resource_mutex);				// resource
	glv_r_print_value(&instance->resource,glv_instance);
	glv_r_print_template(&instance->resource,glv_instance,instance_string);
	pthread_mutex_unlock(&instance->resource_mutex);			// resource
}

int glv_allocUserData(void *glv_instance,size_t size)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	int instanceType;
	void *data;

	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == 0){
		printf("glv_allocUserData:bad instance\n");
		return(GLV_ERROR);
	}

	if(instance->user_data != NULL){
		free(instance->user_data);
		instance->user_data = NULL;
	}
	data = malloc(size);
	if(!data){
		return(GLV_ERROR);
	}
	memset(data,0,size);
	instance->user_data = data;
	return(GLV_OK);
}

void *glv_getUserData(void *glv_instance)
{
	_GLV_INSTANCE_t *instance = glv_instance;
	int instanceType;

	instanceType = glv_getInstanceType(glv_instance);
	if(instanceType == 0){
		printf("glv_getUserData:bad instance\n");
		return(GLV_ERROR);
	}

	return(instance->user_data);
}

glvInstanceId glv_getInstanceId(void *glv_instance)
{
	_GLV_INSTANCE_t *instance = glv_instance;

	if(instance == NULL){
		return(0);
	}

	return(instance->Id);
}

char *glv_strdup(char *str)
{
	return strdup(str);
}

void *glv_malloc(size_t size)
{
#if 0
	char *ptr = malloc(size);
	printf("glv_malloc %p %d\n",ptr,*ptr);
	return(ptr);
#else
	return malloc(size);
#endif
}

void *glv_cmalloc(size_t nmemb, size_t size)
{
	return calloc(nmemb,size);
}

void *glv_realloc(void *ptr, size_t size)
{
	return realloc(ptr,size);
}

void glv_free(void *ptr)
{
	printf("glv_free %p\n",ptr);
	free(ptr);
}

void *glv_memset(void *dest, int c, size_t size)
{
	return memset(dest,c,size);
}

void *glv_memcpy(void *dest,void *src,size_t size)
{
	return memcpy(dest,src,size);
}
