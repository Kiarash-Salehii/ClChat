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

    int key = _getch();
    if (key == 0 || key == 0xE0)
    {
        switch (_getch())
        {
        case 0x48: return KEY_UP;
        case 0x50: return KEY_DOWN;
        case 0x4B: return KEY_LEFT;
        case 0x4D: return KEY_RIGHT;
        default:   return KEY_NONE;
        }
    }

    switch (key)
    {
    case '\r': return KEY_ENTER;
    }

    return key;
}

_key_t console_poll_key(void)
{
    if(console_key_available())
    {
        return console_read_key();
    }
}

#endif