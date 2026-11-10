// Camera. Original: Camera.cpp (GC zCamera.cpp; PC SPEED2.EXE 441070..4412E0, 447280, 44F5A0). Exact PC layout.
// Field names come from what the rewritten code does with them (no symbol names for the members).
#pragma once
#include <cstddef>
#include <cstdint>
#include "bWare/bMath.h"

// One camera state (current / previous / per-second velocity), 0xD0 bytes; copied as a block (rep movs 0x34 dwords).
struct CameraParams {
    bMatrix4 mMatrix;                 // +0x00 view matrix
    bVector3 mPosition;               // +0x40 -(translation row x transpose(rotation)): eye position
    float mPositionW;                 // +0x4C (only the velocity key writes it: 0)
    bVector3 mDirection;              // +0x50 normalized column 2 of the matrix
    float mDirectionW;                // +0x5C
    bVector3 mTarget;                 // +0x60 position + direction * mTargetDistance
    float mTargetW;                   // +0x6C
    bVector4 mNoiseFrequency1;        // +0x70 ApplyNoise: Noise(freq * time) * amplitude
    bVector4 mNoiseAmplitude1;        // +0x80
    bVector4 mNoiseFrequency2;        // +0x90
    bVector4 mNoiseAmplitude2;        // +0xA0
    float mTargetDistance;            // +0xB0 (ctor 10.0)
    float _unknownB4;                 // +0xB4 (ctor 0)
    float _unknownB8;                 // +0xB8 (ctor 0)
    float _unknownBC;                 // +0xBC (ctor 0.5)
    float _unknownC0;                 // +0xC0 (ctor 10000.0)
    uint16_t mu16Fov;                 // +0xC4 field of view (16-bit angle; ctor 0x36FB)
    uint8_t _unknownC6[0xA];
};

// Camera::JollyRancherResponse (GC 803A1510 / PC 0x82A480, zero-initialized .bss): debug-tool response. On PC
// nothing writes it (bFunkCallASync is a bare `ret`), so SetCameraMatrix's override branch never runs.
struct JollyRancherResponseData {
    int32_t mValid;                   // +0x00
    uint8_t _unknown04[0xC];
    bMatrix4 mMatrix;                 // +0x10 (translation in centimetres: scaled by 0.01)
};

struct Camera {
    CameraParams mCurrent;            // +0x000
    CameraParams mPrevious;           // +0x0D0 copy of mCurrent before each SetCameraMatrix
    CameraParams mVelocity;           // +0x1A0 (current - previous) / mElapsedTime (matrix part unused)
    uint8_t mbClearVelocity;          // +0x270 PC: byte (GC: int)
    uint8_t _unknown271[3];
    float mElapsedTime;               // +0x274 SetCameraMatrix's time argument (ctor 1.0)
    uint32_t _unknown278;             // +0x278 (ctor 0x80000000)
    uint32_t _unknown27C;             // +0x27C (ctor RealTimeFrames)
    uint32_t _unknown280;             // +0x280 (ctor 0)
    uint32_t _unknown284;             // +0x284 (ctor 0)
    uint8_t _unknown288[8];           // PC arrays of Camera use stride 0x290 (e.g. 77E2B0: 6 at 0x871BE0)

    // GC 8005F260 / PC 44F5A0 (__thiscall, returns this): defaults, then SetCameraMatrix(default matrix, 1.0) twice.
    // Only the fields listed in the header are written (PC writes 0x270 as a byte).
    Camera *Construct();
    // GC 8005F448 / PC 447280 (__thiscall, ret 8). No-op while Camera::StopUpdating (PC 0x828640). Keeps the old
    // state in mPrevious, takes the matrix (or the Jolly Rancher override), derives position/direction/target and,
    // when elapsed > 0, the per-second velocity of every parameter.
    void SetCameraMatrix(const bMatrix4 *matrix, float elapsed);
    // GC 8005FB68 / PC 441070 (__thiscall, ret 4): debug link; sends the matrix (translation * 100) and name
    // through bFunkCallASync (PC: empty). No observable effect on PC.
    void CommunicateWithJollyRancher(const char *name) const;
    // GC 8005FCA8 / PC 441120 (__thiscall): debug tool feed (bFunkCallASync, empty on PC).
    void UpdateSeeulatorTool() const;
    // GC 80060014 / PC 441340 (__thiscall, ret 0xC): matrix = matrix x (noise translation x/y, FOV-relative
    // rotations about X then Y). Noise sums of two octave sets scaled by intensity.
    void ApplyNoise(bMatrix4 *matrix, float time, float intensity) const;
    // GC 8005FFA8 / PC 4412E0 (__thiscall, ret 4): angle scaled from this camera's FOV to the baseline FOV
    // (global aBaselineFovNoise, PC 0x7EC1F0): bASin(float(float(bSin(a) * float(bSin(fov/2))) / bSin(base/2))).
    uint16_t FovRelativeAngle(uint16_t angle) const;
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(CameraParams) == 0xD0);
static_assert(offsetof(CameraParams, mPosition) == 0x40);
static_assert(offsetof(CameraParams, mDirection) == 0x50);
static_assert(offsetof(CameraParams, mTarget) == 0x60);
static_assert(offsetof(CameraParams, mNoiseFrequency1) == 0x70);
static_assert(offsetof(CameraParams, mNoiseAmplitude2) == 0xA0);
static_assert(offsetof(CameraParams, mTargetDistance) == 0xB0);
static_assert(offsetof(CameraParams, mu16Fov) == 0xC4);
static_assert(offsetof(JollyRancherResponseData, mMatrix) == 0x10);
static_assert(offsetof(Camera, mPrevious) == 0xD0);
static_assert(offsetof(Camera, mVelocity) == 0x1A0);
static_assert(offsetof(Camera, mbClearVelocity) == 0x270);
static_assert(offsetof(Camera, mElapsedTime) == 0x274);
static_assert(offsetof(Camera, _unknown278) == 0x278);
static_assert(offsetof(Camera, _unknown284) == 0x284);
static_assert(sizeof(Camera) == 0x290);
#endif
