// FEButtonMap: a package's selectable objects and pad navigation between them. Original: zFe2.cpp.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMath.h"

struct FEObject;

// PC: embedded in FEPackage at +0x90 (GC +0x9C), 8 bytes.
struct FEButtonMap {
    FEObject **mppButtons;   // +0x0
    uint32_t mu32Count;      // +0x4

    // GC 8013E05C / PC 504BB0 (thiscall, ret 0xC; `this` unused): screen position of an object = its position track
    // (data +0x1C) through its render context's transform (game interface +0x2C, context id FEObject +0x20).
    void ComputeButtonLocation(FEObject *obj, void *gi, FEVector2 *out) const;
    // GC 8013DDE8 / PC 5189E0 (thiscall, ret 0x10): best button from `from` in direction `dir` (0..7, table PC
    // 0x7FA358). Pass 0 from the real position, passes 1..4 from positions shifted +-640 / +-480 when `wrap` has
    // the pass bit (PC 0x7F7CB0: 1, 1, 2, 2). Score: 1500 if the angle is off (dot^2 < 0.25), else
    // (1 - dot^2) * 200 + distance; nullptr if nothing beats 1500.
    FEObject *GetButtonFrom(FEObject *from, uint32_t dir, void *gi, uint32_t wrap) const;
};
#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEButtonMap) == 8);
#endif

// PC 504B50 (__cdecl; GC FEMultMatrix(FEVector3*, FEMatrix4 const*, FEVector3 const*) 8013DD08): out = m * (v, 1), FEng row-vector
// layout (out.x = m0 x + m4 y + m8 z + m12), x87 order of the original.
void FEMultMatrix(FEVector3 *out, const float *m, const FEVector3 *v);
