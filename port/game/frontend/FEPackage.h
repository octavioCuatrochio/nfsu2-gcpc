// FEng package (a loaded menu: objects, scripts, ...). Original: zFe2.cpp. Only what the rewritten members use.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
struct FEObject;
struct FEScript;

struct FEPackage {
    uint8_t _unknown00[0x14];
    uint8_t mbFastInterp;            // +0x14 use FEKeyInterpFast (done-track skipping + dirty-flag tracking);
                                     //       also gates script time handling / events in UpdateObject
    uint8_t _unknown15[0x1B];
    void *mpEngine;                  // +0x30 FEngine (passed to IssueScriptMessages)
    int32_t mi32ScriptDelta;         // +0x34 time added to the current script per UpdateObject
    uint8_t _unknown38[0xC];
    FEMinList mObjects;              // +0x44 top-level objects

    // GC 801399F8 / PC 541DB0: evaluate the script's tracks at its current time into the object's data.
    void UpdateObjectTracks(FEObject *obj, FEScript *script) const;
    // GC 8013958C / PC 54CE60: advance the object's current script (events, end handling: chain / stop / loop /
    // ping-pong), evaluate tracks, then type-specific updates (list boxes, groups, movies) with `delta`.
    void UpdateObject(FEObject *obj, int32_t delta);
    void UpdateGroup(struct FEGroup *group, int32_t delta);   // GC 80139520 / PC 504910
    void Update(void *engine, int32_t delta);                 // GC 80139BD0 / PC 54D190: every top-level object
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEPackage, mbFastInterp) == 0x14 && offsetof(FEPackage, mpEngine) == 0x30);
static_assert(offsetof(FEPackage, mi32ScriptDelta) == 0x34 && offsetof(FEPackage, mObjects) == 0x44);
#endif
