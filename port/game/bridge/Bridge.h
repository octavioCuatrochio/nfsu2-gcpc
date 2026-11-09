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

struct bASinTableEntry;
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
void bWareFree(void *p);
int32_t FEngLoadCounter();                                                         // PC 0x8384D0 (game global)
::SlotPool *LevelSorterSlots();                                                    // PC *(0x8384B0) (game global)
void cFEng_PopPackage(::cFEng *fe, const char *name);                              // PC 0x537580 (__thiscall)
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
void FECodeListBox_ScrollSelection(void *listBox, int32_t dx, int32_t dy);         // 52BD70
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
