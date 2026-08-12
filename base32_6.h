#ifndef BASE32_H
#define BASE32_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    void base32_encode6(const uint8_t input[6], char output[11]);
    int base32_decode6(const char input[10], uint8_t output[6]);
#ifdef __cplusplus
}
#endif

#endif