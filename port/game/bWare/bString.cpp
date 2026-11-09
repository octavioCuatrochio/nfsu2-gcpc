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

namespace {
inline signed char Up(signed char c) { return (c >= 'a' && c <= 'z') ? static_cast<signed char>(c & 0x5F) : c; }
inline const signed char *S(const char *p) { return reinterpret_cast<const signed char *>(p); }
}

int bStrLen(const char *s)
{
    int n = 0;
    if (s)
        while (s[n]) ++n;
    return n;
}

char *bStrCpy(char *dst, const char *src)
{
    char *d = dst;
    while ((*d++ = *src++) != 0) {}
    return dst;
}

char *bStrNCpy(char *dst, const char *src, int n)
{
    for (char *d = dst; n != 0; ++d, ++src) {
        --n;
        if ((*d = *src) == 0) break;
    }
    return dst;
}

int bStrCmp(const char *a, const char *b)
{
    signed char x, y;
    do {
        x = *S(a++);
        y = *S(b++);
    } while (x && y && x == y);
    return x - y;
}

int bStrICmp(const char *a, const char *b)
{
    signed char x, y;
    do {
        x = Up(*S(a++));
        y = Up(*S(b++));
    } while (x && y && x == y);
    return x - y;
}

// Shared tail of bStrNCmp / bStrNICmp (see the header). `fold` = case-insensitive.
static int NCmp(const signed char *a, const signed char *b, int n, bool fold)
{
    for (;;) {
        if (n-- == 0) break;
        signed char x = *a, y;
        if (!x || !(y = *b)) break;
        if (fold) { x = Up(x); y = Up(y); }
        ++a; ++b;
        if (x != y) break;
    }
    if (n < 0) return 0;
    if (*a) return 1;
    if (*b) return -1;
    return fold ? Up(a[-1]) - Up(b[-1]) : a[-1] - b[-1];
}

int bStrNCmp(const char *a, const char *b, int n) { return NCmp(S(a), S(b), n, false); }
int bStrNICmp(const char *a, const char *b, int n) { return NCmp(S(a), S(b), n, true); }

char *bStrCat(char *dst, const char *a, const char *b)
{
    int i = 0;
    for (; *a; ++a) dst[i++] = *a;
    for (; *b; ++b) dst[i++] = *b;
    dst[i] = 0;
    return dst;
}

char *bStrChr(const char *s, int c)
{
    if (!s) return nullptr;
    for (;; ++s) {
        if (*S(s) == c) return const_cast<char *>(s);
        if (!*s) return nullptr;
    }
}

char *bToUpper(char *s)
{
    for (; *s; ++s) *s = static_cast<char>(Up(static_cast<signed char>(*s)));
    return s;
}
