// Small FEng value types used by key tracks. Original: zFe2.cpp (GC: mostly inlined).
#pragma once
#include <cstdint>

struct FEVector2 {
    float x, y;
};

struct FEVector3 {
    float x, y, z;
};

struct FEQuaternion {
    float x, y, z, w;
};

struct FEColor {                     // 4 signed channels (track values, not packed)
    int32_t c[4];
};

// PC 4FE6B0 (__thiscall a, out, b; GC: inlined in SetupMoveToTracks). Hamilton product a*b, evaluated in the
// original's order.
FEQuaternion FEQuatMultiply(const FEQuaternion &a, const FEQuaternion &b);
// PC 505580 / GC FEColor::operator-: per-channel subtraction (32-bit wraparound).
FEColor FEColorSub(const FEColor &a, const FEColor &b);
