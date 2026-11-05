
#ifndef TERMINAL_H_
#define TERMINAL_H_

#ifdef __cplusplus
extern "C" {
#endif

int get_terminal_size(int* width, int* height);
int get_terminal_cursor_position(int* x, int* y);
int set_terminal_cursor_position(int x, int y);
void terminal_clear_line(void);
void terminal_clear(void);

#ifdef __cplusplus
}
#endif

#endif TERMINAL_H_
