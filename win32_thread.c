
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


#ifdef _WIN32

#include "thread.h"
#include <process.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

thread_result_t thread_destroy(thread_t* Thread)
{
    if (!Thread || !Thread->id) return THREAD_FAILED;
    BOOL r = CloseHandle((HANDLE)Thread->id);
    Thread->id = NULL;
    return r ? THREAD_SUCCESS : THREAD_FAILED;
}

thread_result_t thread_suspend(thread_t* Thread)
{
    if (!Thread || !Thread->id) return THREAD_FAILED;
    DWORD e = SuspendThread((HANDLE)Thread->id);
    return (e != (DWORD)-1) ? THREAD_SUCCESS : THREAD_FAILED;
}

thread_result_t thread_resume(thread_t* Thread)
{
    if (!Thread || !Thread->id) return THREAD_FAILED;
    DWORD e = ResumeThread((HANDLE)Thread->id);
    return (e != (DWORD)-1) ? THREAD_SUCCESS : THREAD_FAILED;
}

thread_result_t thread_begin(thread_t* Thread, THREADCALLBACK func, size_t numArgs, ...)
{
	if(Thread == 0) return THREAD_FAILED;
	if(func == (void*)0) return THREAD_FAILED;
    memset(Thread, 0, sizeof(thread_t));
	
	void** data;
	if(numArgs != 0)
	{
		if(numArgs > 1)
		data = calloc(numArgs, sizeof(void*));
		va_list list;
		va_start(list, numArgs);
		if (numArgs > 1)
			for (size_t i = 0; i < numArgs; i++)
			{
				void* val = va_arg(list, void*);
				data[i] = val;
			}
		else
			data = va_arg(list, void*);
		
		va_end(list);
	}
	else
	{
		data = calloc(1, sizeof(void*));
	}
	
	

    Thread->callback = func;
	Thread->user_data = data;
	Thread->id = (HANDLE)_beginthread(func, 0, data);
    return (Thread->id == (void*)0) ? THREAD_FAILED : THREAD_SUCCESS;
}

#endif