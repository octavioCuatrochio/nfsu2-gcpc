#include "Camera.h"
#include <cstring>
#include "CameraNoise.h"
#include "bWare/bMath.h"
#include "bWare/bString.h"
#include "bridge/Bridge.h"
#include "frontend/FEMath.h"   // FEFtol (MSVC _ftol)

namespace {
using L = long double;

// GC 80058484 bFunkCallASync (debug host link) / PC 4022C0: a bare `ret` in the retail PC build.
void bFunkCallASync(const char *, int32_t, const void *, int32_t) {}

// GC cameralink.1482 / PC 0x82A4F8: SetCameraMatrix's function static (set while the override is active).
int32_t sCameraLink = 0;
}

Camera *Camera::Construct()
{
    mbClearVelocity = 0;
    _unknown278 = 0x80000000u;
    mElapsedTime = 1.0f;
    _unknown27C = bridge::RealTimeFrames();
    _unknown280 = 0;
    _unknown284 = 0;
    mCurrent.mTargetDistance = 10.0f;
    mCurrent._unknownB4 = 0.0f;
    mCurrent._unknownB8 = 0.0f;
    mCurrent.mu16Fov = 0x36FB;
    mCurrent._unknownBC = 0.5f;
    mCurrent._unknownC0 = 10000.0f;
    mCurrent.mNoiseFrequency1 = {1.0f, 1.0f, 1.0f, 1.0f};
    mCurrent.mNoiseFrequency2 = {1.0f, 1.0f, 1.0f, 1.0f};
    mCurrent.mNoiseAmplitude1 = {0.0f, 0.0f, 0.0f, 0.0f};
    mCurrent.mNoiseAmplitude2 = {0.0f, 0.0f, 0.0f, 0.0f};
    const bMatrix4 initial = {{1.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, -1.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, -1.0f, 100.0f,
                               0.0f, 0.0f, 1200.0f, 1.0f}};
    SetCameraMatrix(&initial, 1.0f);
    SetCameraMatrix(&initial, 1.0f);
    return this;
}

void Camera::SetCameraMatrix(const bMatrix4 *matrix, float elapsed)
{
    if (bridge::CameraStopUpdating() != 0) return;
    std::memcpy(&mPrevious, &mCurrent, sizeof(CameraParams));
    mElapsedTime = elapsed;
    const JollyRancherResponseData *jr = bridge::JollyRancherResponse();
    bMatrix4 jrMatrix;
    if (jr->mValid != 0 && bridge::DisableCommunication() == 0) {
        if (sCameraLink == 0) sCameraLink = 1;
        jrMatrix = jr->mMatrix;
        jrMatrix.m[12] = static_cast<float>(L(jrMatrix.m[12]) * 0.01f);
        jrMatrix.m[13] = static_cast<float>(L(jrMatrix.m[13]) * 0.01f);
        jrMatrix.m[14] = static_cast<float>(L(jrMatrix.m[14]) * 0.01f);
        jrMatrix.m[15] = 1.0f;
        matrix = &jrMatrix;
    } else if (sCameraLink != 0) {
        sCameraLink = 0;
    }
    std::memmove(&mCurrent.mMatrix, matrix, sizeof(bMatrix4));   // PC 4012B0 copy

    // eTransposeMatrix (D3DX, exact) + inlined eMulVector: position = -(t x transpose(R)), t = row 3.
    const float *m = mCurrent.mMatrix.m;
    const L tx = m[12], ty = m[13], tz = m[14];
    const float px = static_cast<float>(((L(m[0]) * tx + L(m[1]) * ty) + L(m[2]) * tz) + m[3]);
    const float py = static_cast<float>(((L(m[4]) * tx + L(m[5]) * ty) + L(m[6]) * tz) + m[7]);
    const float pz = static_cast<float>(((L(m[8]) * tx + L(m[9]) * ty) + L(m[10]) * tz) + m[11]);
    mCurrent.mPosition = {-px, -py, -pz};
    const bVector3 column2 = {m[2], m[6], m[10]};
    bNormalize(&mCurrent.mDirection, &column2);
    const float dist = mCurrent.mTargetDistance;
    const bVector3 &dir = mCurrent.mDirection;
    mCurrent.mTarget.x = static_cast<float>(L(dist) * dir.x);
    mCurrent.mTarget.y = static_cast<float>(L(dist) * dir.y);
    mCurrent.mTarget.z = static_cast<float>(L(dist) * dir.z);
    mCurrent.mTarget.x = static_cast<float>(L(mCurrent.mTarget.x) + mCurrent.mPosition.x);
    mCurrent.mTarget.y = static_cast<float>(L(mCurrent.mTarget.y) + mCurrent.mPosition.y);
    mCurrent.mTarget.z = static_cast<float>(L(mCurrent.mTarget.z) + mCurrent.mPosition.z);

    if (mbClearVelocity != 0) {
        mbClearVelocity = 0;
        std::memcpy(&mPrevious, &mCurrent, sizeof(CameraParams));
        mElapsedTime = 1.0f;
    }
    if (!(mElapsedTime > 0.0f)) return;   // fcomp 0.0, C0|C3: also skips NaN

    // PC keeps 1/elapsed in x87 (GC rounds it to float). Vector differences are rounded to float first; scalar
    // differences are not.
    const L inv = L(1.0f) / mElapsedTime;
    const CameraParams &c = mCurrent, &p = mPrevious;
    CameraParams &v = mVelocity;
    auto vec3 = [inv](bVector3 &out, float &w, const bVector3 &a, const bVector3 &b) {
        out.x = static_cast<float>(L(a.x) - b.x);
        out.y = static_cast<float>(L(a.y) - b.y);
        out.z = static_cast<float>(L(a.z) - b.z);
        w = 0.0f;
        out.x = static_cast<float>(inv * out.x);
        out.y = static_cast<float>(inv * out.y);
        out.z = static_cast<float>(inv * out.z);
    };
    auto vec4 = [inv](bVector4 &out, const bVector4 &a, const bVector4 &b) {
        out.x = static_cast<float>(L(a.x) - b.x);
        out.y = static_cast<float>(L(a.y) - b.y);
        out.z = static_cast<float>(L(a.z) - b.z);
        out.w = static_cast<float>(L(a.w) - b.w);
        out.x = static_cast<float>(inv * out.x);
        out.y = static_cast<float>(inv * out.y);
        out.z = static_cast<float>(inv * out.z);
        out.w = static_cast<float>(inv * out.w);
    };
    vec3(v.mPosition, v.mPositionW, c.mPosition, p.mPosition);
    vec3(v.mDirection, v.mDirectionW, c.mDirection, p.mDirection);
    vec3(v.mTarget, v.mTargetW, c.mTarget, p.mTarget);
    const int32_t fovDelta = static_cast<uint16_t>(c.mu16Fov - p.mu16Fov);
    v.mTargetDistance = static_cast<float>((L(c.mTargetDistance) - p.mTargetDistance) * inv);
    v._unknownB4 = static_cast<float>((L(c._unknownB4) - p._unknownB4) * inv);
    v._unknownB8 = static_cast<float>((L(c._unknownB8) - p._unknownB8) * inv);
    v._unknownBC = static_cast<float>((L(c._unknownBC) - p._unknownBC) * inv);
    v._unknownC0 = static_cast<float>((L(c._unknownC0) - p._unknownC0) * inv);
    v.mu16Fov = static_cast<uint16_t>(FEFtol(L(fovDelta) * inv));
    vec4(v.mNoiseFrequency1, c.mNoiseFrequency1, p.mNoiseFrequency1);
    vec4(v.mNoiseFrequency2, c.mNoiseFrequency2, p.mNoiseFrequency2);
    vec4(v.mNoiseAmplitude1, c.mNoiseAmplitude1, p.mNoiseAmplitude1);
    vec4(v.mNoiseAmplitude2, c.mNoiseAmplitude2, p.mNoiseAmplitude2);
}

void Camera::CommunicateWithJollyRancher(const char *name) const
{
    if (bridge::DisableCommunication() != 0) return;
    struct {
        const void *response;   // &JollyRancherResponse
        int32_t disable;        // DisableCommunication (0 here)
        bMatrix4 matrix;        // translation in centimetres, w = 1
        char name[0x18];
    } packet;
    packet.response = bridge::JollyRancherResponse();
    packet.disable = 0;
    packet.matrix = mCurrent.mMatrix;
    packet.matrix.m[12] = static_cast<float>(L(packet.matrix.m[12]) * 100.0f);
    packet.matrix.m[15] = 1.0f;
    packet.matrix.m[13] = static_cast<float>(L(packet.matrix.m[13]) * 100.0f);
    packet.matrix.m[14] = static_cast<float>(L(packet.matrix.m[14]) * 100.0f);
    bStrCpy(packet.name, name);   // only caller (PC 44EFE0) passes "SpeedCam"
    bFunkCallASync("JR2Server", 1, &packet, 0x60);
}

void Camera::UpdateSeeulatorTool() const
{
    if (bridge::SeeulatorToolActive() != 0) bFunkCallASync("Seeulator", 3, this, 0xD0);
}

void Camera::ApplyNoise(bMatrix4 *matrix, float time, float intensity) const
{
    const CameraParams &c = mCurrent;
    const float f0 = static_cast<float>(L(c.mNoiseFrequency1.x) * time);
    const float f1 = static_cast<float>(L(c.mNoiseFrequency1.y) * time);
    const float f2 = static_cast<float>(L(c.mNoiseFrequency1.z) * time);
    const float f3 = static_cast<float>(L(c.mNoiseFrequency1.w) * time);
    const float n0 = static_cast<float>(Noise(f0));
    const float n1 = static_cast<float>(Noise(f1));
    const float n2 = static_cast<float>(Noise(f2));
    const L n3 = Noise(f3);
    const float a0 = static_cast<float>(L(n0) * c.mNoiseAmplitude1.x);
    const float a1 = static_cast<float>(L(n1) * c.mNoiseAmplitude1.y);
    const float a2 = static_cast<float>(L(n2) * c.mNoiseAmplitude1.z);
    const float a3 = static_cast<float>(n3 * c.mNoiseAmplitude1.w);
    const float g0 = static_cast<float>(L(c.mNoiseFrequency2.x) * time);
    const float g1 = static_cast<float>(L(c.mNoiseFrequency2.y) * time);
    const float g2 = static_cast<float>(L(c.mNoiseFrequency2.z) * time);
    const float g3 = static_cast<float>(L(c.mNoiseFrequency2.w) * time);
    const float m0 = static_cast<float>(Noise(g0));
    const float m1 = static_cast<float>(Noise(g1));
    const float m2 = static_cast<float>(Noise(g2));
    const L m3 = Noise(g3);
    const L b0 = L(m0) * c.mNoiseAmplitude2.x;   // PC: stays in x87
    const float b1 = static_cast<float>(L(m1) * c.mNoiseAmplitude2.y);
    const float b2 = static_cast<float>(L(m2) * c.mNoiseAmplitude2.z);
    const float b3 = static_cast<float>(m3 * c.mNoiseAmplitude2.w);
    const float v0 = static_cast<float>((b0 + a0) * intensity);
    const float v1 = static_cast<float>((L(b1) + a1) * intensity);
    const float s2 = static_cast<float>(L(b2) + a2);
    const float s3 = static_cast<float>(L(b3) + a3);
    const float v2 = static_cast<float>(L(s2) * intensity);
    const float v3 = static_cast<float>(L(s3) * intensity);

    bMatrix4 noise;
    bIdentity(&noise);
    noise.m[12] = v0;
    noise.m[13] = v1;
    const uint16_t rx = FovRelativeAngle(static_cast<uint16_t>(FEFtol(L(v2) * 65536.0f) / 360));
    bridge::eRotateX(&noise, &noise, rx);
    const uint16_t ry = FovRelativeAngle(static_cast<uint16_t>(FEFtol(L(v3) * 65536.0f) / 360));
    bridge::eRotateY(&noise, &noise, ry);

    // Inlined eMulMatrix(matrix, matrix, noise): out[i][j] = sum_k p[i][k] * noise[k][j], summed in the
    // original's per-element order (k0 + k1) + k2 + k3, rounded once; then copied back (PC 4012B0).
    static const uint8_t kRow0[4][4] = {{0, 3, 2, 1}, {3, 2, 1, 0}, {2, 1, 3, 0}, {2, 1, 3, 0}};
    static const uint8_t kRowN[4][4] = {{3, 0, 1, 2}, {0, 3, 1, 2}, {0, 3, 1, 2}, {0, 3, 1, 2}};
    const float *pm = matrix->m;
    bMatrix4 out;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            const uint8_t *k = i == 0 ? kRow0[j] : kRowN[j];
            auto t = [&](int kk) { return L(pm[i * 4 + kk]) * noise.m[kk * 4 + j]; };
            out.m[i * 4 + j] = static_cast<float>(((t(k[0]) + t(k[1])) + t(k[2])) + t(k[3]));
        }
    }
    *matrix = out;
}

uint16_t Camera::FovRelativeAngle(uint16_t angle) const
{
    const float half = static_cast<float>(bSin(static_cast<uint16_t>(mCurrent.mu16Fov >> 1)));
    const float scaled = static_cast<float>(bSin(angle) * half);
    const float ratio = static_cast<float>(L(scaled) / bSin(static_cast<uint16_t>(bridge::BaselineFovNoise() >> 1)));
    return bASin(ratio);
}
