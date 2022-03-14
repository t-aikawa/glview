/*
 * Copyright © 2016 Hitachi, Ltd.
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

#ifndef INC_PTHREAD_TIMER_H
#define INC_PTHREAD_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include "pthread_msq.h"

#define PTHREAD_TIMER_ONLY_ONCE		(1)
#define PTHREAD_TIMER_REPEAT		(2)

#define PTHREAD_TIMER_ERROR		(0)
#define PTHREAD_TIMER_OK		(1)

int pthreadCheckTimer(pthread_t threadId,int id,int count);
void pthreadInitializeTimer(void);
void pthreadTerminateTimer(void);
int pthreadCreate_uTimer(pthread_t threadId,pthread_msq_id_t *queue,size_t userData1,size_t userData2,int group,int id,int type,struct timespec *reqWaitTime);
int pthreadCreate_mTimer(pthread_t threadId,pthread_msq_id_t *queue,size_t userData1,size_t userData2,int group,int id,int type,int mTime);
int pthreadStartTimer(pthread_t threadId,int id);
int pthreadStopTimer(pthread_t threadId,int id);


#ifdef __cplusplus
}
#endif

#endif	/* INC_PTHREAD_TIMER_H */
