#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "console.h"

#ifdef __cplusplus
extern "C"
{
#endif


    /*
    *
    *time format types :
    * %YYYY  : used to show years
    * %MM    : used to show months
    * %DD    : used to show days
    * %hh    : used to show hours (24 hour time)
    * %mm    : used to show minutes
    * %ss    : used to show seconds
    *
    * For example :
    *   [%YYYY:%MM:%DD]
    * Or
    *   {%YYYY:%MM:%DD %hh %mm %ss}
    * Or
    *   [%hh %mm %ss]
    * Or
    *   '%YYYY %MM %DD %hh %mm %ss'
    * Or
    *   [%YYYY/%MM/%DD %hh:%mm:%ss]
    */
int log_time_2file(FILE* file, const char* timeFormat, const char* const message);
int logger_init(void);
void logger_shutdown(void);

    /* Thread-safe formatted output (atomic for each call). */
#ifdef __GNUC__
#define LOGGER_PRINTF_ATTR(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define LOGGER_PRINTF_ATTR(fmt_idx, arg_idx)
#endif

void logger_printf(const char* fmt, ...) LOGGER_PRINTF_ATTR(1, 2);
void logger_vprintf(const char* fmt, va_list vl);

    /* Thread-safe prompt + read. Prints the formatted prompt, flushes,
       then reads into buf using fgets while holding the console lock. */
char* logger_prompt_gets(char* buf, size_t bufsize, key_t* terminater_keys, size_t terminater_keys_len, const char* prompt_fmt, ...);

    /*
    *
    *time format types :
    * %YYYY  : used to show years
    * %MM    : used to show months
    * %DD    : used to show days
    * %hh    : used to show hours (24 hour time)
    * %mm    : used to show minutes
    * %ss    : used to show seconds
    *
    * For example :
    *   [%YYYY:%MM:%DD]
    * Or
    *   {%YYYY:%MM:%DD %hh %mm %ss}
    * Or
    *   [%hh %mm %ss]
    * Or
    *   '%YYYY %MM %DD %hh %mm %ss'
    * Or
    *   [%YYYY/%MM/%DD %hh:%mm:%ss]
    */

int log_time_printf(const char* const fmt, const char* timeFormat, ...);

// Can be used to lock the logger for multiple operations, but be careful to avoid deadlocks.
void logger_lock(void);
void logger_unlock(void);
#ifdef __cplusplus
}
#endif

#endif