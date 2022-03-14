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

#define _GNU_SOURCE

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include "pthread_timer.h"


#define PTHREAD_TIMER_STOP	(0)
#define PTHREAD_TIMER_START	(1)

typedef struct _pthreadTimerTable {
	pthread_t	threadId;
	size_t		userData1;
	size_t		userData2;
	int		group;
	int 	id;
	int 	type;
	int 	active;
	struct timespec reqWaitTime;	/* タイマーの経過待ち時間 */
	long	reqCount;
	struct timespec absWaitTime;	/* 絶対待ち時間　reqWaitTimeと現在時間から生成する */
	pthread_msq_id_t	*queue;
} PTHREADTIMERTABLE_t;

static pthread_t timer_threadId;
static sem_t timer_sem;
static int running=1;
#define PTHREAD_TIMER_TABLE_MAX		(10)
static PTHREADTIMERTABLE_t pthread_timer_table[PTHREAD_TIMER_TABLE_MAX];

/* ----------------------------------------------------------------- */
static pthread_mutex_t pthread_timer_mutex = PTHREAD_MUTEX_INITIALIZER;

static int pthreadCalcAbsWaitTime(struct timespec *crtTime,struct timespec *reqWaitTime,struct timespec *absWaitTime)
{
#if 0
	time_t tv_sec;
	__syscall_slong_t tv_nsec;

	tv_sec  = mTime / 1000;
	tv_nsec = (mTime % 1000) * 1000000;
#endif

	absWaitTime->tv_sec  = crtTime->tv_sec + reqWaitTime->tv_sec + (crtTime->tv_nsec + reqWaitTime->tv_nsec) / 1000000000;
	absWaitTime->tv_nsec = (crtTime->tv_nsec + reqWaitTime->tv_nsec) % 1000000000;
	return(PTHREAD_TIMER_OK);
}

int pthreadCreate_uTimer(pthread_t threadId,pthread_msq_id_t *queue,size_t userData1,size_t userData2,int group,int id,int type,struct timespec *reqWaitTime)
{
	int i;

	if((type != PTHREAD_TIMER_ONLY_ONCE) &&
			(type != PTHREAD_TIMER_REPEAT)){
		return(PTHREAD_TIMER_ERROR);
	}

	if(id == 0){
		return(PTHREAD_TIMER_ERROR);
	}

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if((pthread_timer_table[i].id == id)&&
			(pthread_equal(pthread_timer_table[i].threadId,threadId)))
		{
			pthread_timer_table[i].group		= group;
			pthread_timer_table[i].userData1	= userData1;
			pthread_timer_table[i].userData2	= userData2;
			pthread_timer_table[i].type			= type;
			pthread_timer_table[i].active		= PTHREAD_TIMER_STOP;
			pthread_timer_table[i].reqWaitTime.tv_sec	= reqWaitTime->tv_sec;
			pthread_timer_table[i].reqWaitTime.tv_nsec	= reqWaitTime->tv_nsec;
			pthread_timer_table[i].absWaitTime.tv_sec	= 0;
			pthread_timer_table[i].absWaitTime.tv_nsec	= 0;
			//printf("pthreadCreateTimer:timer reuse  [%d] \n",i);
			pthread_mutex_unlock(&pthread_timer_mutex);
			return(PTHREAD_TIMER_OK);
		}
	}
	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if(pthread_timer_table[i].id == 0)
		{
			pthread_timer_table[i].threadId		= threadId;
			pthread_timer_table[i].userData1	= userData1;
			pthread_timer_table[i].userData2	= userData2;
			pthread_timer_table[i].group		= group;
			pthread_timer_table[i].id			= id;
			pthread_timer_table[i].type			= type;
			pthread_timer_table[i].active		= PTHREAD_TIMER_STOP;
			pthread_timer_table[i].reqWaitTime.tv_sec	= reqWaitTime->tv_sec;
			pthread_timer_table[i].reqWaitTime.tv_nsec	= reqWaitTime->tv_nsec;
			pthread_timer_table[i].absWaitTime.tv_sec	= 0;
			pthread_timer_table[i].absWaitTime.tv_nsec	= 0;
			pthread_timer_table[i].queue		= queue;
			//printf("pthreadCreateTimer:timer create [%d] \n",i);
			pthread_mutex_unlock(&pthread_timer_mutex);
			return(PTHREAD_TIMER_OK);
		}
	}

	pthread_mutex_unlock(&pthread_timer_mutex);
	fprintf(stderr,"pthreadCreateTimer:timer table over [%d] \n",i);
	/* table over */
	return(PTHREAD_TIMER_ERROR);
}

int pthreadCreate_mTimer(pthread_t threadId,pthread_msq_id_t *queue,size_t userData1,size_t userData2,int group,int id,int type,int mTime)
{
	struct timespec reqWaitTime;
	int rc;

	reqWaitTime.tv_sec  = mTime / 1000;
	reqWaitTime.tv_nsec = (mTime % 1000) * 1000000;

	rc = pthreadCreate_uTimer(threadId,queue,userData1,userData2,group,id,type,&reqWaitTime);
	return(rc);
}

int pthreadStartTimer(pthread_t threadId,int id)
{
    static struct timespec crtTime;
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if((pthread_timer_table[i].id == id)&&
			(pthread_equal(pthread_timer_table[i].threadId,threadId)))
		{
			if (clock_gettime(CLOCK_REALTIME, &crtTime) == 0){
			    pthreadCalcAbsWaitTime(&crtTime,&pthread_timer_table[i].reqWaitTime,&pthread_timer_table[i].absWaitTime);
				pthread_timer_table[i].active = PTHREAD_TIMER_START;
				sem_post(&timer_sem); /* タイマーの待を解除 */
			}
			break;
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);

	return(PTHREAD_TIMER_OK);
}

int pthreadStopTimer(pthread_t threadId,int id)
{
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if((pthread_timer_table[i].id == id)&&
			(pthread_equal(pthread_timer_table[i].threadId,threadId)))
		{
			pthread_timer_table[i].active = PTHREAD_TIMER_STOP;
			pthread_timer_table[i].reqCount++;
			pthread_mutex_unlock(&pthread_timer_mutex);
			return(PTHREAD_TIMER_OK);
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);
	return(PTHREAD_TIMER_OK);
}

int pthreadCheckTimer(pthread_t threadId,int id,int count)
{
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if((pthread_timer_table[i].id == id)&&
			(pthread_equal(pthread_timer_table[i].threadId,threadId)))
		{
			if(pthread_timer_table[i].reqCount == count){
				pthread_mutex_unlock(&pthread_timer_mutex);
				return(PTHREAD_TIMER_OK);
			}
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);

	return(PTHREAD_TIMER_ERROR);
}

int pthreadGroupStopTimer(pthread_t threadId,int group)
{
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if((pthread_timer_table[i].group == group)&&
			(pthread_equal(pthread_timer_table[i].threadId,threadId)))
		{
			pthread_timer_table[i].active = PTHREAD_TIMER_STOP;
			pthread_timer_table[i].reqCount++;
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);

	return(PTHREAD_TIMER_OK);
}
int pthreadAllStopTimer(pthread_t threadId)
{
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if(pthread_equal(pthread_timer_table[i].threadId,threadId))
		{
			pthread_timer_table[i].active = PTHREAD_TIMER_STOP;
			pthread_timer_table[i].reqCount++;
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);

	return(PTHREAD_TIMER_OK);
}
int pthreadDestroyTimer(void)
{
	int i;

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		pthread_timer_table[i].id		= 0;
		pthread_timer_table[i].active	= PTHREAD_TIMER_STOP;
		pthread_timer_table[i].reqCount++;
	}
	pthread_mutex_unlock(&pthread_timer_mutex);

	sem_destroy(&timer_sem); /* セマフォを削除する */
	pthread_mutex_destroy(&pthread_timer_mutex); /* ミューテックスを破壊する */

	return(PTHREAD_TIMER_OK);
}

int pthreadGetWaitAbsTime(struct timespec *absWaitTime)
{
	int i;
	long min_sec,min_nsec;

    clock_gettime(CLOCK_REALTIME, absWaitTime);

	pthread_mutex_lock(&pthread_timer_mutex);

	min_sec  = -1;
	min_nsec = -1;

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		if(pthread_timer_table[i].active == PTHREAD_TIMER_START)
		{
			if(min_sec < 0){
				min_sec = pthread_timer_table[i].absWaitTime.tv_sec;
				min_nsec = pthread_timer_table[i].absWaitTime.tv_nsec;
			}else{
				if(min_sec > pthread_timer_table[i].absWaitTime.tv_sec){
					min_sec = pthread_timer_table[i].absWaitTime.tv_sec;
					min_nsec = pthread_timer_table[i].absWaitTime.tv_nsec;
				}else if(min_sec == pthread_timer_table[i].absWaitTime.tv_sec){
					if(min_nsec > pthread_timer_table[i].absWaitTime.tv_nsec){
						min_nsec = pthread_timer_table[i].absWaitTime.tv_nsec;
					}
				}
			}
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);
	if(min_sec == -1){
		absWaitTime->tv_sec += 60;
		return(PTHREAD_TIMER_OK);
	}
	absWaitTime->tv_sec  = min_sec;
	absWaitTime->tv_nsec = min_nsec;
	return(PTHREAD_TIMER_OK);
}

void pthreadSendTime(void)
{
    static struct timespec crtTime;
	pthread_msq_msg_t smsg;
	pthread_msq_id_t *queue;
	int i,time_flag;

	clock_gettime(CLOCK_REALTIME, &crtTime);

	pthread_mutex_lock(&pthread_timer_mutex);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		time_flag = 0;
		if(pthread_timer_table[i].active == PTHREAD_TIMER_START)
		{
			if(crtTime.tv_sec > pthread_timer_table[i].absWaitTime.tv_sec){
				time_flag = 1;
			}else if(crtTime.tv_sec == pthread_timer_table[i].absWaitTime.tv_sec){
				if(crtTime.tv_nsec > pthread_timer_table[i].absWaitTime.tv_nsec){
					time_flag = 1;
				}
			}
			if(time_flag == 1){
				if(pthread_timer_table[i].type == PTHREAD_TIMER_ONLY_ONCE){
					pthread_timer_table[i].active = PTHREAD_TIMER_STOP;
				}else{
					pthreadCalcAbsWaitTime(&crtTime,&pthread_timer_table[i].reqWaitTime,&pthread_timer_table[i].absWaitTime);
			    }
				smsg.data[0] = pthread_timer_table[i].userData1;
				smsg.data[1] = pthread_timer_table[i].userData2;
				smsg.data[2] = pthread_timer_table[i].group;
				smsg.data[3] = pthread_timer_table[i].id;
				smsg.data[4] = pthread_timer_table[i].reqCount;
				queue		 = pthread_timer_table[i].queue;

				pthread_mutex_unlock(&pthread_timer_mutex);

				if(queue != NULL){
					pthread_msq_msg_send(queue,&smsg,0);
				}

				pthread_mutex_lock(&pthread_timer_mutex);
			}
		}
	}
	pthread_mutex_unlock(&pthread_timer_mutex);
}

void *pthreadTimerProc(void* no_arg)
{
    static struct timespec absWaitTime;
    int rc;

	do{
	    pthreadGetWaitAbsTime(&absWaitTime);

	    //printf("pthreadTimerProc:sem_timedwait():call\n");
	    while ((rc = sem_timedwait(&timer_sem, &absWaitTime)) == -1 && errno == EINTR){
			if(running == 0){
				//printf("pthreadTimerProc terminate.\n");
				return(NULL);
			}
	        continue;       /* Restart if interrupted by handler */
		}

		if(running == 0){
			//printf("pthreadTimerProc terminate.\n");
			return(NULL);
		}
#if 0
	    /* Check what happened */
	    if (rc == -1) {
	        if (errno == ETIMEDOUT){
	            printf("pthreadTimerProc:sem_timedwait:timed out\n");
	        	/* タイマーに設定した時間が経過した */
	        }else{
	        	printf("pthreadTimerProc:sem_timedwait:error (%d)\n",errno);
	        }
	    } else{
	        printf("pthreadTimerProc:sem_timedwait:succeeded\n");
	        /* タイマーの再設定要求が発生 */
	    }
#endif
	    pthreadSendTime();
	}while(1);

	return(NULL);
}

void pthreadInitializeTimer(void)
{
	int		pret = 0;
	int		i;

	pthread_mutex_init(&pthread_timer_mutex,NULL);

	for(i=0;i<PTHREAD_TIMER_TABLE_MAX;i++)
	{
		pthread_timer_table[i].threadId		= 0;
		pthread_timer_table[i].group		= 0;
		pthread_timer_table[i].id			= 0;
		pthread_timer_table[i].type			= 0;
		pthread_timer_table[i].active		= PTHREAD_TIMER_STOP;
		pthread_timer_table[i].reqCount		= 0;
		pthread_timer_table[i].reqWaitTime.tv_sec	= 0;
		pthread_timer_table[i].reqWaitTime.tv_nsec	= 0;
		pthread_timer_table[i].absWaitTime.tv_sec	= 0;
		pthread_timer_table[i].absWaitTime.tv_nsec	= 0;
		pthread_timer_table[i].queue		= NULL;
	}
    if (sem_init(&timer_sem, 0, 0) == -1){
		fprintf(stderr,"pthreadTimeProc:Error: sem_init() failed\n");
    	return;
    }

	// スレッド生成
	pret = pthread_create(&timer_threadId, NULL, pthreadTimerProc, (void *)NULL);

	if (0 != pret) {

		return;
	}

	pthread_setname_np(timer_threadId,"pthread_timer");

	return;
}

void pthreadTerminateTimer(void)
{
	running = 0;
	sem_post(&timer_sem); /* タイマーの待を解除 */
	pthread_join(timer_threadId,NULL);
}
