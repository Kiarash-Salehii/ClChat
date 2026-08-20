#include "terminal.h"
#include "logger.h"
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
int get_terminal_size(int* width, int* height)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
		return 0;

	if(width != 0)
		*width = (int)(info.srWindow.Right - info.srWindow.Left + 1);
	if (height != 0)
		*height = (int)(info.srWindow.Bottom - info.srWindow.Top + 1);

	return 1;
}
int get_terminal_cursor_position(int* x, int* y)
{
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
		return 0;

	if (x != 0)
		*x = (int)info.dwCursorPosition.X;
	if (y != 0)
		*y = (int)info.dwCursorPosition.Y;

	return 1;
}
void terminal_clear(void)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD cells, written;
	COORD home = { 0, 0 };

	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
		return;

	cells = csbi.dwSize.X * csbi.dwSize.Y;

	FillConsoleOutputCharacterA(hConsole, ' ', cells, home, &written);

	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cells, home, &written);

	SetConsoleCursorPosition(hConsole, home);
}
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
int get_terminal_size(int* width, int* height)
{
	struct winsize ws;

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) != 0 &&
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 &&
		ioctl(STDERR_FILENO, TIOCGWINSZ, &ws) != 0)
		return 0;

	if (width != 0)
		{
			*width = (int)ws.ws_col;
		}
	if (height != 0)
		{
			*height = (int)ws.ws_row;
		}

	return 1;
}

int get_terminal_cursor_position(int* x, int* y)
{
	struct termios old, raw;
	char buf[32];
	int i = 0;

	if (tcgetattr(STDIN_FILENO, &old) == -1)
		return 0;

	raw = old;
	raw.c_lflag &= ~(ICANON | ECHO);

	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1)
		return 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		goto fail;

	while (i < (int)sizeof(buf) - 1)
	{
		if (read(STDIN_FILENO, &buf[i], 1) != 1)
			goto fail;

		if (buf[i] == 'R')
			break;

		i++;
	}

	buf[i + 1] = '\0';

	tcsetattr(STDIN_FILENO, TCSANOW, &old);

	int _x, _y;
	if (sscanf(buf, "\x1b[%d;%dR", &_x, &_y) != 2)
		return 0;

	if (x != 0)
		*x = _x;
	if (y != 0)
		*y = _y;

	return 1;

fail:
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return 0;
}

void terminal_clear(void)
{
	logger_printf("\033[2J\033[H");
	fflush(stdout);
}
#endif
