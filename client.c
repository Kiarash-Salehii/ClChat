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
	client_thread_wraper_t* wrap = (client_thread_wraper_t*)arg;

	print_color("1- [c]hat.\n2- send [f]ile.\n3- send [i]mage.\n4- [q]uit.\n", CLR_BLUE, CLR_BG_BLACK);

	char buffer[2048];
	key_t escaper_key[] = { '\n', KEY_ESCAPE, '\r' };
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
		key_t key;
		if (key = console_poll_key())
		{
			clear_line();
			switch (tolower(key))
			{
			case 'c':
			{
				get_str(buffer, sizeof(buffer), escaper_key, sizeof(escaper_key) / sizeof(key_t));
				logger_printf("\n");

				/* strip newline */
				size_t len = strlen(buffer);
				if (len > 0 && buffer[len - 1] == '\n') buffer[--len] = '\0';
				if (len == 0) continue;

				packet_t packet;
				packet_begin(&packet, PKT_CHAT, get_new_packet_id(PKT_CHAT));
				packet_write_u64(&packet, wrap->user_id);
				packet_write_u32(&packet, (uint32_t)len);
				packet_append(&packet, buffer, (uint32_t)len);

				packet_send(&packet, wrap->sock);

				packet_destroy(&packet);
				break;
			}
			case 'f':
			{
				/* unchanged file-read logic up to packet construction (kept original behavior) */
				logger_printf("give the path to the file you wanna send : ");
				get_str(buffer, sizeof(buffer), escaper_key, sizeof(escaper_key) / sizeof(key_t));

				char file_name[2048];
				logger_printf("give it a name : ");
				get_str(file_name, sizeof(file_name), escaper_key, sizeof(escaper_key) / sizeof(key_t));

				/* strip newline */
				size_t path_len = strlen(buffer);
				if (path_len > 0 && buffer[path_len - 1] == '\n') buffer[--path_len] = '\0';
				if (path_len == 0) continue;

				size_t filename_len = strlen(file_name);
				if (filename_len > 0 && file_name[filename_len - 1] == '\n') file_name[--filename_len] = '\0';
				if (filename_len == 0) continue;

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

				void* file_data = malloc(file_size);
				size_t read_size = fread(file_data, 1, file_size, file);
				fclose(file);
				if (read_size != file_size)
				{
					printf_color("read data size doesn't match the file size. file size : %ld -- read size : %ld\n", CLR_RED, CLR_BG_BLACK, file_size, read_size);
					free(file_data);
					continue;
				}

				packet_t packet;
				packet_begin(&packet, PKT_FILE, get_new_packet_id(PKT_FILE));
				packet_write_u64(&packet, wrap->user_id);
				packet_write_u32(&packet, (uint32_t)filename_len);
				packet_write_u32(&packet, (uint32_t)file_size);
				packet_append(&packet, file_name, (uint32_t)filename_len);
				packet_append(&packet, file_data, (uint32_t)file_size);
				packet_send(&packet, wrap->sock);
				packet_destroy(&packet);
				break;
			}
			case 'i':
			{
				/* similar approach to 'f' case */
				logger_printf("give the path to the image you wanna send : ");
				get_str(buffer, sizeof(buffer), escaper_key, sizeof(escaper_key) / sizeof(key_t));

				/* strip newline */
				size_t path_len = strlen(buffer);
				if (path_len > 0 && buffer[path_len - 1] == '\n') buffer[--path_len] = '\0';
				if (path_len == 0) continue;

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

				void* file_data = malloc(file_size);
				size_t read_size = fread(file_data, 1, file_size, file);
				fclose(file);
				if (read_size != file_size)
				{
					printf_color("read data size doesn't match the file size. file size : %ld -- read size : %ld\n", CLR_RED, CLR_BG_BLACK, file_size, read_size);
					free(file_data);
					continue;
				}

				uint32_t extension_length = 0;
				char* extension = 0;

				for (int i = path_len - 1; i > 0; i--)
				{
					if (buffer[i] != 0 || buffer[i] != ' ')
						++extension_length;
					if (buffer[i] == '.')
					{
						extension = &buffer[i + 1];
						break;
					}
				}

				packet_t packet;
				packet_begin(&packet, PKT_IMAGE, get_new_packet_id(PKT_IMAGE));
				packet_write_u64(&packet, wrap->user_id);
				packet_write_u32(&packet, (uint32_t)file_size);
				packet_write_u32(&packet, extension_length);
				packet_append(&packet, file_data, (uint32_t)file_size);
				packet_append(&packet, extension, extension_length);
				packet_send(&packet, wrap->sock);
				packet_destroy(&packet);
				break;
			}
			case 'q':
			{
				key_t termins[] = { '\n', '\r' };
				char stack_buf[3];
				logger_prompt_gets(stack_buf, sizeof(stack_buf), termins, 2, "Are you sure to quit chat? (Y/N) ");
				if (tolower(*stack_buf) == 'n') break;
				if (!wrap->username) break;
				size_t len = strlen(wrap->username);
				packet_t packet;
				packet_begin(&packet, PKT_QUIT, get_new_packet_id(PKT_QUIT));
				packet_write_u64(&packet, wrap->user_id);
				packet_write_u32(&packet, (uint32_t)strlen(wrap->username));
				packet_append(&packet, wrap->username, (uint32_t)len);
				packet_send(&packet, wrap->sock);
				packet_destroy(&packet);

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
		char room_code[11];
		logger_printf("What is code of your room (number) : ");
		scanf("%s", room_code);
		logger_printf("Is your room have a pin (Y/N) : ");
		char ans[124] = { 0 };
		scanf("%s", ans);
		if ((*ans = tolower(ans[0])) > EOF)
		{
			if (*ans == 'y')
			{
				logger_printf("enter pin (number) : ");
				scanf("%hd", &room_pin);
				room_has_pin = 1;
			}
			else if (*ans != 'n')
			{
				printf("invalid input.\n");
			}
		}
		logger_printf("choose an username for yourself : ");
		scanf("%s", username);
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
	thread_begin(&sender_thread, client_sender_thread_call, 1, &thread_wrap_data);
	packet_t packet;
	memset(&packet, 0, sizeof(packet));
	rewind(stdin);
	rewind(stdout);
	while (1)
	{
		mutex_lock(&client_mutex);
		if (!connection_alive) 
		{
			mutex_unlock(&client_mutex);
			break;
		}
		mutex_unlock(&client_mutex);

		fflush(stdout);
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
			logger_printf("%s\n", (const char*)packet.payload.chunks[0].ptr);
			logger_unlock();
		}
		else if (packet.header.type == PKT_FILE)
		{
			clear_line();
			print_color("You have recived a file. Do you wanna keep it? (Y/N) ", CLR_GREEN, CLR_BG_BLACK);
			char buffer[20];
			if (!fgets(buffer, sizeof(buffer), stdin))
				break;
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
			if (!fgets(buffer, sizeof(buffer), stdin))
				break;
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