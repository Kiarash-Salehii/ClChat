#include "client.h"

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


typedef struct
{
	uint8_t* connection_alive;
	const char* username;
	uint64_t user_id;
	int sock;
} client_thread_wraper_t;

mutex_t client_mutex;

void client_sender_thread_call(void* arg)
{
	client_thread_wraper_t* wrap = (client_thread_wraper_t*)((void**)arg)[0];

	print_color("1- [c]hat.\n2- send [f]ile.\n3- send [i]mage.\n4- [q]uit.\n", CLR_BLUE, CLR_BG_BLACK);

	char buffer[2048];
	_key_t escaper_key[] = { '\n', KEY_ESCAPE, '\r' };
	while (1)
	{
		mutex_lock(&client_mutex);
		if (!*wrap->connection_alive)
		{
			mutex_unlock(&client_mutex);
			break;
		}
		mutex_unlock(&client_mutex);

		print_color("choose an option : \r", CLR_BLUE, CLR_BG_BLACK);
		fflush(stdout);
		_key_t key;
		_key_t termins[] = { '\n', '\r' };
		if (key = console_poll_key())
		{
			clear_line();
			switch (tolower(key))
			{
			case 'c':
			{
				logger_printf("> ");
				get_str(buffer, sizeof(buffer), escaper_key, sizeof(escaper_key) / sizeof(_key_t));
				logger_printf("\n");

				/* strip newline */
				size_t len = strlen(buffer);

				packet_send_chat(&wrap->sock, 1, wrap->user_id, buffer, len);
				break;
			}
			case 'f':
			{
				/* unchanged file-read logic up to packet construction (kept original behavior) */
				logger_prompt_gets(buffer, sizeof(buffer), escaper_key, sizeof(escaper_key) / sizeof(_key_t), "give the path to the file you wanna send : ");

				char file_name[2048];
				logger_prompt_gets(file_name, sizeof(file_name), escaper_key, sizeof(escaper_key) / sizeof(_key_t), "give it a name : ");

				/* strip newline */
				size_t path_len = strlen(buffer);
				size_t filename_len = strlen(file_name);
				

				FILE* file = fopen(buffer, "rb");
				if (file == 0)
				{
					int err = errno;
					printf_color("Error %d: %s\n", CLR_RED, CLR_BG_BLACK, err, strerror(err));
					continue;
				}
				fseek(file, 0, SEEK_END);
				size_t file_size = ftell(file);
				rewind(file);
				if (file_size == 0)
				{
					print_color("zero byte file size.\n", CLR_BRIGHT_RED, CLR_BG_BLACK);
					fclose(file);
					continue;
				}

				packet_send_file(&wrap->sock, 1, wrap->user_id, file, file_name, filename_len, 1, FILE_STREAMING_BUFFER_SIZE);
				break;
			}
			case 'i':
			{
				/* similar approach to 'f' case */
				logger_prompt_gets(buffer, sizeof(buffer), escaper_key, sizeof(escaper_key) / sizeof(_key_t), "give the path to the image you wanna send : ");

				/* strip newline */
				size_t path_len = strlen(buffer);

				FILE* file = fopen(buffer, "rb");
				if (file == 0)
				{
					int err = errno;
					printf_color("Error %d: %s\n", CLR_RED, CLR_BG_BLACK, err, strerror(err));
					continue;
				}
				fseek(file, 0, SEEK_END);
				size_t file_size = ftell(file);
				rewind(file);
				if (file_size == 0)
				{
					print_color("0 byte file size.\n", CLR_RED, CLR_BG_BLACK);
					fclose(file);
					continue;
				}

				packet_send_image(&wrap->sock, 1, wrap->user_id, file, 1, FILE_STREAMING_BUFFER_SIZE);
				break;
			}
			case 'q':
			{
				logger_prompt_gets(buffer, sizeof(buffer), escaper_key, sizeof(escaper_key) / sizeof(_key_t), "Are you sure to quit chat? (Y/N) ");
				if (tolower(*buffer) == 'n') break;
				if (!wrap->username) break;
				size_t len = strlen(wrap->username);
				packet_send_quit(&wrap->sock, 1, wrap->user_id, wrap->username, len);

				mutex_lock(&client_mutex);
				*wrap->connection_alive = 0;
				mutex_unlock(&client_mutex);
				break;
			}
			case EOF:
			{
				clear_line();
				print_color("invalid input.", CLR_RED, CLR_BG_BLACK);
				fflush(stdout);
				for (int i = 0; i < 0xffffff; i++);
			}
			}
		}
	}
}



int client_connect(void)
{
	ipv4 ip = { 0 };
	uint32_t username_len;
	uint16_t port;
	uint16_t room_pin = 0;
	uint8_t room_has_pin = 0;
	char username[256] = { 0 };
	{
		char ans[512] = { 0 };
		_key_t escape_key[] = { '\r', '\n' };
		size_t escape_key_len = sizeof(escape_key) / sizeof(_key_t);
		char room_code[11];
		logger_prompt_gets(ans, sizeof(ans), escape_key, escape_key_len, "What is code of your room (number) : ");
		sscanf(ans, "%s", room_code);
		logger_prompt_gets(ans, sizeof(ans), escape_key, escape_key_len, "Is your room have a pin (Y/N) : ");
		if ((*ans = tolower(ans[0])) > EOF)
		{
			if (*ans == 'y')
			{
				logger_prompt_gets(ans, sizeof(ans), escape_key, escape_key_len, "enter pin (number) : ");
				sscanf(ans, "%hd", &room_pin);
				room_has_pin = 1;
			}
			else if (*ans != 'n')
			{
				logger_printf("invalid input.\n");
			}
		}
		logger_prompt_gets(ans, sizeof(ans), escape_key, escape_key_len, "choose an username for yourself : ");
		sscanf(ans, "%s", username);
		username_len = strlen(username);
		uint8_t ip_port[6] = { 0 };
		base32_decode6(room_code, ip_port);
		memcpy(&ip, ip_port, 4);
		port = *(uint16_t*)&ip_port[4];
	}

	terminal_clear();

	/* Ensure sockets are initialized (WSA on Windows) */
	if (socket_init() != 0)
	{
		fprintf(stderr, "socket initialization failed\n");
		return -1;
	}

	int sock = -1;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
	{
		perror("socket");
		socket_cleanup();
		return -1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	/* ip.ip_32 is expected to be in network byte order based on how server stored it */
	addr.sin_addr.s_addr = ip.ip_32;

	int conn_ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
	if (conn_ret < 0)
	{
		perror("connect");
		close_socket(sock);
		socket_cleanup();
		printf("Could not connect.\n");
		return -1;
	}

	//debug log only
	logger_printf("connected to the server.\n");

	packet_t login;
	packet_begin(&login, PKT_LOGIN, get_new_packet_id(PKT_LOGIN));
	packet_write_u32(&login, (uint32_t)room_pin);
	packet_write_u32(&login, username_len);
	packet_append(&login, username, username_len);
	packet_send(&login, sock);

	packet_t login_ok;
	memset(&login_ok, 0, sizeof(login_ok));
	packet_recv(sock, &login_ok);
	if (login_ok.header.type == PKT_CHAT)
	{
		fwrite(login_ok.payload.chunks[0].ptr, login_ok.payload.chunks[0].size, 1, stdout);
		/* leave socket open until program termination */
		/* cleanup sockets when done */
		return 1;
	}
	else if (login_ok.header.type != PKT_LOGIN_OK)
	{
		packet_destroy(&login_ok);
		close_socket(sock);
		socket_cleanup();
		return -1;
	}
	uint64_t user_id = login_ok.payload.buffer->login_ok.user_id;

	packet_destroy(&login_ok);
	packet_destroy(&login);

	uint8_t connection_alive = 1;

	mutex_init(&client_mutex);

	client_thread_wraper_t thread_wrap_data = { 0 };
	thread_wrap_data.connection_alive = &connection_alive;
	thread_wrap_data.sock = sock;
	thread_wrap_data.user_id = user_id;
	thread_wrap_data.username = username;
	thread_t sender_thread;
	thread_begin(&sender_thread, (THREADCALLBACK)client_sender_thread_call, 1, &thread_wrap_data);
	packet_t packet;
	memset(&packet, 0, sizeof(packet));
	while (1)
	{
		mutex_lock(&client_mutex);
		if (!connection_alive) 
		{
			mutex_unlock(&client_mutex);
			break;
		}
		mutex_unlock(&client_mutex);

		packet_recv(sock, &packet);
		clear_line();
		if (packet.header.type == PKT_CHAT)
		{
			clear_line();
			logger_lock();
			if (packet.payload.buffer->chat.user_id == 0)
			{
				print_color("ADMIN --- ", CLR_RED, CLR_BG_BLACK);
			}
			printf("%s\n", (const char*)packet.payload.chunks[0].ptr);
			logger_unlock();
		}
		else if (packet.header.type == PKT_FILE)
		{
			clear_line();
			print_color("You have recived a file. Do you wanna keep it? (Y/N) ", CLR_GREEN, CLR_BG_BLACK);
			char buffer[20];
			logger_lock();
			_key_t escaper[] = {'\r', '\n'};
			get_str(buffer, sizeof(buffer), escaper, 2);
			logger_unlock();
			if (tolower(*buffer) == 'n') continue;
			if (packet.payload.buffer->file.user_id == 0)
			{
				print_color("ADMIN send you a file.\n", CLR_RED, CLR_BG_BLACK);
			}
			FILE* file = fopen((const char*)packet.payload.chunks[0].ptr, "wb");
			if (file == 0)
			{
				int err = errno;
				printf_color("%s\n", CLR_RED, CLR_BG_BLACK, strerror(err));
			}
			fwrite(packet.payload.chunks[1].ptr, 1, packet.payload.buffer->file.file_size, file);
			fclose(file);
		}
		else if (packet.header.type == PKT_IMAGE)
		{
			clear_line();
			print_color("You have recived an image. Do you wanna keep it? (Y/N) ", CLR_GREEN, CLR_BG_BLACK);
			char buffer[20];
			logger_lock();
			_key_t escaper[] = { '\r', '\n' };
			get_str(buffer, sizeof(buffer), escaper, 2);
			logger_unlock();
			if (tolower(*buffer) == 'n') continue;
			char file_name[124];
			sprintf(file_name, "%u.%s", time(0) / UINT32_MAX, (const char*)packet.payload.chunks[1].ptr);
			FILE* file = fopen(file_name, "wb");
			if (file == 0)
			{
				int err = errno;
				printf_color("%s\n", CLR_RED, CLR_BG_BLACK, strerror(err));
			}
			fwrite(packet.payload.chunks[0].ptr, 1, packet.payload.buffer->image.image_size, file);
			fclose(file);
		}
		else if (packet.header.type == PKT_QUIT)
		{
			clear_line();
			if (packet.payload.buffer->quit.user_id == 0)
			{
				print_color("ADMIN -- exited from the chat.\n", CLR_RED, CLR_BG_BLACK);
			}
			else
			{
				logger_printf("%s exited from chat room.\n", (const char*)packet.payload.chunks[0].ptr);
			}
		}
	}

	/* clean up before exit */
	close_socket(sock);
	socket_cleanup();
	return 0;
}
