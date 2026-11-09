// FEng package (a loaded menu: objects, scripts, ...). Original: zFe2.cpp. Only what the rewritten members use.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
struct FEObject;
struct FEScript;

// Game-side visitor (FEObjectCallback): vtable slot 1 = bool Function(FEObject *), false stops the walk.
struct FEObjectCallback {
    void *vtable;
};

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
    // GC 8013931C / PC 534C70: send the script events with from <= time < to (to + 1 at the script end).
    void IssueScriptMessages(void *engine, FEObject *obj, FEScript *script, int32_t from, int32_t to);
    void UpdateGroup(struct FEGroup *group, int32_t delta);   // GC 80139520 / PC 504910
    void Update(void *engine, int32_t delta);                 // GC 80139BD0 / PC 54D190: every top-level object
    // Pre-order walk over all objects (groups recurse after their own callback); false if a callback stopped it.
    bool ForAllObjects(FEObjectCallback *cb);                   // GC 801391DC / PC 504880
    bool ForAllChildren(struct FEGroup *group, FEObjectCallback *cb);   // GC 80139144 / PC 504830
    FEObject *FindObjectByGUID(uint32_t guid);                  // GC 801392C8 / PC 517FE0 (FEFindByGUID: obj+0xC)
    FEObject *FindObjectByHash(uint32_t nameHash);              // GC 80139274 / PC 517F80 (FEFindByHash: obj+0x10)
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEPackage, mbFastInterp) == 0x14 && offsetof(FEPackage, mpEngine) == 0x30);
static_assert(offsetof(FEPackage, mi32ScriptDelta) == 0x34 && offsetof(FEPackage, mObjects) == 0x44);
#endif
