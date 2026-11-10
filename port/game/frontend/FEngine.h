// FEng engine object. Original: zFe2.cpp. Only the fields the rewritten members use (PC offsets).
#pragma once
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
#include "FENode.h"
struct FEObject;
struct FEPackage;
struct FEMessageResponse;

// Queued message (0x20 bytes, game heap). GC FEMessageNode.
struct FEMessageNode {
    FEMinNode node;                  // +0x00 vtable PC 0x79AB60
    FEObject *mpObject;              // +0x0C sender object
    FEObject *mpTarget;              // +0x10 target object or special value (-4/-5/-6)
    FEPackage *mpPackage;            // +0x14
    uint32_t mu32MsgId;              // +0x18
    uint32_t mu32Arg;                // +0x1C
};

// Queued package stack operation (0x20 bytes, game heap). GC FEPackageCommand; PC vtable 0x79AB64 (slot 0 51ABE0).
// Types: 1 pop, 2 push, 3 switch, 4 / 8 user transfer (bool false / true), 0x12 push for modal.
struct FEPackageCommand {
    FENode node;                     // +0x00 name = package to load (not set for pop / user transfer)
    uint32_t mu32Type;               // +0x14
    uint32_t mu32Mask;               // +0x18 control mask argument (meaning depends on the type)
    FEPackage *mpPackage;            // +0x1C package that queued it
};

// Type library entries (FETypeLib; PC 505480 finds a type, 5147F0 a parameter by index).
struct FETypeParam {
    FEMinNode node;
    uint32_t _unknown0C[2];
    uint32_t mu32ParamType;          // +0x14 (low byte used) FEKeyTrack::mu8ParamType
    uint32_t mu32ValueSize;          // +0x18 (low byte used)
    uint32_t mu32Offset;             // +0x1C byte offset in FEObject::mpData
};
struct FETypeNode {
    FEMinNode node;
    uint32_t _unknown0C[2];
    FEMinList mParams;               // +0x14
    uint32_t mu32Type;               // +0x24 FEObject factory type
};

// Pad state (0x88): button bits now / before and how long (in update ticks) each held button has been down.
struct FEJoyPad {
    uint32_t mu32Previous;           // +0x00
    uint32_t mu32Current;            // +0x04
    uint32_t mHoldTime[32];          // +0x08

    void Update(uint32_t bits, uint32_t delta);   // GC FEJoyPad::Update / PC 504FA0 (ret 8)
};
// Mouse state (FEngine +0x0C, 0x20 bytes).
struct FEMouse {
    int32_t mi32X, mi32Y, mi32Wheel; // +0x00
    uint16_t mu16Previous;           // +0x0C buttons
    uint16_t mu16Current;            // +0x0E
    uint32_t mHoldTime[3];           // +0x10
    uint8_t _unknown1C;
    uint8_t mbMoved;                 // +0x1D x or y changed in the last Update
    uint8_t _pad1E[2];

    struct Info { int16_t x, y, wheel; uint16_t buttons; };   // game interface +0x44 output
    void Update(const Info &info, uint32_t delta);  // GC FEMouse::Update / PC 50B0C0 (ret 8)
};

struct FEngine {
    uint8_t _unknown000;             // +0x00 FEng running (messages, packed updates); copied to FEPackage::mbFastInterp
    uint8_t mbFlag01;                // +0x01 input enabled ("Enabling/Disabling inputs"); set by 0x59BED120 / 0x7E4D1288
    uint8_t mbMouseEnabled;          // +0x02
    uint8_t mbFlag03, mbFlag04;      // +0x03 / +0x04 FEPackageReader::Load flags
    uint8_t _unknown005[3];
    FEJoyPad *mpJoyPads;             // +0x08 mu32NumJoyPads entries
    FEMouse mMouse;                  // +0x0C
    uint32_t _unknown02C;            // +0x2C = +0x30 after input processing
    uint32_t _unknown030;
    uint32_t mu32ButtonsToRelease;   // +0x34 bit k: subtract mReleaseTime[k] from every pad's hold time k
    uint32_t mReleaseTime[19];       // +0x38
    uint8_t _unknown084[0xD4 - 0x84];
    uint32_t mu32NumJoyPads;         // +0xD4
    uint8_t _unknown0D8[4];
    // Active packages: FEMinList at +0xDC (vtable, count, head, tail); kept as fields because code uses the head.
    void *mpActiveListVTable;        // +0xDC
    uint32_t mu32NumPackages;        // +0xE0
    FEPackage *mpPackages;           // +0xE4 first active package (packages are linked through their FEMinNode)
    FEPackage *mpPackagesTail;       // +0xE8
    FEMinList mIdlePackages;         // +0xEC loaded but inactive packages (cFEng::SetUseIdleList)
    FEMinList mLibraries;            // +0xFC library packages (FEngine::AddToLibraryList; refcount in FEPackage +0x18)
    void *mpUIData;                  // +0x10C game-side handler (vtable: +0x18 SendMessageToGame, +0x54 message hook)
    uint8_t _unknown110[0x4114 - 0x110];
    FEMinList mMessages;             // +0x4114 queued messages
    FEMinList mPackageCommands;      // +0x4124 queued package stack operations (FEPackageCommand)
    FEMinList mTypes;                // +0x4134 FETypeNode list (object types and their key-track parameters)
    uint8_t _unknown4144[4];
    uint32_t mu32NumPackageMarkers;  // +0x4148
    char macPackageMarkers[16][0x100];  // +0x414C stack of package names (RecordPackageMarker)
    int32_t mi32LastButtonIndex;     // +0x514C next slot of mLastButtons (ring, 0..31)
    struct LastButton { uint32_t mu32PackageHash, mu32ButtonGUID; } mLastButtons[32];   // +0x5150
    uint8_t _unknown5250;
    uint8_t mbForwardQueuedMessages; // +0x5251 also hand every queued message to mpUIData (vtable +0x54)

    // GC 80147A54 / PC 51AA20. Like the original, allocation failure crashes (writes through null).
    void QueueMessage(uint32_t msg, FEObject *obj, FEPackage *pkg, FEObject *target, uint32_t arg);
    // GC 80147F50 / PC 541FC0: deliver and delete every queued message (see the target switch in the .cpp).
    void ProcessMessageQueue();
    void ProcessGlobalMessage(FEPackage *pkg, uint32_t msg, uint32_t arg);   // GC 801484AC (PC: inlined)
    // GC 8014840C / PC 5367E0: list boxes first try the scroll messages, then the object's own response.
    void ProcessObjectMessage(FEObject *obj, FEPackage *pkg, uint32_t msg, uint32_t arg);
    // GC 80148504 / PC 52C7C0: run a response list (opcodes: see the .cpp).
    void ProcessResponses(FEMessageResponse *resp, FEObject *obj, FEPackage *pkg, uint32_t arg);
    bool ProcessListBoxResponses(FEObject *listBox, FEPackage *pkg, uint32_t msg);       // GC 801482B4 / PC 51B000
    // Package stack queue (GC 80147B64.. / PC 51AB20..). Each appends an FEPackageCommand to mPackageCommands and
    // adjusts control masks (PC: the FEPackage mask setters are inlined).
    void QueuePackageSwitch(const char *name, FEPackage *pkg);                       // GC 80147B64 / PC 51AB20
    void QueuePackagePush(const char *name, FEPackage *pkg, uint32_t mask);          // GC 80147C00 / PC 51AC10
    FEMinList &ActiveList() { return *reinterpret_cast<FEMinList *>(&mpActiveListVTable); }   // +0xDC
    // GC 80145CF8 / PC 555860 (ret 0xC): an idle package (by name, then by its base name) is reactivated (objects
    // reset to script 0x1744B3), else the game supplies the data and it is loaded; then priority, control mask,
    // fast-interp flag (engine +0), game notified (interface +0xC) and inserted by priority. Null if unavailable.
    FEPackage *PushPackage(const char *name, uint8_t priority, uint32_t mask);
    // GC 80145A94 / PC 5362E0 (ret 4): false if not active. Removes it, drops its queued commands and messages
    // (sent by it or targeting one of its objects); idle-capable packages (+0x15) go to the idle list, others
    // release their libraries and resources and are deleted (if the game agrees).
    bool UnloadPackage(FEPackage *pkg);
    void AddToIdleList(FEPackage *pkg);              // GC 80145EA0 / PC 51A8B0 (tail of mIdlePackages)
    void RemoveFromLibraryList(FEPackage *pkg);      // GC 80145F24 / PC 51A910
    void UnloadLibraryPackage(FEPackage *pkg);       // GC 80145C30 / PC 52C500
    void AddPackageByPriority(FEPackage *pkg);       // PC 518280 (GC: inlined): after the last with priority <= pkg's
    // GC 80146028 / PC 558C00 (ret 8): game pre/post hooks (+0x5251), input (pads, mouse, per-package processing,
    // hold-time releases), then object updates in steps of at most 20 (unless `wholeStep`) with message and
    // package-command processing between steps; disabled engines only update the objects.
    void Update(uint32_t delta, uint32_t wholeStep);
    void QueuePackagePushForModal(const char *name, FEPackage *pkg, uint32_t mask);  // GC 80147CA0 / PC 51ACE0
    void QueuePackagePop(FEPackage *pkg);                                            // GC 80147D50 / PC 51ADB0
    void QueuePackageUserTransfer(FEPackage *pkg, bool flag, uint32_t mask);         // GC 80147EC4 / PC 51AF50
    // GC 80147DF0 / PC 51AE80: unqueue the newest push command (type bit 1) named `name` and restore every package's
    // masks. The node is unlinked but NOT freed (leaks, as in the original). Null name -> false.
    bool PackagePopQueuedPushPackage(const char *name);
    // GC 801489A0 / PC 5559C0: run and delete every queued FEPackageCommand (see the .cpp).
    void ProcessPackageCommands();
    // Package markers / last buttons. Only caller on PC: ProcessResponses (RecallPackageMarker inlined there).
    bool RecordPackageMarker(const char *name);                      // GC 80148D4C / PC 51B070: false when 16 are stored
    void ClearPackageMarkers();                                      // GC 80148DCC / PC 50B390
    void RecordLastPackageButton(uint32_t pkgHash, uint32_t guid);   // GC 80148CAC / PC 50B300: replaces older entries
    uint32_t RecallLastPackageButton(uint32_t pkgHash) const;        // GC 80148D1C / PC 50B360: 0 if none
    bool ProcessCodeListBoxResponses(FEObject *listBox, FEPackage *pkg, uint32_t msg);   // GC 80148360 / PC 52C750
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEMessageNode) == 0x20 && sizeof(FEPackageCommand) == 0x20 && offsetof(FEPackageCommand, mpPackage) == 0x1C);
static_assert(offsetof(FEngine, mPackageCommands) == 0x4124 && offsetof(FEngine, mi32LastButtonIndex) == 0x514C);
static_assert(offsetof(FEngine, mLastButtons) == 0x5150);
static_assert(offsetof(FEngine, mpPackages) == 0xE4 && offsetof(FEngine, mpUIData) == 0x10C && offsetof(FEngine, mMessages) == 0x4114);
static_assert(offsetof(FEngine, mMouse) == 0xC && offsetof(FEngine, mReleaseTime) == 0x38 && offsetof(FEngine, mu32NumJoyPads) == 0xD4);
static_assert(sizeof(FEJoyPad) == 0x88 && sizeof(FEMouse) == 0x20 && offsetof(FEMouse, mbMoved) == 0x1D);
static_assert(offsetof(FEngine, mIdlePackages) == 0xEC && offsetof(FEngine, mpActiveListVTable) == 0xDC);
static_assert(offsetof(FEngine, mbForwardQueuedMessages) == 0x5251 && offsetof(FEngine, mu32NumPackageMarkers) == 0x4148);
#endif
