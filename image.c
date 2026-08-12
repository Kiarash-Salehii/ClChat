#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "image.h"

enum image_type detect_image_type(const uint8_t* data, size_t size)
{
    static const uint8_t png[] = {
        0x89, 0x50, 0x4E, 0x47,
        0x0D, 0x0A, 0x1A, 0x0A
    };

    static const uint8_t jpeg[] = {
        0xFF, 0xD8, 0xFF
    };

    static const uint8_t bmp[] = {
        0x42, 0x4D
    };

    static const uint8_t gif87[] = {
        'G', 'I', 'F', '8', '7', 'a'
    };

    static const uint8_t gif89[] = {
        'G', 'I', 'F', '8', '9', 'a'
    };

    static const uint8_t webp[] = {
        'W', 'E', 'B', 'P'
    };

    static const uint8_t tiff_le[] = {
        'I', 'I', 0x2A, 0x00
    };

    static const uint8_t tiff_be[] = {
        'M', 'M', 0x00, 0x2A
    };

    static const uint8_t ico[] = {
        0x00, 0x00, 0x01, 0x00
    };

    if (size >= sizeof(png) && memcmp(data, png, sizeof(png)) == 0)
        return IMAGE_PNG;

    if (size >= sizeof(jpeg) && memcmp(data, jpeg, sizeof(jpeg)) == 0)
        return IMAGE_JPEG;

    if (size >= sizeof(bmp) && memcmp(data, bmp, sizeof(bmp)) == 0)
        return IMAGE_BMP;

    if (size >= sizeof(gif87) && memcmp(data, gif87, sizeof(gif87)) == 0)
        return IMAGE_GIF;

    if (size >= sizeof(gif89) && memcmp(data, gif89, sizeof(gif89)) == 0)
        return IMAGE_GIF;

    if (size >= 12 &&
        memcmp(data, "RIFF", 4) == 0 &&
        memcmp(data + 8, webp, 4) == 0)
        return IMAGE_WEBP;

    if (size >= sizeof(tiff_le) && memcmp(data, tiff_le, sizeof(tiff_le)) == 0)
        return IMAGE_TIFF;

    if (size >= sizeof(tiff_be) && memcmp(data, tiff_be, sizeof(tiff_be)) == 0)
        return IMAGE_TIFF;

    if (size >= sizeof(ico) && memcmp(data, ico, sizeof(ico)) == 0)
        return IMAGE_ICO;

    return IMAGE_UNKNOWN;
}