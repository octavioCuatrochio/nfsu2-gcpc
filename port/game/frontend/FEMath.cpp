#include "FEMath.h"

FEQuaternion FEQuatMultiply(const FEQuaternion &a, const FEQuaternion &b)
{
    float cx = a.y * b.z - b.y * a.z;
    float cy = b.x * a.z - a.x * b.z;
    float cz = b.y * a.x - a.y * b.x;
    FEQuaternion r;
    r.x = (a.w * b.x + b.w * a.x) + cx;
    r.y = (b.w * a.y + b.y * a.w) + cy;
    r.z = (a.w * b.z + b.w * a.z) + cz;
    r.w = b.w * a.w - ((b.x * a.x + b.y * a.y) + b.z * a.z);
    return r;
}

FEColor FEColorSub(const FEColor &a, const FEColor &b)
{
    FEColor r;
    for (int i = 0; i < 4; ++i)
        r.c[i] = static_cast<int32_t>(static_cast<uint32_t>(a.c[i]) - static_cast<uint32_t>(b.c[i]));
    return r;
}
