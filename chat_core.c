#include "chat_core.h"
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(_MSC_VER)
#define _CHAT_WINDOWS_

#endif

typedef struct
{
	int server;
	int* clients;
	uint32_t client_len;
} _chat_room_t;

typedef struct 
{
	chat_setup_t setup;
	_chat_room_t room;
} _chat_t;

void create_chat(chat_t* chat)
{
	_chat_t* _chat = calloc(1, sizeof(_chat_t));
	if (!_chat)
	{
		return;
	}
	memcpy(chat, &_chat, sizeof(void*));
#ifdef _CHAT_WINDOWS_
	int wsas = WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
}

void destroy_chat(chat_t chat)
{
	free(((_chat_t*)chat)->room.clients);
	free((void*)chat);
#ifdef _WIN32
	WSACleanup();
#endif
}

chat_room_t setup_room(chat_t _chat, chat_setup_t* setup_setting)
{
	if (!_chat || !setup_setting)
	{
		return 0;
	}
	_chat_t* chat = (_chat_t*)_chat;
	memcpy(&chat->setup, setup_setting, sizeof(chat_setup_t));

	/* create socket and validate */
#ifdef _CHAT_WINDOWS_
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) { return 0; }
	chat->room.server = (int)s;
#else
	int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0) { return 0; }
	chat->room.server = s;
#endif

	/* allow immediate reuse of the address (helps during development/restarts) */
	int opt = 1;
#ifdef _CHAT_WINDOWS_
	setsockopt((SOCKET)chat->room.server, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
	setsockopt(chat->room.server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

	/* prepare address */
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(chat->setup.port ? chat->setup.port : 0);

	/* NOTE: ensure `chat->setup.ip` is in the expected byte order.
	   If it's a host-order uint32_t, wrap with htonl(). */
	if (chat->setup.is_private_ip)
	{
		addr.sin_addr.s_addr = chat->setup.ip.ip_32; /* or htonl(chat->setup.ip) if ip is host-order */
	}
	else
	{
		addr.sin_addr.s_addr = 0;
	}

	/* bind with error handling and cleanup */
	if (bind(chat->room.server, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
#ifdef _CHAT_WINDOWS_
		closesocket((SOCKET)chat->room.server);
#else
		close(chat->room.server);
#endif
		return 0;
	}

	/* clamp backlog to SOMAXCONN if necessary */
	int backlog = chat->setup.max_user > 0 ? chat->setup.max_user : 0x7fffffff;

	if (backlog > 0x7fffffff) backlog = 0x7fffffff;

	if (listen(chat->room.server, backlog) < 0)
	{
#ifdef _CHAT_WINDOWS_
		closesocket((SOCKET)chat->room.server);
#else
		close(chat->room.server);
#endif
		return 0;
	}

	return (chat_room_t)&chat->room;
}

int chat_accept_user(chat_room_t _room)
{
	_chat_room_t* room = (_chat_room_t*)_room;
	int client = (int)accept(room->server, 0, 0);
	if (client < 0)
	{
		return client;
	}

	
	if (room->client_len == 0)
	{
		room->clients = calloc(1, sizeof(int));
		++room->client_len;
	}
	else
	{
		++room->client_len;
		room->clients = realloc(room->clients, room->client_len * sizeof(int));
		if (room->clients == 0)
		{
			return -1;
		}
	}

	room->clients[room->client_len - 1] = client;
	return client;
}

void chat_kick_by_sock(int user)
{
	if (user < 0) return;
	closesocket(user);
}

void chat_kick_users(chat_room_t _room)
{
	_chat_room_t* room = (_chat_room_t*)_room;
	for (uint32_t i = 0; i < room->client_len; i++)
	{
		closesocket(room->clients[i]);
	}
	free(room->clients);
}

int chat_last_connected_user(chat_room_t _room)
{
	return ((_chat_room_t*)_room)->clients[((_chat_room_t*)_room)->client_len - 1];
}

int* get_chat_connected_users(chat_room_t _room)
{
	return ((_chat_room_t*)_room)->clients;
}

int get_chat_connected_user_length(chat_room_t _room)
{
	return ((_chat_room_t*)_room)->client_len;
}
