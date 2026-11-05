#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "chat_core.h"
#include "chat_packet.h"
#include "base32_6.h"
#include "thread.h"
#include "color_printer.h"
#include "mutex.h"
#include "console.h"
#include "terminal.h"
#include "logger.h"
#include "net_interface.h"
#include "tui_selector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_STREAMING_BUFFER_SIZE 65536u

int create_server(void);
int get_str(char buffer[], int count, _key_t* escapers, int escapers_count);
void clear_line(); 

#ifdef __cplusplus
}
#endif

#endif SERVER_H
