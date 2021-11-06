/*
 * Copyright © 2011 Benjamin Franzke
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
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>

#include "glview.h"

static glvWindow	simple_egl_frame = NULL;
static glvInstanceId simple_egl_frame_id = 0;
static glvWindow	simple_egl_window = NULL;

struct geometry {
	int width, height;
};

typedef struct window {
	struct geometry geometry;
	struct {
		GLuint rotation_uniform;
		GLuint pos;
		GLuint col;
		GLuint program;
	} gl;
	uint32_t benchmark_time, frames;
	int delay;
}SIMPLE_EGL_WINDOW_USER_DATA_t;

static const char *vert_shader_text =
	"uniform mat4 rotation;\n"
	"attribute vec4 pos;\n"
	"attribute vec4 color;\n"
	"varying vec4 v_color;\n"
	"void main() {\n"
	"  gl_Position = rotation * pos;\n"
	"  v_color = color;\n"
	"}\n";

static const char *frag_shader_text =
	"precision mediump float;\n"
	"varying vec4 v_color;\n"
	"void main() {\n"
	"  gl_FragColor = v_color;\n"
	"}\n";

static GLuint create_shader(struct window *window, const char *source, GLenum shader_type)
{
	GLuint shader;
	GLint status;

	shader = glCreateShader(shader_type);
	assert(shader != 0);

	glShaderSource(shader, 1, (const char **) &source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetShaderInfoLog(shader, 1000, &len, log);
		fprintf(stderr, "Error: compiling %s: %.*s\n",
			shader_type == GL_VERTEX_SHADER ? "vertex" : "fragment",
			len, log);
		exit(1);
	}

	return shader;
}

static void init_gl(struct window *window)
{
	GLuint frag, vert;
	GLuint program;
	GLint status;

	frag = create_shader(window, frag_shader_text, GL_FRAGMENT_SHADER);
	vert = create_shader(window, vert_shader_text, GL_VERTEX_SHADER);

	program = glCreateProgram();
	window->gl.program = program;

	glAttachShader(program, frag);
	glAttachShader(program, vert);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		char log[1000];
		GLsizei len;
		glGetProgramInfoLog(program, 1000, &len, log);
		fprintf(stderr, "Error: linking:\n%.*s\n", len, log);
		exit(1);
	}

	glUseProgram(program);

	window->gl.pos = 0;
	window->gl.col = 1;

	glBindAttribLocation(program, window->gl.pos, "pos");
	glBindAttribLocation(program, window->gl.col, "color");
	glLinkProgram(program);

	window->gl.rotation_uniform = glGetUniformLocation(program, "rotation");
}

static void redraw(glvWindow glv_win)
{
	struct window *window = glv_getUserData(glv_win);
	uint32_t time;
	static const GLfloat verts[3][2] = {
		{ -0.5, -0.5 },
		{  0.5, -0.5 },
		{  0,    0.5 }
	};
	static const GLfloat colors[3][3] = {
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 }
	};
	GLfloat angle;
	GLfloat rotation[4][4] = {
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
		{ 0, 0, 0, 1 }
	};
	static const uint32_t speed_div = 5, benchmark_interval = 5;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	if (window->frames == 0)
		window->benchmark_time = time;
	if (time - window->benchmark_time > (benchmark_interval * 1000)) {
		printf("simple-egl[%s]: %d frames in %d seconds: %f fps\n",glvWindow_getWindowName(glv_win),
		       window->frames,
		       benchmark_interval,
		       (float) window->frames / benchmark_interval);
		window->benchmark_time = time;
		window->frames = 0;
	}

	angle = (time / speed_div) % 360 * M_PI / 180.0;
	rotation[0][0] =  cos(angle);
	rotation[0][2] =  sin(angle);
	rotation[2][0] = -sin(angle);
	rotation[2][2] =  cos(angle);

	glViewport(0, 0, window->geometry.width, window->geometry.height);

	glUniformMatrix4fv(window->gl.rotation_uniform, 1, GL_FALSE,(GLfloat *) rotation);

	glClearColor(0.0, 0.0, 0.0, 0.5);
	glClear(GL_COLOR_BUFFER_BIT);

	glVertexAttribPointer(window->gl.pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
	glVertexAttribPointer(window->gl.col, 3, GL_FLOAT, GL_FALSE, 0, colors);
	glEnableVertexAttribArray(window->gl.pos);
	glEnableVertexAttribArray(window->gl.col);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisableVertexAttribArray(window->gl.pos);
	glDisableVertexAttribArray(window->gl.col);

	usleep(window->delay);

	window->frames++;
}

// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------
static int simple_egl_window_init(glvWindow glv_win,int width, int height)
{
	glv_allocUserData(glv_win,sizeof(struct window));
	struct window *window = glv_getUserData(glv_win);

	window->geometry.width 	= width;
	window->geometry.height	= height;

	init_gl(window);

	return(GLV_OK);
}

static int simple_egl_window_reshape(glvWindow glv_win,int width, int height)
{
	struct window *window = glv_getUserData(glv_win);

	glViewport(0, 0, width, height);

	window->geometry.width 	= width;
	window->geometry.height	= height;

	return(GLV_OK);
}

static int simple_egl_window_redraw(glvWindow glv_win,int drawStat)
{
    redraw(glv_win);
	return(GLV_OK);
}

static int simple_egl_window_update(glvWindow glv_win,int drawStat)
{
    redraw(glv_win);
	glvReqSwapBuffers(glv_win);
	return(GLV_OK);
}

static int simple_egl_window_endDraw(glvWindow glv_win,glvTime time)
{
	glvOnReDraw(glv_win);
	return(GLV_OK);
}

static int simple_egl_sub_window_terminate(glvWindow glv_win)
{
	struct window *window = glv_getUserData(glv_win);
	printf("simple_egl_window_terminate\n");

	glUseProgram(0);

	if(window->gl.program != 0){
		glDeleteProgram(window->gl.program);
	}

	return(GLV_OK);
}

static const struct glv_window_listener _simple_egl_sub_window_listener = {
	.init		= simple_egl_window_init,
	.reshape	= simple_egl_window_reshape,
	.redraw		= simple_egl_window_redraw,
	.update 	= simple_egl_window_update,
	.timer		= NULL,
	.gesture	= NULL,
	.userMsg	= NULL,
	.terminate	= simple_egl_sub_window_terminate,
	.endDraw	= simple_egl_window_endDraw,
	.beauty		= 0,
};
const struct glv_window_listener *simple_egl_sub_window_listener = &_simple_egl_sub_window_listener;

static int simple_egl_main_window_terminate(glvWindow glv_win)
{
	extern glvWindow	glv_main_window;
	struct window *window = glv_getUserData(glv_win);
	printf("simple_egl_window_terminate\n");

	glUseProgram(0);

	if(window->gl.program != 0){
		glDeleteProgram(window->gl.program);
	}

	return(GLV_OK);
}

static const struct glv_window_listener _simple_egl_main_window_listener = {
	.init		= simple_egl_window_init,
	.reshape	= simple_egl_window_reshape,
	.redraw		= simple_egl_window_redraw,
	.update 	= simple_egl_window_update,
	.timer		= NULL,
	.gesture	= NULL,
	.userMsg	= NULL,
	.terminate	= simple_egl_main_window_terminate,
	.endDraw	= simple_egl_window_endDraw,
	.beauty		= 0,
};
const struct glv_window_listener *simple_egl_main_window_listener = &_simple_egl_main_window_listener;

int simple_egl_frame_start(glvWindow glv_frame_window,int width, int height)
{
	printf("sub_frame_start [%s] width = %d , height = %d\n",glvWindow_getWindowName(glv_frame_window),width,height);
	glvCreateWindow(glv_frame_window,simple_egl_sub_window_listener,&simple_egl_window,"simple-egl-sub-window",
			0, 0, width, height,GLV_WINDOW_ATTR_DEFAULT);
	glvOnReDraw(simple_egl_window);
	return(GLV_OK);
}

static int simple_egl_frame_configure(glvWindow glv_win,int width, int height)
{
	//printf("simple_egl__frame_configure width = %d , height = %d\n",width,height);
	glvOnReShape(simple_egl_window,-1,-1,width,height);
	return(GLV_OK);
}

static int simple_egl_frame_terminate(glvWindow glv_win)
{
	printf("simple_egl_frame_terminate\n");
	return(GLV_OK);
}

static const struct glv_frame_listener _simple_egl_frame_window_listener = {
	.start		= simple_egl_frame_start,
//	.configure	= simple_egl_frame_configure,
	.terminate	= simple_egl_frame_terminate,
	.back		= GLV_FRAME_BACK_DRAW_OFF,
	.shadow		= GLV_FRAME_SHADOW_DRAW_ON,
};
static const struct glv_frame_listener *simple_egl_frame_window_listener = &_simple_egl_frame_window_listener;

void simple_egl_window_test(glvWindow glv_frame_window)
{
	int window_width  = 400;
	int winodw_height = 400;

	if(glvWindow_isAliveWindow(glv_frame_window,simple_egl_frame_id) == GLV_INSTANCE_DEAD){
		simple_egl_frame_id = glvCreateFrameWindow(glv_frame_window,simple_egl_frame_window_listener,&simple_egl_frame,"simple-egl-frame","simple egl",window_width, winodw_height);
		printf("simple_egl_frame create\n");
	}else{
		glvDestroyWindow(&simple_egl_window);
		glvDestroyWindow(&simple_egl_frame);
		printf("simple_egl_frame delete\n");
	}
}
