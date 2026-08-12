#ifndef _WIN32

#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include "console.h"

static struct termios old_termios;

int console_init(void)
{
    struct termios t;

    if (tcgetattr(STDIN_FILENO, &old_termios) != 0)
        return -1;

    t = old_termios;

    t.c_lflag &= ~(ICANON | ECHO);

    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &t) != 0)
        return -1;

    return 0;
}

void console_shutdown(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

int console_key_available(void)
{
    fd_set fds;
    struct timeval tv = {0, 0};

    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    return select(STDIN_FILENO + 1,
                  &fds,
                  NULL,
                  NULL,
                  &tv) > 0;
}

int console_read_key(void)
{
    unsigned char ch;

    if (read(STDIN_FILENO, &ch, 1) != 1)
        return -1;

    return ch;
}

#endif