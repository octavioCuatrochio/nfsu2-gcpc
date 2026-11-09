#include "CameraNoise.h"
#include "frontend/FEMath.h"   // FEFtol (MSVC _ftol)

namespace {
using L = long double;
uint32_t Hash(uint32_t n)
{
    return (n * (n * n * 0x3D73u + 0xC0AE5u) - 0x2DF722F3u) & 0x7FFFFFFFu;
}
}

long double NoiseInterpolated(float x)
{
    L f = L(FEFtol(L(x)));
    if (f > L(x)) f = f - 1.0f;
    const int32_t i = FEFtol(f);
    const uint32_t a = static_cast<uint32_t>(i), b = a + 1;
    const L t = L(x) - L(i);
    const L v0 = L(1.0f) - L(static_cast<int32_t>(Hash(a ^ (a << 13)))) * 9.313225746154785e-10f;
    const L lo = v0 * (L(1.0f) - t);
    const L v1 = L(1.0f) - L(static_cast<int32_t>(Hash(b ^ (b << 13)))) * 9.313225746154785e-10f;
    return lo + v1 * t;
}

long double Noise(float x)
{
    float sum = 0.0f, freq = 1.0f, amp = 1.0f;
    for (int i = 0; i < 6; ++i) {
        const float arg = static_cast<float>(L(freq) * x);
        sum = static_cast<float>(NoiseInterpolated(arg) * amp + sum);
        freq = static_cast<float>(L(freq) + freq);
        amp = static_cast<float>(L(0.5f) * amp);
    }
    return sum;
}
