#include "bMath.h"
#include <cstring>
#include "bridge/Bridge.h"
#include "frontend/FEMath.h"   // FEFtol (MSVC __ftol2)

namespace {
using L = long double;
// x87 fchs: flip the sign bit after the operation (compilers may otherwise fold -(a*b) into a*(-b), which differs
// for NaN results).
inline L Fchs(L x)
{
    volatile L v = x;   // opaque (exact for long double): the negation must follow the operation
    return -v;
}
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

namespace {
uint32_t NextSeed(uint32_t s)
{
    const uint32_t t = s ^ 0x1D872B41u, u = (t >> 5) ^ t;
    return (u << 27) ^ u ^ t;
}
}

uint32_t bRandom(int32_t range, uint32_t *seed)
{
    const uint32_t r = *seed % static_cast<uint32_t>(range);
    *seed = NextSeed(*seed);
    return r;
}

long double bRandom(float range, uint32_t *seed)
{
    const uint32_t r = *seed % 0x7FFFFFFFu;   // < 2^31: the original's "+ 2^32 if negative" never applies
    *seed = NextSeed(*seed);
    return L(int32_t(r)) * 4.656612873077393e-10f * range;
}

uint32_t bRandom(int32_t range) { return bRandom(range, bridge::BRandomSeed()); }
long double bRandom(float range) { return bRandom(range, bridge::BRandomSeed()); }

long double bFMod(float a, float b)
{
    const L m = __builtin_fabsl(L(b));
    const L q = L(a) / m;
    L f = L(FEFtol(q));
    if (f > q) f = f - 1.0f;
    return m * (q - f);
}

int32_t bSignedMod(int32_t a, int32_t b)
{
    if (b < 0) b = static_cast<int32_t>(0u - static_cast<uint32_t>(b));
    uint32_t c = static_cast<uint32_t>(a / b) * static_cast<uint32_t>(b);
    if (a < 0) c -= static_cast<uint32_t>(b);
    return static_cast<int32_t>(static_cast<uint32_t>(a) - c);
}

long double bDistBetween(const bVector3 *a, const bVector3 *b)
{
    const L dx = L(a->x) - b->x, dy = L(a->y) - b->y, dz = L(a->z) - b->z;
    return __builtin_sqrtl(dz * dz + dy * dy + dx * dx);
}

namespace {
// 2D: the length is stored to a float stack slot, compared unrounded, divided as float.
void Normalize2(bVector2 *out, const bVector2 *in, L len)
{
    const float lenF = static_cast<float>(len);
    if (len == 0.0f) { out->x = 1.0f; out->y = 0.0f; return; }
    const L y = in->y;
    const L inv = L(1.0f) / lenF;
    out->x = static_cast<float>(inv * in->x);
    out->y = static_cast<float>(y * inv);
}
// 3D / 4D: x, y, z scaled by the unrounded 1/len; z read before any write.
void Normalize3(float *out, const float *in, L len)
{
    const L inv = L(1.0f) / len;
    const float z = in[2];
    const L y = in[1];
    out[0] = static_cast<float>(inv * in[0]);
    out[1] = static_cast<float>(y * inv);
    out[2] = static_cast<float>(L(z) * inv);
}
}

void bNormalize(bVector2 *out, const bVector2 *in)
{
    const L x = in->x, y = in->y;
    Normalize2(out, in, __builtin_sqrtl(y * y + x * x));
}

void bNormalize(bVector2 *out, const bVector2 *in, float scale)
{
    const L x = in->x, y = in->y;
    Normalize2(out, in, __builtin_sqrtl(y * y + x * x) / scale);
}

void bNormalize(bVector3 *out, const bVector3 *in)
{
    const L x = in->x, y = in->y, z = in->z;
    const L len = __builtin_sqrtl(x * x + y * y + z * z);
    if (len == 0.0f) { out->x = 1.0f; out->y = 0.0f; out->z = 0.0f; return; }
    Normalize3(&out->x, &in->x, len);
}

void bNormalize(bVector3 *out, const bVector3 *in, float scale)
{
    const L x = in->x, y = in->y, z = in->z;
    const L len = __builtin_sqrtl(x * x + y * y + z * z) / scale;
    if (len == 0.0f) { out->x = scale; out->y = 0.0f; out->z = 0.0f; return; }
    Normalize3(&out->x, &in->x, len);
}

void bNormalize(bVector4 *out, const bVector4 *in)
{
    const L x = in->x, y = in->y, z = in->z, w = in->w;
    const L len = __builtin_sqrtl(x * x + y * y + z * z + w * w);
    if (len == 0.0f) { out->x = 1.0f; out->y = 0.0f; out->z = 0.0f; return; }
    Normalize3(&out->x, &in->x, len);
}

void bScaleAdd(bVector2 *out, const bVector2 *a, const bVector2 *b, float s)
{
    const L y = L(s) * b->y + a->y;
    const L x = L(s) * b->x + a->x;
    out->x = static_cast<float>(x);
    out->y = static_cast<float>(y);
}

void bIdentity(bMatrix4 *m)
{
    for (int i = 0; i < 16; ++i) m->m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
}

// Generated from the PC asm by tooling/x87sym.py (summation order per element is the original's).
void eMulMatrix(bMatrix4 *out, const bMatrix4 *pa, const bMatrix4 *pb)
{
    const bMatrix4 &a = *pa, &b = *pb;
    float r[16];
    r[0] = float(((((L(a.m[0]) * L(b.m[0])) + (L(a.m[3]) * L(b.m[12]))) + (L(a.m[2]) * L(b.m[8]))) + (L(b.m[4]) * L(a.m[1]))));
    r[1] = float(((((L(b.m[13]) * L(a.m[3])) + (L(a.m[1]) * L(b.m[5]))) + (L(b.m[9]) * L(a.m[2]))) + (L(b.m[1]) * L(a.m[0]))));
    r[2] = float(((((L(b.m[10]) * L(a.m[2])) + (L(b.m[2]) * L(a.m[0]))) + (L(a.m[3]) * L(b.m[14]))) + (L(b.m[6]) * L(a.m[1]))));
    r[3] = float(((((L(b.m[15]) * L(a.m[3])) + (L(a.m[2]) * L(b.m[11]))) + (L(a.m[0]) * L(b.m[3]))) + (L(b.m[7]) * L(a.m[1]))));
    r[4] = float(((((L(a.m[6]) * L(b.m[8])) + (L(a.m[7]) * L(b.m[12]))) + (L(b.m[4]) * L(a.m[5]))) + (L(a.m[4]) * L(b.m[0]))));
    r[5] = float(((((L(a.m[6]) * L(b.m[9])) + (L(a.m[4]) * L(b.m[1]))) + (L(a.m[5]) * L(b.m[5]))) + (L(a.m[7]) * L(b.m[13]))));
    r[6] = float(((((L(a.m[7]) * L(b.m[14])) + (L(a.m[6]) * L(b.m[10]))) + (L(b.m[6]) * L(a.m[5]))) + (L(a.m[4]) * L(b.m[2]))));
    r[7] = float(((((L(b.m[15]) * L(a.m[7])) + (L(a.m[6]) * L(b.m[11]))) + (L(a.m[4]) * L(b.m[3]))) + (L(b.m[7]) * L(a.m[5]))));
    r[8] = float(((((L(a.m[10]) * L(b.m[8])) + (L(a.m[11]) * L(b.m[12]))) + (L(b.m[4]) * L(a.m[9]))) + (L(a.m[8]) * L(b.m[0]))));
    r[9] = float(((((L(a.m[10]) * L(b.m[9])) + (L(a.m[8]) * L(b.m[1]))) + (L(a.m[9]) * L(b.m[5]))) + (L(a.m[11]) * L(b.m[13]))));
    r[10] = float(((((L(a.m[11]) * L(b.m[14])) + (L(a.m[10]) * L(b.m[10]))) + (L(b.m[6]) * L(a.m[9]))) + (L(a.m[8]) * L(b.m[2]))));
    r[11] = float(((((L(b.m[15]) * L(a.m[11])) + (L(a.m[10]) * L(b.m[11]))) + (L(a.m[8]) * L(b.m[3]))) + (L(b.m[7]) * L(a.m[9]))));
    r[12] = float(((((L(a.m[14]) * L(b.m[8])) + (L(a.m[15]) * L(b.m[12]))) + (L(b.m[4]) * L(a.m[13]))) + (L(a.m[12]) * L(b.m[0]))));
    r[13] = float(((((L(a.m[14]) * L(b.m[9])) + (L(a.m[12]) * L(b.m[1]))) + (L(a.m[13]) * L(b.m[5]))) + (L(a.m[15]) * L(b.m[13]))));
    r[14] = float(((((L(a.m[15]) * L(b.m[14])) + (L(a.m[14]) * L(b.m[10]))) + (L(b.m[6]) * L(a.m[13]))) + (L(a.m[12]) * L(b.m[2]))));
    r[15] = float(((((L(b.m[15]) * L(a.m[15])) + (L(a.m[14]) * L(b.m[11]))) + (L(a.m[12]) * L(b.m[3]))) + (L(b.m[7]) * L(a.m[13]))));
    std::memcpy(out->m, r, sizeof r);   // PC 4012B0 copy
}

// Generated from the PC asm by tooling/x87sym.py.
void bInvertMatrix(bMatrix4 *out, const bMatrix4 *in)
{
    const bMatrix4 s = *in;
    out->m[3] = 0.0f;
    const float t1 = float(((L(s.m[10]) * L(s.m[4])) - (L(s.m[8]) * L(s.m[6]))));
    const float t2 = float(((L(s.m[9]) * L(s.m[4])) - (L(s.m[8]) * L(s.m[5]))));
    const float t3 = float((L(0x1.0000000000000p+0f) / (((((L(s.m[5]) * L(s.m[10])) - (L(s.m[9]) * L(s.m[6]))) * L(s.m[0])) - (L(t1) * L(s.m[1]))) + (L(t2) * L(s.m[2])))));
    const float t4 = float((((L(s.m[5]) * L(s.m[10])) - (L(s.m[9]) * L(s.m[6]))) * L(t3)));
    out->m[0] = float((((L(s.m[5]) * L(s.m[10])) - (L(s.m[9]) * L(s.m[6]))) * L(t3)));
    const float t5 = float(Fchs((((L(s.m[10]) * L(s.m[1])) - (L(s.m[9]) * L(s.m[2]))) * L(t3))));
    out->m[1] = float(Fchs((((L(s.m[10]) * L(s.m[1])) - (L(s.m[9]) * L(s.m[2]))) * L(t3))));
    const float t6 = float((((L(s.m[6]) * L(s.m[1])) - (L(s.m[5]) * L(s.m[2]))) * L(t3)));
    out->m[2] = float((((L(s.m[6]) * L(s.m[1])) - (L(s.m[5]) * L(s.m[2]))) * L(t3)));
    const float t7 = float(Fchs((L(t1) * L(t3))));
    out->m[4] = float(Fchs((L(t1) * L(t3))));
    out->m[7] = 0.0f;
    out->m[11] = 0.0f;
    out->m[15] = 1.0f;
    const float t8 = float((((L(s.m[10]) * L(s.m[0])) - (L(s.m[8]) * L(s.m[2]))) * L(t3)));
    out->m[5] = float((((L(s.m[10]) * L(s.m[0])) - (L(s.m[8]) * L(s.m[2]))) * L(t3)));
    const float t9 = float(Fchs((((L(s.m[6]) * L(s.m[0])) - (L(s.m[4]) * L(s.m[2]))) * L(t3))));
    out->m[6] = float(Fchs((((L(s.m[6]) * L(s.m[0])) - (L(s.m[4]) * L(s.m[2]))) * L(t3))));
    const float t10 = float((L(t2) * L(t3)));
    out->m[8] = float((L(t2) * L(t3)));
    const float t11 = float(Fchs((((L(s.m[9]) * L(s.m[0])) - (L(s.m[8]) * L(s.m[1]))) * L(t3))));
    out->m[9] = float(Fchs((((L(s.m[9]) * L(s.m[0])) - (L(s.m[8]) * L(s.m[1]))) * L(t3))));
    out->m[10] = float((((L(s.m[0]) * L(s.m[5])) - (L(s.m[4]) * L(s.m[1]))) * L(t3)));
    out->m[12] = float(Fchs((((L(t10) * L(s.m[14])) + (L(t7) * L(s.m[13]))) + (L(t4) * L(s.m[12])))));
    out->m[13] = float(Fchs((((L(t11) * L(s.m[14])) + (L(t8) * L(s.m[13]))) + (L(t5) * L(s.m[12])))));
    out->m[14] = float(Fchs(((((((L(s.m[0]) * L(s.m[5])) - (L(s.m[4]) * L(s.m[1]))) * L(t3)) * L(s.m[14])) + (L(t9) * L(s.m[13]))) + (L(t6) * L(s.m[12])))));
}

void bMulMatrix(bMatrix4 *out, const bMatrix4 *a, const bMatrix4 *b) { eMulMatrix(out, b, a); }

void bTransposeMatrix(bMatrix4 *out, const bMatrix4 *in)
{
    bMatrix4 t = *in;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) out->m[c * 4 + r] = t.m[r * 4 + c];
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
