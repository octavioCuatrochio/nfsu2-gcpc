#include "bWideString.h"

int bWideStrLen(const uint16_t *s)
{
    int n = 0;
    if (s)
        while (s[n]) ++n;
    return n;
}

void CopyString(uint16_t *dst, const uint16_t *src)
{
    if (!dst) return;
    if (src)
        while (*src) *dst++ = *src++;
    *dst = 0;
}

uint16_t *bStrCpy(uint16_t *dst, const char *src)
{
    uint16_t *start = dst;
    // movsx: signed char -> 16-bit; the terminator is copied too
    for (;; ++dst, ++src) {
        *dst = static_cast<uint16_t>(static_cast<int16_t>(static_cast<signed char>(*src)));
        if (*src == 0) break;
    }
    return start;
}
