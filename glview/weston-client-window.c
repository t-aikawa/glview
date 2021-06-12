/*
 * Copyright © 2008 Kristian Høgsberg
 * Copyright © 2012-2013 Collabora, Ltd.
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

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>

#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <xkbcommon/xkbcommon.h>

#include "xdg-shell-client-protocol.h"
#include "xdg-shell-unstable-v6-client-protocol.h"
#include "ivi-application-client-protocol.h"

#include "glview.h"
#include "weston-client-window.h"
#include "glview_local.h"

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

/*
 * The following correspondences between file names and cursors was copied
 * from: https://bugs.kde.org/attachment.cgi?id=67313
 */

static const char *bottom_left_corners[] = {
	"bottom_left_corner",
	"sw-resize",
	"size_bdiag"
};

static const char *bottom_right_corners[] = {
	"bottom_right_corner",
	"se-resize",
	"size_fdiag"
};

static const char *bottom_sides[] = {
	"bottom_side",
	"s-resize",
	"size_ver"
};

static const char *grabbings[] = {
	"grabbing",
	"closedhand",
	"208530c400c041818281048008011002"
};

static const char *left_ptrs[] = {
	"left_ptr",
	"default",
	"top_left_arrow",
	"left-arrow"
};

static const char *left_sides[] = {
	"left_side",
	"w-resize",
	"size_hor"
};

static const char *right_sides[] = {
	"right_side",
	"e-resize",
	"size_hor"
};

static const char *top_left_corners[] = {
	"top_left_corner",
	"nw-resize",
	"size_fdiag"
};

static const char *top_right_corners[] = {
	"top_right_corner",
	"ne-resize",
	"size_bdiag"
};

static const char *top_sides[] = {
	"top_side",
	"n-resize",
	"size_ver"
};

static const char *xterms[] = {
	"xterm",
	"ibeam",
	"text"
};

static const char *hand1s[] = {
	"hand1",
	"pointer",
	"pointing_hand",
	"e29285e634086352946a0e7090d73106"
};

static const char *watches[] = {
	"watch",
	"wait",
	"0426c94ea35c87780ff01dc239897213"
};

static const char *move_draggings[] = {
	"dnd-move"
};

static const char *copy_draggings[] = {
	"dnd-copy"
};

static const char *forbidden_draggings[] = {
	"dnd-none",
	"dnd-no-drop"
};

static const char *h_double_arrow[] = {
	"sb_h_double_arrow",
	"h_double_arrow"
};

struct cursor_alternatives {
	const char **names;
	size_t count;
};

static const struct cursor_alternatives cursors[] = {
	{bottom_left_corners, ARRAY_LENGTH(bottom_left_corners)},
	{bottom_right_corners, ARRAY_LENGTH(bottom_right_corners)},
	{bottom_sides, ARRAY_LENGTH(bottom_sides)},
	{grabbings, ARRAY_LENGTH(grabbings)},
	{left_ptrs, ARRAY_LENGTH(left_ptrs)},
	{left_sides, ARRAY_LENGTH(left_sides)},
	{right_sides, ARRAY_LENGTH(right_sides)},
	{top_left_corners, ARRAY_LENGTH(top_left_corners)},
	{top_right_corners, ARRAY_LENGTH(top_right_corners)},
	{top_sides, ARRAY_LENGTH(top_sides)},
	{xterms, ARRAY_LENGTH(xterms)},
	{hand1s, ARRAY_LENGTH(hand1s)},
	{watches, ARRAY_LENGTH(watches)},
	{move_draggings, ARRAY_LENGTH(move_draggings)},
	{copy_draggings, ARRAY_LENGTH(copy_draggings)},
	{forbidden_draggings, ARRAY_LENGTH(forbidden_draggings)},
	{h_double_arrow, ARRAY_LENGTH(h_double_arrow)},
};

static void input_set_pointer_image_index(struct _glvinput *glv_input, int index)
{
	struct wl_buffer *buffer;
	struct wl_cursor *cursor;
	struct wl_cursor_image *image;

	if (glv_input->pointer == NULL){
		return;
	}

	cursor = glv_input->wl_dpy->cursors[glv_input->current_cursor];
	if (cursor == NULL){
		return;
	}

	if (index >= (int) cursor->image_count) {
		fprintf(stderr, "input_set_pointer_image_index:cursor index out of range\n");
		return;
	}

	image = cursor->images[index];
	buffer = wl_cursor_image_get_buffer(image);
	if (buffer == NULL){
		return;
	}

	wl_surface_attach(glv_input->pointer_surface, buffer, 0, 0);
	wl_surface_damage(glv_input->pointer_surface, 0, 0,image->width, image->height);
	wl_surface_commit(glv_input->pointer_surface);
	wl_pointer_set_cursor(glv_input->pointer, glv_input->pointer_enter_serial,
			      glv_input->pointer_surface,image->hotspot_x, image->hotspot_y);
	//printf("wl_pointer_set_cursor\n");
}

void weston_client_window__input_set_pointer_image(struct _glvinput *input, int cursor)
{
// printf("weston_client_window__input_set_pointer_image cursor = %d serial(%d > %d) current_cursor = %d\n",cursor,input->pointer_enter_serial,input->cursor_serial,input->current_cursor);
	if (input->pointer == NULL){
		return;
	}
	if((input->pointer_enter_serial <= input->cursor_serial) &&
		(cursor == input->current_cursor)){
		return;
	}
	//printf("weston_client_window__input_set_pointer_image cursor = %d\n",cursor);

	input->current_cursor = cursor;
	input->cursor_serial = input->pointer_enter_serial;

	input_set_pointer_image_index(input, 0);
}

void weston_client_window__create_cursors(WL_DISPLAY_t *display)
{
	int size = 32;
	unsigned int i, j;
	struct wl_cursor *cursor;

	display->cursor_theme = wl_cursor_theme_load(NULL, size, display->shm);
	if (!display->cursor_theme) {
		fprintf(stderr, "could not load cursor theme.\n");
		return;
	}
	display->cursors = calloc(1,ARRAY_LENGTH(cursors) * sizeof display->cursors[0]);

	for (i = 0; i < ARRAY_LENGTH(cursors); i++) {
		cursor = NULL;
		for (j = 0; !cursor && j < cursors[i].count; ++j)
			cursor = wl_cursor_theme_get_cursor(
			    display->cursor_theme, cursors[i].names[j]);

		if (!cursor)
			fprintf(stderr, "could not load cursor '%s'\n",
				cursors[i].names[0]);

		display->cursors[i] = cursor;
	}
}

void weston_client_window__display_watch_fd(struct _glv_display *display,
		 int fd, uint32_t events, struct task *task)
{
	struct epoll_event ep;

	ep.events = events;
	ep.data.ptr = task;
	epoll_ctl(display->epoll_fd, EPOLL_CTL_ADD, fd, &ep);
}

void weston_client_window__display_unwatch_fd(struct _glv_display *display, int fd)
{
	epoll_ctl(display->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void weston_client_window__display_exit(struct _glv_display *display)
{
	display->running = 0;
}

static void handle_display_data(struct task *task, uint32_t events)
{
	struct _glv_display *display = wl_container_of(task, display, display_task);
	struct epoll_event ep;
	int ret;
	struct wl_display	*wl_display = ((GLV_DISPLAY_t*)display)->wl_dpy.display;

	display->display_fd_events = events;

	if (events & EPOLLERR || events & EPOLLHUP) {
		weston_client_window__display_exit(display);
		return;
	}

	if (events & EPOLLIN) {
		ret = wl_display_dispatch(wl_display);
		if (ret == -1) {
			weston_client_window__display_exit(display);
			return;
		}
	}

	if (events & EPOLLOUT) {
		ret = wl_display_flush(wl_display);
		if (ret == 0) {
			ep.events = EPOLLIN | EPOLLERR | EPOLLHUP;
			ep.data.ptr = &display->display_task;
			epoll_ctl(display->epoll_fd, EPOLL_CTL_MOD,
				  display->display_fd, &ep);
		} else if (ret == -1 && errno != EAGAIN) {
			weston_client_window__display_exit(display);
			return;
		}
	}
}

int weston_shared_os_compatibility__os_fd_set_cloexec(int fd)
{
	long flags;

	if (fd == -1)
		return -1;

	flags = fcntl(fd, F_GETFD);
	if (flags == -1)
		return -1;

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
		return -1;

	return 0;
}

static int weston_shared_os_compatibility__set_cloexec_or_close(int fd)
{
	if (weston_shared_os_compatibility__os_fd_set_cloexec(fd) != 0) {
		close(fd);
		return -1;
	}
	return fd;
}

static int weston_shared_os_compatibility__os_epoll_create_cloexec(void)
{
	int fd;

#ifdef EPOLL_CLOEXEC
	fd = epoll_create1(EPOLL_CLOEXEC);
	if (fd >= 0)
		return fd;
	if (errno != EINVAL)
		return -1;
#endif

	fd = epoll_create(1);
	return weston_shared_os_compatibility__set_cloexec_or_close(fd);
}

void weston_client_window__display_create(struct _glv_display *display)
{
	struct wl_display	*wl_display = ((GLV_DISPLAY_t*)display)->wl_dpy.display;

	display->epoll_fd = weston_shared_os_compatibility__os_epoll_create_cloexec();
	display->display_fd = wl_display_get_fd(wl_display);
	display->display_task.run = handle_display_data;
	weston_client_window__display_watch_fd(display, display->display_fd, EPOLLIN | EPOLLERR | EPOLLHUP,
			 &display->display_task);
}

void weston_client_window__display_destroy(struct _glv_display *display)
{
	struct wl_display	*wl_display = ((GLV_DISPLAY_t*)display)->wl_dpy.display;
	close(display->epoll_fd);

	if (!(display->display_fd_events & EPOLLERR) &&
	    !(display->display_fd_events & EPOLLHUP))
		wl_display_flush(wl_display);
}

void weston_client_window__display_run(struct _glv_display *display)
{
	struct task *task;
	struct epoll_event ep[16];
	int i, count, ret;
	struct wl_display	*wl_display = ((GLV_DISPLAY_t*)display)->wl_dpy.display;

	display->running = 1;
	while (1) {
		_glvWindowMsgHandler_dispatch(display);
		_glvGcGarbageBox();

		wl_display_dispatch_pending(wl_display);

		if (!display->running)
			break;

		ret = wl_display_flush(wl_display);
		if (ret < 0 && errno == EAGAIN) {
			ep[0].events =
				EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
			ep[0].data.ptr = &display->display_task;

			epoll_ctl(display->epoll_fd, EPOLL_CTL_MOD,
				  display->display_fd, &ep[0]);
		} else if (ret < 0) {
			break;
		}

		count = epoll_wait(display->epoll_fd,
				   ep, ARRAY_LENGTH(ep), -1);
		for (i = 0; i < count; i++) {
			task = ep[i].data.ptr;
			task->run(task, ep[i].events);
		}
	}
}
