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

struct FELibraryRef {               // 0xC bytes
    uint32_t mu32Hash;
    uint32_t _unknown04[2];
};

struct FEPackage {
    uint8_t _unknown00[0x14];
    uint8_t mbFastInterp;            // +0x14 use FEKeyInterpFast (done-track skipping + dirty-flag tracking);
                                     //       also gates script time handling / events in UpdateObject
    uint8_t _unknown15[7];
    uint32_t mu32ControlMask;        // +0x1C (GC +0x28)
    uint32_t mu32SavedControlMask;   // +0x20 (GC +0x2C)
    uint32_t mu32InputControlMask;   // +0x24 (GC +0x30) mask used by input
    uint32_t mu32SavedInputControlMask;   // +0x28 (GC +0x34)
    uint8_t _unknown2C[4];
    void *mpEngine;                  // +0x30 FEngine (passed to IssueScriptMessages)
    int32_t mi32ScriptDelta;         // +0x34 time added to the current script per UpdateObject
    uint8_t _unknown38[0xC];
    FEMinList mObjects;              // +0x44 top-level objects
    uint8_t _unknown54[0x30];
    uint32_t mu32NumLibraryRefs;     // +0x84
    struct FELibraryRef *mpLibraryRefs;   // +0x88 array, 0xC bytes each, hash first
    FEObject *mpCurrentButton;       // +0x8C

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
    // Control masks (GC 801390CC.., PC: only SetControlMask 5047E0 is a function, the others are inlined).
    void SetControlMask(uint32_t m) { mu32ControlMask = mu32SavedControlMask = mu32InputControlMask = mu32SavedInputControlMask = m; }
    void SetControlMaskOnly(uint32_t m) { mu32InputControlMask = mu32ControlMask = m; }
    void RestoreControlMask() { mu32SavedInputControlMask = mu32ControlMask = mu32SavedControlMask; }
    void SetControlMaskUsedByInputOnly(uint32_t m) { mu32InputControlMask = m; }
    void RestoreControlMaskUsedByInput() { mu32InputControlMask = mu32SavedInputControlMask; }
    struct FELibraryRef *FindLibraryReference(uint32_t hash) const;   // GC 8013A224 / PC 504960
    // GC 80139C2C / PC 52B3A0: with notify, the old button gets 0x55D1E635 and the new one 0xABC08912 (each sent
    // to the button and to target -5).
    void SetCurrentButton(void *engine, FEObject *button, bool notify);
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEPackage, mbFastInterp) == 0x14 && offsetof(FEPackage, mpEngine) == 0x30);
static_assert(offsetof(FEPackage, mi32ScriptDelta) == 0x34 && offsetof(FEPackage, mObjects) == 0x44);
static_assert(offsetof(FEPackage, mu32ControlMask) == 0x1C && offsetof(FEPackage, mu32SavedInputControlMask) == 0x28);
static_assert(offsetof(FEPackage, mu32NumLibraryRefs) == 0x84 && offsetof(FEPackage, mpCurrentButton) == 0x8C);
static_assert(sizeof(FELibraryRef) == 0xC);
#endif
