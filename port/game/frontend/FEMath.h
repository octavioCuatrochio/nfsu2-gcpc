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
// PC 504500 / 5045A0 (two identical copies, eps 1e-6 at 0x7992EC / 0x7992F0; GC: Close() x3): every component
// of a within eps of b. NaN -> false.
bool FEVector3Close(const FEVector3 &a, const FEVector3 &b);
// PC 504640: every channel within +-1 (channels tested 2, 1, 0, 3; 32-bit wraparound like the original).
bool FEColorClose(const FEColor &a, const FEColor &b);
// PC 505580 / GC FEColor::operator-: per-channel subtraction (32-bit wraparound).
FEColor FEColorSub(const FEColor &a, const FEColor &b);
