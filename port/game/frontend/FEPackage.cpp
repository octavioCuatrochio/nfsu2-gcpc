#include "FEPackage.h"
#include "FEObject.h"
#include "FEScript.h"
#include "bridge/Bridge.h"

namespace {
enum : uint32_t {
    kFlagTrack0Active = 0x800000,    // first track still animating
    kFlagTracksRunning = 0x1000000,  // some track with data index < 16 is not done (flag bit 7 clear)
    kFlagAnimating = 0x2000000,
};
}

// PC 541DB0. Tracks are only evaluated while the object's alpha (data dword 3) is non-zero, except track 0 when it
// targets data index 0 (the color itself). As in the original (and GC), the loop starts again at track 0, and
// the clears keep the bits set in uHoldDirtyFlags.
void FEPackage::UpdateObjectTracks(FEObject *obj, FEScript *script) const
{
    auto *data = static_cast<uint8_t *>(obj->mpData);
    int32_t time = static_cast<int32_t>(script->mi32Time);
    FEKeyTrack *tracks = script->mpTracks;
    auto alpha = [&] { return *reinterpret_cast<const uint32_t *>(data + 0xC); };
    uint32_t &flags = obj->mu32Flags;
    if (!mbFastInterp) {
        if (tracks && tracks[0].mi8DataIndex == 0) FEKeyInterp(&tracks[0], time, data);
        uint8_t n = static_cast<uint8_t>(script->mu32NumTracks);
        if (alpha() != 0 && n != 0)
            for (uint8_t i = 0; i < n; ++i) FEKeyInterp(&tracks[i], time, data + tracks[i].mi8DataIndex * 4);
    } else {
        if (!tracks || tracks[0].mi8DataIndex != 0) {
            flags &= bridge::HoldDirtyFlags() | ~kFlagTrack0Active;
        } else if (tracks[0].mu8Flags & 0x80) {
            flags &= bridge::HoldDirtyFlags() | ~kFlagTrack0Active;
            FEKeyInterpFast(&tracks[0], time, data);
        } else {
            flags |= kFlagTrack0Active;
            FEKeyInterpFast(&tracks[0], time, data);
        }
        uint8_t done = 0x80;
        uint8_t n = static_cast<uint8_t>(script->mu32NumTracks);
        bool running = false;
        if (alpha() != 0 && n != 0) {
            for (uint8_t i = 0; i < n; ++i) {
                if (tracks[i].mi8DataIndex < 0x10) done &= tracks[i].mu8Flags;
                FEKeyInterpFast(&tracks[i], time, data + tracks[i].mi8DataIndex * 4);
            }
            running = done == 0;
        }
        if (running) flags |= kFlagTracksRunning;
        else flags &= bridge::HoldDirtyFlags() | ~kFlagTracksRunning;
    }
    if (flags & 0x1C00000) flags |= kFlagAnimating;
}
