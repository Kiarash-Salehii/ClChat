#include "server.h"
#include "client.h"

#include <signal.h>

int get_str(char buffer[], int count, _key_t* escapers, int escapers_count)
{
	if (count <= 0) return 0;
	int index = 0;
	buffer[0] = '\0';

	while (1)
	{
		/* wait for key */
		if (!console_key_available())
			continue;

		_key_t key = console_read_key();
		if (key == -1)
			continue; /* no key actually read */

		/* If this key matches any escaper, terminate and return length */
		for (int i = 0; i < escapers_count; ++i)
		{
			if (escapers[i] == key)
			{
				buffer[index] = '\0';
				return index;
			}
		}

		/* Handle backspace (ASCII BS = 8) gracefully */
		if (key == 8 || key == KEY_BACKSPACE || key == 127)
		{
			if (index > 0)
			{
				--index;
				buffer[index] = '\0';
				/* erase on console: backspace, space, backspace */
				logger_printf("\b \b");
				fflush(stdout);
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
				fflush(stdout);
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

void clear_line()
{
	int width;
	char buf_stack[4092];
	if (get_terminal_size(&width, 0))
	{
		for (int i = 0; i < width; i++)
			buf_stack[i] = ' ';
		buf_stack[width] = '\0';
		logger_printf("\r%s\r", buf_stack);
		fflush(stdout);
	}
}

void termination_signal(int sig)
{
	putc('\n', stdout);

	socket_cleanup();
	console_shutdown();
	logger_shutdown();

	exit(sig);
}
#include "tui_selector.h"

int main(int argc, char **args)
{
	if (logger_init() != 0)
	{
		fprintf(stderr, "Failed to initialize logger\n");
	}
	if (socket_init() != 0)
	{
		fprintf(stderr, "Failed to initialize sockets\n");
		return 1;
	}
	if (console_init() != 0)
	{
		fprintf(stderr, "Failed to initialize console input\n");
		return 1;
	}
	signal(SIGINT, termination_signal);

	_key_t escape_key[] = { '\r', '\n' };
	uint8_t exit = 0;
	while(!exit)
	{
		int service_selected;
		print_color("Hello and welcome to clchat.\n\
Here you can create your own server and have your friend connect to you\n\
Or you can have your friend became the server and easily connect throw the code that he/she gave u\n\
choose an service and then press enter to continue : \n", CLR_BRIGHT_GREEN, CLR_BG_BLACK);

		const char* services[] =
		{
			"Connect to a room",
			"Create a room"
		};
		service_selected = tui_seletor(services, 2, '>', 3);
		terminal_clear();
		if (service_selected == 0)
		{
			if (client_connect() < 0)
			{
				logger_printf("some error happned during connection.\n");
				exit = 1;
			}
		}
		else if (service_selected == 1)
		{
			if (create_server() < 0)
			{
				logger_printf("some error happned during creation.\n");
				exit = 1;
			}
		}
		else
		{
			logger_printf("invalid input. %s\n", service_selected);
			exit = 1;
		}
	}

	/* global cleanup */
	socket_cleanup();
	console_shutdown();
	logger_shutdown();
	return 0;
}
