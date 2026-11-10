// FEng package (a loaded menu: objects, scripts, ...). Original: zFe2.cpp. Only what the rewritten members use.
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
struct FEObject;
struct FEScript;
struct FEMessageResponse;
struct FEPackageData;   // FEPackageManager.h

// Game-side visitor (FEObjectCallback): vtable slot 1 = bool Function(FEObject *), false stops the walk.
struct FEObjectCallback {
    void *vtable;
};

struct FELibraryRef {               // 0xC bytes
    uint32_t mu32Hash;
    uint32_t _unknown04[2];
};

struct FEMessageTargets {           // 0x10 bytes
    uint32_t mu32MsgId;
    uint32_t mu32Capacity;           // +0x4
    uint32_t mu32Count;              // +0x8
    FEObject **mpObjects;            // +0xC

    void SetCapacity(uint32_t n);    // GC (inlined) / PC 517570 (ret 4): 0 frees and clears all; count kept otherwise
};

struct FEPackageBufferNode {        // FEPackage::mList9C entries (GC +0xA8 list)
    FEMinNode node;
    uint32_t _unknown0C;
    void *mpBuffer;                  // +0x10 owned (operator delete)
};

struct FEPackage {
    FEMinNode node;                  // +0x00 FEngine package list link
    const char *mpcName;             // +0x0C
    uint32_t mu32NameHash;           // +0x10
    uint8_t mbFastInterp;            // +0x14 use FEKeyInterpFast (done-track skipping + dirty-flag tracking);
                                     //       also gates script time handling / events in UpdateObject
    uint8_t _unknown15[3];
    int32_t mi32Priority;            // +0x18 (GC +0x24) stack level given to PushPackage; <0: not pushed
    uint32_t mu32ControlMask;        // +0x1C (GC +0x28)
    uint32_t mu32SavedControlMask;   // +0x20 (GC +0x2C)
    uint32_t mu32InputControlMask;   // +0x24 (GC +0x30) mask used by input
    uint32_t mu32SavedInputControlMask;   // +0x28 (GC +0x34)
    uint8_t _unknown2C[4];
    void *mpEngine;                  // +0x30 FEngine (passed to IssueScriptMessages)
    int32_t mi32ScriptDelta;         // +0x34 time added to the current script per UpdateObject
    char *mpcFilename;               // +0x38 owned copy (SetFilename)
    FEPackage *mpParent;             // +0x3C (GC +0x48) package that pushed this one (gets its masks back on pop)
    FEPackageData *mpPackageData;    // +0x40 (GC +0x4C) game FEPackageData (set by Activate); its +0x24 is the render info
    FEMinList mObjects;              // +0x44 top-level objects
    FEMinList mResponses;            // +0x54 package-level FEMessageResponses ("global" handlers)
    uint32_t mu32NumResources;       // +0x64 (GC +0x70) resource requests, given to the game on Startup
    void *mpResources;               // +0x68 (GC +0x74) array (operator new[])
    uint32_t mu32NumMessageTargets;  // +0x6C
    struct FEMessageTargets *mpMessageTargets;   // +0x70 per-message object lists (new[] with count cookie at -4)
    FEMinList mList74;               // +0x74 constructed with vtable 0x79B878, purged as a plain FEMinList
    uint32_t mu32NumLibraryRefs;     // +0x84
    struct FELibraryRef *mpLibraryRefs;   // +0x88 array, 0xC bytes each, hash first
    FEObject *mpCurrentButton;       // +0x8C
    FEObject **mppButtons;           // +0x90 (GC +0x9C) selectable objects
    uint32_t mu32NumButtons;         // +0x94 (GC +0xA0)
    void *mp98;                      // +0x98 (GC +0xA4) owned buffer
    FEMinList mList9C;               // +0x9C (GC +0xA8) nodes own a buffer at node +0x10
    uint8_t mbFlagsAC[4];            // +0xAC all 1 after construction

    // GC 80138CEC / PC 52B100 (vtable 0x79B8F8)
    void Construct();
    // GC 80138DF0 / PC 52B1E0 (non-deleting) and the scalar deleting dtor PC 534C50 (vtable slot 0)
    void Destruct();
    FEPackage *ScalarDeletingDestructor(uint32_t flags);
    void SetFilename(const char *name);                         // GC 80138F64 / PC 517F30 (ret 4)
    // GC 80138FD4 / PC 52B370 (ret 4): game interface vtable slot 1 (pkg, numResources, resources), then
    // ConnectObjectResources; returns the game's answer.
    bool Startup(void *gameInterface);
    void ConnectObjectResources();                              // GC 8013A0FC / PC 518140
    // GC 8013A150 / PC 518190 (ret 4): new entries {0, -1, 0}, keeps the first min(n, old); n == 0 frees the array
    // but (PC) leaves the count.
    void SetNumLibraryRefs(uint32_t n);

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
    FEMessageResponse *FindResponse(uint32_t msg) const;        // GC 80139110 (PC: inlined)
    FEMessageTargets *GetMessageTargets(uint32_t msg) const;    // GC 801372C0 (PC: inlined)
    // GC 80139C2C / PC 52B3A0: with notify, the old button gets 0x55D1E635 and the new one 0xABC08912 (each sent
    // to the button and to target -5).
    void SetCurrentButton(void *engine, FEObject *button, bool notify);
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(FEPackage, mbFastInterp) == 0x14 && offsetof(FEPackage, mpEngine) == 0x30);
static_assert(offsetof(FEPackage, mi32ScriptDelta) == 0x34 && offsetof(FEPackage, mObjects) == 0x44);
static_assert(offsetof(FEPackage, mppButtons) == 0x90 && offsetof(FEPackage, mu32NumButtons) == 0x94);
static_assert(offsetof(FEPackage, mi32Priority) == 0x18 && offsetof(FEPackage, mpParent) == 0x3C);
static_assert(offsetof(FEPackage, mu32ControlMask) == 0x1C && offsetof(FEPackage, mu32SavedInputControlMask) == 0x28);
static_assert(offsetof(FEPackage, mu32NumLibraryRefs) == 0x84 && offsetof(FEPackage, mpCurrentButton) == 0x8C);
static_assert(sizeof(FELibraryRef) == 0xC && sizeof(FEMessageTargets) == 0x10);
static_assert(offsetof(FEPackage, mResponses) == 0x54 && offsetof(FEPackage, mpMessageTargets) == 0x70);
static_assert(offsetof(FEPackageBufferNode, mpBuffer) == 0x10 && offsetof(FEPackage, mpcFilename) == 0x38 && offsetof(FEPackage, mList9C) == 0x9C && sizeof(FEPackage) == 0xB0);
#endif
