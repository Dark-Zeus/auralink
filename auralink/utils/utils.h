#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>
// Convert hexadecimal 16-bit color to 565 format
inline uint16_t hex565(uint32_t hexcolor) {
    uint8_t r = (hexcolor >> 16) & 0xFF;
    uint8_t g = (hexcolor >> 8) & 0xFF;
    uint8_t b = hexcolor & 0xFF;
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
#endif // UTILS_H