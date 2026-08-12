#include "chat_packet.h"
#include <string.h>
#include <limits.h>

static const size_t packet_sizes[] =
{
	sizeof(login_packet_t),
	sizeof(login_ok_packet_t),
	sizeof(chat_packet_t),
	sizeof(file_packet_t),
	sizeof(image_packet_t),
	sizeof(quit_packet_t)
};

static uint16_t packet_id = 1;

uint16_t get_new_packet_id(packet_type_t type)
{
	/* keep packet_id in valid 16-bit range and mix in type */
	if (packet_id >= UINT16_MAX - 16) /* small guard, avoid overflow */
		packet_id = 1;
	uint16_t id = (uint16_t)(packet_id + (uint16_t)type);
	packet_id += 7;
	return id;
}

void packet_begin(packet_t* packet, packet_type_t type, uint16_t packet_id)
{
	memset(packet, 0, sizeof(packet_t));
	packet->header.type = type;
	packet->header.version = PACKET_VERSION;
	size_t packet_size = packet_sizes[type];
	packet->payload.buffer = malloc(packet_size);
	packet->payload.packet_size = packet_size;
	packet->header.packet_id = packet_id;
}

/* write helpers already used byte-wise, they cast to uint8_t* */
void packet_write_u32(packet_t* packet, uint32_t value)
{
	memcpy((uint8_t*)packet->payload.buffer + packet->payload.index,
		&value,
		sizeof(value));

	packet->payload.index += sizeof(value);
	packet->header.length += sizeof(value);
}

void packet_write_u64(packet_t* packet, uint64_t value)
{
	memcpy((uint8_t*)packet->payload.buffer + packet->payload.index,
		&value,
		sizeof(value));

	packet->payload.index += sizeof(value);
	packet->header.length += sizeof(value);
}

static void data_append(packet_t* packet, const void* ptr, size_t size, uint8_t own_memory)
{
	if (packet->payload.chunk_length == 0)
	{
		++packet->payload.chunk_length;
		packet->payload.chunks = malloc(sizeof(*packet->payload.chunks));
	}
	else
	{
		++packet->payload.chunk_length;
		packet->payload.chunks = realloc(packet->payload.chunks, packet->payload.chunk_length * sizeof(*packet->payload.chunks));
	}
	packet->payload.chunks[packet->payload.chunk_length - 1].ptr = ptr;
	packet->payload.chunks[packet->payload.chunk_length - 1].size = size;
	packet->payload.chunks[packet->payload.chunk_length - 1].own_memory = own_memory;
}

void packet_append(packet_t* packet, const void* ptr, size_t size)
{
	data_append(packet, ptr, size, 0);
	packet->header.length += (uint32_t)size;
}

/* Return 0 on success, -1 on error. Ensure pointer and remaining size are advanced. */
static int send_all(int sock, const void* ptr, size_t size, int flags)
{
	size_t sent = 0;
	while (sent < size)
	{
		size_t n = send(sock, (const uint8_t*)ptr + sent, (int)(size - sent), flags);
		if (n <= 0)
		{
			return -1;
		}
		sent += (size_t)n;
	}
	return 0;
}

/* Return 0 on success, -1 on error. Ensure pointer and remaining size are advanced. */
static int recv_all(int sock, void* ptr, size_t size, int flags)
{
	size_t recvd = 0;
	while (recvd < size)
	{
		size_t n = recv(sock, (uint8_t*)ptr + recvd, (int)(size - recvd), flags);
		if (n <= 0)
		{
			return -1;
		}
		recvd += (size_t)n;
	}
	return 0;
}

void packet_send(packet_t* packet, int sock)
{
	if (send_all(sock, &packet->header, sizeof(packet_header_t), 0) < 0)
	{
		perror("send header packet");
		return;
	}
	if (send_all(sock, packet->payload.buffer, packet->payload.packet_size, 0) < 0)
	{
		perror("send payload packet");
		return;
	}
	for (int i = 0; i < packet->payload.chunk_length; i++)
	{
		if (send_all(sock, packet->payload.chunks[i].ptr, packet->payload.chunks[i].size, 0) < 0)
		{
			perror("send payload data");
			return;
		}
	}
}

void packet_destroy(packet_t* packet)
{
	if (packet->payload.chunks)
	{
		for (int i = 0; i < packet->payload.chunk_length; i++)
		{
			if (packet->payload.chunks[i].own_memory)
			{
				if (packet->payload.chunks[i].ptr)
				{
					free((void*)packet->payload.chunks[i].ptr);
				}
			}
		}
		free(packet->payload.chunks);
		packet->payload.chunks = NULL;
		packet->payload.chunk_length = 0;
	}
	if (packet->payload.buffer)
	{
		free(packet->payload.buffer);
		packet->payload.buffer = NULL;
		packet->payload.packet_size = 0;
	}
}

typedef void (*packet_recv_handeler)(int sock, packet_t* packet);

static void chat_recv_handel(int sock, packet_t* packet)
{
	char* message = malloc(packet->payload.buffer->chat.message_length + 1);
	if (recv_all(sock, message, packet->payload.buffer->chat.message_length, 0) < 0)
	{
		free(message);
		return;
	}
	message[packet->payload.buffer->chat.message_length] = 0;
	data_append(packet, message, packet->payload.buffer->chat.message_length, 1);
}

static void login_recv_handel(int sock, packet_t* packet)
{
	char* username = malloc(packet->payload.buffer->login.username_length + 1);
	if (recv_all(sock, username, packet->payload.buffer->login.username_length, 0) < 0)
	{
		free(username);
		return;
	}
	username[packet->payload.buffer->login.username_length] = 0;
	data_append(packet, username, packet->payload.buffer->login.username_length, 1);
}

static void login_ok_recv_handel(int sock, packet_t* packet)
{
	char* username = malloc(packet->payload.buffer->login_ok.username_length + 1);
	if (recv_all(sock, username, packet->payload.buffer->login_ok.username_length, 0) < 0)
	{
		free(username);
		return;
	}
	username[packet->payload.buffer->login_ok.username_length] = 0;
	data_append(packet, username, packet->payload.buffer->login_ok.username_length, 1);
}

static void file_recv_handel(int sock, packet_t* packet)
{
	size_t fname_len = packet->payload.buffer->file.filename_length;
	size_t fsize = packet->payload.buffer->file.file_size;

	char* filename = malloc(fname_len + 1);
	if (!filename) return;
	if (recv_all(sock, filename, fname_len, 0) < 0)
	{
		free(filename);
		return;
	}
	filename[fname_len] = 0;

	char* file = malloc(fsize);
	if (!file) { free(filename); return; }
	if (recv_all(sock, file, fsize, 0) < 0)
	{
		free(filename);
		free(file);
		return;
	}

	data_append(packet, filename, fname_len, 1);
	data_append(packet, file, fsize, 1);
}

static void image_recv_handel(int sock, packet_t* packet)
{
	void* image = malloc(packet->payload.buffer->image.image_size);
	if (!image) return;
	if (recv_all(sock, image, packet->payload.buffer->image.image_size, 0) < 0)
	{
		free(image);
		return;
	}
	data_append(packet, image, packet->payload.buffer->image.image_size, 1);
}

static void quit_recv_handel(int sock, packet_t* packet)
{
	char* username = malloc(packet->payload.buffer->quit.username_length + 1);
	if (!username) return;
	if (recv_all(sock, username, packet->payload.buffer->quit.username_length, 0) < 0)
	{
		free(username);
		return;
	}
	username[packet->payload.buffer->quit.username_length] = '\0';
	data_append(packet, username, packet->payload.buffer->quit.username_length, 1);
}

packet_recv_handeler recv_handel[] =
{
	login_recv_handel,
	login_ok_recv_handel,
	chat_recv_handel,
	file_recv_handel,
	image_recv_handel,
	quit_recv_handel
};

void packet_recv(int sock, packet_t* packet)
{
	packet_destroy(packet);
	memset(packet, 0, sizeof(packet_t));
	if (recv_all(sock, &packet->header, sizeof(packet_header_t), 0) < 0)
	{
		return;
	}

	/* validate type index before using it */
	if ((size_t)packet->header.type >= (sizeof(packet_sizes) / sizeof(packet_sizes[0])))
	{
		/* unknown packet type -> drop or handle as desired */
		return;
	}

	size_t packet_size = packet_sizes[packet->header.type];

	packet->payload.buffer = malloc(packet_size);
	if (!packet->payload.buffer) return;

	if (recv_all(sock, packet->payload.buffer, packet_size, 0) < 0)
	{
		packet_destroy(packet);
		return;
	}

	recv_handel[packet->header.type](sock, packet);
}

void packet_read_u32(packet_t* packet, uint32_t* u32)
{
	memcpy(u32, (uint8_t*)packet->payload.buffer + packet->payload.index, sizeof(uint32_t));
	packet->payload.index += sizeof(uint32_t);
}

void packet_read_u64(packet_t* packet, uint64_t* u64)
{
	memcpy(u64, (uint8_t*)packet->payload.buffer + packet->payload.index, sizeof(uint64_t));
	packet->payload.index += sizeof(uint64_t);
}

void packet_send_chat(int* sock, size_t sock_len, uint32_t uid, const char* message, size_t message_len)
{
	if (sock == 0) return;
	if (sock_len == 0) return;
	if (message == 0) return;
	packet_t packet;
	packet_begin(&packet, PKT_CHAT, get_new_packet_id(PKT_CHAT));
	packet_write_u64(&packet, uid);
	packet_write_u32(&packet, (uint32_t)message_len);
	packet_append(&packet, message, message_len);
	if (sock_len == 1)
		packet_send(&packet, *sock);
	else
	{
		for (size_t i = 0; i < sock_len; i++)
		{
			packet_send(&packet, sock[i]);
		}
	}
	packet_destroy(&packet);
}
void packet_send_file(int* sock, size_t sock_len, uint32_t uid, FILE* file, const char* file_name, size_t file_name_len, uint8_t bool_stream_file, size_t stream_buffer_size)
{
	if (sock == 0) return;
	if (sock_len == 0) return;
	if (file == 0) return;
	if (bool_stream_file && stream_buffer_size == 0) return;
	char stack_buffer[124];
	if (file_name == 0)
	{
		file_name = stack_buffer;
		file_name_len = (size_t)snprintf(file_name, sizeof(stack_buffer), "%lu", (unsigned long)time(0));
	}

	fseek(file, 0, SEEK_END);
	size_t file_size = (size_t)ftell(file);
	rewind(file);

	packet_t packet;
	packet_begin(&packet, PKT_FILE, get_new_packet_id(PKT_FILE));
	packet_write_u64(&packet, uid);
	packet_write_u32(&packet, (uint32_t)bool_stream_file);
	packet_write_u32(&packet, (uint32_t)stream_buffer_size);
	packet_write_u32(&packet, (uint32_t)file_name_len);
	packet_write_u32(&packet, (uint32_t)file_size);
	void* send_buffer = malloc(bool_stream_file ? stream_buffer_size : file_size);

	for (size_t i = 0; i < sock_len; i++)
	{
		if (send_all(sock[i], &packet.header, sizeof(packet_header_t), 0) < 0)
		{
			perror("send header packet");
			free(send_buffer);
			packet_destroy(&packet);
			return;
		}

		if (send_all(sock[i], packet.payload.buffer, packet.payload.packet_size, 0) < 0)
		{
			perror("send payload packet");
			free(send_buffer);
			packet_destroy(&packet);
			return;
		}

		if (send_all(sock[i], file_name, file_name_len, 0) < 0)
		{
			perror("send payload packet");
			free(send_buffer);
			packet_destroy(&packet);
			return;
		}
	}

	if (bool_stream_file)
	{
		while (1)
		{
			int bytes_read = (int)fread(send_buffer, 1, stream_buffer_size, file);

			if (bytes_read > 0)
			{
				for (size_t i = 0; i < sock_len; i++)
				{
					if (send_all(sock[i], send_buffer, (size_t)bytes_read, 0) < 0)
					{
						perror("send streaming data");
						free(send_buffer);
						packet_destroy(&packet);
						return;
					}
				}
			}

			if ((size_t)bytes_read < stream_buffer_size)
			{
				if (feof(file))
					break;

				if (ferror(file))
				{
					free(send_buffer);
					packet_destroy(&packet);
					return;
				}
			}
		}
	}
	else
	{
		size_t bytes_read = fread(send_buffer, 1, file_size, file);
		if (bytes_read == 0 && ferror(file))
		{
			free(send_buffer);
			packet_destroy(&packet);
			return;
		}

		for (size_t i = 0; i < sock_len; i++)
		{
			if (send_all(sock[i], send_buffer, bytes_read, 0) < 0)
			{
				perror("send streaming data");
				free(send_buffer);
				packet_destroy(&packet);
				return;
			}
		}
	}

	free(send_buffer);
	packet_destroy(&packet);
}
void packet_send_image(int* sock, size_t sock_len, uint32_t uid, FILE* file, uint8_t bool_stream_file, size_t stream_buffer_size)
{
	if (sock == 0) return;
	if (sock_len == 0) return;
	if (file == 0) return;
	if (bool_stream_file && stream_buffer_size == 0) return;

	fseek(file, 0, SEEK_END);
	size_t file_size = (size_t)ftell(file);
	rewind(file);

	packet_t packet;
	packet_begin(&packet, PKT_IMAGE, get_new_packet_id(PKT_IMAGE));
	packet_write_u64(&packet, uid);
	packet_write_u32(&packet, (uint32_t)bool_stream_file);
	packet_write_u32(&packet, (uint32_t)stream_buffer_size);
	packet_write_u32(&packet, (uint32_t)file_size);
	void* send_buffer = malloc(bool_stream_file ? stream_buffer_size : file_size);

	uint8_t stack_buffer[32];
	fread(stack_buffer, 1, sizeof(stack_buffer), file);
	enum image_type type = detect_image_type(stack_buffer, sizeof(stack_buffer));
	packet_write_u32(&packet, (uint32_t)type);
	fseek(file, 0, SEEK_SET);

	for (size_t i = 0; i < sock_len; i++)
	{
		if (send_all(sock[i], &packet.header, sizeof(packet_header_t), 0) < 0)
		{
			perror("send header packet");
			free(send_buffer);
			packet_destroy(&packet);
			return;
		}

		if (send_all(sock[i], packet.payload.buffer, packet.payload.packet_size, 0) < 0)
		{
			perror("send payload packet");
			free(send_buffer);
			packet_destroy(&packet);
			return;
		}
	}

	if (bool_stream_file)
	{
		while (1)
		{
			int bytes_read = (int)fread(send_buffer, 1, stream_buffer_size, file);

			if (bytes_read > 0)
			{
				for (size_t i = 0; i < sock_len; i++)
				{
					if (send_all(sock[i], send_buffer, (size_t)bytes_read, 0) < 0)
					{
						perror("send streaming data");
						free(send_buffer);
						packet_destroy(&packet);
						return;
					}
				}
			}

			if ((size_t)bytes_read < stream_buffer_size)
			{
				if (feof(file))
					break;

				if (ferror(file))
				{
					free(send_buffer);
					packet_destroy(&packet);
					return;
				}
			}
		}
	}
	else
	{
		size_t bytes_read = fread(send_buffer, 1, file_size, file);
		if (bytes_read == 0 && ferror(file))
		{
			free(send_buffer);
			packet_destroy(&packet);
			return;
		}

		for (size_t i = 0; i < sock_len; i++)
		{
			if (send_all(sock[i], send_buffer, bytes_read, 0) < 0)
			{
				perror("send streaming data");
				free(send_buffer);
				packet_destroy(&packet);
				return;
			}
		}
	}

	free(send_buffer);
	packet_destroy(&packet);
}
void packet_send_quit(int* sock, size_t sock_len, uint32_t uid, const char* username, size_t username_len)
{
	packet_t packet;
	packet_begin(&packet, PKT_QUIT, get_new_packet_id(PKT_QUIT));
	packet_write_u64(&packet, uid);
	packet_write_u32(&packet, (uint32_t)username_len);
	packet_append(&packet, username, username_len);
	for (size_t i = 0; i < sock_len; i++)
	{
		packet_send(&packet, sock[i]);
	}
	packet_destroy(&packet);
}


