
#include <stdio.h>
//#include <ibus.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
//#include <gio/gio.h>
//#include <gobject/gtype.h>
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

#ifdef USE_FCITX5
#include <fcitx-gclient/fcitxgclient.h>
#else
#include <fcitx/frontend.h>
#include <fcitx-gclient/fcitxclient.h>
#endif

#define FCITX_ID -3

#ifdef USE_FCITX5
#define FcitxClient FcitxGClient
#define FcitxPreeditItem FcitxGPreeditItem

#define fcitx_client_focus_in(client) fcitx_g_client_focus_in(client)
#define fcitx_client_focus_out(client) fcitx_g_client_focus_out(client)
#define fcitx_client_process_key_sync fcitx_g_client_process_key_sync
#define fcitx_client_set_capacity(client, flag) fcitx_g_client_set_capability(client, flag)
#define fcitx_client_set_cursor_rect fcitx_g_client_set_cursor_rect
#define fcitx_client_close_ic(client)
#define fcitx_client_enable_ic(client)

/* The 5th argument of fcitx_g_client_process_key_sync() is 'isRelease' */
#define FCITX_PRESS_KEY (0)
#define FCITX_RELEASE_KEY (1)

/* See fcitx-utils/capabilityflags.h */
#define CAPACITY_CLIENT_SIDE_UI (1ull << 39) //(1 << 0)
#define CAPACITY_PREEDIT (1 << 1)
#define CAPACITY_CLIENT_SIDE_CONTROL_STATE (1 << 2)
#define CAPACITY_FORMATTED_PREEDIT (1 << 4)
#endif

#if 1
#define IM_FCITX_DEBUG 1
#endif

static void connected(FcitxClient *client, void *data);

/*
 * fcitx doesn't support wayland, so the positioning of gtk-based candidate window of fcitx
 * doesn't work correctly on wayland.
 */
#define KeyPress 2 /* see uitoolkit/fb/ui_display.h */
//#define USE_IM_CANDIDATE_SCREEN

#define glv_list_insert_head(list, new)		((new)->next = (list) , (list) = (new))
#define glv_list_next(list)					((list) ? (list)->next : NULL)

typedef struct im_fcitx {
	FcitxClient		*client;
	FcitxKeyState	modifiers;
	gboolean		is_enabled;
} im_fcitx_t;


static int ref_count = 0;
static int fcitx_fd = -3;
static int fcitx_im_preedit_filled_len=0;
static im_fcitx_t *fcitx_preedit = NULL;

static void connection_handler(struct task *task, uint32_t events)
{
	g_main_context_iteration(g_main_context_default(), FALSE);
	printf("call connection_handler\n");
}


static void update_formatted_preedit(FcitxClient *client, GPtrArray *list, int cursor_pos,void *data)
{
	struct _glvinput *glv_input = data;
	im_fcitx_t *fcitx;
	int *utf32_string;
	uint8_t *utf32_attr;
	int utf32_length;
	size_t fcitx_str_len;	// バイト数
	size_t fcitx_text_len;	// バイト数ではなく、文字数

	fcitx = glv_input->im;

	fcitx_preedit = fcitx;

	fcitx_str_len = 0;

	// バイト数を数える
	if (list->len > 0) {
		FcitxPreeditItem *item;
		guint count;

    	for (count = 0; count < list->len; count++) {
      		size_t str_len;

    		item = g_ptr_array_index(list, count);
    		str_len = strlen(item->string);
			fcitx_str_len += str_len;
		}
	
	}
	printf("update_formatted_preedit:fcitx_str_len = %ld\n",fcitx_str_len);

	utf32_string = malloc(sizeof(int)     * (fcitx_str_len + 1));
	utf32_attr   = malloc(sizeof(uint8_t) * (fcitx_str_len + 1));

	fcitx_text_len = 0;
	if (list->len > 0) {
		FcitxPreeditItem *item;
		guint count,in_index,out_index;
		int *utf32_item_string;
		int utf32_item_length;

		// ------------------------------------------------------------------------------
#if 1
    	for (count = 0; count < list->len; count++) {
    		item = g_ptr_array_index(list, count);
			printf("item type[%d]: %x\n",count,item->type);
		}
#endif
		// ------------------------------------------------------------------------------
		out_index = 0;
    	for (count = 0; count < list->len; count++) {
      		size_t str_len;
			int is_underlined = 0;
			int is_reverse_color = 0;

    		item = g_ptr_array_index(list, count);

			is_underlined = 1;	// 現状、1固定
			if(item->type != 0){
				is_reverse_color = 1;
			}

    		str_len = strlen(item->string);

			printf("update_preedit_text = %d[%s] length = %ld\n",count,item->string,str_len);

			utf32_item_string = malloc(sizeof(int) * (str_len + 1));
			utf32_item_length = glvFont_string_to_utf32(item->string,str_len,utf32_item_string,str_len);
			printf("utf32_item_length = %d\n",utf32_item_length);
			for(in_index=0;in_index < utf32_item_length;in_index++){
				utf32_string[out_index] = utf32_item_string[in_index];
				utf32_attr[out_index] = is_underlined + is_reverse_color;
				out_index++;
				//printf("utf32_item_string[0] = %x\n",utf32_item_string[0]);
			}
#if 0
			{
				char *utf8;
				utf8 = malloc(utf32_item_length * 4 + 1);
				glvFont_utf32_to_string(utf32_item_string,utf32_item_length,utf8,utf32_item_length * 4);
				printf("update_preedit_text utf32_item_string [%s]\n",utf8);
				free(utf8);
			}
#endif
			free(utf32_item_string);
		}
		fcitx_text_len = out_index;
	}
	//printf("utf32_string[0] = %x\n",utf32_string[0]);
	{
		char *utf8;
		utf8 = malloc(fcitx_text_len * 4 + 1);
		glvFont_utf32_to_string(utf32_string,fcitx_text_len,utf8,fcitx_text_len * 4);
		printf("update_preedit_text all [%s] fcitx_text_len = %ld\n",utf8,fcitx_text_len);
		free(utf8);
	}
	fcitx_im_preedit_filled_len = utf32_length = fcitx_text_len;
	glv_input->im_state = GLV_KEY_STATE_IM_PREEDIT;
	_glvOnTextInput((glvDisplay)glv_input->glv_dpy,GLV_KEY_KIND_IM,GLV_KEY_STATE_IM_PREEDIT,0,utf32_string,utf32_attr,utf32_length);
	free(utf32_string);
	free(utf32_attr);
}

static void commit_string(FcitxClient *client, char *str, void *data)
{
	int *utf32_string;
	int utf32_length;
	struct _glvinput *glv_input = data;
	im_fcitx_t *fcitx;

	fcitx = glv_input->im;

	size_t fcitx_text_len;

	fcitx_im_preedit_filled_len = 0;

	fcitx_text_len = strlen(str); // バイト数

	if(fcitx_text_len == 0){
		GLV_IF_DEBUG_IME_INPUT printf("hide_preedit_text do nothing\n");
		/* do nothing */
	}else{
		GLV_IF_DEBUG_IME_INPUT printf("commit_text = [%s] length = %ld\n",str,strlen(str));
		printf("commit_text = [%s] length = %ld\n",str,strlen(str));

		utf32_string = malloc(sizeof(int) * (fcitx_text_len + 1));
		utf32_length = glvFont_string_to_utf32(str,strlen(str),utf32_string,fcitx_text_len);
		glv_input->im_state = GLV_KEY_STATE_IM_COMMIT;
		_glvOnTextInput((glvDisplay)glv_input->glv_dpy,GLV_KEY_KIND_IM,GLV_KEY_STATE_IM_COMMIT,0,utf32_string,NULL,utf32_length);
		free(utf32_string);
	}
}

static void glv_ime_key_modifiers(struct _glvinput *glv_input,xkb_mod_mask_t mask)
{
	im_fcitx_t *fcitx;
	if(glv_input->im == NULL){
		return;
	}
	fcitx = glv_input->im;

    fcitx->modifiers = 0;
    if(mask & glv_input->shift_mask)
        fcitx->modifiers |= FcitxKeyState_Shift;
    if(mask & glv_input->lock_mask)
        fcitx->modifiers |= FcitxKeyState_CapsLock;
    if(mask & glv_input->control_mask)
        fcitx->modifiers |= FcitxKeyState_Ctrl;
    if(mask & glv_input->mod1_mask)
        fcitx->modifiers |= FcitxKeyState_Alt;
    if(mask & glv_input->mod2_mask)
        fcitx->modifiers |= FcitxKeyState_NumLock;
//    if(mask & glv_input->mod3_mask)
//        fcitx->modifiers |= IBUS_MOD3_MASK;
    if(mask & glv_input->mod4_mask)
        fcitx->modifiers |= FcitxKeyState_Super;
    if(mask & glv_input->mod5_mask)
        fcitx->modifiers |= FcitxKeyState_ScrollLock;
    if(mask & glv_input->super_mask)
        fcitx->modifiers |= FcitxKeyState_Super2;
    if(mask & glv_input->hyper_mask)
        fcitx->modifiers |= FcitxKeyState_Hyper;
    if(mask & glv_input->meta_mask)
        fcitx->modifiers |= FcitxKeyState_Meta;
}

static int glv_ime_key_event(struct _glvinput *glv_input,int keycode, int ksym, int state_,int type)
{
	im_fcitx_t *fcitx;
	u_int state;
	static guint32		t=1;

	if(glv_input->im == NULL){
		return 1;
	}
	fcitx = glv_input->im;

	state = fcitx->modifiers;

do{
#ifdef USE_FCITX5
  if (!fcitx_g_client_is_valid(fcitx->client)) {
    connection_handler();
	break;
  }
#else
  if (!fcitx_client_is_valid(fcitx->client)) {
    connection_handler(NULL,0);
	break;
  }
#endif
	//connected(fcitx->client,glv_input);

  if (state & FcitxKeyState_IgnoredMask) {
    /* Is put back in forward_key_event */
    state &= ~FcitxKeyState_IgnoredMask;
  } else if (fcitx_client_process_key_sync(
                 fcitx->client,ksym,
                 keycode - 8,
                 state, type == GLV_KeyPress ? FCITX_PRESS_KEY : FCITX_RELEASE_KEY,
#if 1
                 0L /* CurrentTime */
#else
				t++
#endif
                 )) {
    fcitx->is_enabled = TRUE;
    //state_ = state;
    //memcpy(&fcitx->prev_key, event, sizeof(XKeyEvent));

    g_main_context_iteration(g_main_context_default(), FALSE);

    return 0;
  } else {
    fcitx->is_enabled = FALSE;

    if (fcitx_im_preedit_filled_len > 0) {
      g_main_context_iteration(g_main_context_default(), FALSE);
    }
  }
}while(0);

	return 1;
}

static void connected(FcitxClient *client, void *data)
{
	im_fcitx_t *fcitx;
	int x;
	int y;

	struct _glvinput *glv_input = data;

#if 1
  printf("Connected to FCITX server\n");
#endif

  fcitx = glv_input->im;

  fcitx_client_set_capacity(client,CAPACITY_PREEDIT | CAPACITY_FORMATTED_PREEDIT);
  fcitx_client_focus_in(client);

#if 0
  /*
   * XXX
   * To show initial status window (e.g. "Mozc") at the correct position.
   * Should be moved to enable_im() but "enable-im" event doesn't work. (fcitx 4.2.9.1)
   */
  if ((*fcitx->im.listener->get_spot)(fcitx->im.listener->self, NULL, 0, &x, &y)) {
    u_int line_height;

    line_height = (*fcitx->im.listener->get_line_height)(fcitx->im.listener->self);
    fcitx_client_set_cursor_rect(fcitx->client, x, y - line_height, 0, line_height);
  }
#endif
}

#ifndef USE_FCITX5
static void disconnected(FcitxClient *client, void *data) {
  im_fcitx_t *fcitx;

	struct _glvinput *glv_input = data;

  fcitx =  glv_input->im;

  fcitx->is_enabled = FALSE;

//  if (fcitx->im.stat_screen) {
//    (*fcitx->im.stat_screen->destroy)(fcitx->im.stat_screen);
//    fcitx->im.stat_screen = NULL;
//  }
}

static void close_im(FcitxClient *client, void *data) { disconnected(client, data); }
#endif

static void forward_key(FcitxClient *client, guint keyval, guint state, gint type, void *data)
{
	GLV_IF_DEBUG_IME_INPUT printf("forward_key_event state = %x , keyval = %d\n",state,keyval + 8);
	printf("forward_key_event state = %x , keyval = %d\n",state,keyval + 8);
}

int glv_ime_setIMECandidatePotition(int candidate_pos_x,int candidate_pos_y)
{
	if(fcitx_preedit == NULL){
		return(GLV_ERROR);
	}

	fcitx_client_set_cursor_rect(fcitx_preedit->client, candidate_pos_x, candidate_pos_y, 0, 0);

	return(GLV_OK);
}

static FcitxConnection *_connection = NULL;

int glv_ime_startFcitx(struct _glvinput *glv_input)
{
	static int is_init=0;
	im_fcitx_t *fcitx = NULL;

	if(!is_init) {
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		g_type_init();
#pragma GCC diagnostic warning "-Wdeprecated-declarations"
		is_init = 1;
	}

	if(!(fcitx = calloc(1, sizeof(im_fcitx_t)))) {
		printf("glv_ime_startFcitx: malloc failed.\n");
		return(GLV_OK);
  }

#ifdef USE_FCITX5
  if (!(fcitx->client = fcitx_g_client_new())) {
    goto error;
  }
  fcitx_g_client_set_program(fcitx->client, "glview");
#else
#if 0
  if (!(fcitx->client = fcitx_client_new())) {
    goto error;
  }
#else
        _connection = fcitx_connection_new();
        g_object_ref_sink(_connection);
  if (!(fcitx->client = fcitx_client_new_with_connection(_connection))) {
    goto error;
  }
#endif
#endif

//---------------------------------------------------------------------------------------------

  g_signal_connect(fcitx->client, "connected", G_CALLBACK(connected), glv_input);
#ifndef USE_FCITX5
  g_signal_connect(fcitx->client, "disconnected", G_CALLBACK(disconnected), glv_input);
  g_signal_connect(fcitx->client, "close-im", G_CALLBACK(close_im), glv_input);
#endif
  g_signal_connect(fcitx->client, "forward-key", G_CALLBACK(forward_key), glv_input);
  g_signal_connect(fcitx->client, "commit-string", G_CALLBACK(commit_string), glv_input);
  g_signal_connect(fcitx->client, "update-formatted-preedit", G_CALLBACK(update_formatted_preedit),glv_input);

//---------------------------------------------------------------------------------------------

  if (ref_count++ == 0) {
		static struct task task;
		task.run = connection_handler;
#if 0
		fcitx_fd = g_socket_get_fd(g_socket_connection_get_socket(
           (GSocketConnection *)g_dbus_connection_get_stream(fcitx_connection_get_g_dbus_connection(_connection))));
		   printf("fcitx_fd = %d\n",fcitx_fd);
#endif
		GDBusConnection *GDBus_connection;
		GDBus_connection = fcitx_connection_get_g_dbus_connection(_connection);
		weston_client_window__display_watch_fd(glv_input->glv_dpy, fcitx_fd, EPOLLIN | EPOLLERR | EPOLLHUP,&task);
    }

#ifdef IM_FCITX_DEBUG
  printf("New object was created. ref_count is %d.\n", ref_count);
#endif

#ifdef USE_FCITX5
  /* I don't know why, but connected() is not called without this. */
  usleep(100000);
#endif

	glv_input->im = fcitx;

	glv_input->ime_key_event = glv_ime_key_event;
	glv_input->ime_key_modifiers = glv_ime_key_modifiers;
	glv_input->glv_dpy->ime_setCandidatePotition = glv_ime_setIMECandidatePotition;

	GLV_IF_DEBUG_IME_INPUT printf("glv_ime_startFcitx:start.\n");
	printf("glv_ime_startFcitx:start.\n");

    //fcitx_client_enable_ic(fcitx->client);
	//fcitx_client_focus_in(fcitx->client);
	g_main_context_iteration(g_main_context_default(), FALSE);

	printf("fcitx-mozc(dbus)は正常に動作しません。\n");
	printf("・数文字入力しないと、fcitxサーバーに接続されません。\n");
	printf("・変換文字が遅れて表示される場合があります。\n");

	return(GLV_OK);

//---------------------------------------------------------------------------------------------
error:
  if (fcitx) {
    if (fcitx->client) {
      g_object_unref(fcitx->client);
    }
    free(fcitx);
  }
	return(GLV_ERROR);
}
