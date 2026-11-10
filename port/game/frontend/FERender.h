// FEng rendering helpers: matrices, quaternion rotation, per-package object sorting globals. Original: zFe2.cpp.
#pragma once
#include <cstdint>
#include "FEMath.h"

struct FEPackage;

struct FEMatrix4 {
    float m[16];   // row-major, translation in m[12..14]
};

// PC 5187A0 (__cdecl; GC FEMultMatrix(FEMatrix4*, FEMatrix4 const*, FEMatrix4 const*)): out[i*4+j] =
// sum_k b[i*4+k] * a[k*4+j] (= b x a); computed into a temporary, so out may alias.
void FEMultMatrix(FEMatrix4 *out, const FEMatrix4 *a, const FEMatrix4 *b);
// PC 504A50 (thiscall on the quaternion, ret 4; GC FEQuaternion::GetMatrix): rotation matrix, zero translation.
void FEQuatGetMatrix(const FEQuaternion *q, FEMatrix4 *out);

// Object sorting globals (PC 0x838488: num, max, most, buffer; GC NumSortedFEObjects...).
struct FESortGlobals {
    int32_t mi32Num;       // 0x838488
    uint32_t mu32Max;      // 0x83848C
    int32_t mi32Most;      // 0x838490 high-water mark
    void *mpBuffer;        // 0x838494
};
void InitFEngObjectSorting(FEPackage *pkg, void *buffer, int32_t size);   // GC 8014A0A4 / PC 50B410 (__cdecl)
void CloseFEngObjectSorting(FEPackage *pkg);                              // GC 8014A0B4 / PC 50B430 (__cdecl)
