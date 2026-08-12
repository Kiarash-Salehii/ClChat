#pragma once

#ifndef COLOR_PRINTER_H
#define COLOR_PRINTER_H

typedef enum
{
    CLR_BLACK,
    CLR_RED,
    CLR_GREEN,
    CLR_YELLOW,
    CLR_BLUE,
    CLR_MAGENTA,
    CLR_CYAN,
    CLR_WHITE,
    CLR_BRIGHT_BLACK,
    CLR_BRIGHT_RED,
    CLR_BRIGHT_GREEN,
    CLR_BRIGHT_YELLOW,
    CLR_BRIGHT_BLUE,
    CLR_BRIGHT_MAGENTA,
    CLR_BRIGHT_CYAN,
    CLR_BRIGHT_WHITE,
    CLR_BG_BLACK,
    CLR_BG_RED,
    CLR_BG_GREEN,
    CLR_BG_YELLOW,
    CLR_BG_BLUE,
    CLR_BG_MAGENTA,
    CLR_BG_CYAN,
    CLR_BG_WHITE,
    CLR_BG_BRIGHT_BLACK,
    CLR_BG_BRIGHT_RED,
    CLR_BG_BRIGHT_GREEN,
    CLR_BG_BRIGHT_YELLOW,
    CLR_BG_BRIGHT_BLUE,
    CLR_BG_BRIGHT_MAGENTA,
    CLR_BG_BRIGHT_CYAN,
    CLR_BG_BRIGHT_WHITE,
} text_color;

typedef enum
{
    STL_RESET = 0,
    STL_BOLD,
    STL_DIM,
    STL_ITALIC,
    STL_UNDERLINE,
    STL_BLINK,
    STL_REVERSE,
    STL_HIDDEN,
    STL_STRIKETTHROUGH,
} text_style;

void print_highlighted(const char* text, text_color color);
void printf_highlighted(const char* text, text_color color, ...);
void print_color(const char* text, text_color fg_color, text_color bg_color);
void printf_color(const char* text, text_color fg_color, text_color bg_color, ...);

#endif