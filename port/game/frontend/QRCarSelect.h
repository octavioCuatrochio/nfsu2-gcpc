// GC UIQRCarSelect (zFe.cpp): quick race car select (UI_QRCarSelect.fng): the player's car stable filtered by
// type (stock / tuned / career / ...), 3 rating sliders, the 3D garage view (CarViewer). PC: also the online / LAN
// car select (game modes 8 / 0x100 / 0x200: network paths stay the game's).
#pragma once
#include <cstddef>
#include <cstdint>
#include "MenuScreen.h"
#include "FEScrollBar.h"
#include "FEWidget.h"

// PC 0x1B8, vtable 0x79770C (0 scalar dtor 4EEB70, 2 NotificationMessage 4FC1F0, 3 NotifySoundMessage 4E2280).
struct UIQRCarSelect {
    MenuScreen base;                 // +0x000
    uint8_t mb4C, mb4D, mb4E, _pad4F;   // +0x04C (ctor: 0, 1, 1)
    struct { void *next, *prev; } mCars;   // +0x050 SelectableCar list (node at car +0x10)
    void *mpCurrent;                 // +0x058 current SelectableCar
    int32_t mPlayer;                 // +0x05C 0 / 1
    int32_t _unknown60;              // +0x060 number of cars in the list
    int32_t _unknown64;              // +0x064 1-based index of the current car
    int32_t mDialog;                 // +0x068 ChooseTransmission dialog handle
    FEScrollBar mScrollBar;          // +0x06C
    int32_t _unknownC4;              // +0x0C4 RealTimer of the last scroll (0: garage ride up to date)
    FEObject *mpLogo;                // +0x0C8 "ManufacturerLogo"
    FEObject *mpBadge;               // +0x0CC "CarBadge"
    FEObject *mpStringD0, *mpStringD4;   // +0x0D0 0xCF6F0439 / 0xABA93D3F images (mouse: previous / next list)
    cSlider mAccel;                  // +0x0D8
    cSlider mTopSpeed;               // +0x11C
    cSlider mHandling;               // +0x160
    uint8_t mb1A4;                   // +0x1A4 = 1
    uint8_t mbArrow1Shown;           // +0x1A5 0x444969FD (not at the first car)
    uint8_t mbArrow2Shown;           // +0x1A6 0x444969FE (not at the last car)
    uint8_t mbActive;                // +0x1A7 set by 0x35F8620B: accept / scroll / back messages handled
    uint32_t mOnlineHandle;          // +0x1A8 car handle kept for the online / LAN screens
    int32_t _unknown1AC;             // +0x1AC
    int32_t _unknown1B0;             // +0x1B0 = -1
    uint8_t mbMemcard;               // +0x1B4 split screen: memory card screen entered (0x8867412D clears)
    uint8_t _pad1B5[3];

    UIQRCarSelect *Construct(const ScreenConstructorData *data);   // GC 800DDD04 / PC 4F9D90 (ret 4)
    void Destruct();                                              // GC 800DE0D4 / PC 4E20B0
    UIQRCarSelect *ScalarDeletingDestructor(uint32_t f);          // PC 4EEB70 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t port);   // GC 800DE290 / PC 4FC1F0
    // GC 800DF4D0 / PC 4B2310: help / title labels per mode and car kind, logos (GENERICPLACEHOLDER when the
    // texture is missing), scroll bar + arrows, filter name label, help bar mask, sliders.
    void RefreshHeader();
    // GC 800DF330 / PC 4FA2A0 (ret 4): split screen: player's filter, label + control mask, list rebuilt with the
    // player's selected car current, top garage ride, 2P_PressStart.fng modal for player 2 once.
    void SetupScreenForPlayer(int32_t player);
    // GC 800DFF44 / PC 4E2590 (ret 4): 0x9120409E previous / 0xB5971BF1 next car (arrow blink, selection, garage
    // scroll direction); new car: garage car type (or a random ride of the type, PC), scroll time, header.
    void ScrollCars(uint32_t msg);
    // GC 800E0150 / PC 4EED10 (ret 4): 0x5073EF13 / 0xD9FEEC59 cycle the view filter per game mode (skipping
    // empty kinds), rebuild, header, online group / slot info.
    void ScrollCarLists(uint32_t msg);
    void DeleteCurrentCar();                                      // GC 800E081C / PC 4EF090
    static void ExitToMain(const char *pkg);                      // PC 4B22C0 (__cdecl): to UI_Main.fng (memcard if modified)

    // GC 800DE238 / 800DE264 (PC: inlined): CurrentViewFilter (PC 0x7F444C) <-> the active profile's (PC
    // *0x85AD28) filter at UserProfile +0xE0.
    static void SetViewFilter(uint32_t filter);
    static void UpdateViewFilter();
    static uint32_t GetUnlockFilter();          // GC 800DFB7C / PC 497DA0: modes 8 / 0x100 -> 0x1F8, else 1
    int32_t GetNumberOfCarsForFilter(uint32_t filter) const;   // GC 800DFBBC / PC 497EE0 (ret 4)
    void InitSlider();                          // GC 800DF194 / PC 497BF0: accel / top speed / handling sliders
    void UpdateSliders();                       // GC 800DF29C / PC 497CD0: current car's ratings
    void ChooseTransmission();                  // GC 800DFA78 / PC 497D50: auto / manual dialog
    void Setup();                               // GC 800DF09C / PC 4EEB90
    void RebuildCarListFromFilter();            // GC 800DFB14 / PC 4EEC90
    void ClearCarList();                        // GC 800DFEC4 / PC 4B2860: unlink + delete (vslot 0, 1) each car
    // GC 800DFC28 / PC 4E22E0 (ret 4): a SelectableCar (game, 0x930: node +0x10, handle +0x920) per filtered record
    // whose car type is allowed and unlocked (chosen career slots only); the quick race selection (first car if
    // none) becomes current; its ride goes to the top garage view (CarViewer::SetRideInfo inlined, reason 1).
    void BuildCarList(uint32_t filter);
    // GC 80117BBC / PC 4E2280 (ret 0x14): split screen: other player's messages silent; 0x5073EF13 -> 6,
    // 0xD9FEEC59 -> 7, accept / 0x406415E3 silent, else FilterSoundForHorizontalMenu(.., true).
    uint32_t NotifySoundMessage(uint32_t msg, uint32_t arg, uint32_t event) const;
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(UIQRCarSelect, mpCurrent) == 0x58 && offsetof(UIQRCarSelect, mScrollBar) == 0x6C &&
              offsetof(UIQRCarSelect, mpLogo) == 0xC8 && offsetof(UIQRCarSelect, mAccel) == 0xD8 &&
              offsetof(UIQRCarSelect, mHandling) == 0x160 && offsetof(UIQRCarSelect, mbActive) == 0x1A7 &&
              offsetof(UIQRCarSelect, mbMemcard) == 0x1B4 && sizeof(UIQRCarSelect) == 0x1B8);
#endif
