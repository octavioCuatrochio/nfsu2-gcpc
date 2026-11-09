#include "FEMath.h"
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
