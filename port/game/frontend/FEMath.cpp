#include "FEMath.h"
#include "bWare/bMath.h"
#include <cmath>
#include <initializer_list>

// Mirrors PC 4FE6B0's x87 evaluation: products and sums stay in registers (long double here; on i686 this is the
// same x87 arithmetic under the same control word), except the z cross term, which the original spills to a float.
FEQuaternion FEQuatMultiply(const FEQuaternion &a, const FEQuaternion &b)
{
    using L = long double;
    L cx = L(a.y) * b.z - L(b.y) * a.z;
    L cy = L(b.x) * a.z - L(a.x) * b.z;
    float cz = static_cast<float>(L(b.y) * a.x - L(a.y) * b.x);
    FEQuaternion r;
    r.x = static_cast<float>((L(a.w) * b.x + L(b.w) * a.x) + cx);
    r.y = static_cast<float>((L(b.w) * a.y + L(b.y) * a.w) + cy);
    r.z = static_cast<float>((L(a.w) * b.z + L(b.w) * a.z) + cz);
    r.w = static_cast<float>(L(b.w) * a.w - ((L(b.x) * a.x + L(b.y) * a.y) + L(b.z) * a.z));
    return r;
}

namespace {
// The original adds/subtracts eps in an x87 register (precision = current FPU control word, not float).
// long double does the same on i686 (x87); on other targets it is at least as precise as the 53-bit default.
bool Close(float a, float b)
{
    const long double eps = 1e-6f;
    if (!(static_cast<long double>(a) + eps >= b)) return false;   // fails on "less" and on unordered
    if (!(static_cast<long double>(a) - eps <= b)) return false;
    return true;
}
}  // namespace

bool FEVector3Close(const FEVector3 &a, const FEVector3 &b)
{
    return Close(a.x, b.x) && Close(a.y, b.y) && Close(a.z, b.z);
}

bool FEColorClose(const FEColor &a, const FEColor &b)
{
    for (int i : {2, 1, 0, 3}) {
        auto ai = static_cast<uint32_t>(a.c[i]);
        if (static_cast<int32_t>(ai + 1) < b.c[i]) return false;
        if (static_cast<int32_t>(ai - 1) > b.c[i]) return false;
    }
    return true;
}

FEColor FEColorSub(const FEColor &a, const FEColor &b)
{
    FEColor r;
    for (int i = 0; i < 4; ++i)
        r.c[i] = static_cast<int32_t>(static_cast<uint32_t>(a.c[i]) - static_cast<uint32_t>(b.c[i]));
    return r;
}

int32_t FEFtol(long double x)
{
    if (!(x > -9.2e18L && x < 9.2e18L)) return 0;   // x87 "integer indefinite" (0x8000000000000000): low dword 0
    return static_cast<int32_t>(static_cast<uint32_t>(static_cast<uint64_t>(static_cast<int64_t>(x))));
}

namespace {
using L = long double;
int32_t LerpInt(int32_t a, int32_t b, float t, int32_t base)
{
    int32_t diff = static_cast<int32_t>(static_cast<uint32_t>(b) - static_cast<uint32_t>(a));
    uint32_t r = static_cast<uint32_t>(FEFtol(L(diff) * t + 0.5f));
    return static_cast<int32_t>(r + static_cast<uint32_t>(base) + static_cast<uint32_t>(a));
}
}  // namespace

void FELerpInteger(int32_t a, int32_t b, float t, const int32_t *base, int32_t *out) { *out = LerpInt(a, b, t, *base); }

void FELerpVector2(const FEVector2 *a, const FEVector2 *b, float t, const FEVector2 *base, FEVector2 *out)
{
    out->x = static_cast<float>(((L(b->x) - a->x) * t + a->x) + base->x);
    out->y = static_cast<float>(((L(b->y) - a->y) * t + base->y) + a->y);
}

void FELerpVector3(const FEVector3 *a, const FEVector3 *b, float t, const FEVector3 *base, FEVector3 *out)
{
    out->x = static_cast<float>(((L(b->x) - a->x) * t + a->x) + base->x);
    out->y = static_cast<float>(((L(b->y) - a->y) * t + base->y) + a->y);
    out->z = static_cast<float>(((L(b->z) - a->z) * t + base->z) + a->z);
}

void FELerpColor(const FEColor *a, const FEColor *b, float t, const FEColor *base, FEColor *out)
{
    FEColor r;
    for (int i = 0; i < 4; ++i) r.c[i] = LerpInt(a->c[i], b->c[i], t, base->c[i]);
    *out = r;
}

void FEQuatScale(FEQuaternion *out, const FEQuaternion *q, float s)
{
    FEQuaternion r{static_cast<float>(L(s) * q->x), static_cast<float>(L(s) * q->y), static_cast<float>(L(s) * q->z),
                   static_cast<float>(L(s) * q->w)};
    *out = r;
}

void FEQuatAdd(FEQuaternion *out, const FEQuaternion *a, const FEQuaternion *b)
{
    FEQuaternion r{static_cast<float>(L(a->x) + b->x), static_cast<float>(L(a->y) + b->y), static_cast<float>(L(a->z) + b->z),
                   static_cast<float>(L(a->w) + b->w)};
    *out = r;
}

void FEQuatSub(FEQuaternion *out, const FEQuaternion *a, const FEQuaternion *b)
{
    FEQuaternion r{static_cast<float>(L(a->x) - b->x), static_cast<float>(L(a->y) - b->y), static_cast<float>(L(a->z) - b->z),
                   static_cast<float>(L(a->w) - b->w)};
    *out = r;
}

void FEQuatNormalize(FEQuaternion *q)
{
    L x = q->x, y = q->y, z = q->z, w = q->w;
    L inv = 1.0f / std::sqrt(((x * x + y * y) + z * z) + w * w);
    if (inv <= 9.999999747378752e-06f) { *q = FEQuaternion{0.f, 0.f, 0.f, 1.f}; return; }
    q->x = static_cast<float>(inv * q->x);
    q->y = static_cast<float>(inv * q->y);
    q->z = static_cast<float>(inv * q->z);
    q->w = static_cast<float>(inv * q->w);
}

void FELerpQuaternion(const FEQuaternion *a, const FEQuaternion *b, float t, const FEQuaternion *base, FEQuaternion *out)
{
    FEQuaternion q = *b;
    L cosv = ((L(b->w) * a->w + L(b->z) * a->z) + L(b->y) * a->y) + L(b->x) * a->x;
    if (cosv < 0) {
        cosv = -cosv;
        q = FEQuaternion{-q.x, -q.y, -q.z, -q.w};
    }
    FEQuaternion res;
    if (cosv < 0.9990000128746033f) {
        uint16_t angle = static_cast<uint16_t>(0x4000 - bASin(static_cast<float>(cosv)));   // acos
        float theta = static_cast<float>(L(int32_t(angle)) * 9.58738019107841e-05f);
        float sinTheta = static_cast<float>(bSinF(theta));
        float sinT = static_cast<float>(bSinF(static_cast<float>(L(theta) * t)));
        float sin1T = static_cast<float>(bSinF(static_cast<float>((1.0f - L(t)) * theta)));
        FEQuaternion r{static_cast<float>(L(sinT) * q.x), static_cast<float>(L(sinT) * q.y),
                       static_cast<float>(L(sinT) * q.z), static_cast<float>(L(sinT) * q.w)};
        FEQuaternion s;
        FEQuatScale(&s, a, sin1T);
        L sumX = L(r.x) + s.x;                                   // stays in a register in the original
        float sumY = static_cast<float>(L(r.y) + s.y);
        float sumZ = static_cast<float>(L(r.z) + s.z);
        float sumW = static_cast<float>(L(r.w) + s.w);
        float inv = static_cast<float>(1.0f / L(sinTheta));
        res = FEQuaternion{static_cast<float>(sumX * inv), static_cast<float>(L(sumY) * inv),
                           static_cast<float>(L(sumZ) * inv), static_cast<float>(L(sumW) * inv)};
    } else {
        FEQuaternion d, sc;
        FEQuatSub(&d, b, a);
        FEQuatScale(&sc, &d, t);
        FEQuatAdd(&res, a, &sc);
        FEQuatNormalize(&res);
    }
    *out = FEQuatMultiply(res, *base);
}

uint32_t FEColorPack(const FEColor &c)
{
    auto ch = [](int32_t v) -> uint32_t { return v < 0 ? 0u : v > 0xFF ? 0xFFu : static_cast<uint32_t>(v); };
    return ch(c.c[3]) << 24 | ch(c.c[2]) << 16 | ch(c.c[1]) << 8 | ch(c.c[0]);
}
