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
// Key lerps used by FEInterpLinear (GC FELerp*, PC __cdecl): out = lerp(a, b, t) + base, in the original's x87
// evaluation order. Integer/color: truncate(diff * t + 0.5) like MSVC _ftol, then + base + a (32-bit wrap).
void FELerpInteger(int32_t a, int32_t b, float t, const int32_t *base, int32_t *out);                  // PC 5050D0
void FELerpVector2(const FEVector2 *a, const FEVector2 *b, float t, const FEVector2 *base, FEVector2 *out);   // PC 505110
void FELerpVector3(const FEVector3 *a, const FEVector3 *b, float t, const FEVector3 *base, FEVector3 *out);   // PC 505150
void FELerpColor(const FEColor *a, const FEColor *b, float t, const FEColor *base, FEColor *out);       // PC 5051A0
// MSVC __ftol2 (PC 75BED4): truncate toward zero through a 64-bit integer, low 32 bits.
int32_t FEFtol(long double x);
// PC 505580 / GC FEColor::operator-: per-channel subtraction (32-bit wraparound).
FEColor FEColorSub(const FEColor &a, const FEColor &b);
