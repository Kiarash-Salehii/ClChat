
#ifndef _TUI_SELECTOR_H_
#define _TUI_SELECTOR_H_
#include "logger.h"
#include "terminal.h"
#include <stdint.h>
#include <stdlib.h>

#define max(x, y) ((x) > (y) ? (x) : (y))

typedef enum 
{
	TUI_NULL_ARG,
	TUI_MEM_ALLOC_FAILED
} tui_error;

// returns the name index seleted
// return < 0 if failed. to get the error multiply it by -1 to get the tui_result
int tui_seletor(char* names[], uint32_t count, char select_char, uint32_t select_margin);

#endif