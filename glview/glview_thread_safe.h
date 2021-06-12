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

///------------------------------------------------------------------------------
// 静的変数
//------------------------------------------------------------------------------
// スレッド固有バッファのキー
static pthread_key_t _thread_safe_buffer_key;
// 1 回限りのキーの初期化
static pthread_once_t _thread_safe_buffer_key_once = PTHREAD_ONCE_INIT;
//------------------------------------------------------------------------------
// 内部関数プロトタイプ
//------------------------------------------------------------------------------
static void _thread_safe_buffer_destroy(void *buf);
static void _thread_safe_buffer_key_alloc(void);
static void _thread_safe_buffer_alloc(size_t size);
static void init_thread_safe_buffer(void);
static THREAD_SAFE_BUFFER_t *get_thread_safe_buffer();
//------------------------------------------------------------------------------
// 関数
//------------------------------------------------------------------------------
// スレッド固有のバッファを解放する
static void _thread_safe_buffer_destroy(void *buf)
{
	free(buf);
}

// キーを確保する */
static void _thread_safe_buffer_key_alloc(void)
{
	pthread_key_create(&_thread_safe_buffer_key, _thread_safe_buffer_destroy);
}

// スレッド固有のバッファを確保する
static void _thread_safe_buffer_alloc(size_t size)
{
	pthread_once(&_thread_safe_buffer_key_once, _thread_safe_buffer_key_alloc);
	pthread_setspecific(_thread_safe_buffer_key, malloc(size));
}

static void init_thread_safe_buffer(void)
{
	// スレッド固有のバッファを確保
	_thread_safe_buffer_alloc(sizeof(THREAD_SAFE_BUFFER_t));

	THREAD_SAFE_BUFFER_t* buffer = get_thread_safe_buffer();

	// 初期化
	memset(buffer, 0, sizeof(THREAD_SAFE_BUFFER_t));
}

static THREAD_SAFE_BUFFER_t *get_thread_safe_buffer(void)
{
	// スレッド固有バッファから取得
	return((THREAD_SAFE_BUFFER_t*)pthread_getspecific(_thread_safe_buffer_key));
}
