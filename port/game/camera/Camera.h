// Camera. Original: Camera.cpp (GC zCamera). Only the fields the rewritten members use (PC offsets).
#pragma once
#include <cstddef>
#include <cstdint>

struct Camera {
    uint8_t _unknown000[0xC4];
    uint16_t mu16Fov;                // +0xC4 field of view (16-bit angle)

    // GC 8005FFA8 / PC 4412E0 (__thiscall, ret 4): angle scaled from this camera's FOV to the baseline FOV
    // (global aBaselineFovNoise, PC 0x7EC1F0): bASin(float(float(bSin(a) * float(bSin(fov/2))) / bSin(base/2))).
    uint16_t FovRelativeAngle(uint16_t angle) const;
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(Camera, mu16Fov) == 0xC4);
#endif
