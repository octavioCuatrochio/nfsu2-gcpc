#include "Camera.h"
#include "bWare/bMath.h"
#include "bridge/Bridge.h"

uint16_t Camera::FovRelativeAngle(uint16_t angle) const
{
    using L = long double;
    const float half = static_cast<float>(bSin(static_cast<uint16_t>(mu16Fov >> 1)));
    const float scaled = static_cast<float>(bSin(angle) * half);
    const float ratio = static_cast<float>(L(scaled) / bSin(static_cast<uint16_t>(bridge::BaselineFovNoise() >> 1)));
    return bASin(ratio);
}
