#include "server.h"
#include "client.h"

#include <signal.h>

int get_str(char buffer[], int count, key_t* escapers, int escapers_count)
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
				printf("%c", (char)key);
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

typedef int sock_close_t;
static int socket_init(void)
{
	WSADATA wsa;
	return (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) ? 0 : -1;
}
static void socket_cleanup(void)
{
	WSACleanup();
}
static void close_socket(int s)
{
	if (s != INVALID_SOCKET) closesocket((SOCKET)s);
}
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

typedef int SOCKET;
static int socket_init(void) { return 0; }
static void socket_cleanup(void) { (void)0; }
static void close_socket(int s) { if (s >= 0) close(s); }
#endif

void termination_signal(int sig)
{
	socket_cleanup();
	console_shutdown();
	logger_shutdown();

	exit(sig);
}

int main(void)
{
	if (logger_init() != 0)
	{
		fprintf(stderr, "Failed to initialize logger\n");
	}
	/* initialize sockets for the whole process early (Windows WSA) */
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

	uint8_t exit = 0;
	while(!exit)
	{
		print_highlighted("Hello and welcome to clchat.\n\
	Here you can create your own server and have your friend connect to you\n\
	Or you can have your friend became the server and easily connect throw the code that he/she gave u\n\
	1 - Connect to a room\n\
	2 - Create a room\n\
	choose an service and then press enter to continue : ", CLR_BG_BLACK);
		int service_selected;
		(void)scanf("%d", &service_selected);
		terminal_clear();
		if (service_selected == 1)
		{
			if (client_connect() < 0)
			{
				logger_printf("some error happned during connection.\n");
				exit = 1;
			}
		}
		else if (service_selected == 2)
		{
			if (create_server() < 0)
			{
				logger_printf("some error happned during creation.\n");
				exit = 1;
			}
		}
		else
		{
			logger_printf("invalid input.\n");
			exit = 1;
		}
	}

	/* global cleanup */
	socket_cleanup();
	console_shutdown();
	logger_shutdown();
	return 0;
}