
#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>
#include <stdint.h>

enum image_type {
    IMAGE_UNKNOWN,
    IMAGE_JPEG,
    IMAGE_PNG,
    IMAGE_GIF,
    IMAGE_BMP,
    IMAGE_WEBP,
    IMAGE_TIFF,
    IMAGE_ICO
};

typedef struct image_file
{
    FILE* file;
    unsigned int size;
    enum image_type type;
} image_file;

enum image_type detect_image_type(const uint8_t* data, size_t size);

#endif