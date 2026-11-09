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

void FEInterpLinear(FEKeyTrack *track, int32_t time, void *out) { bridge::FEInterpLinear(track, time, out); }

namespace {
// FEKeyInterp (GC 801401EC, inlined on PC): 0 step, 1/3 linear, others leave `out` alone.
void KeyInterp(FEKeyTrack *track, int32_t time, void *out)
{
    switch (track->mu8InterpType) {
    case 0: FEInterpNone(track, time, out); break;
    case 1:
    case 3: FEInterpLinear(track, time, out); break;
    default: break;
    }
}

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
    KeyInterp(track, time, out);
    if (CountKeys(track) == 0) track->mu8Flags |= 0x80;
}

// PC 535570. Difference: for interpolation types the original does not evaluate (2, >3) it adds an uninitialized
// stack value; here that value is zero.
void FEKeyTrack::SetBaseValueAt(int32_t time, const FEColor &value)
{
    FEColor cur{{0, 0, 0, 0}};
    KeyInterp(this, time, &cur);
    FEColor d = FEColorSub(value, cur);
    FEColor base;
    std::memcpy(&base, &mBaseKey._data[1], sizeof base);
    for (int i = 0; i < 4; ++i) base.c[i] = static_cast<int32_t>(static_cast<uint32_t>(base.c[i]) + static_cast<uint32_t>(d.c[i]));
    std::memcpy(&mBaseKey._data[1], &base, sizeof base);
    mu8Flags &= 0x7F;
}
