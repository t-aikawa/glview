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

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <xkbcommon/xkbcommon.h>

#include <wayland-client.h>
#include <wayland-util.h>
#include "glview.h"
#include "weston-client-window.h"
#include "glview_local.h"

glvInstanceId _glv_instance_Id = 0;

void _glv_sheet_with_wiget_reshape_cb(GLV_WINDOW_t *glv_window)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		glv_sheet->reqDrawWigetsFlag = 0;
		if(glv_sheet->eventFunc.reshape){
			int rc;
			rc = (glv_sheet->eventFunc.reshape)((glvWindow)glv_window,(glvSheet)glv_sheet,glv_window->width,glv_window->height);
			if(rc != GLV_OK){
				fprintf(stderr,"_glv_sheet_with_wiget_reshape_cb:sheet reshape error\n");
			}
			if(glv_sheet->reqDrawWigetsFlag == 1){
				glv_sheet->reqDrawWigetsFlag = 0;
				wl_list_for_each(glv_wiget, &glv_sheet->wiget_list, link){
					//printf("2 _glv_sheet_with_wiget_reshape_cb %s\n",glv_window->name);
					if((glv_wiget->instance.alive == GLV_INSTANCE_ALIVE) && (glv_wiget->visible == GLV_VISIBLE) && (glv_wiget->eventFunc.redraw != NULL)){
						//printf("_glv_sheet_with_wiget_reshape_cb: redraw wiget id = %ld\n",glv_wiget->instance.Id);
						rc = (glv_wiget->eventFunc.redraw)((glvWindow)glv_window,(glvSheet)glv_sheet,(glvWiget)glv_wiget);
						if(rc != GLV_OK){
							fprintf(stderr,"_glv_sheet_with_wiget_reshape_cb:wiget reshape error\n");
						}	
					}
				}
			}
		}
	}
}

void _glv_sheet_with_wiget_redraw_cb(GLV_WINDOW_t *glv_window)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		if(glv_sheet->eventFunc.redraw){
			rc = (glv_sheet->eventFunc.redraw)((glvWindow)glv_window,(glvSheet)glv_sheet,GLV_STAT_DRAW_REDRAW);
			if(rc != GLV_OK){
				fprintf(stderr,"_glv_sheet_with_wiget_redraw_cb:sheet redraw error\n");
			}
		}else
		if(glv_sheet->eventFunc.update){
			rc = (glv_sheet->eventFunc.update)((glvWindow)glv_window,(glvSheet)glv_sheet,GLV_STAT_DRAW_REDRAW);
			if(rc != GLV_OK){
				fprintf(stderr,"_glv_sheet_with_wiget_redraw_cb:sheet update error\n");
			}
		}
		// -------------------------------------------------------
		wl_list_for_each(glv_wiget, &glv_sheet->wiget_list, link){
			if((glv_wiget->instance.alive == GLV_INSTANCE_ALIVE) && (glv_wiget->visible == GLV_VISIBLE) && (glv_wiget->eventFunc.redraw != NULL)){
				//printf("_glv_sheet_with_wiget_redraw_cb: redraw wiget id = %ld\n",glv_wiget->instance.Id);
				rc = (glv_wiget->eventFunc.redraw)((glvWindow)glv_window,(glvSheet)glv_sheet,(glvWiget)glv_wiget);
				if(rc != GLV_OK){
					fprintf(stderr,"_glv_sheet_with_wiget_redraw_cb:wiget redraw error\n");
				}
			}
		}
		// -------------------------------------------------------
	}
}

void _glv_sheet_with_wiget_update_cb(GLV_WINDOW_t *glv_window)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		glv_sheet->reqDrawWigetsFlag = 0;
		if(glv_sheet->eventFunc.update){
			rc = (glv_sheet->eventFunc.update)((glvWindow)glv_window,(glvSheet)glv_sheet,GLV_STAT_DRAW_UPDATE);
			if(rc != GLV_OK){
				fprintf(stderr,"_glv_sheet_with_wiget_update_cb:sheet update error\n");
			}
		}else
		if(glv_sheet->eventFunc.redraw){
			glv_sheet->reqDrawWigetsFlag = 1;
			rc = (glv_sheet->eventFunc.redraw)((glvWindow)glv_window,(glvSheet)glv_sheet,GLV_STAT_DRAW_REDRAW);
			if(rc != GLV_OK){
				fprintf(stderr,"_glv_sheet_with_wiget_update_cb:sheet redraw error\n");
			}
		}
		// -------------------------------------------------------
		if(glv_sheet->reqDrawWigetsFlag == 1){
			glv_sheet->reqDrawWigetsFlag = 0;
			wl_list_for_each(glv_wiget, &glv_sheet->wiget_list, link){
				//printf("2 _glv_sheet_with_wiget_update_cb %s\n",glv_window->name);
				if((glv_wiget->instance.alive == GLV_INSTANCE_ALIVE) && (glv_wiget->visible == GLV_VISIBLE) && (glv_wiget->eventFunc.redraw != NULL)){
					//printf("_glv_sheet_with_wiget_update_cb: redraw wiget id = %ld\n",glv_wiget->instance.Id);
					rc = (glv_wiget->eventFunc.redraw)((glvWindow)glv_window,(glvSheet)glv_sheet,(glvWiget)glv_wiget);
					if(rc != GLV_OK){
						fprintf(stderr,"_glv_sheet_with_wiget_update_cb:wiget redraw error\n");
					}	
				}
			}
		}
		// -------------------------------------------------------
	}
}

void _glv_sheet_userMsg_cb(GLV_WINDOW_t *glv_window,int kind,void *data)
{
	GLV_SHEET_t		*glv_sheet;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		if(glv_sheet->eventFunc.userMsg){
			rc = (glv_sheet->eventFunc.userMsg)((glvWindow)glv_window,(glvSheet)glv_sheet,kind,data);
			if(rc != GLV_OK){
				fprintf(stderr,"_glv_sheet_userMsg_cb:sheet userMsg error\n");
			}
		}
	}
}

void _glv_sheet_action_front(GLV_WINDOW_t *glv_window)
{
	GLV_SHEET_t		*glv_sheet;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		if(glv_sheet->eventFunc.action){
			glvOnAction(glv_sheet,GLV_ACTION_WIGET,glv_sheet->select_wiget_Id);
		}
	}
}

void _glv_sheet_action_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int action,glvInstanceId selectId)
{
	GLV_SHEET_t		*glv_sheet;
//	GLV_WIGET_t		*glv_wiget;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.Id == sheetId) && (glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		if(glv_sheet->eventFunc.action){
			rc = (glv_sheet->eventFunc.action)((glvWindow)glv_window,(glvSheet)glv_sheet,action,selectId);
			if(rc != GLV_OK){
				fprintf(stderr,"_glv_sheet_action_cb:sheet action error\n");
			}
		}
	}
}

void _glv_window_and_sheet_mousePointer_front(GLV_WINDOW_t *glv_window,int type,glvTime time,int x,int y,int pointer_left_stat)
{
	GLV_SHEET_t		*glv_sheet;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;

	if(glv_sheet != NULL){
		if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
			if(glv_sheet->eventFunc.mousePointer){
				_glvOnMousePointer(glv_sheet,type,time,x,y,pointer_left_stat);
			}
		}
	}
	if(glv_window->eventFunc.mousePointer != NULL){
		_glvOnMousePointer(glv_window,type,time,x,y,pointer_left_stat);
	}
}

void _glv_sheet_mousePointer_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,int type,glvTime time,int x,int y,int pointer_left_stat)
{
	GLV_SHEET_t		*glv_sheet;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.Id == sheetId) && (glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		if(glv_sheet->eventFunc.mousePointer){
			rc = (glv_sheet->eventFunc.mousePointer)((glvWindow)glv_window,(glvSheet)glv_sheet,type,time,x,y,pointer_left_stat);
			if(rc != GLV_OK){
				fprintf(stderr,"_glv_sheet_mousePointer_cb:sheet mousePointer error\n");
			}
		}
	}
}

void _glv_wiget_mousePointer_front(struct _glvinput *glv_input,GLV_WINDOW_t *glv_window,int type,glvTime time,int x,int y,int pointer_left_stat)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		// -------------------------------------------------------------------------------
		wl_list_for_each_reverse(glv_wiget, &glv_sheet->wiget_list, link){
			if(glv_wiget->instance.alive == GLV_INSTANCE_ALIVE){
				if((glv_wiget->visible == GLV_VISIBLE) &&
					(glv_wiget->sheet_x < x ) &&
					((glv_wiget->sheet_x + glv_wiget->width) > x) &&
					(glv_wiget->sheet_y < y ) &&
					((glv_wiget->sheet_y + glv_wiget->height) > y)){
					glv_sheet->select_wiget_x = x - glv_wiget->sheet_x;
					glv_sheet->select_wiget_y = y - glv_wiget->sheet_y;
					if(pointer_left_stat == GLV_MOUSE_EVENT_LEFT_PRESS){
						if(glv_wiget->attr & (GLV_WIGET_ATTR_TEXT_INPUT_FOCUS | GLV_WIGET_ATTR_PUSH_ACTION)){
							// kye borad input
							if(glv_input->glv_dpy->kb_input_wigetId != glv_wiget->instance.Id){
								_glvOnFocus((glvDisplay)glv_input->glv_dpy,GLV_STAT_OUT_FOCUS,glv_wiget);
								glv_input->glv_dpy->kb_input_windowId = glv_window->instance.Id;
								glv_input->glv_dpy->kb_input_sheetId  = glv_sheet->instance.Id;
								glv_input->glv_dpy->kb_input_wigetId  = glv_wiget->instance.Id;
								_glvOnFocus((glvDisplay)glv_input->glv_dpy,GLV_STAT_IN_FOCUS,glv_wiget);
							}
						}
					}
					if(	((glv_wiget->attr & GLV_WIGET_ATTR_POINTER_MOTION) == 0) &&
						(type == GLV_MOUSE_EVENT_MOTION) &&
						(pointer_left_stat == GLV_MOUSE_EVENT_LEFT_RELEASE)){
						//  が設定されていない場合は、マウスリリース時のモーションイベントは発生させない
					}else{
						if(glv_wiget->eventFunc.mousePointer){
							//printf("_glv_wiget_mousePointer_front\n");
							//printf("(%d,%d) - wiget (%d,%d)-(%d,%d) \n",x,y,glv_wiget->sheet_x,glv_wiget->sheet_y,glv_wiget->width,glv_wiget->height);
							//printf("glv_sheet->select_wiget_x = %d , glv_sheet->select_wiget_y = %d\n",glv_sheet->select_wiget_x,glv_sheet->select_wiget_y);
							_glvOnMousePointer(glv_wiget,type,time,glv_sheet->select_wiget_x,glv_sheet->select_wiget_y,pointer_left_stat);
						}
					}
					break;
				}
			}
		}
		// -------------------------------------------------------------------------------
	}
}

void _glv_wiget_mousePointer_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int type,glvTime time,int x,int y,int pointer_left_stat)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.Id == sheetId) && (glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		// -------------------------------------------------------------------------------
		wl_list_for_each_reverse(glv_wiget, &glv_sheet->wiget_list, link){
			if(glv_wiget->instance.Id == wigetId){
				if((glv_wiget->instance.alive == GLV_INSTANCE_ALIVE) && (glv_wiget->visible == GLV_VISIBLE)){
					if(glv_wiget->eventFunc.mousePointer){
						//printf("_glv_wiget_mousePointer_cb\n");
						//rc = (glv_wiget->eventFunc.mousePointer)((glvWindow)glv_window,(glvSheet)glv_sheet,(glvWiget)glv_wiget,type,time,glv_sheet->select_wiget_x,glv_sheet->select_wiget_y,pointer_left_stat);
						rc = (glv_wiget->eventFunc.mousePointer)((glvWindow)glv_window,(glvSheet)glv_sheet,(glvWiget)glv_wiget,type,time,x,y,pointer_left_stat);
						if(rc != GLV_OK){
							fprintf(stderr,"_glv_wiget_mousePointer_cb:wiget mousePointer error\n");
						}
					}							
				}
				break;
			}
		}
		// -------------------------------------------------------------------------------
	}
}

void _glv_window_and_sheet_mouseButton_front(GLV_WINDOW_t *glv_window,int type,glvTime time,int x,int y,int pointer_stat)
{
	GLV_SHEET_t		*glv_sheet;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;

	if(glv_sheet != NULL){
		if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
			if(glv_sheet->eventFunc.mouseButton){
				_glvOnMouseButton(glv_sheet,type,time,x,y,pointer_stat);
			}
		}
	}
	if(glv_window->eventFunc.mouseButton != NULL){
		_glvOnMouseButton(glv_window,type,time,x,y,pointer_stat);
	}
}

void _glv_sheet_mouseButton_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,int type,glvTime time,int x,int y,int pointer_left_stat)
{
	GLV_SHEET_t		*glv_sheet;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.Id == sheetId) && (glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		if(glv_sheet->eventFunc.mouseButton){
			rc = (glv_sheet->eventFunc.mouseButton)((glvWindow)glv_window,(glvSheet)glv_sheet,type,time,x,y,pointer_left_stat);
			if(rc != GLV_OK){
				fprintf(stderr,"_glv_sheet_mouseButton_cb:sheet mouseButton error\n");
			}
		}
	}
}

void _glv_wiget_mouseButton_front(struct _glvinput *glv_input,GLV_WINDOW_t *glv_window,int type,glvTime time,int x,int y,int pointer_stat)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
    glvInstanceId          input_windowId;
    glvInstanceId          input_sheetId;
    glvInstanceId          input_wigetId;
	if(glv_window == NULL) return;

	input_windowId = glv_input->glv_dpy->kb_input_windowId;
	input_sheetId  = glv_input->glv_dpy->kb_input_sheetId;
	input_wigetId  = glv_input->glv_dpy->kb_input_wigetId;

    if(glv_window->instance.Id != input_windowId) return;

	glv_sheet = glv_window->active_sheet;

    if(glv_sheet == NULL) return;
    if(glv_sheet->instance.Id != input_sheetId) return;

    if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
        // -------------------------------------------------------------------------------
        wl_list_for_each_reverse(glv_wiget, &glv_sheet->wiget_list, link){
            if(glv_wiget->instance.alive == GLV_INSTANCE_ALIVE){
                if((glv_wiget->visible == GLV_VISIBLE) && (glv_wiget->instance.Id == input_wigetId)){
                    glv_sheet->select_wiget_x = x - glv_wiget->sheet_x;
                    glv_sheet->select_wiget_y = y - glv_wiget->sheet_y;
                    if(glv_wiget->eventFunc.mouseButton){
                        //printf("_glv_wiget_mouseButton_front\n");
                        _glvOnMouseButton(glv_wiget,type,time,glv_sheet->select_wiget_x,glv_sheet->select_wiget_y,pointer_stat);
                    }
                    break;
                }
            }
        }
        // -------------------------------------------------------------------------------
    }
}

void _glv_wiget_mouseButton_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int type,glvTime time,int x,int y,int pointer_stat)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.Id == sheetId) && (glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		// -------------------------------------------------------------------------------
		wl_list_for_each_reverse(glv_wiget, &glv_sheet->wiget_list, link){
			if(glv_wiget->instance.Id == wigetId){
				if((glv_wiget->instance.alive == GLV_INSTANCE_ALIVE) && (glv_wiget->visible == GLV_VISIBLE)){
					if(glv_wiget->eventFunc.mouseButton){
						//printf("_glv_wiget_mouseButton_cb\n");
						rc = (glv_wiget->eventFunc.mouseButton)((glvWindow)glv_window,(glvSheet)glv_sheet,(glvWiget)glv_wiget,type,time,x,y,pointer_stat);
						if(rc != GLV_OK){
							fprintf(stderr,"_glv_wiget_mouseButton_cb:wiget mouseButton error\n");
						}
					}							
				}
				break;
			}
		}
		// -------------------------------------------------------------------------------
	}
}

void _glv_window_and_sheet_mouseAxis_front(GLV_WINDOW_t *glv_window,int type,glvTime time,int value)
{
	GLV_SHEET_t		*glv_sheet;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;

	if(glv_sheet != NULL){
		if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
			if(glv_sheet->eventFunc.mouseAxis){
				_glvOnMouseAxis(glv_window,type,time,value);
			}
		}
	}
	if(glv_window->eventFunc.mouseAxis != NULL){
		_glvOnMouseAxis(glv_window,type,time,value);
	}
}

void _glv_sheet_mouseAxis_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,int type,glvTime time,int value)
{
	GLV_SHEET_t		*glv_sheet;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.Id == sheetId) && (glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		if(glv_sheet->eventFunc.mouseButton){
			rc = (glv_sheet->eventFunc.mouseAxis)((glvWindow)glv_window,(glvSheet)glv_sheet,type,time,value);
			if(rc != GLV_OK){
				fprintf(stderr,"_glv_sheet_mouseAxis_cb:sheet mouseAxis error\n");
			}
		}
	}
}

void _glv_wiget_mouseAxis_front(GLV_WINDOW_t *glv_window,int type,glvTime time,int value)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
    glvInstanceId	input_wigetId;

	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
    if(glv_sheet == NULL) return;

	input_wigetId = glv_sheet->pointer_focus_wiget_Id;

    if((glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
        // -------------------------------------------------------------------------------
        wl_list_for_each_reverse(glv_wiget, &glv_sheet->wiget_list, link){
            if(glv_wiget->instance.alive == GLV_INSTANCE_ALIVE){
                if((glv_wiget->visible == GLV_VISIBLE) && (glv_wiget->instance.Id == input_wigetId)){
					//printf("_glv_wiget_mouseAxis_front\n");
                    if(glv_wiget->eventFunc.mouseAxis){
                        //printf("_glv_wiget_mouseAxis_front\n");
                        _glvOnMouseAxis(glv_wiget,type,time,value);
                    }
                    break;
                }
            }
        }
        // -------------------------------------------------------------------------------
    }
}

void _glv_wiget_mouseAxis_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int type,glvTime time,int value)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.Id == sheetId) && (glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		// -------------------------------------------------------------------------------
		wl_list_for_each_reverse(glv_wiget, &glv_sheet->wiget_list, link){
			if(glv_wiget->instance.Id == wigetId){
				if((glv_wiget->instance.alive == GLV_INSTANCE_ALIVE) && (glv_wiget->visible == GLV_VISIBLE)){
					//printf("_glv_wiget_mouseAxis_cb\n");
					if(glv_wiget->eventFunc.mouseAxis){
						//printf("_glv_wiget_mouseAxis_cb\n");
						rc = (glv_wiget->eventFunc.mouseAxis)((glvWindow)glv_window,(glvSheet)glv_sheet,(glvWiget)glv_wiget,type,time,value);
						if(rc != GLV_OK){
							fprintf(stderr,"_glv_wiget_mouseAxis_cb:wiget mouseAxis error\n");
						}
					}							
				}
				break;
			}
		}
		// -------------------------------------------------------------------------------
	}
}

void _glv_wiget_key_input_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int type,int state,uint32_t kyesym,int *in_text,uint8_t *attr,int length)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
	int rc;
	if(glv_window == NULL) return;
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet == NULL) return;

	if((glv_sheet->instance.Id == sheetId) && (glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
		// -------------------------------------------------------------------------------
		wl_list_for_each_reverse(glv_wiget, &glv_sheet->wiget_list, link){
			if(glv_wiget->instance.Id == wigetId){
				if((glv_wiget->instance.alive == GLV_INSTANCE_ALIVE) && (glv_wiget->visible == GLV_VISIBLE)){
					if(glv_wiget->eventFunc.input){
						rc = (glv_wiget->eventFunc.input)((glvWindow)glv_window,(glvSheet)glv_sheet,(glvWiget)glv_wiget,type,state,kyesym,in_text,attr,length);
						if(rc != GLV_OK){
							fprintf(stderr,"_glv_wiget_key_input_cb:wiget input error\n");
						}
					}							
				}
				break;
			}
		}
		// -------------------------------------------------------------------------------
	}
}

void _glv_wiget_focus_cb(GLV_WINDOW_t *glv_window,glvInstanceId sheetId,glvInstanceId wigetId,int focus_stat,glvInstanceId in_windowId,glvInstanceId in_sheetId,glvInstanceId in_wigetId)
{
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
	GLV_WIGET_t		*glv_in_wiget = NULL;
	GLV_WINDOW_t	*glv_in_window = NULL;
	int rc;
	if(glv_window == NULL) return;

	if(in_windowId != 0){
		glv_in_window = _glvGetWindow(glv_window->glv_dpy,in_windowId);
	}
	if(glv_in_window != NULL){
		glv_sheet = glv_in_window->active_sheet;
		if(glv_sheet){
			if((glv_sheet->instance.Id == in_sheetId) && (glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
				// -------------------------------------------------------------------------------
				wl_list_for_each_reverse(glv_wiget, &glv_sheet->wiget_list, link){
					if(glv_wiget->instance.Id == in_wigetId){
						if((glv_wiget->instance.alive == GLV_INSTANCE_ALIVE) && (glv_wiget->visible == GLV_VISIBLE)){
							glv_in_wiget = glv_wiget;
							//printf("_glv_wiget_focus_cb: in_wigwt glv_in_wiget = %p\n",glv_in_wiget);
						}
						break;
					}
				}
				// -------------------------------------------------------------------------------
			}
		}
	}

	// ---------------------------------------------------------------------------------------
	glv_sheet = glv_window->active_sheet;
	if(glv_sheet){
		if((glv_sheet->instance.Id == sheetId) && (glv_sheet->instance.alive == GLV_INSTANCE_ALIVE) && (glv_sheet->initialized == 1)){
			// -------------------------------------------------------------------------------
			wl_list_for_each_reverse(glv_wiget, &glv_sheet->wiget_list, link){
				if(glv_wiget->instance.Id == wigetId){
					if((glv_wiget->instance.alive == GLV_INSTANCE_ALIVE) && (glv_wiget->visible == GLV_VISIBLE)){
						if(glv_wiget->eventFunc.focus){
							rc = (glv_wiget->eventFunc.focus)((glvWindow)glv_window,(glvSheet)glv_sheet,(glvWiget)glv_wiget,focus_stat,(glvWiget)glv_in_wiget);
							if(rc != GLV_OK){
								fprintf(stderr,"_glv_wiget_focus_cb:wiget focus error\n");
							}
						}							
					}
					break;
				}
			}
			// -------------------------------------------------------------------------------
		}
	}
}

glvInstanceId _glv_wiget_check_wiget_area(GLV_WINDOW_t *glv_window,int x,int y,int button_status)
{
	// button_status 0:button up ,1: button down , 2:check only
	GLV_SHEET_t		*glv_sheet;
	GLV_WIGET_t		*glv_wiget;
	int				findFlag=0;
	glvInstanceId glv_wigetId=0;
	if(glv_window == NULL){
		return(0);
	}
	pthread_mutex_lock(&glv_window->window_mutex);				// window
	glv_sheet = glv_window->active_sheet;

	if(glv_sheet == NULL){
		pthread_mutex_unlock(&glv_window->window_mutex);		// window
		return(0);
	}

	if(glv_sheet->instance.alive != GLV_INSTANCE_ALIVE){
		pthread_mutex_unlock(&glv_window->window_mutex);		// window
		return(0);
	}

	pthread_mutex_lock(&glv_sheet->sheet_mutex);				// sheet
    if((button_status == GLV_MOUSE_EVENT_RELEASE) || (button_status == GLV_MOUSE_EVENT_PRESS)){
		glv_sheet->select_wiget_Id = 0;
		glv_sheet->select_wiget_x = 0;
		glv_sheet->select_wiget_y = 0;
		glv_sheet->select_wiget_button_status = GLV_MOUSE_EVENT_RELEASE;
	}

	wl_list_for_each_reverse(glv_wiget, &glv_sheet->wiget_list, link){
		if((glv_wiget->instance.alive == GLV_INSTANCE_ALIVE) && (glv_wiget->attr != 0)){
			if((glv_wiget->visible == GLV_VISIBLE) &&
				(glv_wiget->sheet_x < x ) &&
				((glv_wiget->sheet_x + glv_wiget->width) > x) &&
				(glv_wiget->sheet_y < y ) &&
				((glv_wiget->sheet_y + glv_wiget->height) > y)){
				findFlag = 1;
				glv_wigetId = glv_wiget->instance.Id;
                if((button_status == GLV_MOUSE_EVENT_RELEASE) || (button_status == GLV_MOUSE_EVENT_PRESS)){
					glv_sheet->select_wiget_Id = glv_wigetId;
					glv_sheet->select_wiget_x = x - glv_wiget->sheet_x;
					glv_sheet->select_wiget_y = y - glv_wiget->sheet_y;
					glv_sheet->select_wiget_button_status = button_status;
				}else{
					if(glv_wiget->attr & GLV_WIGET_ATTR_POINTER_FOCUS){
						//printf("_glv_wiget_check_wiget_area focus wiget id = %ld\n",glv_wigetId);
					}else{
						glv_wigetId = 0;
					}
				}
				glv_sheet->pointer_focus_wiget_Id = glv_wigetId;
				glv_sheet->pointer_focus_wiget_x = x - glv_wiget->sheet_x;
				glv_sheet->pointer_focus_wiget_y = y - glv_wiget->sheet_y;
				break;
			}
		}
	}
	if(findFlag == 0){
		glv_sheet->pointer_focus_wiget_Id = 0;
	}
	//printf("glv_wigetId = %ld , glv_sheet->pointer_focus_wiget_Id %ld\n",glv_wigetId,glv_sheet->pointer_focus_wiget_Id);

	pthread_mutex_unlock(&glv_sheet->sheet_mutex);				// sheet
	pthread_mutex_unlock(&glv_window->window_mutex);			// window
	return(glv_wigetId);
}

glvSheet glvCreateSheet(glvWindow glv_win,const struct glv_sheet_listener *listener,char *name)
{
	GLV_SHEET_t *glv_sheet;
	GLV_WINDOW_t *glv_window= (GLV_WINDOW_t*)glv_win;
	glv_sheet = (GLV_SHEET_t *)malloc(sizeof(GLV_SHEET_t));
	if(!glv_sheet){
		return(NULL);
	}
	memset(glv_sheet,0,sizeof(GLV_SHEET_t));

	_glvInitInstance(&glv_sheet->instance,GLV_INSTANCE_TYPE_SHEET);

	wl_list_init(&glv_sheet->wiget_list);
	glv_sheet->glv_window = glv_window;
	glv_sheet->glv_dpy = glv_window->glv_dpy;
	glv_sheet->initialized = 0;
    glv_sheet->name = name;

#ifdef GLV_PTHREAD_MUTEX_RECURSIVE
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&glv_sheet->sheet_mutex,&attr);	// sheet
#else
	pthread_mutex_init(&glv_sheet->sheet_mutex,NULL);	// sheet
#endif

	pthread_mutex_lock(&glv_window->window_mutex);				// window
	wl_list_insert(glv_window->sheet_list.prev, &glv_sheet->link);
	pthread_mutex_unlock(&glv_window->window_mutex);			// window

	if(listener != NULL){
		glvSheet_setHandler_class(glv_sheet,listener->class);
		glvSheet_setHandler_init(glv_sheet,listener->init);
		glvSheet_setHandler_reshape(glv_sheet,listener->reshape);
		glvSheet_setHandler_redraw(glv_sheet,listener->redraw);
		glvSheet_setHandler_update(glv_sheet,listener->update);
		glvSheet_setHandler_timer(glv_sheet,listener->timer);
		glvSheet_setHandler_mousePointer(glv_sheet,listener->mousePointer);
		glvSheet_setHandler_mouseButton(glv_sheet,listener->mouseButton);
		glvSheet_setHandler_mouseAxis(glv_sheet,listener->mouseAxis);		
		glvSheet_setHandler_action(glv_sheet,listener->action);
		glvSheet_setHandler_userMsg(glv_sheet,listener->userMsg);
		glvSheet_setHandler_terminate(glv_sheet,listener->terminate);
	}
	GLV_IF_DEBUG_INSTANCE printf(GLV_DEBUG_INSTANCE_COLOR"glvCreateSheet id = %ld [%s] [%s]\n"GLV_DEBUG_END_COLOR,glv_sheet->instance.Id,glv_sheet->name,glv_window->name);
	return(glv_sheet);
}
int glvWindow_activeSheet(glvWindow glv_win,glvSheet sheet)
{
	GLV_WINDOW_t *glv_window= (GLV_WINDOW_t*)glv_win;
	GLV_SHEET_t *glv_sheet = (GLV_SHEET_t*)sheet;
	GLV_SHEET_t *sheet2;
	int		flag = 0;

	pthread_mutex_lock(&glv_window->window_mutex);			// window

	wl_list_for_each(sheet2, &glv_window->sheet_list, link){
		if(sheet2 == glv_sheet){
			glv_window->active_sheet = glv_sheet;
			flag = 1;
			break;
		}
	}

	pthread_mutex_unlock(&glv_window->window_mutex);		// window

	if(flag == 0){
		return(GLV_ERROR);
	}
	return(GLV_OK);
}

int glvWindow_inactiveSheet(glvWindow glv_win)
{
	GLV_WINDOW_t *glv_window= (GLV_WINDOW_t*)glv_win;
	pthread_mutex_lock(&glv_window->window_mutex);			// window
	glv_window->active_sheet = NULL;
	pthread_mutex_unlock(&glv_window->window_mutex);		// window
	return(GLV_OK);
}

void glvDestroySheet(glvSheet sheet)
{
	GLV_SHEET_t *glv_sheet = (GLV_SHEET_t*)sheet;
	int gc_lock=0;

	if(pthread_equal(glv_sheet->glv_dpy->threadId,pthread_self())){
		if(_glv_garbage_box.gc_run == 0){
			gc_lock = 1;
		}
	}else{
		gc_lock = 1;
	}

	glv_sheet->instance.alive = GLV_INSTANCE_DEAD;

	wl_list_remove(&glv_sheet->link);

	{
		struct _glv_wiget *tmp;
		struct _glv_wiget *glv_wiget;

		pthread_mutex_lock(&glv_sheet->sheet_mutex);				// sheet
		wl_list_for_each_safe(glv_wiget, tmp, &glv_sheet->wiget_list, link)
			glvDestroyWiget((glvWiget)glv_wiget);
		pthread_mutex_unlock(&glv_sheet->sheet_mutex);				// sheet
	}

	if(gc_lock == 1) pthread_mutex_lock(&_glv_garbage_box.pthread_mutex);			// garbage box
	wl_list_insert(_glv_garbage_box.sheet_list.prev, &glv_sheet->link);
	if(gc_lock == 1) pthread_mutex_unlock(&_glv_garbage_box.pthread_mutex);			// garbage box
}

void _glvGcDestroySheet(GLV_SHEET_t *glv_sheet)
{
	wl_list_remove(&glv_sheet->link);

	if(glv_sheet->eventFunc.terminate){
		int rc;
		rc = (glv_sheet->eventFunc.terminate)((glvSheet)glv_sheet);
		if(rc != GLV_OK){
			fprintf(stderr,"_glvDestroySheet:glv_sheet->eventFunc.terminate error\n");
		}
	}

	glvDestroyResource(&glv_sheet->instance);

	GLV_IF_DEBUG_INSTANCE printf(GLV_DEBUG_INSTANCE_COLOR"_glvGcDestroySheet id = %ld [%s]\n"GLV_DEBUG_END_COLOR,glv_sheet->instance.Id,glv_sheet->name);
	glv_sheet->instance.oneself = NULL;
	pthread_mutex_destroy(&glv_sheet->sheet_mutex);
	free(glv_sheet);
}

int glvSheet_reqSwapBuffers(glvSheet sheet)
{
	GLV_SHEET_t *glv_sheet = (GLV_SHEET_t*)sheet;

	glv_sheet->glv_window->reqSwapBuffersFlag = 1;

	return(GLV_OK);
}

int glvSheet_reqDrawWigets(glvSheet sheet)
{
	GLV_SHEET_t *glv_sheet = (GLV_SHEET_t*)sheet;

	glv_sheet->reqDrawWigetsFlag = 1;
	return(GLV_OK);
}

int glvSheet_getSelectWigetStatus(glvSheet sheet,GLV_WIGET_STATUS_t *wigetStatus)
{
	GLV_SHEET_t *glv_sheet = (GLV_SHEET_t*)sheet;

	wigetStatus->focusId = glv_sheet->pointer_focus_wiget_Id;
	wigetStatus->focus_x = glv_sheet->pointer_focus_wiget_x;
	wigetStatus->focus_y = glv_sheet->pointer_focus_wiget_y;
	wigetStatus->selectId = glv_sheet->select_wiget_Id;
	wigetStatus->selectStatus = glv_sheet->select_wiget_button_status;
	wigetStatus->select_x = glv_sheet->select_wiget_x;
	wigetStatus->select_y = glv_sheet->select_wiget_y;

	//printf("wigetStatus->focusId = %d\n",wigetStatus->focusId);
	return(GLV_OK);
}

int glvWiget_setFocus(glvWiget wiget)
{
	GLV_WIGET_t *glv_wiget = (GLV_WIGET_t*)wiget;
	GLV_DISPLAY_t	*glv_dpy = glv_wiget->glv_dpy;

	if(glv_dpy->kb_input_wigetId != glv_wiget->instance.Id){
		_glvOnFocus((glvDisplay)glv_dpy,GLV_STAT_OUT_FOCUS,glv_wiget);
		glv_dpy->kb_input_windowId = glv_wiget->glv_sheet->glv_window->instance.Id;
		glv_dpy->kb_input_sheetId  = glv_wiget->glv_sheet->instance.Id;
		glv_dpy->kb_input_wigetId  = glv_wiget->instance.Id;
		_glvOnFocus((glvDisplay)glv_dpy,GLV_STAT_IN_FOCUS,glv_wiget);
	}
	return(GLV_OK);
}

int glvWiget_kindSelectWigetStatus(glvWiget wiget,GLV_WIGET_STATUS_t *wigetStatus)
{
	glvInstanceId wiget_Id;
	int kind;

	wiget_Id = glv_getInstanceId(wiget);

	if((wigetStatus->selectId == wiget_Id) && (wigetStatus->selectStatus == GLV_MOUSE_EVENT_PRESS)){
		kind = GLV_WIGET_STATUS_PRESS;
	}else if(wigetStatus->focusId == wiget_Id){
		kind = GLV_WIGET_STATUS_FOCUS;
	}else{
		kind = GLV_WIGET_STATUS_RELEASE;
	}
	return(kind);
}

glvWiget glvCreateWiget(glvSheet sheet,const struct glv_wiget_listener *listener,int attr)
{
	GLV_WIGET_t *glv_wiget;
	GLV_SHEET_t *glv_sheet = (GLV_SHEET_t*)sheet;
	glv_wiget = (GLV_WIGET_t *)malloc(sizeof(GLV_WIGET_t));
	if(!glv_wiget){
		return(NULL);
	}

	memset(glv_wiget,0,sizeof(GLV_WIGET_t));

	_glvInitInstance(&glv_wiget->instance,GLV_INSTANCE_TYPE_WIGET);

	glv_wiget->attr	= attr;
	glv_wiget->glv_sheet = glv_sheet;
	glv_wiget->glv_dpy = glv_sheet->glv_dpy;

	pthread_mutex_lock(&glv_sheet->sheet_mutex);				// sheet
	wl_list_insert(glv_sheet->wiget_list.prev, &glv_wiget->link);
	pthread_mutex_unlock(&glv_sheet->sheet_mutex);			// sheet

	if(listener != NULL){
		glvWiget_setHandler_class(glv_wiget,listener->class);
		glv_wiget->attr |= listener->attr;
		glvWiget_setHandler_init(glv_wiget,listener->init);
		glvWiget_setHandler_redraw(glv_wiget,listener->redraw);
		glvWiget_setHandler_mousePointer(glv_wiget,listener->mousePointer);
        glvWiget_setHandler_mouseButton(glv_wiget,listener->mouseButton);
		glvWiget_setHandler_mouseAxis(glv_wiget,listener->mouseAxis);
		glvWiget_setHandler_input(glv_wiget,listener->input);
		glvWiget_setHandler_focus(glv_wiget,listener->focus);
		glvWiget_setHandler_terminate(glv_wiget,listener->terminate);
	}
	GLV_IF_DEBUG_INSTANCE printf(GLV_DEBUG_INSTANCE_COLOR"glvCreateWiget id = %ld [%s] [%s]\n"GLV_DEBUG_END_COLOR,glv_wiget->instance.Id,glv_sheet->name,glv_sheet->glv_window->name);
	glv_wiget->instance.oneself = NULL;
	return(glv_wiget);
}

void glvDestroyWiget(glvWiget wiget)
{
	GLV_WIGET_t *glv_wiget = (GLV_WIGET_t*)wiget;
	int gc_lock=0;

	if(pthread_equal(glv_wiget->glv_dpy->threadId,pthread_self())){
		if(_glv_garbage_box.gc_run == 0){
			gc_lock = 1;
		}
	}else{
		gc_lock = 1;
	}

	glv_wiget->instance.alive = GLV_INSTANCE_DEAD;
	
	wl_list_remove(&glv_wiget->link);
	if(gc_lock == 1) pthread_mutex_lock(&_glv_garbage_box.pthread_mutex);			// garbage box
	wl_list_insert(_glv_garbage_box.wiget_list.prev, &glv_wiget->link);
	if(gc_lock == 1) pthread_mutex_unlock(&_glv_garbage_box.pthread_mutex);			// garbage box
}

void _glvGcDestroyWiget(GLV_WIGET_t *glv_wiget)
{
	wl_list_remove(&glv_wiget->link);

	if(glv_wiget->eventFunc.terminate){
		int rc;
		rc = (glv_wiget->eventFunc.terminate)((glvSheet)glv_wiget);
		if(rc != GLV_OK){
			fprintf(stderr,"_glvDestroyWiget:glv_wiget->eventFunc.terminate error\n");
		}
	}

	glvDestroyResource(&glv_wiget->instance);

	GLV_IF_DEBUG_INSTANCE printf(GLV_DEBUG_INSTANCE_COLOR"_glvGcDestroyWiget id = %ld\n"GLV_DEBUG_END_COLOR,glv_wiget->instance.Id);
	free(glv_wiget);
}

int glvWiget_setWigetGeometry(glvWiget wiget,GLV_WIGET_GEOMETRY_t *geometry)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t*)wiget;

	if(glv_wiget == NULL){
		return(GLV_ERROR);
	}

	glv_wiget->sheet_x = geometry->x;
	glv_wiget->sheet_y = geometry->y;
	glv_wiget->width = geometry->width;
	glv_wiget->height = geometry->height;
	glv_wiget->scale = geometry->scale;
	return(GLV_OK);
}

int glvWiget_getWigetGeometry(glvWiget wiget,GLV_WIGET_GEOMETRY_t *geometry)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t*)wiget;

	if(glv_wiget == NULL){
		memset(geometry,0,sizeof(GLV_WIGET_GEOMETRY_t));
		return(GLV_ERROR);
	}

	geometry->x = glv_wiget->sheet_x;
	geometry->y = glv_wiget->sheet_y;
	geometry->width = glv_wiget->width;
	geometry->height = glv_wiget->height;
	geometry->scale = glv_wiget->scale;
	geometry->wigetId = glv_wiget->instance.Id;
	geometry->sheetId = glv_wiget->glv_sheet->instance.Id;
	geometry->windowId = glv_wiget->glv_sheet->glv_window->instance.Id;
	return(GLV_OK);
}

int glvWiget_setWigetVisible(glvWiget wiget,int visible)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t*)wiget;

	if(glv_wiget == NULL){
		return(GLV_ERROR);
	}

	glv_wiget->visible = visible;
	return(GLV_OK);
}

int glvWiget_getWigetVisible(glvWiget wiget)
{
	GLV_WIGET_t *glv_wiget=(GLV_WIGET_t*)wiget;

	if(glv_wiget == NULL){
		return(GLV_INVISIBLE);
	}

	return(glv_wiget->visible);
}
