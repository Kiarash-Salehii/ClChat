
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
#define HTTP_CLIENT_LIB
#include "server.h"

static uint32_t get_address()
{
	http_client_t http_client;
	http_connect(&http_client, "api.ipify.org", 80);
	http_get(&http_client, "/");
	int content_length = 0;
	int line = 0;
	for (int i = 0; i < http_client.response.body_len; i++)
	{

		if (http_client.response.body[i] == '\n')
		{
			++line;
		}
		if (line == 3)
		{
			(void)sscanf(&http_client.response.body[i], "Content-Length: %d", &content_length);
		}
	}
	uint32_t address = (uint32_t)inet_addr(http_client.response.body + http_client.response.body_len - content_length);
	http_close(&http_client);
	return address;
}

static uint64_t hash(uint8_t* str, uint32_t str_len)
{
	unsigned long hash = 5381;
	int c;

	for (int i = 0; i < str_len; i++)
	{
		c = *str++;
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}

	return hash;
}

typedef struct
{
	uint64_t id;
	int sock;
	const char* username;
	size_t username_len;
} user_t;

typedef struct
{
	user_t** users;
	uint32_t* user_count;
	uint8_t* room_alive;
} server_thread_wraper_t;

static mutex_t server_mutex;

static void server_reciver_thread_call(void* arg)
{
	server_thread_wraper_t* wrap = ((void**)arg)[0];
	packet_t packet;
	while (1)
	{
		/* Take a short snapshot under the mutex, then release it before doing any blocking I/O */
		mutex_lock(&server_mutex);
		if (!*(wrap->room_alive))
		{
			mutex_unlock(&server_mutex);
			break;
		}

		uint32_t user_count_snapshot = *wrap->user_count;
		user_t* users_snapshot = *wrap->users;

		if (user_count_snapshot == 0)
		{
			mutex_unlock(&server_mutex);
			/* small busy-wait avoidance: you may replace with a platform sleep if desired */
			continue;
		}

		/* allocate local copy of the user entries while still under the lock */
		user_t* local_users = malloc(sizeof(user_t) * user_count_snapshot);
		if (!local_users)
		{
			mutex_unlock(&server_mutex);
			continue;
		}
		for (uint32_t i = 0; i < user_count_snapshot; ++i)
			local_users[i] = users_snapshot[i];

		/* now release */
		mutex_unlock(&server_mutex);

		for (uint32_t i = 0; i < user_count_snapshot; ++i)
		{
			rewind(stdin);
			if (local_users[i].sock > 0)
			{
				memset(&packet, 0, sizeof(packet_t));
				packet_recv(local_users[i].sock, &packet);
				if (packet.header.type == PKT_CHAT)
				{
					int user_index = -1;
					const char* username = NULL;
					size_t username_length = 0;
					for (uint32_t j = 0; j < user_count_snapshot; ++j)
					{
						if (packet.payload.buffer->chat.user_id == local_users[j].id)
						{
							user_index = (int)j;
							username = local_users[j].username ? local_users[j].username : "(unknown)";
							username_length = local_users[j].username_len;
							break;
						}
					}

					char stack_buffer[4092];
					char msg_fmt[] = "u%ld->%s --- %s";
					char* msg = stack_buffer;
					int size_needed = sprintf(stack_buffer, msg_fmt, packet.payload.buffer->chat.user_id, username, (const char*)packet.payload.chunks[0].ptr);
					if (size_needed > sizeof(stack_buffer))
					{
						msg = malloc(size_needed + 1);
						if (msg == 0)
							continue;
						sprintf(msg, msg_fmt, packet.payload.buffer->chat.user_id, username, (const char*)packet.payload.chunks[0].ptr);
					}

					clear_line();
					logger_printf("%s\n", msg);

					uint32_t msg_len = (uint32_t)size_needed;
					packet_t msg_packet;
					packet_begin(&msg_packet, PKT_CHAT, get_new_packet_id(PKT_CHAT));
					packet_write_u64(&msg_packet, packet.payload.buffer->chat.user_id);
					packet_write_u32(&msg_packet, msg_len);
					packet_append(&msg_packet, msg, msg_len);

					for (uint32_t j = 0; j < user_count_snapshot; ++j)
					{
						if ((int)j == user_index) continue;
						packet_send(&msg_packet, local_users[j].sock);
					}

					if(sizeof(stack_buffer) < size_needed) free(msg);
					/* Assume packet_destroy or similar is not required here for incoming packet (kept original behavior) */
				}

				else if (packet.header.type == PKT_FILE)
				{
					int user_index = -1;
					const char* username = NULL;
					size_t username_length = 0;
					for (uint32_t j = 0; j < user_count_snapshot; ++j)
					{
						if (packet.payload.buffer->chat.user_id == local_users[j].id)
						{
							user_index = (int)j;
							username = local_users[j].username;
							username_length = local_users[j].username_len;
							break;
						}
					}

					char msg_buffer[1024];
					sprintf(msg_buffer, "\"%s\" sent a file.", username);
					uint32_t msg_len = strlen(msg_buffer);
					logger_printf("%s\n", msg_buffer);

					packet_t msg_packet;
					packet_begin(&msg_packet, PKT_CHAT, get_new_packet_id(PKT_CHAT));
					packet_write_u64(&msg_packet, packet.payload.buffer->chat.user_id);
					packet_write_u32(&msg_packet, msg_len);
					packet_append(&msg_packet, msg_buffer, msg_len);

					for (uint32_t j = 0; j < user_count_snapshot; ++j)
					{
						if ((int)j == user_index) continue;
						packet_send(&msg_packet, local_users[j].sock);
						packet_send(&packet, local_users[j].sock);
					}

					clear_line();
					print_color("You have recived a file. Do you wanna keep it? (Y/N) ", CLR_GREEN, CLR_BG_BLACK);
					char buffer[20];
					if (!fgets(buffer, sizeof(buffer), stdin))
						break;
					if (tolower(*buffer) == 'n') continue;
					if (packet.payload.buffer->file.user_id == 0)
					{
						clear_line();
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
					int user_index = -1;
					const char* username = NULL;
					size_t username_length = 0;
					for (uint32_t j = 0; j < user_count_snapshot; ++j)
					{
						if (packet.payload.buffer->chat.user_id == local_users[j].id)
						{
							user_index = (int)j;
							username = local_users[j].username;
							username_length = local_users[j].username_len;
							break;
						}
					}

					char msg_buffer[1024];
					sprintf(msg_buffer, "\"%s\" sent a image.", username);
					uint32_t msg_len = strlen(msg_buffer);
					clear_line();
					logger_printf("%s\n", msg_buffer);

					packet_t msg_packet;
					packet_begin(&msg_packet, PKT_CHAT, get_new_packet_id(PKT_CHAT));
					packet_write_u64(&msg_packet, packet.payload.buffer->chat.user_id);
					packet_write_u32(&msg_packet, msg_len);
					packet_append(&msg_packet, msg_buffer, msg_len);

					for (uint32_t j = 0; j < user_count_snapshot; ++j)
					{
						if ((int)j == user_index) continue;
						packet_send(&msg_packet, local_users[j].sock);
						packet_send(&packet, local_users[j].sock);
					}

					clear_line();
					print_color("You have recived an image. Do you wanna keep it? (Y/N) ", CLR_GREEN, CLR_BG_BLACK);
					char buffer[20];
					if (!fgets(buffer, sizeof(buffer), stdin))
						break;
					if (tolower(*buffer) == 'n') continue;
					char file_name[124];
					sprintf(file_name, "%u.%s", time(0) % UINT32_MAX, (const char*)packet.payload.chunks[1].ptr);
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
		}

		free(local_users);
	}
}

static void server_sender_thread_call(void* arg)
{
	server_thread_wraper_t* wrap = ((void**)arg)[0];

	print_color("1- [c]hat.\n2- send [f]ile.\n3- send [i]mage.\n4- [q]uit.\n", CLR_BLUE, CLR_BG_BLACK);
	char buffer[2048];
	_key_t escaper_key[] = { '\n', KEY_ESCAPE, '\r' };
	while (1)
	{
		mutex_lock(&server_mutex);
		if (!*(wrap->room_alive))
		{
			mutex_unlock(&server_mutex);
			break;
		}
		mutex_unlock(&server_mutex);

		print_color("choose an option : \r", CLR_BLUE, CLR_BG_BLACK);
		fflush(stdout);
		_key_t key;

		if (key = console_poll_key())
		{
			mutex_lock(&server_mutex);
				uint32_t user_count_snapshot = *wrap->user_count;
				user_t* users_snapshot = *wrap->users;
				if (user_count_snapshot == 0)
				{
					mutex_unlock(&server_mutex);
					continue;
				}
				int* sockets = malloc(sizeof(int) * user_count_snapshot);
				if (!sockets)
				{
					mutex_unlock(&server_mutex);
					continue;
				}
				for (uint32_t i = 0; i < user_count_snapshot; ++i)
					sockets[i] = users_snapshot[i].sock;
				mutex_unlock(&server_mutex);
			clear_line();
			switch (tolower(key))
			{
			case 'c':
			{
				get_str(buffer, sizeof(buffer), escaper_key, sizeof(escaper_key) / sizeof(_key_t));
				logger_printf("\n");

				/* strip newline */
				size_t len = strlen(buffer);
				if (len > 0 && buffer[len - 1] == '\n') buffer[--len] = '\0';
				if (len == 0) continue;
				
				packet_send_chat(sockets, user_count_snapshot, 0, buffer, len);

				break;
			}
			case 'f':
			{
				/* unchanged file-read logic up to packet construction (kept original behavior) */
				logger_printf("\rgive the path to the file you wanna send : ");
				get_str(buffer, sizeof(buffer), escaper_key, sizeof(escaper_key) / sizeof(_key_t));
				logger_printf("\n");

				char file_name[2048];
				logger_printf("give it a name : ");
				get_str(file_name, sizeof(file_name), escaper_key, sizeof(escaper_key) / sizeof(key_t));
				logger_printf("\n");

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

				packet_send_file(sockets, user_count_snapshot, 0, file, file_name, filename_len, 1, FILE_STREAMING_BUFFER_SIZE);
				//fseek(file, 0, SEEK_END);
				//size_t file_size = ftell(file);
				//rewind(file);
				//if (file_size == 0)
				//{
				//	print_color("zero byte file size.\n", CLR_BRIGHT_RED, CLR_BG_BLACK);
				//	fclose(file);
				//	continue;
				//}

				//void* file_data = malloc(file_size);
				//size_t read_size = fread(file_data, 1, file_size, file);
				//fclose(file);
				//if (read_size != file_size)
				//{
				//	printf_color("read data size doesn't match the file size. file size : %ld -- read size : %ld\n", CLR_RED, CLR_BG_BLACK, file_size, read_size);
				//	free(file_data);
				//	continue;
				//}

				//packet_t packet;
				//packet_begin(&packet, PKT_FILE, get_new_packet_id(PKT_FILE));
				//packet_write_u64(&packet, 0);
				//packet_write_u32(&packet, (uint32_t)filename_len);
				//packet_write_u32(&packet, (uint32_t)file_size);
				//packet_append(&packet, file_name, (uint32_t)filename_len);
				//packet_append(&packet, file_data, (uint32_t)file_size);

				///* snapshot sockets under lock then release before sending */
				//
				//free(file_data);
				//packet_destroy(&packet);
				break;
			}
			case 'i':
			{
				/* similar approach to 'f' case */
				logger_printf("give the path to the image you wanna send : ");
				get_str(buffer, sizeof(buffer), escaper_key, sizeof(escaper_key) / sizeof(_key_t));
				logger_printf("\n");

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

				packet_send_image(sockets, user_count_snapshot, 0, file, 1, FILE_STREAMING_BUFFER_SIZE);
				/*fseek(file, 0, SEEK_END);
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
				packet_write_u64(&packet, 0);
				packet_write_u32(&packet, (uint32_t)file_size);
				packet_write_u32(&packet, extension_length);
				packet_append(&packet, file_data, (uint32_t)file_size);
				packet_append(&packet, extension, extension_length);

				for (uint32_t i = 0; i < user_count_snapshot_i; ++i)
					packet_send(&packet, sockets_i[i]);

				free(sockets_i);
				free(file_data);
				packet_destroy(&packet);*/
				break;
			}
			case 'q':
			{
				packet_send_quit(sockets, user_count_snapshot, 0, "ADMIN", 5);
				break;
			}
			case EOF:
			{
				print_color("\rinvalid input.", CLR_RED, CLR_BG_BLACK);
				fflush(stdout);
				for (int i = 0; i < 0xffffff; i++);
			}
			}
			free(sockets);
		}
	}
}

int create_server(void)
{
	chat_t chat;
	create_chat(&chat);
	chat_setup_t setup = { 0 };
	{
		char ans[124] = { 0 };
		_key_t escape_key[] = { '\r', '\n' };
		size_t escape_key_len = sizeof(escape_key) / sizeof(_key_t);
		logger_prompt_gets(ans, sizeof(ans), escape_key, escape_key_len, "What is gonna be the maxium users of your room (number) : ");
		sscanf(ans, "%d", &setup.max_user);
		logger_prompt_gets(ans, sizeof(ans), escape_key, escape_key_len, "Is your room gonna have a pin (Y/N) : ");
		if ((*ans = tolower(ans[0])) > EOF)
		{
			if (*ans == 'y')
			{
				logger_prompt_gets(ans, sizeof(ans), escape_key, escape_key_len, "Choose a pin for it (number) : ");
				sscanf(ans, "%hd", &setup.room_pass);
				setup.is_room_locked = 1;
			}
			else if (*ans != 'n')
			{
				logger_printf("invalid input.\n");
			}
		}
		uint8_t localip = 0;
		logger_prompt_gets(ans, sizeof(ans), escape_key, escape_key_len, "Are you running on local ip (Y/N) : ");
		if ((*ans = tolower(ans[0])) > EOF)
		{
			if (*ans == 'y')
			{
				setup.is_private_ip = 1;
				setup.ip.ip_8[0] = 127;
				setup.ip.ip_8[3] = 1;
				localip = 1;
			}
			else if (*ans != 'n')
			{
				printf("invalid input.\n");
			}
		}
		logger_prompt_gets(ans, sizeof(ans), escape_key, escape_key_len, "What is the ip port number your gonna use for this room : ");
		sscanf(ans, "%hd", &setup.port);
		if(!localip)
		logger_printf("(make sure that port forwarding is enabled for this port number on your router or check out ./port_forwading_help.txt for more guide)\n\n");
	}

	terminal_clear();

	chat_room_t room = setup_room(chat, &setup);

	logger_printf("\r\n--- your room connecion code : \"\" ---");
	fflush(stdout);

	uint8_t room_code[6] = { 0 };
	if (!setup.is_private_ip)
	{
		uint32_t pub_ip_address = get_address();
		memcpy(room_code, &pub_ip_address, 4);
	}
	else
	{
		memcpy(room_code, setup.ip.ip_8, 4);
	}

	*(uint16_t*)&room_code[4] = setup.port;
	char room_code_encoded_base32[11] = { 0 };
	base32_encode6(room_code, room_code_encoded_base32);

	logger_printf("\r--- your room connecion code : \"%s\" ---\n", room_code_encoded_base32);
	fflush(stdout);

	logger_printf("\nroom is open for the users to connect.\n");

	uint8_t room_alive = 1;
	uint32_t users_count = 1;
	user_t* users = malloc(users_count * sizeof(user_t));

	packet_t packet;
	memset(&packet, 0, sizeof(packet_t));

	int mutex_r = mutex_init(&server_mutex);
	logger_printf("mutex_init() returned %d.\n", mutex_r);
	if(mutex_r != 0)
	{
		
		return mutex_r;
	}

	server_thread_wraper_t server_wrap = { 0 };
	server_wrap.users = &users;
	server_wrap.user_count = &users_count;
	server_wrap.room_alive = &room_alive;
	thread_t sender_thread, reciver_thread;
	thread_begin(&sender_thread, (THREADCALLBACK)server_sender_thread_call, 1, &server_wrap);
	thread_begin(&reciver_thread, (THREADCALLBACK)server_reciver_thread_call, 1, &server_wrap);
	while (1)
	{
		mutex_lock(&server_mutex);
		if (!room_alive)
		{
			mutex_unlock(&server_mutex);
			break;
		}
		mutex_unlock(&server_mutex);

		int sock = chat_accept_user(room);
		if (sock > 0)
		{
			packet_recv(sock, &packet);
			if (packet.header.type == PKT_LOGIN)
			{
				if (setup.is_room_locked && packet.payload.buffer->login.room_pin != setup.room_pass)
				{
					packet_send_chat(&sock, 1, 0, "wrong password.", 16);
					chat_kick_by_sock(sock);
					continue;
				}


				mutex_lock(&server_mutex);
				user_t* tmp = realloc(users, sizeof(*users) * (users_count + 1));
				if (!tmp) {
					// allocation failed: report and close socket, keep users_count unchanged
					print_color("Memory allocation failed for new user.\n", CLR_RED, CLR_BG_BLACK);
					mutex_unlock(&server_mutex);
					packet_send_chat(&sock, 1, 0, "server error: cannot accept new user.", 38);
					closesocket(sock);
					continue;
					// handle failure: send error to client / close socket etc.
				}
				else {
					users = tmp;
					users_count++;
					users[users_count - 1].sock = sock;
					users[users_count - 1].username_len = packet.payload.buffer->login.username_length;
					char* username = malloc(users[users_count - 1].username_len + 1);
					memcpy(username, packet.payload.chunks[0].ptr, users[users_count - 1].username_len);
					username[users[users_count - 1].username_len] = '\0';
					users[users_count - 1].username = username;
					mutex_unlock(&server_mutex);
				}



				uint64_t user_id = hash(users[users_count - 1].username, users[users_count - 1].username_len) % UINT32_MAX;
				for (int i = 0; i < users_count - 1; i++)
				{
					if (users[i].id == user_id)
					{
						++user_id;
					}
				}
				users[users_count - 1].id = user_id;

				packet_t packet_login_ok;
				packet_begin(&packet_login_ok, PKT_LOGIN_OK, get_new_packet_id(PKT_LOGIN_OK));
				packet_write_u64(&packet_login_ok, user_id);
				packet_write_u32(&packet_login_ok, users[users_count - 1].username_len);
				packet_append(&packet_login_ok, users[users_count - 1].username, users[users_count - 1].username_len);
				packet_send(&packet_login_ok, sock);
				packet_destroy(&packet_login_ok);

				char login_msg[1028] = { 0 };
				int len = sprintf(login_msg, "user \"%s\" accepted to the room with %ld id number.\n", users[users_count - 1].username, user_id);
				if (len < 0)
				{

				}

				clear_line();
				print_color(login_msg, CLR_MAGENTA, CLR_BG_BLACK);
				packet_send_chat(&sock, 1, 0, login_msg, (uint32_t)len);
			}
		}
		else if (sock < 0)
		{
			perror("accept");
		}
	}

	mutex_lock(&server_mutex);
	for (int i = 0; i < users_count; i++)
	{
		if (users[i].username)
		{
			free(users[i].username);
		}
	}
	mutex_unlock(&server_mutex);

	mutex_destroy(&server_mutex);
	destroy_chat(chat);
	return 0;
}
