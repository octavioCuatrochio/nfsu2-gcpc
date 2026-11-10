// Bridge: everything rewritten game code needs from parts of the game that are NOT rewritten yet.
//
// Rewritten code calls only these functions for such things, never addresses or platform APIs directly.
// Implementations:
//   host/win32-hook/bridge_win32.cpp  -> calls the original SPEED2.EXE functions (hooked build)
//   tests/bridge_stub.cpp             -> simple stand-ins for native unit tests
//   (later) the native port          -> rewritten implementations
// When the code behind an entry gets rewritten, the entry is removed and callers use the rewrite directly.
#pragma once
#include <cstdint>

struct FEMultiPool;   // frontend/FEMultiPool.h
struct FEMinList;     // frontend/FEMinList.h
struct FEPackage;     // frontend/FEPackage.h
struct cFEng;         // frontend/cFEng.h
struct SlotPoolManager;   // bWare/bSlotPool.h
struct SlotPool;
struct FEPackageManager;   // frontend/FEPackageManager.h
struct FEObject;

struct bASinTableEntry;
struct FESortGlobals;
namespace bridge {

// --- memory: buffers are shared with original game code, so they must come from the game's heap ---
void *Alloc(uint32_t bytes);           // PC 0x575620 -> CRT operator new (NOT zeroed)
void FreeArray(void *p);               // PC 0x575630 -> CRT operator delete[] (null is fine)

// --- frontend text ---
// Global character histogram used to unpack the game's compressed (packed) 8-bit strings into wide
// strings. Null until language data is loaded.            PC: pWideCharHistogram @ 0x8383DC
void *WideCharHistogram();
// WideCharHistogram::UnpackString(dst, dstBytes, packed)  PC 0x50D3C0 (__thiscall, ret 0xC)
void WideCharHistogram_UnpackString(void *histogram, uint16_t *dst, int dstBytes, const char *packed);
// FEng global UseOldFEngDirtyFlag: when set, string changes always mark objects dirty.  PC @ 0x8384D4
bool UseOldFEngDirtyFlag();

// --- FEng objects not rewritten yet ---
void *FEStringVTable();                                                // PC 0x78F53C (hooked: the game's vtable)
// FEString::pLabelCallback (PC 0x838478): if set, calls its vtable[0](callback, string)
void CallLabelCallback(void *feString);

void *FEMinNodeVTable();                                               // PC 0x79A9E8
void *FEMinListVTable();                                               // PC 0x79A9EC
// delete an object through its own vtable: slot 0 = MSVC scalar deleting destructor, called with flag 1
void VirtualScalarDelete(void *object);
void VirtualScalarDtor(void *object, uint32_t flags);                 // vtable slot 0 with explicit flags

void *FEObjectVTable();                                                // PC 0x79B8E0
uint32_t NextFEObjectGUID();                                           // FEngine::SysGUID++ (PC 0x7F7CF8)
::FEMultiPool *ObjDataPool();                                            // global FEMultiPool, PC 0x863770
void *FESlotNodeVTable();                                              // PC 0x79B13C
void *FESlotPoolVTable();                                              // PC 0x79B8DC
// FEObject::pDestructorCallback (PC 0x838474): if set, calls its vtable[0](callback, object)
void CallDestructorCallback(void *object);

// object pools (FEPool.h): pool lists and block vtables
::FEMinList *FEMessageResponsePool();                             // PC 0x8637A0
::FEMinList *FEScriptPool();                                      // PC 0x839C30
void *FEMessageResponseBlockVTable();                                  // PC 0x79BF50 (slot 0 = 53EB20)
void *FEScriptBlockVTable();                                           // PC 0x79BF54 (slot 0 = 53EC20)
void *FEScriptVTable();                                                // PC 0x79AD28 (slot 0 = 53EE30)
void *FEKeyNodeVTable();                                               // PC 0x79AA34
::FEMinList *FEKeyNodePool();                                          // PC 0x839BD0
void *FEKeyNodeBlockVTable();                                          // PC 0x79B9EC (slot 0 = 531BC0)
void *bWareMalloc(uint32_t size, const char *name, int32_t line, uint32_t flags);   // PC 0x440390 (__cdecl)
::SlotPoolManager *TheSlotPoolManager();                                   // PC 0x828530 (game global)
::SlotPool **BPNodeSlotPool();          // PC 0x8284F0 (game global)
int32_t *BPListWantToClose();          // PC 0x8284F4 (game global)
void cFEng_RegisterJoy(::cFEng *fe);                                              // PC 0x52CDD0 (__thiscall)
void *RenderObjectDisconnectVTable();                                              // PC 0x79AB9C
void bWareFree(void *p);
int32_t FEngLoadCounter();                                                         // PC 0x8384D0 (game global)
int32_t *FEngLoadCounterPtr();                                                     // same global (GC gInsideScreenConstructor)
// FEPackageManager callees (game-side screens; PC __cdecl)
// FEPackageReader chunk readers not rewritten yet (PC __thiscall on the reader, bool in AL)
// FEGameInterface (engine +0x10C) vtable slot 0: package data by name; *freeBuffer -> caller frees *buffer
const void *FEGameInterface_GetPackageData(void *gi, const char *name, void **buffer, uint8_t *freeBuffer);
void FEGameInterface_LibraryLoaded(void *gi, ::FEPackage *pkg);   // vtable slot 3 (+0xC): library / package activated
void FEGameInterface_ReleaseResources(void *gi, ::FEPackage *pkg, uint32_t count, void *resources);   // slot 2 (+0x8)
bool FEGameInterface_PackageWillUnload(void *gi, ::FEPackage *pkg);   // slot 4 (+0x10): true -> delete it
bool FEGameInterface_CanUnloadLibrary(void *gi, ::FEPackage *pkg);    // slot 5 (+0x14)
uint32_t FEGameInterface_GetJoyPadBits(void *gi, uint32_t pad);   // +0x40 (one argument)
void FEGameInterface_GetMouseInfo(void *gi, void *info);              // +0x44 (8 bytes: x, y, wheel, buttons)
void FEGameInterface_BeginUpdate(void *gi);                           // +0x5C
void FEGameInterface_EndUpdate(void *gi);                             // +0x60
uint8_t FEGameInterface_GetContextTransform(void *gi, uint32_t context, float *matrix16);   // +0x2C (GC +0x64), bool in AL
uint8_t FEGameInterface_IsPointInObject(void *gi, float x, float y, ::FEObject *obj);   // +0x48 (PC; GC +0x9C), bool in AL
// game interface render slots (PC cFEng vtable 0x79BCD8)
void FEGameInterface_BeginPackageRendering(void *gi, ::FEPackage *pkg, uint32_t flag);       // +0x20
void FEGameInterface_EndPackageRendering(void *gi, ::FEPackage *pkg);                        // +0x24
void FEGameInterface_GenerateRenderContext(void *gi, uint32_t context, ::FEObject *group);   // +0x28
void FEGameInterface_RenderObjectList(void *gi, void *entries, uint32_t count);              // +0x30 ({obj, key} x count)
void FEGameInterface_GetViewTransformation(void *gi, void *matrix);                          // +0x3C (FEMatrix4)
::FESortGlobals *FEngSortGlobals();   // PC 0x838488
bool TextInputFocused();   // PC globals 0x83A9C0 (byte, FocusedTextbox active) && 0x83A9C4 (dword) both nonzero
void SetHoldDirtyFlags(uint32_t v);                                   // uHoldDirtyFlags, PC 0x7F7C8C
// constructors of object types not rewritten yet (types 1, 4, 5, 6, 7, 9, 10, 11; PC 532640/5349C0/52B080/52B9E0/52B0B0/534B90/534DB0/52B480)
::FEObject *FEObjectTypeConstruct(void *mem, uint32_t type);
::FEObject *FEObject_VirtualClone(::FEObject *obj, bool reference);       // vtable slot 1
::FEObject *FEGroup_CopyConstruct(void *mem, ::FEObject *src, bool a, bool b);   // PC 5548A0 (__thiscall, 0x6C bytes)
void FEKeyInterpScript(void *script, uint8_t track, uint32_t time, void *obj);   // PC 541F60 (__cdecl; GC FEKeyInterp 4 args)
// FEGameInterface vtable +0x4C (__thiscall): fill one code list box cell (virtual column, row)
void FEGameInterface_FillCodeListBoxCell(void *gi, void *listBox, int32_t column, int32_t row);
// FEGameInterface vtable +0x18 (__thiscall): message to the game (msg hash, object, arg, extra)
void FEGameInterface_Message(void *gi, uint32_t msg, void *obj, uint32_t arg, uint32_t extra);
void *FECodeListBoxVTable();                                       // PC 0x79B904 (slot 0 = 5352B0)
void *FECodeListBoxDefaultCallback();                              // FECodeListBox::mpDefaultCallback (PC *0x7F7CC4)
void *FEListBoxVTable();                                           // PC 0x79BAE4 (slot 0 = 541D90)
void *FEPackageVTable();                                           // PC 0x79B8F8 (slot 0 = 534C50)
void *FENodeVTable();                                              // PC 0x79AB0C
void *FEPackageList74VTable();                                     // PC 0x79B878 (FEPackage +0x74 list)
void *ResourceConnectorVTable();                                   // PC 0x79AB04 (game-side FEObjectCallback)
// FEGameInterface vtable slot 1 (__thiscall): load the package's resources        (GC slot +0x14)
bool FEGameInterface_LoadResources(void *gi, ::FEPackage *pkg, uint32_t count, void *resources);
void *FEPackageDataVTable();                                       // PC 0x79B15C (slot 0 = 52CC20)
void *ScreenFactory(uint32_t nameHash, ::FEPackage *pkg, int32_t arg);   // PC 0x50B790 (GC 8014A2CC)
uint32_t FindScreenCreateData(uint32_t nameHash);                  // PC 0x50B840 (GC 8014A3A8)
void HackClearCache(::FEPackage *pkg);                             // PC 0x50C550 (GC 8014DFBC)
void FEngine_Update(void *engine, uint32_t a, uint32_t b);         // PC 0x558C00 (__thiscall, GC 80146028)
bool IsCurrentlyHotChunking();                                     // PC 0x571090 (GC 80195514)
::SlotPool *LevelSorterSlots();                                                    // PC *(0x8384B0) (game global)
// MenuScreen::BaseNotify(msg, obj, arg, pkg): game-side screen notification            PC 0x55B5C0 (__thiscall)
void MenuScreen_BaseNotify(void *screen, uint32_t msg, void *obj, uint32_t arg, ::FEPackage *pkg);
void *bMalloc(uint32_t size, uint32_t flags);                                      // PC 0x440820 (__cdecl; GC bMalloc(int,int))
void LZDecompress(const uint8_t *src, uint8_t *dst);                               // PC 0x574670 (__cdecl)
::FEPackageManager *TheFEPackageManager();                                         // PC 0x8637B0 (game global)                                                       // PC 0x440540 (__cdecl)
uint16_t BaselineFovNoise();           // PC 0x7EC1F0 aBaselineFovNoise (game global)
uint32_t *BRandomSeed();              // PC 0x7EA624 (game global)
const uint8_t *BFastATanTable();      // PC 0x7EACB0 (game data, 257 bytes)
const uint16_t *BFixATanTable1();     // PC 0x7EB1C0 (game data, 129 entries: atan(i / 32))
const uint16_t *BFixATanTable2();     // PC 0x7EB2C8 (game data, 129 entries: atan(i / 4))
const ::bASinTableEntry *BASinTable();                            // PC 0x7EA628 (game data)
// FEPackage callees not rewritten yet (PC __thiscall)
// FEngine package loading (not rewritten; __thiscall)
void FEngine_UnloadPackage(void *engine, FEPackage *pkg);                                    // 5362E0 (GC 80145A94)
FEPackage *FEngine_PushPackage(void *engine, const char *name, uint8_t priority, uint32_t mask);   // 555860 (GC 80145CF8)
cFEng *cFEngInstance();                                 // cFEng::pInstance (PC 0x8384C4)
void *FEPackageCommandVTable();                                                   // PC 0x79AB64
void *FEMessageNodeVTable();                                                      // PC 0x79AB60
// game UI handler virtual calls (__thiscall), slot given as a byte offset
void FEUIData_Call4(void *ui, uint32_t slotOffset, uint32_t a, const void *b, uint32_t c, const void *d);
void FEUIData_Call5(void *ui, uint32_t slotOffset, uint32_t a, const void *b, const void *c, const void *d, uint32_t e);
void FEUIData_OnQueueMessage(void *ui, uint32_t msg, void *obj, void *pkg, void *target, uint32_t arg);   // vtable +0x54
void FEngine_SendMessageToGame(void *engine, uint32_t msg, void *obj, void *pkg);   // engine+0x10C vtable slot 6 (msg, obj, 0, pkg)
bool FEObjectCallback_Call(void *callback, void *obj);                               // vtable slot 1, __thiscall
void FEListBox_Update(void *listBox, float delta);                                                        // 517230
void FECodeListBox_Update(void *listBox, float delta);                                                    // 504DC0
uint32_t HoldDirtyFlags();                                             // uHoldDirtyFlags, PC 0x7F7C8C
void *FERefListVTable();                                               // PC 0x79AA30
void *FEMessageResponseVTable();                                       // PC 0x79AA38

}  // namespace bridge
