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
