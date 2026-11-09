#include "bMath.h"
#include "bridge/Bridge.h"
#include "frontend/FEMath.h"   // FEFtol (MSVC __ftol2)

namespace {
using L = long double;
}

// PC 43C2B0
long double bSin(uint16_t angle)
{
    L x = L(int32_t(angle)) * 9.58738019107841e-05f;   // 2pi / 65536 (float constant 0x38C90FDB)
    L s = 1.0f;
    if (!(x < 4.71238899230957f)) x -= 6.2831854820251465f;
    else if (!(x < 1.5707963705062866f)) { x -= 3.1415927410125732f; s = -1.0f; }
    L x2 = x * x, x3 = x2 * x, x5 = x3 * x2, x7 = x5 * x2, x9 = x7 * x2;
    L r = x - x3 * 0.1666666716337204f;
    r = r + x5 * 0.008333333767950535f;
    r = r - x7 * 0.00019841270113829523f;
    r = r + x9 * 2.7557318844628753e-06f;
    return r * s;
}

// PC 43C350
long double bSinF(float radians)
{
    return bSin(static_cast<uint16_t>(FEFtol(L(radians) * 10430.3779296875f)));
}

// PC 43C370 (tail-jumps into bSin with the stack argument + 0x4000; bSin reads its low 16 bits)
long double bCos(uint16_t angle)
{
    return bSin(static_cast<uint16_t>(angle + 0x4000));
}

// PC 43C390. Same range reduction as bSin.
void bSinCos(float *sinOut, float *cosOut, uint16_t angle)
{
    L x = L(int32_t(angle)) * 9.58738019107841e-05f;
    L s = 1.0f;
    if (!(x < 4.71238899230957f)) x -= 6.2831854820251465f;
    else if (!(x < 1.5707963705062866f)) { x -= 3.1415927410125732f; s = -1.0f; }
    const L x2 = x * x, x3 = x2 * x;
    const float x4 = static_cast<float>(x2 * x2);
    const L x5 = x3 * x2;
    const float x6 = static_cast<float>(L(x4) * x2);
    const L x7 = x5 * x2;
    const float x8 = static_cast<float>(L(x6) * x2);
    L r = x - x3 * 0.1666666716337204f;
    r = r + x5 * 0.008333333767950535f;
    r = r - x7 * 0.00019841270113829523f;
    r = r + (x7 * x2) * 2.7557318844628753e-06f;
    *sinOut = static_cast<float>(r * s);
    L c = 1.0f - 0.5f * x2;
    c = c + L(x4) * 0.0416666679084301f;
    c = c - L(x6) * 0.0013888889225199819f;
    c = c + L(x8) * 2.4801587642286904e-05f;
    c = c - (L(x8) * x2) * 2.755731998149713e-07f;
    *cosOut = static_cast<float>(c * s);
}

uint16_t bFastATan(float x, float y)
{
    int q = 0;
    if (x < 0.0f) { x = -x; q = 1; }
    L yy = y;
    if (y < 0.0f) { q ^= 3; yy = -yy; }
    const uint8_t *t = bridge::BFastATanTable();
    uint32_t a;
    if (L(x) > yy) a = uint32_t(t[-FEFtol(yy / x * -256.0f)]) << 5;
    else if (yy > L(x)) a = 0x4000u - (uint32_t(t[-FEFtol(L(x) / yy * -256.0f)]) << 5);
    else a = (yy == 0.0f) ? 0u : 0x2000u;
    switch (q) {
    case 0: return static_cast<uint16_t>(a);
    case 3: return static_cast<uint16_t>(0u - a);
    case 1: return static_cast<uint16_t>(0x8000u - a);
    default: return static_cast<uint16_t>(a + 0x8000u);
    }
}

int32_t bFixATan(int32_t ratio)
{
    const bool neg = ratio < 0;
    int32_t x = neg ? static_cast<int32_t>(0u - static_cast<uint32_t>(ratio)) : ratio;
    uint32_t c, s, frac;
    if (x >= 0x200000) {
        if (x < 0x1000000) { frac = static_cast<uint32_t>(x >> 8); c = 0x3EBA; s = 0x3FD7; }
        else { c = s = 0x3FFF; frac = 0; }
    } else if (x >= 0x40000) {
        const uint16_t *t = bridge::BFixATanTable2() + (x >> 14);
        c = t[0]; s = t[1]; frac = static_cast<uint32_t>(x & 0x3FFF) << 2;
    } else {
        const uint16_t *t = bridge::BFixATanTable1() + (x >> 11);
        c = t[0]; s = t[1]; frac = static_cast<uint32_t>(x & 0x7FF) << 5;
    }
    const uint32_t r = static_cast<uint32_t>(static_cast<int32_t>((s - c) * frac) >> 16) + c;
    return static_cast<int32_t>(neg ? 0u - r : r);
}

int32_t bFixATan(int32_t x, int32_t y)
{
    int q = 0;
    if (x < 0) { q = 1; x = static_cast<int32_t>(0u - static_cast<uint32_t>(x)); }
    if (y < 0) { q ^= 3; y = static_cast<int32_t>(0u - static_cast<uint32_t>(y)); }
    uint32_t a;
    if (x <= (y >> 14)) a = 0x4000;
    else {
        int32_t r;
        if (x == 0) r = (y == 0) ? 0 : static_cast<int32_t>(uint32_t(y <= 0) + 0x7FFFFFFFu);   // only y == INT_MIN
        else {
            const int64_t p = int64_t(0x7FFFFFFF / x) * y;
            r = static_cast<int32_t>(static_cast<uint32_t>(p >> 16) << 1);
        }
        a = static_cast<uint16_t>(bFixATan(r));
    }
    switch (q) {
    case 0: return static_cast<int32_t>(a);
    case 1: return static_cast<int32_t>(0x8000u - a);
    case 2: return static_cast<int32_t>(a - 0x8000u);
    default: return static_cast<int32_t>(0u - a);
    }
}

// PC 43C490. Binary search over 12 segments (each half the previous width), 16 entries each, then a linear
// correction with the entry's slope.
uint16_t bASin(float x)
{
    bool neg = x < 0.f;
    if (neg) x = -x;
    if (!(x < 1.0f) && x == x) return neg ? 0xC000 : 0x4000;   // NaN falls through like the original
    int32_t i = FEFtol(L(x) * 65536.0f);
    int32_t step = 0x8000, acc = 0x8000, seg = 0;
    if (i >= 0x8000) {
        do {
            if (seg >= 11) break;
            step >>= 1;
            acc += step;
            ++seg;
        } while (i >= acc);
    }
    int32_t start = acc - step;
    int32_t idx = (i - start) >> (11 - seg);
    int32_t xk = (step >> 4) * idx + start;
    const bASinTableEntry &e = bridge::BASinTable()[seg * 16 + idx];
    int32_t corr = FEFtol(((L(x) - L(xk) * 1.52587890625e-05f) * e.slope) * -65536.0f);
    uint16_t a = static_cast<uint16_t>(e.angle - static_cast<uint16_t>(corr));
    return neg ? static_cast<uint16_t>(0x10000 - a) : a;
}
