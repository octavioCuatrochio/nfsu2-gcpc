#include "FEHash.h"

uint32_t FEHashUpper(const char *str)
{
    uint32_t h = 0xFFFFFFFFu;
    if (!str) return h;
    for (; *str; ++str) {
        char c = *str;
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 0x20);
        h = h * 33 + static_cast<unsigned char>(c);
    }
    return h;
}
