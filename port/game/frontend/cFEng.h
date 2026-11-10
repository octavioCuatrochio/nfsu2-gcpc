// Game-side frontend manager (cFEng). Original: zFe2.cpp. Only the fields the rewritten members use (PC offsets).
#pragma once
#include <cstddef>
#include <cstdint>

// Level sorter entry (GC: bMalloc'd from LevelSorterSlots; linked in a circular bTList).
struct cFEngLevelSorter {
    cFEngLevelSorter *next;          // +0x00
    cFEngLevelSorter *prev;          // +0x04
    const char *mpcPackageName;      // +0x08
    int32_t mi32Level;               // +0x0C position in the list (ResyncSortingLevels)
    int32_t mi32State;               // +0x10 0/1: pop directly (Popper), else queue the pop
    int32_t mi32Arg;                 // +0x14 (Switcher's third argument)
    uint32_t mu32Flags;              // +0x18 (GC: 8 checked by Pusher)
};

struct FEngine;
struct FEObject;
struct FEPackage;

// PC 0x7F91C8 (GC MapJoyEventToFEPad): joystick event -> FEng pad bit, per-port state.
struct FEngJoyMapEntry {
    uint32_t mu32Event;              // +0x00 JoyEvent
    uint32_t mu32Mask;               // +0x04 FEJoyPad bit
    const char *mpcName;             // +0x08 "FEPad Up", ...
    uint32_t mState[4];              // +0x0C last value per port
};
constexpr int kFEngJoyMapCount = 21;

// GC FEResourceRequest (0x18; package "RsRq" chunk, given to the game interface).
struct FEResourceRequest {
    uint32_t _unknown00;             // +0x00
    const char *mpcName;             // +0x04 file name (path / extension stripped by LoadResources)
    uint32_t mu32Type;               // +0x08 4: the handle is a bMalloc'd copy of the name, else its bStringHash
    uint32_t _unknown0C;             // +0x0C
    uintptr_t mHandle;               // +0x10
    uint32_t _unknown14;             // +0x14 0 after loading
};
#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEResourceRequest) == 0x18, "FEResourceRequest");
#endif

// GC FEngIsPackagePushed / PC 52CF60 (__cdecl): in FEngine's active list (outside screen constructors), else
// registered with the package manager and loaded.
bool FEngIsPackagePushed(const char *name);
// FEObjectCallback functors (game vtables; slot 1 = Function, hooked):
bool FEObjectInitializer_Function(void *self, FEObject *obj);        // PC 517EE0 (79AAEC): "Init" script, dirty 0x3C00000
bool FEngMovieStarter_Function(void *self, FEObject *obj);           // GC 8014..., PC 566240 (79AB84): movies start
bool FEngMovieStopper_Function(void *self, FEObject *obj);           // PC 51C8A0 (79AB94): MovieStop for movies
bool RenderObjectDisconnect_Function(void *self, FEObject *obj);     // PC 5374F0 (79AB9C): drop the cached render
bool FEDirtyMaker_Function(void *self, FEObject *obj);               // PC 537630 (79ABF0): dirty + drop the cached render

// PC 50C250 (__cdecl; GC GetBaseName): `path` without directory ('\\' or '/') and extension (last '.' after index 0;
// a '.' before the last separator yields "").
void GetBaseName(char *out, const char *path);

struct cFEng {
    void *vtable;                    // +0x00 FEGameInterface (PC 0x79BCD8)
    uint8_t mbDrawingForeground;     // +0x04 set around FEngine::Render in DrawForeground (cleared by the ctor)
    uint8_t mbDrawingBackground;     // +0x05 set around DrawBackground
    uint8_t mbRendering;             // +0x06 set by BeginPackageRendering
    uint8_t _unknown07;
    FEngine *mpEngine;               // +0x08 (GC +0x10)
    void *mpRender;                  // +0x0C cFEngRender (GC +0x14)
    cFEngLevelSorter *mpSortHead;    // +0x10 (GC +0x18) list sentinel: {head, tail}; empty -> points at itself
    cFEngLevelSorter *mpSortTail;    // +0x14

    // GC 8014EC70 / PC 51CCD0 (__thiscall, ret 4): first entry whose name matches (bStrICmp); null if none.
    cFEngLevelSorter *FindInLevelSortingList(const char *name);
    // Joystick event registration (PC table 0x7F91C8: 21 x FEngJoyMapEntry; registered flag 0x8384B8).
    void RegisterJoy();                        // GC 8014E090 / PC 52CDD0: handlers for every mapped event + 0x34
    void UnRegisterJoy();                      // GC 8014E1BC / PC 50C680 (`this` unused)
    uint32_t GetJoyPadMask(uint8_t pad);       // GC 8014E240 / PC 50C6D0 (ret 4): mask bits of the events held on pad
    // GC 8014E13C / PC 50C630 (__cdecl handler): records `state` for `port` on every entry mapped to `event`.
    static void HandleJoy(uint32_t port, uint32_t event, uint8_t state, void *user);
    // GC 8014D75C / PC 55DD50 (ret 0x10): 0x05922615 disables / 0x7E4D1288 enables FEng input (FEngine +1),
    // anything else goes to FEPackageManager::NotificationMessage on the global manager.
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, FEPackage *pkg);
    // GC 8014E848 / PC 51CBA0 (ret 8): loaded `from` gets message 0x5ECA113D, then a queued switch to `to`.
    void SwitchFEngPackages(const char *from, const char *to);
    // GC 8014E9A4 / PC 51CC00 (ret 4): loaded -> message 0x90CA113D + queued pop; else unqueue a pending push.
    void QueuePopPackage(const char *name);
    // GC 8014EAE8 / PC 51CC60 (ret 0xC; this unused): last button with that name hash; enable (== 1) clears
    // FEObject flag 0x04000000, anything else sets it.
    // GC 8014ECD8 / PC 5376B0: number the entries in order and copy the number to each loaded package's priority.
    void ResyncSortingLevels();
    // GC 8014ED3C / PC 555D00 (ret 0x14): new sorter entry for `name`, placed by flags & 6 (0 front, 2 before
    // `parent` (front if missing), 4 after `parent` (tail if missing), 6 before the frontmost flag-8 entry (tail if
    // none)); flags & 0x11: state 2 + queued push (flag 1: normal, else modal) on cFEng::pInstance; otherwise
    // state 1 + immediate FEngine::PushPackage unless already active. Then renumber.
    void Pusher(uint32_t flags, const char *name, const char *parent, uint32_t mask, int32_t arg);
    // GC 8014EF3C / PC 5376F0 (ret 4): remove the entry (pop now when state is 0/1, else queue) and renumber.
    void Popper(const char *name);
    // GC 8014F044 / PC 5377A0 (ret 0xC): switch packages and replace the entry for `from` with a new one for `to`
    // (no null checks, like the original).
    void Switcher(const char *from, const char *to, int32_t arg);
    // GC 8014E8FC / PC 537580 (ret 4): loaded package: message 0xD9CA113D, disconnect every object from the
    // renderer (RenderObjectDisconnect callback, game vtable), then FEngine::UnloadPackage (bridge).
    void PopPackage(const char *name);
    // GC 8014EFD0 / PC 537750 (ret 4): PopPackage and drop the sorter entry (no renumbering).
    void ForceUnloadPackage(const char *name);
    static void SetButtonState(const char *pkgName, uint32_t buttonHash, bool enable);
    // GC 8014EA20 / PC 52CE40 (ret 8): the loaded package's button with that name hash (the last match) becomes
    // current (FEPackage::SetCurrentButton(engine, button, true)).
    void SetCurrentButton(const char *pkgName, uint32_t buttonHash);
    // GC 8014E798 / 8014E7F0, PC 51CB20 / 51CB60 (ret 0xC): parent looked up by name (null name -> no parent).
    void QueuePushPackage(const char *name, const char *parent, uint32_t mask);
    void QueuePushModalPackage(const char *name, const char *parent, uint32_t mask);
    // FEGameInterface callbacks (vtable 0x79BCD8).
    bool LoadResources(FEPackage *pkg, int32_t count, FEResourceRequest *req);     // GC 8014D644 / PC 50C300 (ret 0xC)
    bool UnloadResources(FEPackage *pkg, int32_t count, FEResourceRequest *req);   // GC 8014D700 / PC 50C3A0: frees type 4
    void EndPackageRendering(FEPackage *pkg);                     // GC 8014DA68 / PC 537480: cFEngRender::PackageFinished
    void GenerateRenderContext(uint32_t context, FEObject *group);   // GC 8014D82C / PC 51C660: cFEngRender (ret 8)
    void RenderObjectList(void *entries, uint32_t count);         // PC 50C2C0 (ret 8): RenderObject (virtual) last to first
    // GC 8014D928 / PC 51C670 (ret 4): visible (flag 1 clear) objects go to the render list while +0x06 is set.
    void RenderObject(FEObject *obj);
    void GetViewTransformation(void *matrix);                     // GC 8014D978 / PC 50C490: identity
    bool DoesPointTouchObject(float x, float y, FEObject *obj);   // GC 8014EC4C / PC 537690 (ret 0xC)
    void OutputWarning(const char *text, int32_t level);          // GC 8014E328 / PC 50C7C0: nothing (release build)
    void NotifySoundMessage(uint32_t a, uint32_t b, uint32_t c, uint32_t d);   // GC 8014D7FC / PC 52CDC0 -> manager
    // GC 8014E034 / PC 51C990 (ret 0xC): *flag = 0, then FEPackageManager::GetPackageData(name).
    uint8_t *GetPackageData(const char *name, uint32_t unused, uint8_t *flag);
    // GC 8014D99C / PC 51C690 (ret 8): rendering flag (+0x06) = not (disc error and not the error screen 0x942C98B5)
    // and (no FEngPleaseRenderSinglePackage or it is this one) and package visible and not (`magazine` and
    // UI_MAGAZINEBACK.FNG); then cFEngRender::PrepForPackage (inlined): clear every context's +0xC0 byte.
    void BeginPackageRendering(FEPackage *pkg, bool magazine);
    // GC 8014D850 / PC 50C3D0 (ret 8): identity, or the render context's matrix (context +0x10) when id != 0.
    bool GetContextTransform(uint16_t context, float *matrix);
    // GC 8014EC48 (empty) / PC 50C860 (ret 4): PC-only mouse: position relative to the 640x480 centre (_ftol),
    // buttons at +6 (swapped when the mouse is left-handed).
    void GetMouseInfo(void *info);
    // GC 8014D468 / PC 5372E0: interface vtable, empty sort list, FEngine (operator new 0x5254), LevelSorterSlots
    // pool (0x1C x 32), engine->mpUIData = this, 2 pads, objects initialised (51A7E0), engine flags 0 and 2 set (and
    // byte 0 copied to every package's +0x14), cFEngRender (new 0x3420).
    cFEng *Construct();
    // GC 8014D570 / PC 5373F0 (scalar 5425E0): pool, renderer, joystick handlers, FEngine, sorter entries.
    void Destroy();
    // GC 8014DA8C / PC 54E4A0 (ret 4): every object reset to its "Init" script (FEPackage::InitializePackage,
    // inlined); package +0x14 = 1; Update(engine, 0) unless +0x16; package data activated with the sorter entry's
    // argument; movies started; priority = the entry's level. No null check on the sorter entry (PC).
    void InitPackage(FEPackage *pkg);
    // GC 8014DF08 / PC 51C8C0 (ret 4): movies stopped, cached renders dropped, package data unactivated. True.
    bool PackageWillUnload(FEPackage *pkg);
    // GC 8014E6D8 / PC 537540: UI_MagazineBack.fng rendered (RenderPackageSpecial) when pushed.
    void DrawBackground();
    void DrawForeground();   // GC 8014E488 (PC: inlined in FEngRenderForeground 5378C0): flag +0x04 around Render
    // GC 8014E5C4 / PC 51CA50 (ret 8): own sort buffer (0x20C nodes), objects (groups skipped) through
    // RenderObjectSpecial with the view matrix, sorted, rendered through the interface's own vtable.
    void RenderPackageSpecial(FEPackage *pkg, uint32_t flag);
    // GC 8014E4C8 / PC 51C9B0 (ret 0x10): visible (data alpha != 0) objects at pivot + position through `matrix`;
    // render context set; z > 0 (or NaN) adds {obj, float bits of z} to the sorter.
    void RenderObjectSpecial(FEObject *obj, const float *matrix, uint16_t context, void *sorter);
    // GC 8014E354 / PC 5662C0: FEngine::Update(RealTimeElapsed * 1200, state 6 && WorldTimeElapsed > 0), joystick type
    // changes -> MakeLoadedPackagesDirty, memory card tick (ms), movies, chyron requests.
    void Service();
    // GC 8014EBD0 / PC 50C7E0: every object of every active package dirty (0x400000) and its cached render dropped.
    void MakeLoadedPackagesDirty();
    cFEngLevelSorter *SortSentinel() { return reinterpret_cast<cFEngLevelSorter *>(&mpSortHead); }
};

// FEng API (PC: most callers inline these; out-of-line copies listed)
void FEngStartInteraction(const char *name, int32_t arg);                 // GC 8014F360 / PC 555E80
void FEngShowPackageModal(const char *name, const char *parent, uint32_t mask, int32_t arg);      // GC 8014F550 / PC 555EB0
void FEngShowPackageTrueModal(const char *name, const char *parent, uint32_t mask, int32_t arg);  // GC 8014F598 / PC 555EE0
void FEngShowPackage(const char *name, const char *parent, uint32_t mask, int32_t arg);           // GC 8014F5E0 / PC 555F10
void FEngPushPackageSpecial(uint32_t flags, const char *name, const char *parent, uint32_t mask, int32_t arg);   // GC 8014F648 / PC 555F50
void FEngSetControlMask(const char *name, uint32_t mask);               // GC 8014F424 / PC 537950
void FEngSwitchPackages(const char *from, const char *to, int32_t arg); // GC 8014F4D8 / PC 537980
void FEngPopPackage(const char *name);                                  // GC 8014F51C / PC 5379A0

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(cFEng, mpEngine) == 8 && offsetof(cFEng, mpSortHead) == 0x10 && offsetof(cFEngLevelSorter, mpcPackageName) == 8);
#endif

// GC FEngRenderForeground 8014F23C / PC 5378C0 (__cdecl): with DrawFEng (PC 0x7F91C4) and cFEng: DrawForeground,
// CardiogramSystem::Draw and RenderMinimap (both the folded empty 4022C0 on PC), DrawFEGraphs (PC 510090); PC adds the
// mouse cursor (50B460 at the mouse position) when no movie plays and the cursor flag (byte 0x7FA29C) is set.
void FEngRenderForeground();
// GC FEngRenderBackground 8014F2A8 / PC 537930 (__cdecl): DrawBackground under the same conditions.
void FEngRenderBackground();
// PC 537DA0 (__cdecl, PC only; caller 4E4A5A): every active package pushed by `parent` (FEPackage +0x3C) has its own
// children popped first (recursion), then cFEng::Popper (inlined). The next link is read before the recursion.
void FEngPopChildPackages(const FEPackage *parent);
// PC 50C7B0 (ret 0xC; GC cFEng::SetCellData 8014E320, a static code-list-box callback): false.
bool cFEng_SetCellData(void *listBox, uint32_t a, uint32_t b);
