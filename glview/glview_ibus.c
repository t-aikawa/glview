
#include <stdio.h>
#include <ibus.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <gio/gio.h>
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

#define glv_list_insert_head(list, new)		((new)->next = (list) , (list) = (new))
#define glv_list_next(list)					((list) ? (list)->next : NULL)

#if IBUS_CHECK_VERSION(1, 5, 0)
#define ibus_input_context_is_enabled(context) (TRUE)
#endif

typedef struct im_ibus {
	IBusInputContext *context;
	IBusModifierType modifiers;
	gboolean is_enabled;
	struct im_ibus *next;
} im_ibus_t;

static IBusBus *ibus_bus;
static int ibus_bus_fd = -1;
static int ibus_im_preedit_filled_len=0;
static im_ibus_t *ibus_list = NULL;
static im_ibus_t *ibus_preedit = NULL;

static void update_preedit_text(IBusInputContext *context, IBusText *text, gint cursor_pos,
                                gboolean visible, gpointer data)
{
	struct _glvinput *glv_input = data;
	im_ibus_t *ibus;
	u_int ibus_text_len;  // バイト数ではなく、文字数
	int index;
	int *utf32_string;
	uint8_t *utf32_attr;
	int utf32_length;

	ibus = (im_ibus_t*)glv_input->im;

	ibus_preedit = ibus;

	ibus_text_len = ibus_text_get_length(text); // バイト数ではなく、文字数

	utf32_string = malloc(sizeof(int)     * (ibus_text_len + 1));
	utf32_attr   = malloc(sizeof(uint8_t) * (ibus_text_len + 1));
	//printf("update_preedit_text: len = %d %ld\n",len,strlen(text->text));

	utf32_length = glvFont_string_to_utf32(text->text,strlen(text->text),utf32_string,ibus_text_len);
	GLV_IF_DEBUG_IME_INPUT printf("update_preedit_text: ibus = %d strlen = %ld utf32 = %d\n",ibus_text_len,strlen(text->text),utf32_length);
	if(utf32_length != ibus_text_len){
		printf("update_preedit_text:utf32の文字数が異常 len = %d %d\n",ibus_text_len,utf32_length);
	}

	for(index=0;index<utf32_length;index++){
		u_int count;
		IBusAttribute *attr;
		int is_underlined = 0;
		int is_reverse_color = 0;
		for (count = 0; (attr = ibus_attr_list_get(text->attrs, count)); count++){
			if(attr->start_index <= index && index < attr->end_index){
			if(attr->type == IBUS_ATTR_TYPE_UNDERLINE){
				is_underlined = (attr->value != IBUS_ATTR_UNDERLINE_NONE);
			} else if(attr->type == IBUS_ATTR_TYPE_BACKGROUND){
				is_reverse_color = 2;
			}
		}
	}
	//printf("char = %04x , underlined = %d , reverse_color = %d\n",utf32_string[index],is_underlined,is_reverse_color);
	utf32_attr[index] = is_underlined + is_reverse_color;
  }

	GLV_IF_DEBUG_IME_INPUT printf("update_preedit_text = [%s]\n",text->text);
	ibus_im_preedit_filled_len = ibus_text_len;
	glv_input->im_state = GLV_KEY_STATE_IM_PREEDIT;
	_glvOnTextInput((glvDisplay)glv_input->glv_dpy,GLV_KEY_KIND_IM,GLV_KEY_STATE_IM_PREEDIT,0,utf32_string,utf32_attr,utf32_length);
	free(utf32_string);
	free(utf32_attr);
}

static void hide_preedit_text(IBusInputContext *context, gpointer data)
{
	struct _glvinput *glv_input = data;
	GLV_IF_DEBUG_IME_INPUT printf("hide_preedit_text\n");
	ibus_im_preedit_filled_len = 0;
	glv_input->im_state = GLV_KEY_STATE_IM_HIDE;
	_glvOnTextInput((glvDisplay)glv_input->glv_dpy,GLV_KEY_KIND_IM,GLV_KEY_STATE_IM_PREEDIT,0,NULL,NULL,0);
}

static void commit_text(IBusInputContext *context, IBusText *text, gpointer data)
{
	int *utf32_string;
	int utf32_length;
	struct _glvinput *glv_input = data;
	u_int ibus_text_len;  // バイト数ではなく、文字数

	ibus_im_preedit_filled_len = 0;

	ibus_text_len = ibus_text_get_length(text); // バイト数ではなく、文字数

	if(ibus_text_len == 0){
		GLV_IF_DEBUG_IME_INPUT printf("hide_preedit_text do nothing\n");
		/* do nothing */
	}else{
		GLV_IF_DEBUG_IME_INPUT printf("commit_text = [%s] length = %ld\n",text->text,strlen(text->text));
		//printf("commit_text = [%02x][%02x][%02x] length = %d\n",text->text[0],text->text[1],text->text[2],strlen(text->text));

		utf32_string = malloc(sizeof(int) * (ibus_text_len + 1));
		utf32_length = glvFont_string_to_utf32(text->text,strlen(text->text),utf32_string,ibus_text_len);
		glv_input->im_state = GLV_KEY_STATE_IM_COMMIT;
		_glvOnTextInput((glvDisplay)glv_input->glv_dpy,GLV_KEY_KIND_IM,GLV_KEY_STATE_IM_COMMIT,0,utf32_string,NULL,utf32_length);
		free(utf32_string);
	}
}

static void forward_key_event(IBusInputContext *context, guint keyval, guint keycode, guint state,gpointer data)
{
	GLV_IF_DEBUG_IME_INPUT printf("forward_key_event state = %x , keycode = %d\n",state,keycode);
}

static void connection_handler(struct task *task, uint32_t events)
{
	g_main_context_iteration(g_main_context_default(), FALSE);
	//printf("call connection_handler\n");
}

static int add_event_source(struct _glvinput *glv_input)
{
	//printf("call add_event_source\n");
  /*
   * GIOStream returned by g_dbus_connection_get_stream() is forcibly
   * regarded as GSocketConnection.
   */
	if((ibus_bus_fd = g_socket_get_fd(g_socket_connection_get_socket(
           (GSocketConnection *)g_dbus_connection_get_stream(ibus_bus_get_connection(ibus_bus))))) == -1) {
		return 0;
	}

    {
		static struct task task;
		task.run = connection_handler;
		weston_client_window__display_watch_fd(glv_input->glv_dpy, ibus_bus_fd, EPOLLIN | EPOLLERR | EPOLLHUP,&task);
    }

	return 1;
}

static void remove_event_source(struct _glvinput *glv_input)
{
	weston_client_window__display_unwatch_fd(glv_input->glv_dpy,ibus_bus_fd);
	ibus_bus_fd = -1;
}

void glv_ime_key_modifiers(struct _glvinput *glv_input,xkb_mod_mask_t mask)
{
	im_ibus_t *ibus;
	if(glv_input->im == NULL){
		return;
	}
	ibus = glv_input->im;

    ibus->modifiers = 0;
    if(mask & glv_input->shift_mask)
        ibus->modifiers |= IBUS_SHIFT_MASK;
    if(mask & glv_input->lock_mask)
        ibus->modifiers |= IBUS_LOCK_MASK;
    if(mask & glv_input->control_mask)
        ibus->modifiers |= IBUS_CONTROL_MASK;
    if(mask & glv_input->mod1_mask)
        ibus->modifiers |= IBUS_MOD1_MASK;
    if(mask & glv_input->mod2_mask)
        ibus->modifiers |= IBUS_MOD2_MASK;
    if(mask & glv_input->mod3_mask)
        ibus->modifiers |= IBUS_MOD3_MASK;
    if(mask & glv_input->mod4_mask)
        ibus->modifiers |= IBUS_MOD4_MASK;
    if(mask & glv_input->mod5_mask)
        ibus->modifiers |= IBUS_MOD5_MASK;
    if(mask & glv_input->super_mask)
        ibus->modifiers |= IBUS_SUPER_MASK;
    if(mask & glv_input->hyper_mask)
        ibus->modifiers |= IBUS_HYPER_MASK;
    if(mask & glv_input->meta_mask)
        ibus->modifiers |= IBUS_META_MASK;
}

int glv_ime_key_event(struct _glvinput *glv_input,int keycode, int ksym, int state_,int type)
{
	im_ibus_t *ibus;
	if(glv_input->im == NULL){
		return 1;
	}
	ibus = glv_input->im;

	if(!ibus->context) {
		return 1;
	}

	int state = ibus->modifiers;

	if(state & IBUS_IGNORED_MASK) {
		/* Is put back in forward_key_event */
		state &= ~IBUS_IGNORED_MASK;
	} else if(ibus_input_context_process_key_event(ibus->context,ksym,(keycode - 8),
                                                  state | (type == GLV_KeyRelease ? IBUS_RELEASE_MASK: 0))){
		gboolean is_enabled_old;

		is_enabled_old = ibus->is_enabled;
		ibus->is_enabled = ibus_input_context_is_enabled(ibus->context);

#if !IBUS_CHECK_VERSION(1, 5, 0)
		if(ibus->is_enabled != is_enabled_old){
			return 0;
		}else
#else
		UNUSED(is_enabled_old);
#endif
		if(ibus->is_enabled) {
			g_main_context_iteration(g_main_context_default(), FALSE);
			return 0;
		}
	} else if(ibus_im_preedit_filled_len > 0) {
		/* Pressing "q" in preediting. */
		g_main_context_iteration(g_main_context_default(), FALSE);
	}

	return 1;
}

static void set_engine(IBusInputContext *context, gchar *name)
{

	GLV_IF_DEBUG_IME_INPUT printf("iBus engine %s -> %s\n",ibus_engine_desc_get_name(ibus_input_context_get_engine(context)), name);
	ibus_input_context_set_engine(context, name);
}

static IBusInputContext *context_new(struct _glvinput *glv_input,im_ibus_t *ibus, char *engine)
{
	IBusInputContext *context;

	if(!(context = ibus_bus_create_input_context(ibus_bus, "navi"))) {
		return NULL;
	}

	ibus_input_context_set_capabilities(context,(IBUS_CAP_PREEDIT_TEXT | IBUS_CAP_FOCUS));

	g_signal_connect(context, "update-preedit-text", G_CALLBACK(update_preedit_text), glv_input);
	g_signal_connect(context, "hide-preedit-text", G_CALLBACK(hide_preedit_text), glv_input);
	g_signal_connect(context, "commit-text", G_CALLBACK(commit_text), glv_input);
	g_signal_connect(context, "forward-key-event", G_CALLBACK(forward_key_event), glv_input);

	if(engine) {
		set_engine(context, engine);
	}

	GLV_IF_DEBUG_IME_INPUT printf("Engine is %s in context_new()\n",ibus_engine_desc_get_name(ibus_input_context_get_engine(context)));

	//printf("Engine is %s in context_new()\n",ibus_engine_desc_get_name(ibus_input_context_get_engine(context)));

	return(context);
}

static void connected(IBusBus *bus, gpointer data)
{
	im_ibus_t *ibus;

	if(bus != ibus_bus || !ibus_bus_is_connected(ibus_bus) || !add_event_source(data)) {
		return;
	}

	for (ibus = ibus_list; ibus; ibus = glv_list_next(ibus_list)) {
		ibus->context = context_new(data,ibus, NULL);
	}
}

static void disconnected(IBusBus *bus, gpointer data)
{
	im_ibus_t *ibus;

	if(bus != ibus_bus) {
		return;
	}

	remove_event_source(data);

	for (ibus = ibus_list; ibus; ibus = glv_list_next(ibus_list)) {
		ibus_proxy_destroy((IBusProxy*)ibus->context);

		ibus->context = NULL;
		ibus->is_enabled = FALSE;
	}
}

void glv_ime_startIbus(struct _glvinput *glv_input)
{
	static int is_init=0;
	im_ibus_t *ibus = NULL;

	if(!is_init) {
		ibus_init();
		/* Don't call ibus_init() again if ibus daemon is not found below. */
		is_init = 1;
	}

	ibus_bus = ibus_bus_new();

	if(!ibus_bus_is_connected(ibus_bus)) {
		printf("glv_ime_startIbus: IBus daemon is not found.\n");
		return;
	}

	if(!add_event_source(glv_input)) {
		printf("glv_ime_startIbus: add_event_source error.\n");
		return;
	}

	g_signal_connect(ibus_bus, "connected", G_CALLBACK(connected), glv_input);
	g_signal_connect(ibus_bus, "disconnected", G_CALLBACK(disconnected), glv_input);

	if(!(ibus = calloc(1, sizeof(im_ibus_t)))) {
		printf("glv_ime_startIbus: calloc failed.\n");
		return;
	}
    char *engine = "(null)";

	if(!(ibus->context = context_new(glv_input,ibus, engine))) {
		printf("glv_ime_startIbus: context_new failed.\n");
		return;
	}
	ibus->is_enabled = FALSE;

	glv_input->im = ibus;

 	glv_list_insert_head(ibus_list, ibus);

	GLV_IF_DEBUG_IME_INPUT printf("glv_ime_startIbus:start.\n");
}

int glvWiget_setIMECandidatePotition(glvWiget wiget,int candidate_pos_x,int candidate_pos_y)
{
    GLV_WIGET_t *glv_wiget=(GLV_WIGET_t*)wiget;
    GLV_WINDOW_t *glv_window = glv_wiget->glv_sheet->glv_window;
	int ibus_candidate_x,ibus_candidate_y;

	if(ibus_preedit == NULL){
		return(GLV_ERROR);
	}

	ibus_candidate_x = glv_window->absolute_x + glv_wiget->sheet_x + candidate_pos_x;
	ibus_candidate_y = glv_window->absolute_y + glv_wiget->sheet_y + candidate_pos_y;

	ibus_input_context_set_cursor_location_relative(ibus_preedit->context,ibus_candidate_x,ibus_candidate_y, 0, 0);

	return(GLV_OK);
}
