#include "bString.h"

uint32_t bStringHash(const char *str, uint32_t seed)
{
    uint32_t hash = seed;
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(str); *p; ++p)
        hash = hash * 33 + *p;
    return hash;
}

uint32_t bStringHash(const char *str)
{
    return bStringHash(str, 0xFFFFFFFFu);
}
