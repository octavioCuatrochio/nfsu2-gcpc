// Magazine group, batch 2 (whole PC files MagazineTextureHandler.c, SCMagazineSetup.c, FEMagazineSetup.c,
// MagazineBackFNGObject.c, MagazineViewFNGObject.c + their _global.c pieces).
// PC "UIMagazineBack.c" (4C4C90 / 4C4D30) is mislabeled: vtable 0x790758 is LoadingTips' (not magazine code).
// UIMagazineReward (565750.., RideInfo / garage car) is not in this batch.
#pragma once
#include <cstddef>
#include <cstdint>
#include "MenuScreen.h"

// PC 0x20, the instance at 0x863780 (MagazineBack reads +0x0C / +0x10 directly at 0x86378C / 0x863790).
struct MagazineTextureHandler {
    int32_t mPack;                   // +0x00 streaming pack index (names at *(0x7FA294 + i * 4))
    uint8_t mbAllocMemory;           // +0x04 TrackStreamer user memory "MagStreamTexureLock"
    uint8_t _pad05[3];
    void *mpMemory;                  // +0x08
    int32_t mbLoaded;                // +0x0C every requested texture has its info
    uint32_t mTextures[4];           // +0x10

    void Init(int32_t pack, bool allocMemory);     // PC 510940 (ret 8): load the pack, wait
    void TexturesLoaded();                          // PC 5109D0 (eLoadStreamingTexture callback via 523B30)
    void UnloadTextures();                          // PC 510A70
    void Close();                                   // PC 523B40: textures, pack, user memory
    void LoadTextures(uint32_t a, uint32_t b, uint32_t c, uint32_t d);   // PC 523BC0 (ret 0x10)
    // GC 801860AC ctor (PC: the global is zero-initialised .bss, no ctor): clears pack, flags and textures.
    void Construct() { mPack = 0; mbAllocMemory = 0; mbLoaded = 0; for (uint32_t &t : mTextures) t = 0; }
};
void MagazineTextureHandler_LoadedCallback(void *handler);   // PC 523B30 (__cdecl)

// GC SCMagazineSetup / FEMagazineSetup: random picks from small fixed hash arrays (bRandom 43C1C0; a count <= 1
// picks 0, the last fields of FEMagazineSetup and the car / background fields of SCMagazineSetup test == 0 only).
struct SCMagazineSetup {
    uint8_t data[0x40];
    void GenerateMagazineSelection(uint32_t *out) const;   // PC 510680 (ret 4): out[0..4]
    void CycleCarSetups(uint32_t *out) const;              // PC 510710 (ret 4): next car setup (out[3] index, out[2] hash)
};
struct FEMagazineSetup {
    uint8_t data[0x70];
    void GenerateMagazineSelection(uint32_t *out) const;   // PC 5105C0 (ret 4): out[0..5]
};

// PC 0x4C (MenuScreen only), vtable 0x79CB64: 0 553BC0, 1 40D080, 2 NotificationMessage 54B0E0, 3 570A70.
struct MagazineBackScreen {
    MenuScreen base;
    MagazineBackScreen *Construct(const ScreenConstructorData *data);   // PC 553B60 (ret 4)
    MagazineBackScreen *ScalarDeletingDestructor(uint32_t f);           // PC 553BC0 (ret 4): MenuScreen dtor
    void NotificationMessage(uint32_t msg);                             // PC 54B0E0 (ret 0x10): tick shows the back texture
};

// PC 0x54, vtable 0x79ACE8: 0 524100, 1 40D080, 2 NotificationMessage 565B50, 3 510EB0 (event 4 / 5 silenced).
// Screen shots go through the handler at 0x832DAC (streaming / memory card code, bridged).
struct MagazineViewScreen {
    MenuScreen base;
    uint32_t mScreenShot;            // +0x4C slot flags (1)
    uint8_t mbLoaded;                // +0x50
    uint8_t mbReady;                 // +0x51 memory card read finished (0x42317A19)
    uint8_t _pad52[2];

    MagazineViewScreen *Construct(const ScreenConstructorData *data);   // PC 553BE0 (ret 4)
    void Destruct();                                                    // PC 510E50
    MagazineViewScreen *ScalarDeletingDestructor(uint32_t f);           // PC 524100 (ret 4)
    void NotificationMessage(uint32_t msg);                             // PC 565B50 (ret 0x10)
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(MagazineTextureHandler) == 0x20 && offsetof(MagazineTextureHandler, mTextures) == 0x10);
static_assert(sizeof(MagazineBackScreen) == 0x4C && sizeof(MagazineViewScreen) == 0x54);
#endif
