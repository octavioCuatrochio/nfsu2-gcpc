// GC UIMagazineSelect / MagazineThing / MagazineViewer / SCMagazineDatabase (zFe2.cpp): the showcase magazine / DVD
// reward browser (UI_MagazineSelect.fng). PC source files UIMagazineSelect.c, MagazineSelectFNGObject.c,
// MagazineThing.c, MagazineViewer.c, SCMagazineDatabase.c (+ their _global.c pieces).
// Game data: showcase records (0x40 each) at *(0x83840C), count *(0x838408): +0x20 unlock key, +0x24 byte kind
// (1 / 3 magazine, 2 / 4 DVD), +0x28 setup hash. Unlocks at 0x862840 (8 bytes: int key, +6 byte unlocked), count
// *(0x86283C). *0x838578 = viewer mode (0 none, 1 magazines, 2 DVDs), 0x837CA0 = package to return to (0x80).
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"

// GC SCMagazineDatabase (PC: methods only, the data is a loaded block): +0 car setups, +4 render setups, +8 magazine
// setups (counts); arrays at +0xC (0x2C each), +0xB0C (0xC each), +0xC8C (0x38 each); each record starts with its hash.
struct SCMagazineDatabase {
    uint32_t mNumCarSetups, mNumRenderSetups, mNumMagazineSetups;   // +0x00
    uint32_t GetNumMagazineSetups() const { return mNumMagazineSetups; }   // PC 430660 (folded: not hooked)
    void *GetCarSetupByHash(uint32_t hash);        // PC 5107D0 (ret 4)
    void *GetRenderSetupByHash(uint32_t hash);     // PC 510800 (ret 4)
    void *GetMagazineSetupByHash(uint32_t hash);   // PC 510840 (ret 4)
    void *GetMagazineSetupByIndex(uint32_t i);     // PC 510880 (ret 4, no bounds check)
};

namespace MagazineViewer {
const uint8_t *ShowcaseRecord(int32_t index);   // PC inlined: null outside [0, count)
void GetImageNameFromShowcaseDataIndex(int32_t index, char *buf, int32_t len);   // PC 510B80 (__cdecl)
void GetImageNameFromFileName(const char *file, char *buf, int32_t len);        // PC 523CB0 (__cdecl)
const char *MakeProfileImageName(int32_t index);   // PC 510AF0 (__cdecl): profile name padded "_" to 7 + index char
const char *GetCurrentImageName();                 // PC 523C70 "SetMagSetupHash": record with hash *0x837FD8 -> 510AF0
int32_t GetCurrentMode();                          // PC 510C30
uint32_t GetMagSetupHash();                        // PC 510C40: *0x837FD8
void EnterIngameMagazineReward();                  // PC 510C50: mode 1
void EnterIngameDvdReward();                       // PC 510C60: mode 2
void ReturnFromIngameReward();                     // PC 510C70: mode 0
void EnterFrontendMagazineReward(const char *from);   // PC 53DD30: mode 1, remember `from`, -> UI_MagazineReward.fng
void EnterFrontendDvdReward(const char *from);        // PC 53DD70: mode 2, same
void EnterFrontendMagazineSelect(const char *from);   // PC 53DDB0: mode 1, -> UI_MagazineSelect.fng (no null check)
void EnterFrontendDvdSelect(const char *from);        // PC 53DDF0: mode 2, same
void ReturnToFrontend(const char *pkg);               // PC 53DE30: mode 0, back to the remembered package
}

// PC 0x4C, vtable 0x79B840: IconOption + the showcase record index; React 53DE60.
struct MagazineThing {
    IconOption base;
    int32_t mIndex;                  // +0x48
    void React(const char *pkg, uint32_t msg);   // accept: *0x837FD8 = record hash, -> *(0x7FA2A0) (the viewer)
};

// PC 0x158, vtable 0x79CE04: 0 557FF0, 1 40D080, 2 NotificationMessage 54B210, 3 570A70, 4 RefreshHeader 510E40
// (= IconScrollerMenu's), 5 Setup 550DD0, 6 AddOption 50DAB0.
struct UIMagazineSelect {
    IconScrollerMenu base;           // +0x000
    uint32_t mScreenShot;            // +0x150 texture handler flags of the loaded screen shot
    uint8_t mbScreenShotLoaded;      // +0x154
    uint8_t _unknown155;             // +0x155
    uint8_t _pad156[2];

    UIMagazineSelect *Construct(const ScreenConstructorData *data);   // GC 80187438 / PC 557F20 (ret 4)
    void Destruct();                                                // GC 8018D83C / PC 558010
    UIMagazineSelect *ScalarDeletingDestructor(uint32_t f);         // PC 557FF0 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 80187784 / PC 54B210
    void Setup();                                                   // GC 80187494 / PC 550DD0
    void BuildShowcaseList();                                       // GC 80187560 / PC 523F60
    void PopulateDataInfoStuff();                                   // GC 80187684 / PC 54B150
    void RefreshHeader() { base.RefreshHeader(); }                  // GC 80187910 / PC 510E40 (jmp 50DA50)
    void DeallocateScreenShot();                                    // GC 801878C0 / PC 510E10
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(MagazineThing) == 0x4C && sizeof(UIMagazineSelect) == 0x158 &&
              offsetof(UIMagazineSelect, mbScreenShotLoaded) == 0x154);
#endif
