// FEng key-track evaluation (key lookup, step interpolation, dispatch). Original: zFe2.cpp.
// Key node layout: next +4, prev +8, time +0xC (_data[0]), value +0x10 (_data[1..]).
#include "FEScript.h"
#include "FEMath.h"
#include "bridge/Bridge.h"
#include <cstring>

namespace {
int32_t KeyTime(const FEKeyNode *k) { return static_cast<int32_t>(k->_data[0]); }
FEKeyNode *Next(const FEKeyNode *k) { return reinterpret_cast<FEKeyNode *>(k->node.next); }
FEKeyNode *Prev(const FEKeyNode *k) { return reinterpret_cast<FEKeyNode *>(k->node.prev); }
const void *Value(const FEKeyNode *k) { return &k->_data[1]; }
}  // namespace

FEKeyNode *FEKeyTrack::GetFirstKey() const
{
    return reinterpret_cast<FEKeyNode *>(mKeys.GetHead());
}

// PC 505260: walk while there is a next key and the key's time is before `time`.
FEKeyNode *FEKeyTrack::GetKeyAt(int32_t time)
{
    if (time <= -1) return &mBaseKey;
    FEKeyNode *k = GetFirstKey();
    if (!k) return &mBaseKey;
    while (Next(k) && KeyTime(k) < time) k = Next(k);
    return k;
}

// PC 5052B0
FEKeyNode *FEKeyTrack::GetDeltaKeyAt(int32_t time) const
{
    FEKeyNode *k = GetFirstKey();
    if (!k) return nullptr;
    while (Next(k) && KeyTime(k) < time) k = Next(k);
    return k;
}

// PC 519D40 (step: the key at or before `time`). Past the track length the lookup uses (length - time), which is
// negative and therefore the base key; its `next` is then used if the base key's time is before `time` (as in
// the original and GC; base keys normally keep the unlinked guard there).
void FEInterpNone(FEKeyTrack *track, int32_t time, void *out)
{
    size_t size = track->mu8ValueSize;
    int32_t len = track->Length();
    const FEKeyNode *src;
    if (time <= len) {
        FEKeyNode *k = track->GetKeyAt(time);
        FEKeyNode *p = Prev(k);
        src = (p && KeyTime(k) > time) ? p : k;
    } else {
        FEKeyNode *k = track->GetKeyAt(len - time);
        FEKeyNode *n = Next(k);
        src = (n && KeyTime(k) < time) ? n : k;
    }
    std::memcpy(out, Value(src), size);
}

namespace {
const uint32_t *Val(const FEKeyNode *k) { return &k->_data[1]; }
float F(const uint32_t *p, int i) { float v; std::memcpy(&v, p + i, 4); return v; }
int32_t I(const uint32_t *p, int i) { return static_cast<int32_t>(p[i]); }

// Blend factor (time - from) / span, rounded to float; span <= 1e-5 (int compared as float) gives 0.
float Factor(int32_t num, int32_t span)
{
    if (static_cast<long double>(span) <= 1e-5f) return 0.f;
    return static_cast<float>(static_cast<long double>(num) / static_cast<long double>(span));
}
int32_t Sub(int32_t a, int32_t b) { return static_cast<int32_t>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b)); }
int32_t Add(int32_t a, int32_t b) { return static_cast<int32_t>(static_cast<uint32_t>(a) + static_cast<uint32_t>(b)); }

void CopyBase(const FEKeyTrack *t, void *out)
{
    static const uint8_t kDwords[7] = {0, 1, 1, 2, 3, 4, 4};
    if (t->mu8ParamType >= 1 && t->mu8ParamType <= 6) std::memcpy(out, &t->mBaseKey._data[1], kDwords[t->mu8ParamType] * 4);
}

// One key's value + base (t == 0 or 1). Quaternions: key * base.
void KeyPlusBase(const FEKeyTrack *t, const uint32_t *k, void *out)
{
    const uint32_t *b = &t->mBaseKey._data[1];
    auto *o = static_cast<uint32_t *>(out);
    auto setf = [&](int i, long double v) { float f = static_cast<float>(v); std::memcpy(o + i, &f, 4); };
    switch (t->mu8ParamType) {
    case 1: o[0] = static_cast<uint32_t>(Add(I(b, 0), I(k, 0))); break;
    case 2: setf(0, static_cast<long double>(F(b, 0)) + F(k, 0)); break;
    case 3: setf(0, static_cast<long double>(F(b, 0)) + F(k, 0)); setf(1, static_cast<long double>(F(k, 1)) + F(b, 1)); break;
    case 4:
        setf(0, static_cast<long double>(F(b, 0)) + F(k, 0));
        setf(1, static_cast<long double>(F(k, 1)) + F(b, 1));
        setf(2, static_cast<long double>(F(k, 2)) + F(b, 2));
        break;
    case 5: {
        FEQuaternion kq, bq;
        std::memcpy(&kq, k, sizeof kq);
        std::memcpy(&bq, b, sizeof bq);
        FEQuaternion r = FEQuatMultiply(kq, bq);
        std::memcpy(out, &r, sizeof r);
        break;
    }
    case 6:
        o[2] = static_cast<uint32_t>(Add(I(k, 2), I(b, 2)));
        o[1] = static_cast<uint32_t>(Add(I(k, 1), I(b, 1)));
        o[0] = static_cast<uint32_t>(Add(I(b, 0), I(k, 0)));
        o[3] = static_cast<uint32_t>(Add(I(k, 3), I(b, 3)));
        break;
    default: break;
    }
}
}  // namespace

// GC 80140318 / PC 52BDC0. Wrap mode = flags & 0x7F: 0 clamp, 1 loop (wraps from the last key to the first over
// the track length), 2 ping-pong (as in the original, it then reads a null "from" key: crash if a key is found).
void FEInterpLinear(FEKeyTrack *track, int32_t time, void *out)
{
    if (!track->GetFirstKey()) { CopyBase(track, out); return; }
    FEKeyNode *from = nullptr, *to = nullptr;
    float t = 0.f;
    switch (track->mu8Flags & 0x7F) {
    case 0:
        to = track->GetDeltaKeyAt(time);
        if (!to) break;
        from = Prev(to);
        if (!from || time >= KeyTime(to)) t = 1.f;
        else t = Factor(Sub(time, KeyTime(from)), Sub(KeyTime(to), KeyTime(from)));
        break;
    case 1: {
        to = track->GetDeltaKeyAt(time);
        if (!to) break;
        int32_t toTime = KeyTime(to);
        if (toTime < time) {                       // past the last key: blend toward the first one
            from = to;
            to = track->GetFirstKey();
            t = Factor(Sub(time, KeyTime(from)), Sub(Add(track->Length(), KeyTime(to)), KeyTime(from)));
        } else if (toTime == time) {
            from = to;
            t = 0.f;
        } else if ((from = Prev(to)) != nullptr) {
            t = Factor(Sub(time, KeyTime(from)), Sub(toTime, KeyTime(from)));
        } else {                                   // before the first key: blend from the key at the track end
            int32_t len = track->Length();
            from = track->GetKeyAt(len);
            int32_t rest = Sub(len, KeyTime(from));
            t = Factor(Add(rest, time), Add(rest, toTime));
        }
        break;
    }
    case 2: {
        int32_t len = track->Length(), tt = time;
        if (tt > len) tt = Sub(Add(len, len), tt);
        to = track->GetDeltaKeyAt(tt);
        break;
    }
    default: break;
    }
    if (!to) { CopyBase(track, out); return; }
    if (t == 0.f) { KeyPlusBase(track, Val(from), out); return; }
    if (t == 1.f) { KeyPlusBase(track, Val(to), out); return; }

    const uint32_t *a = Val(from), *b = Val(to), *base = &track->mBaseKey._data[1];
    switch (track->mu8ParamType) {
    case 1: FELerpInteger(I(a, 0), I(b, 0), t, reinterpret_cast<const int32_t *>(base), static_cast<int32_t *>(out)); break;
    case 2: {
        float r = static_cast<float>(((static_cast<long double>(F(b, 0)) - F(a, 0)) * t + F(a, 0)) + F(base, 0));
        std::memcpy(out, &r, 4);
        break;
    }
    case 3: FELerpVector2(reinterpret_cast<const FEVector2 *>(a), reinterpret_cast<const FEVector2 *>(b), t,
                          reinterpret_cast<const FEVector2 *>(base), static_cast<FEVector2 *>(out)); break;
    case 4: FELerpVector3(reinterpret_cast<const FEVector3 *>(a), reinterpret_cast<const FEVector3 *>(b), t,
                          reinterpret_cast<const FEVector3 *>(base), static_cast<FEVector3 *>(out)); break;
    case 5: FELerpQuaternion(reinterpret_cast<const FEQuaternion *>(a), reinterpret_cast<const FEQuaternion *>(b), t,
                             reinterpret_cast<const FEQuaternion *>(base), static_cast<FEQuaternion *>(out)); break;
    case 6: FELerpColor(reinterpret_cast<const FEColor *>(a), reinterpret_cast<const FEColor *>(b), t,
                        reinterpret_cast<const FEColor *>(base), static_cast<FEColor *>(out)); break;
    default: break;
    }
}

// FEKeyInterp (GC 801401EC, inlined on PC): 0 step, 1/3 linear, others leave `out` alone.
void FEKeyInterp(FEKeyTrack *track, int32_t time, void *out)
{
    switch (track->mu8InterpType) {
    case 0: FEInterpNone(track, time, out); break;
    case 1:
    case 3: FEInterpLinear(track, time, out); break;
    default: break;
    }
}

namespace {
uint32_t CountKeys(const FEKeyTrack *t)   // FERefList::GetNumElements (GC 80142828, inlined on PC)
{
    uint32_t n = 0;
    for (const FEKeyNode *k = t->GetFirstKey(); k; k = Next(k)) ++n;
    return n;
}
}  // namespace

// PC 5354F0: tracks marked done (flag bit 7) are skipped; a track without keys gets marked done.
void FEKeyInterpFast(FEKeyTrack *track, int32_t time, void *out)
{
    if (track->mu8Flags & 0x80) return;
    FEKeyInterp(track, time, out);
    if (CountKeys(track) == 0) track->mu8Flags |= 0x80;
}

// PC 535570. Difference: for interpolation types the original does not evaluate (2, >3) it adds an uninitialized
// stack value; here that value is zero.
void FEKeyTrack::SetBaseValueAt(int32_t time, const FEColor &value)
{
    FEColor cur{{0, 0, 0, 0}};
    FEKeyInterp(this, time, &cur);
    FEColor d = FEColorSub(value, cur);
    FEColor base;
    std::memcpy(&base, &mBaseKey._data[1], sizeof base);
    for (int i = 0; i < 4; ++i) base.c[i] = static_cast<int32_t>(static_cast<uint32_t>(base.c[i]) + static_cast<uint32_t>(d.c[i]));
    std::memcpy(&mBaseKey._data[1], &base, sizeof base);
    mu8Flags &= 0x7F;
}
