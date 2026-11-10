#include "FEButtonMap.h"
#include "bridge/Bridge.h"
#include "FEObject.h"

void FEMultMatrix(FEVector3 *out, const float *m, const FEVector3 *v)
{
    using L = long double;
    const L y = (L(m[9]) * v->z + L(m[1]) * v->x) + L(m[5]) * v->y + m[13];
    const L z = (L(m[10]) * v->z + L(m[2]) * v->x) + L(m[6]) * v->y + m[14];
    const L x = (L(m[8]) * v->z + L(m[4]) * v->y) + L(m[0]) * v->x + m[12];
    out->x = static_cast<float>(x);
    out->y = static_cast<float>(y);
    out->z = static_cast<float>(z);
}

void FEButtonMap::ComputeButtonLocation(FEObject *obj, void *gi, FEVector2 *out) const
{
    const auto *pos = reinterpret_cast<const FEVector3 *>(static_cast<const uint8_t *>(obj->mpData) + 0x1C);
    if (gi && obj->mu16RenderContext) {
        float m[16];
        if (bridge::FEGameInterface_GetContextTransform(gi, obj->mu16RenderContext, m)) {
            FEVector3 v{0.0f, 0.0f, 0.0f};
            FEMultMatrix(&v, m, pos);
            out->x = v.x;
            out->y = v.y;
            return;
        }
    }
    out->x = pos->x;
    out->y = pos->y;
}

namespace {
const FEVector2 kDirVectors[8] = {{0.0f, -1.0f}, {0.70711f, -0.70711f}, {1.0f, 0.0f}, {0.70711f, 0.70711f},
                                  {0.0f, 1.0f}, {-0.70711f, 0.70711f}, {-1.0f, 0.0f}, {-0.70711f, -0.70711f}};   // PC 0x7FA358
const FEVector2 kPassOffsets[5] = {{0, 0}, {640, 0}, {-640, 0}, {0, 480}, {0, -480}};                         // PC 0x7FA398
const uint32_t kPassWrap[5] = {3, 1, 1, 2, 2};                                                                // PC 0x7F7CB0
}  // namespace

FEObject *FEButtonMap::GetButtonFrom(FEObject *from, uint32_t dir, void *gi, uint32_t wrap) const
{
    using L = long double;
    float best = 1e30f;
    uint32_t bestIndex = 0;
    FEVector2 loc{0, 0}, other{0, 0};
    ComputeButtonLocation(from, gi, &loc);
    for (uint32_t pass = 0; pass < 5; ++pass) {
        if (pass && !(kPassWrap[pass] & wrap)) continue;
        const float px = static_cast<float>(L(loc.x) + kPassOffsets[pass].x);
        const float py = static_cast<float>(L(loc.y) + kPassOffsets[pass].y);
        for (uint32_t i = 0; i < mu32Count; ++i) {
            FEObject *o = mppButtons[i];
            if ((o->mu32Flags & 0x4000000) || o == from) continue;
            ComputeButtonLocation(o, gi, &other);
            const L dx = L(other.x) - px, dy = L(other.y) - py;
            const L d = __builtin_sqrtl(dy * dy + dx * dx);
            if (d < L(9.99999974737875e-05f)) continue;
            const float fd = static_cast<float>(d);
            const L inv = 1.0L / fd;
            const float nx = static_cast<float>(dx * inv);
            L dot = inv * dy * kDirVectors[dir].y + L(nx) * kDirVectors[dir].x;
            if (dot >= 0) dot = dot * dot;
            const L score = (dot >= 0.25L) ? (1.0L - dot) * 200.0L + fd : 1500.0L;   // NaN -> 1500
            if (score < best) {
                best = static_cast<float>(score);
                bestIndex = i;
            }
        }
    }
    return best < 1500.0f ? mppButtons[bestIndex] : nullptr;
}
