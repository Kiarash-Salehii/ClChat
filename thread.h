
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

/*
*		copyrighted from https://github.com/Kiarash-Salehii/CF 
*/

#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

#ifndef _WIN32_THREAD_H_
#define _WIN32_THREAD_H_

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*THREADCALLBACK)(void** args);

typedef struct
{
	THREADCALLBACK callback;
	void* id;
	void* user_data;
} thread_t;


typedef enum {
	THREAD_FAILED,
	THREAD_SUCCESS
} thread_result_t;

thread_result_t thread_begin(thread_t*, THREADCALLBACK, void* user_data);
thread_result_t thread_suspend(thread_t*);
thread_result_t thread_resume(thread_t*);
thread_result_t thread_destroy(thread_t*);

#ifdef __cplusplus
}
#endif

#endif // _WIN32_THREAD_H_