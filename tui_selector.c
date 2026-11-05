#include "tui_selector.h"
static char filler_charecter = ' ';

int tui_seletor(char* names[], uint32_t count, char select_char, uint32_t select_margin)
{
	console_init();
	logger_init();

	if (!names) 
	{
		return -TUI_NULL_ARG;
	}
	if (!count) 
	{
		return -TUI_NULL_ARG;
	}

	uint32_t max_name_len = 0;
	for (int i = 0; i < count; i++)
	{
		uint32_t len = strlen(names[i]);
		max_name_len = max(max_name_len, len);
	}

	char* buf = calloc(count, max_name_len + 1);
	if (!buf)
	{
		return -TUI_MEM_ALLOC_FAILED;
	}

	for (int i = 0; i < count; i++)
	{
		uint32_t len = strlen(names[i]);
		char* str = buf + i * (max_name_len + 1);
		strcpy(str, names[i]);
		for (int j = 0; j < max_name_len - len; j++)
		{
			str[j + len] = filler_charecter;
		}
	}

	bool update_needed = true;
	size_t print_buffer_line_size = (max_name_len + select_margin + 1);
	size_t print_buffer_size = count * print_buffer_line_size;
	char* print_buffer = malloc(print_buffer_size + 1);
	if (!print_buffer)
	{
		return -TUI_MEM_ALLOC_FAILED;
	}
	print_buffer[print_buffer_size] = '\0'; // NULL terminator


	int current_row = 0;
	int last_y;
	get_terminal_cursor_position(0, &last_y);
	while (1)
	{
		if (console_key_available())
		{
			_key_t key = console_read_key();

			if (key == KEY_UP)
			{
				if (current_row > 0)
				{
					current_row -= 1;
					update_needed = true;
				}
			}
			else if (key == KEY_DOWN)
			{
				if (current_row < count - 1)
				{
					current_row += 1;
					update_needed = true;
				}
			}
			else if (key == KEY_ENTER || key == KEY_ESCAPE)
			{
				free(print_buffer);
				free(buf);
				return current_row;
			}
		}
		
		if(update_needed)
		{
			for (int i = 0; i < count; i++)
			{
				char* str_name = buf + i * (max_name_len + 1);
				
				for (int j = 0; j < select_margin; j++)
				{
					char _select_char = filler_charecter;
					if (i == current_row && j == select_margin / 2) _select_char = select_char;
					
					*(print_buffer + j + print_buffer_line_size * i) = _select_char;
				}
				
				sprintf(print_buffer + select_margin + print_buffer_line_size * i, "%s\n", str_name);
			}

			set_terminal_cursor_position(0, last_y);
			logger_printf("%s", print_buffer);
			update_needed = false;
		}
	}
}