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

// PC 54CE60
void FEPackage::UpdateObject(FEObject *obj, int32_t delta)
{
    const uint32_t hold = bridge::HoldDirtyFlags();
    uint32_t &flags = obj->mu32Flags;
    if (flags & 0x1C00000) flags |= kFlagAnimating;
    else flags &= hold | ~kFlagAnimating;

    FEScript *sc = obj->mpCurrentScript;
    const int32_t oldTime = sc->mi32Time;
    const int32_t len = sc->mi32Length;
    sc->mi32Time = static_cast<int32_t>(static_cast<uint32_t>(mi32ScriptDelta) + static_cast<uint32_t>(oldTime));
    if (sc->mi32Time < 0) sc->mi32Time = 0;
    const int32_t now = sc->mi32Time;
    auto issue = [&](FEScript *s, int32_t from, int32_t to) {
        bridge::FEPackage_IssueScriptMessages(this, mpEngine, obj, s, from, to);
    };

    if (now >= len) {                                   // reached the end of the script
        if (mbFastInterp) {
            if (sc->mpChainedScript) {
                UpdateObjectTracks(obj, sc);
                int32_t rest = sc->mi32Time - len;
                sc->mi32Time = 0;
                if (sc->mEvents.mu32Count) issue(sc, oldTime, len);
                sc = sc->mpChainedScript;
                obj->mpCurrentScript = sc;                // FEObject::SetCurrentScript (inlined)
                if (sc) obj->SetupMoveToTracks();
                sc->mi32Time = rest;                      // like the original: crash if the chain ends in null
                if (sc->mEvents.mu32Count) issue(sc, 0, rest);
            } else {
                switch (sc->mu32Flags & 3) {
                case 0:
                    if (sc->mEvents.mu32Count) issue(sc, oldTime, len);
                    sc->mi32Time = len + 1;
                    break;
                case 1:
                    if (len <= 0) { sc->mi32Time = 0; break; }
                    if (sc->mEvents.mu32Count) issue(sc, oldTime, len);
                    sc->mi32Time = sc->mi32Time % sc->mi32Length;   // re-read, like the original
                    if (sc->mEvents.mu32Count) issue(sc, 0, sc->mi32Time);
                    obj->SetupMoveToTracks();
                    break;
                case 2:
                    if (len <= 0) { sc->mi32Time = 0; break; }
                    sc->mi32Time = now % (len * 2);
                    break;
                default: break;
                }
            }
        }
    } else if (mbFastInterp && sc->mEvents.mu32Count) {   // still inside: fire the events passed this frame
        switch (sc->mu32Flags & 3) {
        case 0: issue(sc, oldTime, now); break;
        case 1:
            if (now < oldTime) { issue(sc, oldTime, len); issue(sc, 0, sc->mi32Time); }
            else issue(sc, oldTime, now);
            break;
        case 2:
            if (oldTime < len) {
                if (now < len) issue(sc, oldTime, now);
                else { issue(sc, oldTime, len); issue(sc, len, sc->mi32Time - len); }   // unreachable (now < len)
            } else {
                if (now >= len) issue(sc, oldTime - len, now - len);                  // unreachable
                else { issue(sc, oldTime - len, 0); issue(sc, 0, sc->mi32Time); }
            }
            break;
        default: break;
        }
    }

    // Finished, non-forced scripts are not re-evaluated (fast mode).
    bool finished = mbFastInterp && oldTime == sc->mi32Time && oldTime == sc->mi32Length + 1 &&
                    !(flags & FEOBJ_DIRTY);
    if (!finished) UpdateObjectTracks(obj, sc);

    switch (obj->mu32Type) {
    case 4: bridge::FEListBox_Update(obj, static_cast<float>(delta)); break;
    case 5: bridge::FEPackage_UpdateGroup(this, obj, delta); break;
    case 6: bridge::FECodeListBox_Update(obj, static_cast<float>(delta)); break;
    case 7:                                             // FEMovie: playback time right after the FEObject part
        if (mbFastInterp) {
            auto *t = reinterpret_cast<int32_t *>(reinterpret_cast<uint8_t *>(obj) + sizeof(FEObject));
            *t = static_cast<int32_t>(static_cast<uint32_t>(*t) + static_cast<uint32_t>(delta));
        }
        break;
    default: break;
    }
    if (mbFastInterp == 1 && oldTime == sc->mi32Time && oldTime == sc->mi32Length + 1 &&
        !(flags & FEOBJ_DIRTY))
        flags &= hold | ~(kFlagTrack0Active | kFlagTracksRunning);
    flags &= hold | ~FEOBJ_DIRTY;
    if (flags & 0x1C00000) flags |= kFlagAnimating;
}
