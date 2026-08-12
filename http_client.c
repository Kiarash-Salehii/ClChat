#include <stddef.h>
#include <stdio.h>
#define HTTP_CLIENT_LIB
#include "chat_core.h"

void http_connect(http_client_t* client, const char* domain, unsigned short port)
{
#ifdef _CHAT_WINDOWS_
	int wsas = WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
	struct addrinfo hints, * result;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	const char* t_port = calloc(6, 1);
	if (!t_port)
	{
		printf("failed to allocate memory.");
		return;
	}
	sprintf(t_port, "%d", port);
	getaddrinfo(domain, t_port, &hints, &result);
	free(t_port);
	int sock = -1;
	struct addrinfo* p;

	for (p = result; p != NULL; p = p->ai_next)
	{
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

		if (sock < 0)
			continue;

		if (connect(sock, p->ai_addr, p->ai_addrlen) == 0)
			break;

		closesocket(sock);
		sock = -1;
	}

	if (sock < 0)
	{
		printf("Could not connect.\n");
		return 1;
	}

	struct sockaddr* addr = p->ai_addr;
	client->ip_address = *(uint32_t*)&((struct sockaddr_in*)addr)->sin_addr;
	client->domain = domain;
	client->domain_len = strlen(domain);
	client->port_num = port;
	client->sock = sock;

	memset(&client->response, 0, sizeof(client->response));
	freeaddrinfo(result);
}

void http_connect_by_ip(http_client_t* client, const char* ip, unsigned short port)
{
#ifdef _CHAT_WINDOWS_
	int wsas = WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
	
	int sock = -1;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	inet_pton(AF_INET, ip, &addr.sin_addr);
	connect(sock, &addr, sizeof(addr));

	if (sock < 0)
	{
		printf("Could not connect.\n");
		return 1;
	}

	client->ip_address = *(uint32_t*)&addr.sin_addr;
	client->port_num = port;
	client->sock = sock;

	memset(&client->response, 0, sizeof(client->response));
}

const static char get_request_format[] =
"GET %s HTTP/1.1\r\n"
"Host: %s\r\n"
"Connection: close\r\n"
"\r\n";

void http_get(http_client_t* client, const char* get)
{
	size_t request_len = sizeof(get_request_format) + strlen(get) + client->domain_len - 3;
	char* request = malloc(request_len);
	if (!request)
	{
		return;
	}
	request[request_len - 1] = 0;
	sprintf(request, get_request_format, get, client->domain);

	if (send(client->sock, request, request_len - 1, 0) < 0)
	{
		perror("send");
		closesocket(client->sock);
		return 1;
	}

	free(request);

	char buffer[1024];
	int bytes;

	if (client->response.body)
		{
			free(client->response.body);
		}

	while ((bytes = recv(client->sock, buffer, sizeof(buffer) - 1, 0)) > 0)
	{
		if (!client->response.body)
		{
			client->response.body = malloc(bytes + 1);
		}
		else
		{
			client->response.body = realloc(client->response.body, client->response.body_len + bytes);
		}
		if (!client->response.body)
		{
			return;
		}

		memcpy(client->response.body + client->response.body_len, buffer, bytes);
		client->response.body_len += bytes;
		client->response.body[bytes] = 0;
	}
}

void http_send(http_client_t* client, const char* message, size_t messsage_len)
{
	if (send(client->sock, message, messsage_len, 0) < 0)
	{
		perror("send");
		closesocket(client->sock);
		return 1;
	}
}

void http_recv(http_client_t* client)
{
	char buffer[1024];
	int bytes;

	if (client->response.body)
	{
		free(client->response.body);
	}

	while ((bytes = recv(client->sock, buffer, sizeof(buffer) - 1, 0)) > 0)
	{
		if(!client->response.body)
		{
			client->response.body = malloc(bytes + 1);
		}
		else
		{
			client->response.body = realloc(client->response.body, client->response.body_len + bytes);
		}
		if (!client->response.body)
			{
				return;
			}
		
		memcpy(client->response.body + client->response.body_len, buffer, bytes);
		client->response.body_len += bytes;
		client->response.body[bytes] = 0;
	}
}

void http_close(http_client_t* client)
{
	closesocket(client->sock);
	free(client->response.body);
}