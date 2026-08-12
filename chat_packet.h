#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "chat_core.h"
#include "mutex.h"
#include "image.h"

#ifndef _CHAT_PACKET_H_
#define _CHAT_PACKET_H_

#define PACKET_VERSION 1

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        PKT_LOGIN,
        PKT_LOGIN_OK,
        PKT_CHAT,
        PKT_FILE,
        PKT_IMAGE,
        PKT_QUIT,
        PKT_CUSTOM_INFO,
        PKT_NULL
    } packet_type_t;

    typedef struct
    {
        uint16_t type;
        uint16_t version;
        uint32_t length;
        uint16_t packet_id;
    } packet_header_t;

    typedef struct
    {
        uint32_t room_pin;
        uint32_t username_length;
    } login_packet_t;

    typedef struct
    {
        uint64_t user_id;
        uint32_t username_length;
    } login_ok_packet_t;

    typedef struct
    {
        uint64_t user_id;
        uint32_t message_length;
    } chat_packet_t;

    typedef struct
    {
        uint64_t user_id;
        uint32_t bool_file_stream;
        uint32_t stream_size;
        uint32_t filename_length;
        uint32_t file_size;
    } file_packet_t;

    typedef struct
    {
        uint64_t user_id;
        uint32_t bool_file_stream;
        uint32_t stream_size;
        uint32_t image_size;
        enum image_type image_type;
    } image_packet_t;

    typedef struct
    {
        uint64_t user_id;
        uint32_t username_length;
    } quit_packet_t;

    typedef union
    {
        login_packet_t login;
        login_ok_packet_t login_ok;
        chat_packet_t chat;
        file_packet_t file;
        image_packet_t image;
        quit_packet_t quit;
    } union_payload_variables;

    typedef struct
    {
        packet_header_t header;
        struct
        {
            union_payload_variables* buffer;
            size_t packet_size;
            size_t index;
            struct
            {
                const void* ptr;
                size_t size;
                uint8_t own_memory;
            } *chunks;
            size_t chunk_length;
        } payload;
    } packet_t;

    uint16_t get_new_packet_id(packet_type_t type);
    void packet_begin(packet_t* packet, packet_type_t type, uint16_t packet_id);
    void packet_write_u32(packet_t* packet, uint32_t u32);
    void packet_write_u64(packet_t* packet, uint64_t u64);
    void packet_append(packet_t* packet, const void* ptr, size_t size);
    void packet_send(packet_t* packet, int sock);
    void packet_destroy(packet_t* packet);
    void packet_recv(int sock, packet_t* packet);
    void packet_read_u32(packet_t* packet, uint32_t* u32);
    void packet_read_u64(packet_t* packet, uint64_t* u64);
    void packet_send_chat(int* sock, size_t sock_len, uint32_t uid, const char* message, size_t message_len);
    void packet_send_file(int* sock, size_t sock_len, uint32_t uid, FILE* file, const char* file_name, size_t file_name_len, uint8_t bool_stream_file, size_t stream_buffer_size);
    void packet_send_image(int* sock, size_t sock_len, uint32_t uid, FILE* file, uint8_t bool_stream_file, size_t stream_buffer_size);
    void packet_send_quit(int* sock, size_t sock_len, uint32_t uid, const char* username, size_t username_len);
#ifdef __cplusplus
}
#endif

#endif