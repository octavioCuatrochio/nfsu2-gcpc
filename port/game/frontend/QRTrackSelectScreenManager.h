// GC QRTrackSelectScreenManager (zFe.cpp): the quick race screen chain mode select -> mode options / track select
// -> car select (online: lobby / game room). One instance (PC *0x836340) between the front and back screens.
// PC: mostly inlined; screens are 0x14-byte list nodes with next / prev screen functions (PC __thiscall on the
// manager, game addresses 4B2A80.. kept in the nodes so the game's callers still work).
#pragma once
#include <cstddef>
#include <cstdint>

struct QRScreenNode {                // PC 0x14 (GC: same idea)
    QRScreenNode *next, *prev;       // +0x00
    const char *mpName;              // +0x08
    void *mCalcNext;                 // +0x0C PC fn (__thiscall manager) -> next package name
    void *mCalcPrev;                 // +0x10
};

// PC 0x18, vtable 0x79771C (slot 0 scalar dtor 4EF260).
struct QRTrackSelectScreenManager {
    void *vtable;                    // +0x00
    QRScreenNode *mHead, *mTail;     // +0x04 bList of screens (sentinel = &mHead)
    QRScreenNode *mpCurrent;         // +0x0C (GC +0x08)
    const char *mpFront;             // +0x10 UI_Main.fng / PC_OL_Lobby.fng
    const char *mpBack;              // +0x14 UI_QRCarSelect.fng / PC_OL_GameRoom.fng

    static QRTrackSelectScreenManager *&Instance();                     // GC 800E0D58 (PC 0x836340)
    // GC 800E0FDC / PC 4E2900 (ret 8): mode select, mode options, track select nodes (each only once)
    QRTrackSelectScreenManager *Construct(const char *front, const char *back);
    void Destruct();                                                    // GC 800E1164 / PC 4E2A20
    QRTrackSelectScreenManager *ScalarDeletingDestructor(uint32_t f);   // PC 4EF260 (ret 4)
    QRScreenNode *FindScreen(const char *name);                         // GC 800E0F78 / PC 4B2960 (ret 4): bStrICmp
    void Start(const char *from, const char *to);                       // GC 800E0DAC / PC 4CCF30 (ret 8)
    void ChangeToNextScreen();                                          // GC 800E1248 / PC 4B29A0: back -> suicide
    // GC 800E1310 / PC 4B2A00: front -> (split screen + UI_Main.fng: FEDatabase RestorePreviousStable) suicide
    void ChangeToPrevScreen();
    bool IsBackdoor(const char *name) const;    // GC 800E11E0 (PC: inlined): bStrICmp with the back screen
    bool IsFrontdoor(const char *name) const;   // GC 800E1214 (PC: inlined): front screen
    static void CommitSuicide();                                        // GC 800E0D60 / PC 4B2940
    static void EnterMiddle();                                          // GC 800E0EEC / PC 4EF1B0
    static void EnterFront(const char *from);                           // GC 800E0DF8 (PC: inlined, e.g. 4FC050)
    static void EnterBack(const char *from);                            // GC 800E0E40 / PC 4FA470 (race mode -> screen)
    // next / prev screen functions (PC __thiscall on the manager)
    const char *CalcNextScreenFromModeSelect();     // GC 800E1408 / PC 4B2A80
    const char *CalcNextScreenFromModeOptions();    // GC 800E1510 / PC 498220
    const char *CalcNextScreenFromTrackSelect();    // GC 800E1540 / PC 498240 (PC: every case -> mode options)
    const char *CalcPrevScreenFromModeSelect();     // GC 800E15A0 / PC 498290
    const char *CalcPrevScreenFromModeOptions();    // GC 800E15A8 / PC 4B2AF0
    const char *CalcPrevScreenFromTrackSelect();    // GC 800E1650 / PC 4982B0
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(QRScreenNode) == 0x14 && sizeof(QRTrackSelectScreenManager) == 0x18 &&
              offsetof(QRTrackSelectScreenManager, mpCurrent) == 0xC && offsetof(QRTrackSelectScreenManager, mpBack) == 0x14);
#endif
