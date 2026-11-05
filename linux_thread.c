/*
* Copyright (c) 2024-2025 Ki(y)arash Salehi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
* documentation files (the "Software"), to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
* to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of
* the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
* THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#if !defined(_WIN32)

#include "thread.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

/*
 * Note: POSIX does not provide a portable per-thread suspend/resume API.
 * Implementations that try to forcibly stop a single thread (SIGSTOP/SIGCONT)
 * affect the whole process. Therefore thread_suspend/thread_resume return
 * THREAD_FAILED on Linux. thread_destroy will try to cancel the thread.
 */

typedef struct linux_thread_wrap {
    THREADCALLBACK cb;
    void* user_data;
} linux_thread_wrap;

static void* linux_thread_start(void* arg)
{
    linux_thread_wrap* w = (linux_thread_wrap*)arg;
    if (w && w->cb) {
        w->cb(w->user_data);
    }
    free(w);
    return NULL;
}

thread_result_t thread_begin(thread_t* Thread, THREADCALLBACK func, void* user_data)
{
    if (Thread == NULL) return THREAD_FAILED;
    if (func == NULL) return THREAD_FAILED;
    memset(Thread, 0, sizeof(thread_t));

    //void** data = NULL;
    //if (numArgs != 0)
    //{
    //    va_list list;
    //    va_start(list, numArgs);
    //    if (numArgs > 1) {
    //        data = calloc(numArgs, sizeof(void*));
    //        for (size_t i = 0; i < numArgs; ++i) {
    //            void* val = va_arg(list, void*);
    //            data[i] = val;
    //        }
    //    } else {
    //        /* single argument: store the pointer itself (not an array) */
    //        void* single = va_arg(list, void*);
    //        data = malloc(sizeof(void*));
    //        data[0] = single;
    //    }
    //    va_end(list);
    //}
    //else
    //{
    //    data = calloc(1, sizeof(void*));
    //}

    Thread->callback = func;
    Thread->user_data = user_data;

    /* package callback + user_data for the pthread start routine */
    linux_thread_wrap* wrap = malloc(sizeof(*wrap));
    if (!wrap) {
        //free(data);
        return THREAD_FAILED;
    }
    wrap->cb = func;
    wrap->user_data = user_data;

    pthread_t* tid = malloc(sizeof(pthread_t));
    if (!tid) {
        free(wrap);
        //free(data);
        return THREAD_FAILED;
    }

    int r = pthread_create(tid, NULL, linux_thread_start, wrap);
    if (r != 0) {
        free(tid);
        free(wrap);
        //free(data);
        return THREAD_FAILED;
    }

    Thread->id = (void*)tid;
    return THREAD_SUCCESS;
}

thread_result_t thread_destroy(thread_t* Thread)
{
    if (!Thread || !Thread->id) return THREAD_FAILED;
    pthread_t* tid = (pthread_t*)Thread->id;
    int r = pthread_cancel(*tid);
    /* attempt to join the thread to clean up resources */
    pthread_join(*tid, NULL);
    free(tid);

    /* free user_data if it was allocated here */
    if (Thread->user_data) {
        free(Thread->user_data);
        Thread->user_data = NULL;
    }
    Thread->id = NULL;
    return (r == 0) ? THREAD_SUCCESS : THREAD_FAILED;
}

thread_result_t thread_suspend(thread_t* Thread)
{
    /* Not supported on POSIX in a safe, portable way */
    (void)Thread;
    return THREAD_FAILED;
}

thread_result_t thread_resume(thread_t* Thread)
{
    /* Not supported on POSIX in a safe, portable way */
    (void)Thread;
    return THREAD_FAILED;
}

#endif /* !_WIN32 */
