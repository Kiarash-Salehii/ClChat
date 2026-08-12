#include "base32_6.h"

#include <ctype.h>


void base32_encode6(const uint8_t input[6], char output[11])
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

    uint32_t buffer = 0;
    int bits = 0;
    int out = 0;

    for (int i = 0; i < 6; i++)
    {
        buffer = (buffer << 8) | input[i];
        bits += 8;

        while (bits >= 5)
        {
            output[out++] =
                alphabet[(buffer >> (bits - 5)) & 31];
            bits -= 5;
        }
    }

    if (bits)
        output[out++] =
        alphabet[(buffer << (5 - bits)) & 31];

    output[out] = '\0';    // out will always be 10
}

int base32_decode6(const char input[10], uint8_t output[6])
{
    static int initialized = 0;
    static int table[256];

    if (!initialized)
    {
        for (int i = 0; i < 256; i++)
            table[i] = -1;

        for (int i = 0; i < 32; i++)
        {
            table["ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"[i]] = i;
            table["abcdefghijklmnopqrstuvwxyz234567"[i]] = i;
        }

        initialized = 1;
    }

    uint32_t buffer = 0;
    int bits = 0;
    int out = 0;

    for (int i = 0; i < 10; i++)
    {
        int value = table[(unsigned char)input[i]];

        if (value < 0)
            return 0;

        buffer = (buffer << 5) | value;
        bits += 5;

        if (bits >= 8)
        {
            output[out++] =
                (buffer >> (bits - 8)) & 0xFF;
            bits -= 8;
        }
    }

    return out == 6;
}