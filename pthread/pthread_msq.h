/*
 * Copyright © 2016 Hitachi, Ltd.
 * Copyright © 2013 T.Aikawa
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

#ifndef SMPTHREADMSQ_H_
#define SMPTHREADMSQ_H_

#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

#define PTHREAD_MSQ_OK			(0)
#define PTHREAD_MSQ_ERROR		(-1)
#define PTHREAD_MSQ_MSG_NUM		(10)

#define pthread_msq_msg_issender(msg) ((msg)->__sender)
#ifdef __SMS_APPLE__
#define PTHREAD_MSQ_ID_INITIALIZER(sendName,receiveName) {NULL,0,NULL,NULL,sendName,receiveName,PTHREAD_MUTEX_INITIALIZER,0,0,0,NULL}
#else
#define PTHREAD_MSQ_ID_INITIALIZER {NULL,0,{},{},PTHREAD_MUTEX_INITIALIZER,0,0,0,NULL}
#endif /* __SMS_APPLE__ */

/**
 * メッセージ構造体
 */
typedef struct {
	void *__sender;
	size_t data[PTHREAD_MSQ_MSG_NUM];
} pthread_msq_msg_t;

/**
 * メッセージキューID構造体
 */
typedef struct msg_queue_t {
	struct msg_queue_t *oneself;
	int	stop;						// 2021.01.25 append by T.Aikawa
#ifdef __SMS_APPLE__
	sem_t *sendId;
	sem_t *receiveId;
	char *sendName;
	char *receiveName;
#else
	sem_t sendId;
	sem_t receiveId;
#endif /* __SMS_APPLE__ */
	pthread_mutex_t mutex;
	int maxMsgQueueNum;				// 最大メッセージキューイング数
	int fifoIndex;					// リングバッファー内のデータ取り出し位置
	int queueNum;					// キューイング中のデータ数
	pthread_msq_msg_t *ringBuffer;
} pthread_msq_id_t;

#ifdef __cplusplus
extern "C" {
#endif
/* メッセージキューの作成 */
int pthread_msq_create(pthread_msq_id_t *queue, int qsize);
/* メッセージ送信 */
int pthread_msq_msg_send(pthread_msq_id_t *queue, pthread_msq_msg_t *msg, void *sender);
/* メッセージ送信（タイムアウト指定有り）*/
int pthread_msq_msg_timedsend(pthread_msq_id_t *queue, pthread_msq_msg_t *msg, void *sender, const struct timespec *abs_timeout);
/* メッセージ受信 */
int pthread_msq_msg_receive(pthread_msq_id_t *queue, pthread_msq_msg_t *msg);
/* メッセージ受信wait無し */
int pthread_msq_msg_receive_try(pthread_msq_id_t *queue, pthread_msq_msg_t *msg);
/* メッセージ受信（タイムアウト指定有り）*/
int pthread_msq_msg_timedreceive(pthread_msq_id_t *queue, pthread_msq_msg_t *msg, const struct timespec *abs_timeout);
/* メッセージキューの破壊 */
int pthread_msq_destroy(pthread_msq_id_t *queue);
/* メッセージ受信を停止する */
int pthread_msq_stop(pthread_msq_id_t *queue);		// 2021.01.25 append by T.Aikawa
int pthread_msq_starrt(pthread_msq_id_t *queue);	// 2021.01.25 append by T.Aikawa
#ifdef __cplusplus
}
#endif

#endif	/* SMPTHREADMSQ_H_ */

