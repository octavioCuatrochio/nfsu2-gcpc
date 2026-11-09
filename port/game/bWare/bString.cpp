#include "bString.h"
#include <cstring>

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

int32_t bStrToLong(const char *s)
{
    uint32_t v = 0;
    if (s[0] == '0' && s[1] == 'x') {
        for (s += 2; *s; ++s) {
            signed char c = *S(s);
            uint32_t d;
            if (c >= '0' && c <= '9') d = static_cast<uint32_t>(c - '0');
            else {
                c = Up(c);
                if (c < 'A' || c > 'F') break;
                d = static_cast<uint32_t>(c - 0x37);
            }
            v = (v << 4) + d;
        }
        return static_cast<int32_t>(v);
    }
    bool neg = false;
    if (*s == '-') { neg = true; ++s; }
    else if (*s == '+') ++s;
    for (; *s >= '0' && *s <= '9'; ++s) v = v * 10 + static_cast<uint32_t>(*s - '0');
    return static_cast<int32_t>(neg ? 0u - v : v);
}

long double bStrToFloat(const char *s)
{
    const long double kTen = 10.0f, kTenth = 0.1f, kOne = 1.0f, kZero = 0.0f;
    bool neg = false;
    if (*s == '-') { neg = true; ++s; }
    else if (*s == '+') ++s;
    long double acc = kZero;
    for (; *s >= '0' && *s <= '9'; ++s) acc = acc * kTen + static_cast<long double>(*s - '0');
    if (*s == '.') {
        long double frac = kZero, scale = kOne;
        for (;;) {
            const char c = *++s;
            if (c < '0' || c > '9') break;
            scale = scale * kTenth;
            frac = frac + static_cast<long double>(c - '0') * scale;
        }
        long double add = frac;
        if (frac != kZero) {
            float f = static_cast<float>(frac);
            uint32_t bits;
            std::memcpy(&bits, &f, 4);
            ++bits;
            std::memcpy(&f, &bits, 4);
            add = f;
        }
        acc = acc + add;
    }
    if (*s == 'e' || *s == 'E') {
        const int32_t e = bStrToLong(s + 1);
        long double m = kOne;
        if (e > 0) {
            for (uint32_t n = static_cast<uint32_t>(e); n != 0; --n) m = m * kTen;
        } else if (e < 0) {
            for (uint32_t n = 0u - static_cast<uint32_t>(e); n != 0; --n) m = m * kTenth;
        }
        acc = acc * m;
    }
    return neg ? -acc : acc;
}
