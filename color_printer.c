#include "color_printer.h"
#include "logger.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

/* Helper includes */
#include <stddef.h>

#ifdef _WIN32
static WORD win_color[] =
{
0,
FOREGROUND_RED,
FOREGROUND_GREEN,
FOREGROUND_RED | FOREGROUND_GREEN,
FOREGROUND_BLUE,
FOREGROUND_RED | FOREGROUND_BLUE,
FOREGROUND_GREEN | FOREGROUND_BLUE,
FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,

FOREGROUND_INTENSITY,
FOREGROUND_INTENSITY | FOREGROUND_RED,
FOREGROUND_INTENSITY | FOREGROUND_GREEN,
FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
FOREGROUND_INTENSITY | FOREGROUND_BLUE,
FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,

0,
BACKGROUND_RED,
BACKGROUND_GREEN,
BACKGROUND_RED | BACKGROUND_GREEN,
BACKGROUND_BLUE,
BACKGROUND_RED | BACKGROUND_BLUE,
BACKGROUND_GREEN | BACKGROUND_BLUE,
BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,

BACKGROUND_INTENSITY,
BACKGROUND_INTENSITY | BACKGROUND_RED,
BACKGROUND_INTENSITY | BACKGROUND_GREEN,
BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN,
BACKGROUND_INTENSITY | BACKGROUND_BLUE,
BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE,
BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE,
BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
};
#endif

static uint8_t ansi_colors[] =
{
    30,
    31,
    32,
    33,
    34,
    35,
    36,
    37,
    90,
    91,
    92,
    93,
    94,
    95,
    96,
    97,
    40,
    41,
    42,
    43,
    44,
    45,
    46,
    47,
    100,
    101,
    102,
    103,
    104,
    105,
    106,
    107
};

static size_t ansi_count(void)
{
    return sizeof(ansi_colors) / sizeof(ansi_colors[0]);
}

#ifdef _WIN32
static size_t win_color_count(void)
{
    return sizeof(win_color) / sizeof(win_color[0]);
}
#endif

void print_highlighted(const char* text, text_color color)
{
    if (text == NULL) return;

    size_t colors = ansi_count();
    int cidx = (int)color;
    if (cidx < 0 || (size_t)cidx >= colors) cidx = 0;

#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == NULL || h == INVALID_HANDLE_VALUE)
    {
        /* Fallback to ANSI if handle cannot be obtained */
        /* Highlight uses background codes (indices >= 16) */
        size_t bg = (cidx < 16 ? (size_t)(cidx + 16) : (size_t)cidx);
        if (bg >= colors) bg = 0;
        logger_printf("\033[%dm%s\033[0m", ansi_colors[bg], text);
        return;
    }
    logger_lock();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    WORD originalAttrs = 0;
    if (GetConsoleScreenBufferInfo(h, &csbi))
    {
        originalAttrs = csbi.wAttributes;
        WORD fg = (originalAttrs << 4) >> 4; /* lower 4 bits */
        size_t bg_index = (cidx < 16 ? (size_t)(cidx + 16) : (size_t)cidx);
        WORD bg = 0;
        if (bg_index < win_color_count())
            bg = win_color[bg_index];
        if (bg == win_color[CLR_BG_WHITE]) fg = 0;
        WORD highlight = fg | bg;
        BOOL setOk = FALSE;
        if (SetConsoleTextAttribute(h, highlight))
        {
            setOk = TRUE;
        }
        else
        {
            printf("Failed to set console attributes: %lu\n", GetLastError());
        }

        /* Do the write even if SetConsoleTextAttribute failed */
        printf("%s", text);

        /* Always attempt to restore original attributes and log if it fails */
        if (setOk)
        {
            if (!SetConsoleTextAttribute(h, originalAttrs))
            {
                printf("Failed to restore console attributes: %lu\n", GetLastError());
            }
        }
        logger_unlock();
    }
    else
    {
        size_t bg = (cidx < 16 ? (size_t)(cidx + 16) : (size_t)cidx);
        if (bg >= colors) bg = 0;
        logger_printf("\033[%dm%s\033[0m", ansi_colors[bg], text);
    }
#else
    size_t bg = (cidx < 16 ? (size_t)(cidx + 16) : (size_t)cidx);
    if (bg >= colors) bg = 0;
    logger_printf("\033[%dm%s\033[0m", ansi_colors[bg], text);
#endif
}

void printf_highlighted(const char* text, text_color color, ...)
{
    if (text == NULL) return;

    va_list vl;
    va_start(vl, color);

    size_t colors = ansi_count();
    int cidx = (int)color;
    if (cidx < 0 || (size_t)cidx >= colors) cidx = 0;

#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == NULL || h == INVALID_HANDLE_VALUE)
    {
        size_t bg = (cidx < 16 ? (size_t)(cidx + 16) : (size_t)cidx);
        if (bg >= colors) bg = 0;
        logger_printf("\033[%dm", ansi_colors[bg]);
        logger_vprintf(text, vl);
        logger_printf("\033[0m");
        va_end(vl);
        return;
    }
    logger_lock();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    WORD originalAttrs = 0;
    if (GetConsoleScreenBufferInfo(h, &csbi))
    {
        originalAttrs = csbi.wAttributes;
        WORD fg = (originalAttrs << 4) >> 4;
        size_t bg_index = (cidx < 16 ? (size_t)(cidx + 16) : (size_t)cidx);
        WORD bg = 0;
        if (bg_index < win_color_count())
            bg = win_color[bg_index];
        if (bg == win_color[CLR_BG_WHITE]) fg = 0;
        WORD highlight = fg | bg;
        BOOL setOk = FALSE;
        /* set attributes, write, then always try to restore */
        if (SetConsoleTextAttribute(h, highlight))
        {
            setOk = TRUE;
        }
        else
        {
            printf("Failed to set console attributes: %lu\n", GetLastError());
        }

        /* Do the write even if SetConsoleTextAttribute failed */
        vprintf(text, vl);

        /* Always attempt to restore original attributes and log if it fails */
        if (setOk)
        {
            if (!SetConsoleTextAttribute(h, originalAttrs))
            {
                printf("Failed to restore console attributes: %lu\n", GetLastError());
            }
        }
        logger_unlock();
    }
    else
    {
        size_t bg = (cidx < 16 ? (size_t)(cidx + 16) : (size_t)cidx);
        if (bg >= colors) bg = 0;
        logger_printf("\033[%dm", ansi_colors[bg]);
        logger_vprintf(text, vl);
        logger_printf("\033[0m");
    }
#else
    size_t bg = (cidx < 16 ? (size_t)(cidx + 16) : (size_t)cidx);
    if (bg >= colors) bg = 0;
    logger_printf("\033[%dm", ansi_colors[bg]);
    logger_vprintf(text, vl);
    logger_printf("\033[0m");
#endif

    va_end(vl);
}

void print_color(const char* text, text_color fg_color, text_color bg_color)
{
    if (text == NULL) return;

    size_t colors = ansi_count();
    int fidx = (int)fg_color;
    int bidx = (int)bg_color;
    if (fidx < 0 || (size_t)fidx >= colors) fidx = 0;
    if (bidx < 0 || (size_t)bidx >= colors) bidx = 0;

#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == NULL || h == INVALID_HANDLE_VALUE)
    {
        logger_printf("\033[%d;%dm%s\033[0m", ansi_colors[(size_t)fidx], ansi_colors[(size_t)bidx], text);
        return;
    }

    logger_lock();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    WORD originalAttrs = 0;
    if (GetConsoleScreenBufferInfo(h, &csbi))
    {
        originalAttrs = csbi.wAttributes;
        WORD fg = 0;
        WORD bg = 0;
        if ((size_t)fidx < win_color_count()) fg = win_color[fidx];
        if ((size_t)bidx < win_color_count()) bg = win_color[bidx];
        WORD highlight = fg | bg;
        BOOL setOk = FALSE;
        if (SetConsoleTextAttribute(h, highlight))
        {
            setOk = TRUE;
        }
        else
        {
           printf("Failed to set console attributes: %lu\n", GetLastError());
        }

        /* Do the write even if SetConsoleTextAttribute failed */
        printf("%s", text);

        /* Always attempt to restore original attributes and log if it fails */
        if (setOk)
        {
            if (!SetConsoleTextAttribute(h, originalAttrs))
            {
                printf("Failed to restore console attributes: %lu\n", GetLastError());
            }
        }
        logger_unlock();
    }
    else
    {
        logger_printf("\033[%d;%dm%s\033[0m", ansi_colors[(size_t)fidx], ansi_colors[(size_t)bidx], text);
    }
#else
    logger_printf("\033[%d;%dm%s\033[0m", ansi_colors[(size_t)fidx], ansi_colors[(size_t)bidx], text);
#endif
}

void printf_color(const char* fmt, text_color fg_color, text_color bg_color, ...)
{
    if (fmt == NULL) return;

    va_list vl;
    va_start(vl, bg_color);

    size_t colors = ansi_count();
    int fidx = (int)fg_color;
    int bidx = (int)bg_color;
    if (fidx < 0 || (size_t)fidx >= colors) fidx = 0;
    if (bidx < 0 || (size_t)bidx >= colors) bidx = 0;

#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == NULL || h == INVALID_HANDLE_VALUE)
    {
        logger_printf("\033[%d;%dm", ansi_colors[(size_t)fidx], ansi_colors[(size_t)bidx]);
        logger_vprintf(fmt, vl);
        logger_printf("\033[0m");
        va_end(vl);
        return;
    }

    logger_lock();
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    WORD originalAttrs = 0;
    if (GetConsoleScreenBufferInfo(h, &csbi))
    {
        originalAttrs = csbi.wAttributes;
        WORD fg = 0;
        WORD bg = 0;
        if ((size_t)fidx < win_color_count()) fg = win_color[fidx];
        if ((size_t)bidx < win_color_count()) bg = win_color[bidx];
        WORD highlight = fg | bg;
        BOOL setOk = FALSE;
        /* set attributes, write, then always try to restore */
        if (SetConsoleTextAttribute(h, highlight))
        {
            setOk = TRUE;
        }
        else
        {
            printf("Failed to set console attributes: %lu\n", GetLastError());
        }

        /* Do the write even if SetConsoleTextAttribute failed */
        vprintf(fmt, vl);

        /* Always attempt to restore original attributes and log if it fails */
        if (setOk)
        {
            if (!SetConsoleTextAttribute(h, originalAttrs))
            {
                printf("Failed to restore console attributes: %lu\n", GetLastError());
            }
        }
        logger_unlock();
    }
    else
    {
        logger_printf("\033[%d;%dm", ansi_colors[(size_t)fidx], ansi_colors[(size_t)bidx]);
        logger_vprintf(fmt, vl);
        logger_printf("\033[0m");
    }
#else
    logger_printf("\033[%d;%dm", ansi_colors[(size_t)fidx], ansi_colors[(size_t)bidx]);
    logger_vprintf(fmt, vl);
    logger_printf("\033[0m");
#endif

    va_end(vl);
}