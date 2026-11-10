// FEng engine object. Original: zFe2.cpp. Only the fields the rewritten members use (PC offsets).
#pragma once
#include "FETypeLib.h"
#include <cstddef>
#include <cstdint>
#include "FEMinList.h"
#include "FENode.h"
struct FEObject;
struct FEPackage;
struct FEMessageResponse;
struct FEObjectCallback;

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


// Pad state (0x88): button bits now / before and how long (in update ticks) each held button has been down.
struct FEJoyPad {
    uint32_t mu32Previous;           // +0x00
    uint32_t mu32Current;            // +0x04
    uint32_t mHoldTime[32];          // +0x08

    void Update(uint32_t bits, uint32_t delta);   // GC FEJoyPad::Update / PC 504FA0 (ret 8)
    FEJoyPad *Construct();                        // GC 8013FF9C (Reset 8013FFCC inlined) / PC 519AE0: all zero
    // GC 8014004C.. (PC: inlined): all of `mask` down now and not all before / all down now and before / not all
    // down now but all before; HeldFor (PC 504FF0, ret 4) = shortest hold time over the bits (0xFFFFFFFF if none).
    bool WasPressed(uint32_t m) const { return (mu32Current & m) == m && (mu32Previous & m) != m; }
    bool WasHeld(uint32_t m) const { return (mu32Current & m) == m && (mu32Previous & m) == m; }
    bool WasReleased(uint32_t m) const { return (mu32Current & m) != m && (mu32Previous & m) == m; }
    uint32_t HeldFor(uint32_t mask) const;
    void Reset() { *this = FEJoyPad{}; }            // GC 8013FFCC (PC: inlined in the ctor)
    // GC 8014012C (PC: not found, inlined or unused): every bit in `mask` loses `amount` hold time (floored at 0).
    void DecrementHold(uint32_t mask, uint32_t amount);
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
    void Reset() { *this = FEMouse{}; }             // GC 80145354 / 80145384 (PC: inlined in the FEngine ctor)
    bool WasPressed(uint16_t m) const { return (mu16Current & m) == m && (mu16Previous & m) != m; }    // GC 80145454
    bool WasHeld(uint16_t m) const { return (mu16Current & m) == m && (mu16Previous & m) == m; }       // GC 80145488
    bool WasReleased(uint16_t m) const { return (mu16Current & m) != m && (mu16Previous & m) == m; }   // GC 80145500
    // GC 801454B8 / PC 50B150 (ret 4): shortest hold time over the set bits (0xFFFFFFFF if none). Both loop over 32
    // words from +0x10 (past the 3 real ones); the mask is 16-bit, so only words 0..15 can be read.
    uint32_t HeldFor(uint16_t mask) const;
};

// Pad message hashes (PC data 0x7F7D20 pressed[19], 0x7F7D6C held[2], 0x7F7D78 released[19]; GC PadButtonHash...).
extern const uint32_t kFEPadPressedHash[19];
extern const uint32_t kFEPadHeldHash[2];
extern const uint32_t kFEPadReleasedHash[19];

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
    FEObject *mPressedButton[19];    // +0x84 button that got the press message per pad button (for the release)
    uint32_t mu32ButtonMapArg;       // +0xD0 FEButtonWrapMode for FEButtonMap::GetButtonFrom (bit 1 X wrap, bit 2 Y wrap)
    uint32_t mu32NumJoyPads;         // +0xD4
    uint16_t mu16RenderContextCount; // +0xD8 last render context id handed out (Render resets per package)
    uint8_t _unknown0DA[2];
    // Active packages: FEMinList at +0xDC (vtable, count, head, tail); kept as fields because code uses the head.
    void *mpActiveListVTable;        // +0xDC
    uint32_t mu32NumPackages;        // +0xE0
    FEPackage *mpPackages;           // +0xE4 first active package (packages are linked through their FEMinNode)
    FEPackage *mpPackagesTail;       // +0xE8
    FEMinList mIdlePackages;         // +0xEC loaded but inactive packages (cFEng::SetUseIdleList)
    FEMinList mLibraries;            // +0xFC library packages (FEngine::AddToLibraryList; refcount in FEPackage +0x18)
    void *mpUIData;                  // +0x10C game-side handler (vtable: +0x18 SendMessageToGame, +0x54 message hook)
    // +0x110 GC FEObjectSorter<1024> (GC +0x120): objects of the package being rendered, sorted by view depth
    struct SortEntry { FEObject *mpObject; uint32_t mu32Key; };   // key: float bits of view z (> 0)
    struct ObjectSorter {
        uint32_t mu32Count;          // +0x110
        SortEntry mEntries[1024];    // +0x114 (no bounds check, like the original)
        SortEntry mScratch[1024];    // +0x2114
        void SortObjects();          // GC 8030C344 / PC 5114F0: stable LSD radix sort, 4 byte passes, ascending
    } mSorter;
    FEMinList mMessages;             // +0x4114 queued messages
    FEMinList mPackageCommands;      // +0x4124 queued package stack operations (FEPackageCommand)
    FETypeLib mTypeLib;              // +0x4134 object types and their key-track fields (0x14 bytes)
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
    // GC 80145FA8 / PC 52C570 (ret 8): object by name hash in the named active package, or the first active one
    // that has it.
    FEObject *FindObject(uint32_t nameHash, const char *package);
    // GC 80146304 / PC 50B2A0 (__cdecl): pad pressed / held / released message hashes, plus two mouse ones.
    static bool IsInputMessage(uint32_t msg);
    // GC 801463A4 / PC 54D4B0 (ret 4): pad buttons 4..18 -> pressed / held (7, 8) / released messages to the current
    // button or the package; directions (table 0x7F7CFC, diagonals first) with repeat delays 320 / 120 ticks ->
    // direction messages and button navigation (FEButtonMap::GetButtonFrom).
    void ProcessPadsForPackage(FEPackage *pkg);
    // GC 80146FA0 / PC 54E020 (ret 4): mouse buttons 0 (select: mPressedButton[4]) / 1 (0x14DD0705) -> messages to
    // the hit current button or the package (PC: package flags +0xAC / +0xAD gate those); after a move, the first hit
    // selectable button becomes current (PC: not while a text box has focus unless package +0xAF).
    void ProcessMouseForPackage(FEPackage *pkg);
    // Rendering (FERender.cpp). Render: GC 80147540 / PC 536520 (no args): re-sorts the active list by priority,
    // then per package: object sort, Begin/RenderObjectList/End through the game interface.
    void Render();
    // GC 80145530 / PC 536170 (returns this): input state, lists, flags (running, inputs, reader flags on), marker
    // and pressed-button tables cleared, then mTypeLib.Startup(). Leaves mpUIData, joypad hold data, the sorter
    // entries and the +0x34..+0x83 release state untouched (like the original).
    static FEngine *Construct(void *mem);
    // GC 80145724 / PC 51A640: deletes active packages (virtual), purges idle, libraries, messages; frees the pads;
    // clears input/marker/last-button state; purges the type library. (PC: package commands are not purged.)
    void ReleaseAll();
    // GC 801458BC / PC 51A740 (ret 4, byte arg): frees the pads, allocates and resets `count` new ones (count 0: the
    // freed pointer is left in mpJoyPads, like the original), clears the release times.
    void SetNumJoyPads(uint8_t count);
    void Destruct();   // GC 80145680 / PC 52C430: ReleaseAll, then each list (types .. active) reset + purged
    void RenderObject(FEObject *obj, const struct FEMatrix4 *m, uint16_t ctx);   // GC 801478F8 / PC 51A970 (ret 0xC)
    // GC 80147728 / PC 52C5D0 (ret 0x10): group transform (rotation about pivot, then position) * parent; children
    // get a new render context id (game interface +0x28 GenerateRenderContext).
    void RenderGroup(struct FEGroup *group, const struct FEMatrix4 *parent, const struct FEMatrix4 *view, uint16_t ctx);
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
    // GC 80148D9C (PC: inlined into ProcessResponses): pops the newest marker; null if none.
    const char *RecallPackageMarker();
    // GC 80145958 (PC: inlined into the cFEng ctor 5372E0): running flag, copied to every active package.
    void SetExecution(bool on);
    // GC 80145980 / PC 51A7E0: objects back to their initial state (callback 0x79AAEC), pads reset, mouse reset
    // when enabled.
    void SetInitialState();
    // GC 801479F4 (PC: inlined into 51A7E0): every active package until a callback returns false.
    bool ForAllObjects(FEObjectCallback *cb);
    FEPackage *FindIdlePackage(const char *name);   // GC 80145ECC (PC: inlined into PushPackage / FEngFindPackage)
    FEPackage *GetFirstLibrary() const { return reinterpret_cast<FEPackage *>(mLibraries.head); }   // GC 80145EF0
    // GC 80147B20 (PC: inlined into FEPackage::IssueScriptMessages 534C70): game interface (mpUIData) slot 6 with
    // arg 0 (GC passes the event arg).
    void SendMessageToGame(uint32_t msg, FEObject *obj, FEPackage *pkg);
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEMessageNode) == 0x20 && sizeof(FEPackageCommand) == 0x20 && offsetof(FEPackageCommand, mpPackage) == 0x1C);
static_assert(offsetof(FEngine, mPackageCommands) == 0x4124 && offsetof(FEngine, mi32LastButtonIndex) == 0x514C);
static_assert(offsetof(FEngine, mLastButtons) == 0x5150);
static_assert(offsetof(FEngine, mpPackages) == 0xE4 && offsetof(FEngine, mpUIData) == 0x10C && offsetof(FEngine, mMessages) == 0x4114);
static_assert(offsetof(FEngine, mSorter) == 0x110 && offsetof(FEngine, mu16RenderContextCount) == 0xD8);
static_assert(offsetof(FEngine, mMouse) == 0xC && offsetof(FEngine, mReleaseTime) == 0x38 && offsetof(FEngine, mu32NumJoyPads) == 0xD4);
static_assert(sizeof(FEJoyPad) == 0x88 && sizeof(FEMouse) == 0x20 && offsetof(FEMouse, mbMoved) == 0x1D);
static_assert(offsetof(FEngine, mIdlePackages) == 0xEC && offsetof(FEngine, mpActiveListVTable) == 0xDC);
static_assert(offsetof(FEngine, mbForwardQueuedMessages) == 0x5251 && offsetof(FEngine, mu32NumPackageMarkers) == 0x4148);
static_assert(offsetof(FEngine, mTypeLib) == 0x4134 && sizeof(FETypeLib) == 0x14);
#endif
