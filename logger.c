#include "logger.h"
#include "mutex.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"

#define STACK_BUFSIZE 4096

static mutex_t console_mutex;
static int logger_inited = 0;

static void ensure_init(void)
{
    if (!logger_inited) 
    {
        mutex_init(&console_mutex);
        logger_inited = 1;
    }
}

static int get_str(char buffer[], int count, _key_t* escapers, int escapers_count)
{
    if (count <= 0) return 0;
    int index = 0;
    buffer[0] = '\0';

    while (1)
    {
        /* wait for key */
        if (!console_key_available())
            continue;

        key_t key = console_read_key();
        if (key == -1)
            continue; /* no key actually read */

        /* If this key matches any escaper, terminate and return length */
        for (int i = 0; i < escapers_count; ++i)
        {
            if (escapers[i] == key)
            {
                buffer[index] = '\0';
                logger_printf("\n");
                return index;
            }
        }

        /* Handle backspace (ASCII BS = 8) gracefully */
        if (key == 8 || key == KEY_BACKSPACE)
        {
            if (index > 0)
            {
                --index;
                buffer[index] = '\0';
                /* erase on console: backspace, space, backspace */
                printf("\b \b");
            }
            continue;
        }

        /* Accept only printable ASCII characters into buffer */
        if (key >= 32 && key <= 126)
        {
            if (index < count - 1) /* reserve space for terminating NUL */
            {
                buffer[index++] = (char)key;
                buffer[index] = '\0';
                logger_printf("%c", (char)key);
            }
            else
            {
                /* Buffer full: ensure termination and return full length */
                buffer[count - 1] = '\0';
                return count - 1;
            }
        }

        /* Ignore other non-printable/non-handled keys */
    }
}

int logger_init(void)
{
    if (!logger_inited) 
    {
        if (mutex_init(&console_mutex) != 0) return -1;
        logger_inited = 1;
    }
    return 0;
}

void logger_shutdown(void)
{
    if (logger_inited) 
    {
        mutex_destroy(&console_mutex);
        logger_inited = 0;
    }
}

void logger_printf(const char* fmt, ...)
{
    ensure_init();

    char stackbuf[STACK_BUFSIZE];
    char* buf = stackbuf;
    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(stackbuf, sizeof(stackbuf), fmt, ap);
    va_end(ap);

    if (needed < 0) return;
    if ((size_t)needed >= sizeof(stackbuf)) 
    {
        /* allocate larger buffer */
        size_t sz = (size_t)needed + 1;
        buf = (char*)malloc(sz);
        if (!buf) return;
        va_start(ap, fmt);
        vsnprintf(buf, sz, fmt, ap);
        va_end(ap);
    }

    mutex_lock(&console_mutex);
    fputs(buf, stdout);
    fflush(stdout);
    mutex_unlock(&console_mutex);

    if (buf != stackbuf) free(buf);
}

void logger_vprintf(const char* fmt, va_list vl)
{
    ensure_init();

    char stackbuf[STACK_BUFSIZE];
    char* buf = stackbuf;
	va_list vl_copy;
	va_copy(vl_copy, vl);
    int needed = vsnprintf(stackbuf, sizeof(stackbuf), fmt, vl_copy);

    if (needed < 0) return;
    if ((size_t)needed >= sizeof(stackbuf)) 
    {
        /* allocate larger buffer */
        size_t sz = (size_t)needed + 1;
        buf = (char*)malloc(sz);
        if (!buf) return;
        vsnprintf(buf, sz, fmt, vl);
    }

    mutex_lock(&console_mutex);
    fputs(buf, stdout);
    fflush(stdout);
    mutex_unlock(&console_mutex);

    if (buf != stackbuf) free(buf);
}

char* logger_prompt_gets(char* buf, size_t bufsize, _key_t* terminater_keys, size_t terminater_keys_len, const char* prompt_fmt, ...)
{
    ensure_init();

    char stackprompt[STACK_BUFSIZE];
    char* prompt = stackprompt;
    va_list ap;
    va_start(ap, prompt_fmt);
    int needed = vsnprintf(stackprompt, sizeof(stackprompt), prompt_fmt, ap);
    va_end(ap);

    if (needed < 0) return NULL;
    if ((size_t)needed >= sizeof(stackprompt)) 
    {
        size_t sz = (size_t)needed + 1;
        prompt = (char*)malloc(sz);
        if (!prompt) return NULL;
        va_start(ap, prompt_fmt);
        vsnprintf(prompt, sz, prompt_fmt, ap);
        va_end(ap);
    }

    mutex_lock(&console_mutex);
    fputs(prompt, stdout);
    fflush(stdout);
    get_str(buf, (int)bufsize, terminater_keys, terminater_keys_len);
    mutex_unlock(&console_mutex);

    if (prompt != stackprompt) free(prompt);
    return buf;
}

int log_time_2file(FILE* file, const char* timeFormat, const char* const message)
{
    char tFormat[128] = { 0 };
    int tFormat_index = 0;
    size_t i = 0;
    while (timeFormat[i] != '\0')
    {
        if (timeFormat[i] == '%')
        {
            // Handle custom format specifiers
            if (strncmp(&timeFormat[i + 1], "YYYY", 4) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'Y';
                i += 5; // Skip "YYYY"
            }
            else if (strncmp(&timeFormat[i + 1], "MM", 2) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'm';
                i += 3; // Skip "MM"
            }
            else if (strncmp(&timeFormat[i + 1], "DD", 2) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'd';
                i += 3; // Skip "DD"
            }
            else if (strncmp(&timeFormat[i + 1], "hh", 2) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'H';
                i += 3; // Skip "hh"
            }
            else if (strncmp(&timeFormat[i + 1], "mm", 2) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'M';
                i += 3; // Skip "mm"
            }
            else if (strncmp(&timeFormat[i + 1], "ss", 2) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'S';
                i += 3; // Skip "SS"
            }
            else
            {
                // Handle unknown specifiers
                tFormat[tFormat_index++] = timeFormat[i++];
            }
        }
        else
        {
            // Copy other characters as-is
            tFormat[tFormat_index++] = timeFormat[i++];
        }
    }


    tFormat[tFormat_index] = '\0'; // Null-terminate the format string

    // Get the current time
    time_t t;
    time(&t);
    struct tm* tm_local = localtime(&t);

    // Format the time
    char strLocalTime[256] = { 0 };
    strftime(strLocalTime, sizeof(strLocalTime), tFormat, tm_local);

    // Write to the file
    if (file)
    {
		if (file != stdout)
            fprintf(file, "%s%s", strLocalTime, message);
        else
			logger_printf("%s%s", strLocalTime, message);
    }

    return 0;
}

int log_time_printf(const char* const fmt, const char* timeFormat, ...)
{
    char tFormat[128] = { 0 };
    int tFormat_index = 0;
    size_t i = 0;
    while (timeFormat[i] != '\0')
    {
        if (timeFormat[i] == '%')
        {
            // Handle custom format specifiers
            if (strncmp(&timeFormat[i + 1], "YYYY", 4) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'Y';
                i += 5; // Skip "YYYY"
            }
            else if (strncmp(&timeFormat[i + 1], "MM", 2) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'm';
                i += 3; // Skip "MM"
            }
            else if (strncmp(&timeFormat[i + 1], "DD", 2) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'd';
                i += 3; // Skip "DD"
            }
            else if (strncmp(&timeFormat[i + 1], "hh", 2) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'H';
                i += 3; // Skip "hh"
            }
            else if (strncmp(&timeFormat[i + 1], "mm", 2) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'M';
                i += 3; // Skip "mm"
            }
            else if (strncmp(&timeFormat[i + 1], "ss", 2) == 0)
            {
                tFormat[tFormat_index++] = '%';
                tFormat[tFormat_index++] = 'S';
                i += 3; // Skip "SS"
            }
            else
            {
                // Handle unknown specifiers
                tFormat[tFormat_index++] = timeFormat[i++];
            }
        }
        else
        {
            // Copy other characters as-is
            tFormat[tFormat_index++] = timeFormat[i++];
        }
    }


    tFormat[tFormat_index] = '\0'; // Null-terminate the format string

    // Get the current time
    time_t t;
    time(&t);
    struct tm* tm_local = localtime(&t);

    // Format the time
    char strLocalTime[256] = { 0 };
    strftime(strLocalTime, sizeof(strLocalTime), tFormat, tm_local);

    char stackbuf[STACK_BUFSIZE];
    char* buf = stackbuf;
    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(stackbuf, sizeof(stackbuf), fmt, ap);
    va_end(ap);

    if (needed < 0) return -1;
    if ((size_t)needed >= sizeof(stackbuf))
    {
        /* allocate larger buffer */
        size_t sz = (size_t)needed + 1;
        buf = (char*)malloc(sz);
        if (!buf) return -1;
        va_start(ap, fmt);
        vsnprintf(buf, sz, fmt, ap);
        va_end(ap);
    }

	logger_printf("%s%s", strLocalTime, buf);

    if (buf != stackbuf) free(buf);
    return 0;
}

void logger_lock(void)
{
    ensure_init();
	mutex_lock(&console_mutex);
}

void logger_unlock(void)
{
    ensure_init();
    mutex_unlock(&console_mutex);
}
