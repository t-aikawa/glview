/*
 * Copyright © 2010 Kristian Høgsberg
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

// modified by　weston-9.0.0/clients/smoke.c

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#include "glview.h"

#define APP_VERSION_TEXT	"Version 0.1.3 (" __DATE__ ")"
#define APP_TITLE_TEXT		"smoke"
#define APP_NAME_TEXT		APP_TITLE_TEXT " " APP_VERSION_TEXT

typedef struct smoke {
	int		width, height;
	float	scale_x, scale_y;
	int		current;
	struct { float *d, *u, *v; } b[2];
}SMOKE_WINDOW_USER_DATA_t;

static void diffuse(struct smoke *smoke, uint32_t time,
		    float *source, float *dest)
{
	float *s, *d;
	int x, y, k, stride;
	float t, a = 0.0002;

	stride = smoke->width;

	for (k = 0; k < 5; k++) {
		for (y = 1; y < smoke->height - 1; y++) {
			s = source + y * stride;
			d = dest + y * stride;
			for (x = 1; x < smoke->width - 1; x++) {
				t = d[x - 1] + d[x + 1] +
					d[x - stride] + d[x + stride];
				d[x] = (s[x] + a * t) / (1 + 4 * a) * 0.995;
			}
		}
	}
}

static void advect(struct smoke *smoke, uint32_t time,
		   float *uu, float *vv, float *source, float *dest)
{
	float *s, *d;
	float *u, *v;
	int x, y, stride;
	int i, j;
	float px, py, fx, fy;

	stride = smoke->width;

	for (y = 1; y < smoke->height - 1; y++) {
		d = dest + y * stride;
		u = uu + y * stride;
		v = vv + y * stride;

		for (x = 1; x < smoke->width - 1; x++) {
			px = x - u[x];
			py = y - v[x];
			if (px < 0.5)
				px = 0.5;
			if (py < 0.5)
				py = 0.5;
			if (px > smoke->width - 1.5)
				px = smoke->width - 1.5;
			if (py > smoke->height - 1.5)
				py = smoke->height - 1.5;
			i = (int) px;
			j = (int) py;
			fx = px - i;
			fy = py - j;
			s = source + j * stride + i;
			d[x] = (s[0] * (1 - fx) + s[1] * fx) * (1 - fy) +
				(s[stride] * (1 - fx) + s[stride + 1] * fx) * fy;
		}
	}
}

static void project(struct smoke *smoke, uint32_t time,
		    float *u, float *v, float *p, float *div)
{
	int x, y, k, l, s;
	float h;

	h = 1.0 / smoke->width;
	s = smoke->width;
	memset(p, 0, smoke->height * smoke->width);
	for (y = 1; y < smoke->height - 1; y++) {
		l = y * s;
		for (x = 1; x < smoke->width - 1; x++) {
			div[l + x] = -0.5 * h * (u[l + x + 1] - u[l + x - 1] +
						 v[l + x + s] - v[l + x - s]);
			p[l + x] = 0;
		}
	}

	for (k = 0; k < 5; k++) {
		for (y = 1; y < smoke->height - 1; y++) {
			l = y * s;
			for (x = 1; x < smoke->width - 1; x++) {
				p[l + x] = (div[l + x] +
					    p[l + x - 1] +
					    p[l + x + 1] +
					    p[l + x - s] +
					    p[l + x + s]) / 4;
			}
		}
	}

	for (y = 1; y < smoke->height - 1; y++) {
		l = y * s;
		for (x = 1; x < smoke->width - 1; x++) {
			u[l + x] -= 0.5 * (p[l + x + 1] - p[l + x - 1]) / h;
			v[l + x] -= 0.5 * (p[l + x + s] - p[l + x - s]) / h;
		}
	}
}

static void render(glvWindow glv_win)
{
	struct smoke *smoke = glv_getUserData(glv_win);
	unsigned char *dest;
	int x, y, width, height, stride;
	float *s;
	uint32_t *d, c, a;
	int size;

	size = smoke->height * smoke->width;
	dest = calloc(size, sizeof(uint32_t));
	stride = smoke->width * sizeof(uint32_t);

	width = smoke->width;
	height = smoke->height;

	for (y = 1; y < height - 1; y++) {
		s = smoke->b[smoke->current].d + y * smoke->height;
		d = (uint32_t *) (dest + y * stride);
		for (x = 1; x < width - 1; x++) {
			c = (int) (s[x] * 800);
			if (c > 255)
				c = 255;
			a = c;
			if (a < 0x33)
				a = 0x33;
			d[x] = (a << 24) | (c << 16) | (c << 8) | c;
		}
	}
	uint32_t textureID;
	textureID = glvGl_GenTextures(dest,smoke->width,smoke->height);
	if(0 != textureID){
		glvGl_DrawTexturesEx(textureID,
			0.0, 0.0,
			(float)smoke->width  * smoke->scale_x,
			(float)smoke->height * smoke->scale_y,
			0.0, 0.0,
			0.0);

		// テクスチャ解放
		glvGl_DeleteTextures(&textureID);
	}
	free(dest);
}

static void redraw(glvWindow glv_win)
{
	struct smoke *smoke = glv_getUserData(glv_win);
	glvTime time = glvWindow_getLastTime(glv_win);

	diffuse(smoke, time / 30, smoke->b[0].u, smoke->b[1].u);
	diffuse(smoke, time / 30, smoke->b[0].v, smoke->b[1].v);
	project(smoke, time / 30,
		smoke->b[1].u, smoke->b[1].v,
		smoke->b[0].u, smoke->b[0].v);
	advect(smoke, time / 30,
	       smoke->b[1].u, smoke->b[1].v,
	       smoke->b[1].u, smoke->b[0].u);
	advect(smoke, time / 30,
	       smoke->b[1].u, smoke->b[1].v,
	       smoke->b[1].v, smoke->b[0].v);
	project(smoke, time / 30,
		smoke->b[0].u, smoke->b[0].v,
		smoke->b[1].u, smoke->b[1].v);

	diffuse(smoke, time / 30, smoke->b[0].d, smoke->b[1].d);
	advect(smoke, time / 30,
	       smoke->b[0].u, smoke->b[0].v,
	       smoke->b[1].d, smoke->b[0].d);

	render(glv_win);
}

static void smoke_motion_handler(struct smoke *smoke, float x, float y)
{
	int i, i0, i1, j, j0, j1, k, d = 5;

	if (x - d < 1)
		i0 = 1;
	else
		i0 = x - d;
	if (i0 + 2 * d > smoke->width - 1)
		i1 = smoke->width - 1;
	else
		i1 = i0 + 2 * d;

	if (y - d < 1)
		j0 = 1;
	else
		j0 = y - d;
	if (j0 + 2 * d > smoke->height - 1)
		j1 = smoke->height - 1;
	else
		j1 = j0 + 2 * d;

	for (i = i0; i < i1; i++)
		for (j = j0; j < j1; j++) {
			k = j * smoke->width + i;
			smoke->b[0].u[k] += 256 - (random() & 512);
			smoke->b[0].v[k] += 256 - (random() & 512);
			smoke->b[0].d[k] += 1;
		}
}

static int smoke_window_init(glvWindow glv_win,int width, int height)
{
	struct timespec ts;
	int size;
	glv_allocUserData(glv_win,sizeof(struct smoke));
	struct smoke *smoke = glv_getUserData(glv_win);

	glvGl_init();
	glvWindow_setViewport(glv_win,width,height);

	smoke->width = width;
	smoke->height = height;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	srandom(ts.tv_nsec);

	smoke->current = 0;
	size = smoke->height * smoke->width;
	smoke->b[0].d = calloc(size, sizeof(float));
	smoke->b[0].u = calloc(size, sizeof(float));
	smoke->b[0].v = calloc(size, sizeof(float));
	smoke->b[1].d = calloc(size, sizeof(float));
	smoke->b[1].u = calloc(size, sizeof(float));
	smoke->b[1].v = calloc(size, sizeof(float));

	smoke->scale_x = (float)width  / (float)smoke->width;
	smoke->scale_y = (float)height / (float)smoke->height;

	return(GLV_OK);
}

static int smoke_window_reshape(glvWindow glv_win,int width, int height)
{
	struct smoke *smoke = glv_getUserData(glv_win);

	glvWindow_setViewport(glv_win,width,height);

	smoke->scale_x = (float)width  / (float)smoke->width;
	smoke->scale_y = (float)height / (float)smoke->height;

	return(GLV_OK);
}

static int smoke_window_mousePointer(glvWindow glv_win,int glv_mouse_event_type,glvTime glv_mouse_event_time,int glv_mouse_event_x,int glv_mouse_event_y,int pointer_stat)
{
	struct smoke *smoke = glv_getUserData(glv_win);
	//printf("smoke_window_mousePointer %d,%d enevt %d,%d\n",glv_mouse_event_x,glv_mouse_event_y,glv_mouse_event_type,pointer_stat);
	smoke_motion_handler(smoke,glv_mouse_event_x / smoke->scale_x,glv_mouse_event_y / smoke->scale_y);
	return(GLV_OK);
}

static int smoke_window_redraw(glvWindow glv_win,int drawStat)
{
    redraw(glv_win);
	return(GLV_OK);
}

static int smoke_window_update(glvWindow glv_win,int drawStat)
{
    redraw(glv_win);
	glvReqSwapBuffers(glv_win);
	return(GLV_OK);
}

static int smoke_window_endDraw(glvWindow glv_win,glvTime time)
{
	glvOnReDraw(glv_win);
	return(GLV_OK);
}

static int smoke_window_terminate(glvWindow glv_win)
{
	struct smoke *smoke = glv_getUserData(glv_win);
	printf("smoke_window_terminate\n");

	free(smoke->b[0].d);
	free(smoke->b[0].u);
	free(smoke->b[0].v);
	free(smoke->b[1].d);
	free(smoke->b[1].u);
	free(smoke->b[1].v);

	return(GLV_OK);
}

static int smoke_window_cursor(glvWindow glv_win,int width, int height,int pos_x,int pos_y)
{
	return(CURSOR_HAND1);
}

static const struct glv_window_listener _smoke_window_listener = {
	.init			= smoke_window_init,
	.reshape		= smoke_window_reshape,
	.redraw			= smoke_window_redraw,
	.update 		= smoke_window_update,
	.mousePointer	= smoke_window_mousePointer,
	.terminate		= smoke_window_terminate,
	.endDraw		= smoke_window_endDraw,
	.cursor			= smoke_window_cursor,
	.beauty			= 0,
};
const struct glv_window_listener *smoke_window_listener = &_smoke_window_listener;

static const struct glv_frame_listener _main_frame_window_listener = {
	.back	= GLV_FRAME_BACK_DRAW_OFF,
};
static const struct glv_frame_listener *main_frame_window_listener = &_main_frame_window_listener;

// ==============================================================================================
// ==============================================================================================
// ==============================================================================================
int main(int argc, char *argv[])
{
	glvDisplay	glv_dpy;
	glvWindow	glv_frame_window = NULL;
	glvWindow	glv_main_window = NULL;
	int WinWidth = 400, WinHeight = 400;

	fprintf(stdout,"%s\n",APP_NAME_TEXT);

	glv_dpy = glvOpenDisplay(NULL);
	if(!glv_dpy){
		fprintf(stderr,"Error: glvOpenDisplay() failed\n");
		return(-1);
	}

	glvCreateFrameWindow(glv_dpy,main_frame_window_listener,&glv_frame_window,"smoke frame",APP_NAME_TEXT,WinWidth, WinHeight);

	glvCreateWindow(glv_frame_window,smoke_window_listener,&glv_main_window,"smoke window",
			0, 0, WinWidth, WinHeight,GLV_WINDOW_ATTR_DEFAULT | GLV_WINDOW_ATTR_POINTER_MOTION);

	glvOnReDraw(glv_main_window);

	/* ----------------------------------------------------------------------------------------------- */
	glvEnterEventLoop(glv_dpy);		// event loop
	/* ----------------------------------------------------------------------------------------------- */

	// 	終了処理

	glvDestroyWindow(&glv_main_window);

	glvDestroyWindow(&glv_frame_window);

	glvCloseDisplay(glv_dpy);

	printf("all terminated.\n");

	return(0);
}
