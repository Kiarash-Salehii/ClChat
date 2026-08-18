#pragma once

#include <stdint.h>
#ifndef CHAT_CORE_H_
#define CHAR_CORE_H_

#if defined(_WIN32) || defined(WIN32) || defined(__MINGW32__) || defined(_MSC_VER)
#define _CHAT_WINDOWS_
#include <WinSock2.h>
#include <WS2tcpip.h>
WSADATA wsa_data;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

typedef int SOCKET;
static void closesocket(int s) { if (s >= 0) close(s); }
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
	typedef uint8_t err;
	typedef union
	{
		uint8_t ip_8[4];
		uint32_t ip_32;
	} ipv4;
	typedef struct
	{
		ipv4 ip;
		const char* username;
		const char* msg;
		int max_user;
		uint16_t room_pass;
		uint16_t port;
		uint8_t is_room_locked;
		uint8_t is_private_ip;
	} chat_setup_t;
	typedef char* chat_t;
	typedef char* chat_room_t;
	void create_chat(chat_t* chat);
	void destroy_chat(chat_t chat);
	chat_room_t setup_room(chat_t chat, chat_setup_t* setup_setting);
	int chat_accept_user(chat_room_t room);
	void chat_kick_users(chat_room_t room);
	void chat_kick_by_sock(int user);
	int chat_last_connected_user(chat_room_t room);
	int* get_chat_connected_users(chat_room_t room);
	int get_chat_connected_user_length(chat_room_t room);


#ifdef HTTP_CLIENT_LIB
	typedef struct
	{
		const char* domain;
		size_t domain_len;
		unsigned short port_num;
		unsigned int ip_address;
		int sock;
		struct
		{
			char* body;
			size_t body_len;
		} response;
	} http_client_t;

	void http_connect(http_client_t* client, const char* domain, unsigned short port);
	void http_get(http_client_t* client, const char* get);
	void http_send(http_client_t* client, const char* message, size_t messsage_len);
	void http_recv(http_client_t* client);
	void http_close(http_client_t* client);
#endif
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CHAT_CORE_H_ */
