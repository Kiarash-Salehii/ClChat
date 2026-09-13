#ifdef _WIN32

#include <conio.h>
#include "console.h"

int console_init(void)
{
    return 0;
}

void console_shutdown(void)
{
}

int console_key_available(void)
{
    return _kbhit();
}

_key_t console_read_key(void)
{
    if (!_kbhit())
        return -1;

    return _getch();
}

_key_t console_poll_key(void)
{
    if(console_key_available())
    {
        return console_read_key();
    }
}

#endif