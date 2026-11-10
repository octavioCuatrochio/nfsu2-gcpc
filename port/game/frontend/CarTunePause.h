// GC CarTunePauseScreen (zFe2.cpp): the pause icon menu of the performance tuning / dyno screens and its icons.
// State lives in game globals: *0x83A644 0 dyno / 1 tuning, *0x83A63C tuning category (0xFF none), byte 0x83A7A4
// tuning entered from the crib, CarTuneManager at 0x839C40 (bridged, not rewritten).
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"

// Icons (IconOption 0x48, React slot 1, ret 0x14, 0x0C407210 only):
enum class CTKind {
    Resume,          // 79B720: React 53C7D0  pop help bar, unpause 1, screen create data +0x10 = 0
    Restart,         // 79B728: React 564C80  confirm (UI_InGameDialog) yes 0xE1A57D51
    DynoChart,       // 79B730: React 53C830  -> *(0x7FA230) UI_PerformanceDyno_Chart.fng
    QuitToTuning,    // 79B738: React 564CF0  confirm yes 0x526ADEFC (no 0x34DC1BCF)
    Options,         // 79B740: React 53C860  -> UI_PauseOptionsMain.fng
    TuningMaster,    // 79B748: React 53C890  -> *(0x7FA214) UI_PerformanceTuning_Master.fng
    BackToDyno,      // 79B750: React 53C8C0  restore stored specs, back to the dyno
    DiscardChanges,  // 79B758: React 564D60  confirm yes 0x51123592 (ctor 5226C0, mbActsDirectly)
    QuitTuning,      // 79B760: React 564DD0  confirm yes 0x30ED2368 (text by mode)
    GoToSetting,     // 79B768: React 55A580  unsaved changes ? confirm yes 0x2B5DFEF6 : -> *(0x7FA218) (ctor 522710)
};
uint32_t CTVTable(CTKind k);
void CTReact(CTKind k, const char *pkg, uint32_t msg, uint32_t arg4);

// PC 0x158, vtable 0x79D30C: 0 scalar dtor 55A550, 1 40D080, 2 NotificationMessage 5647E0, 3 NotifySoundMessage
// 510EB0, 4 RefreshHeader 549760, 5 Setup 557350, 6 AddOption 50DAB0.
struct CarTunePauseScreen {
    IconScrollerMenu base;           // +0x000
    int32_t mOpenTime;               // +0x150 RealTimer at construction (input ignored in that tick)
    uint8_t mbSaving;                // +0x154 0x24DD40CF started the memory card save
    uint8_t _pad155[3];

    CarTunePauseScreen *Construct(const ScreenConstructorData *data);   // GC 8017DA64 / PC 55A460 (ret 4)
    void Destruct();                                                   // GC 8018D27C / PC 55A570
    CarTunePauseScreen *ScalarDeletingDestructor(uint32_t f);          // PC 55A550 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 8017DADC / PC 5647E0
    static uint32_t NotifySoundMessage(uint32_t event);                 // GC 8018D320 / PC 510EB0: 4 / 5 silenced
    bool IsOkToResume() const;                                         // GC 8017DFD4 / PC 5100E0
    void Setup();                                                      // GC 8017E048 / PC 557350
    void SetupDynoPause();                                             // GC 8017E0CC / PC 53C9E0
    void SetupTuningPause();                                           // GC 8017E324 / PC 5227E0
    void RefreshHeader();                                              // GC 8017E574 / PC 549760
    void DoQuitTuning();                                               // GC 8017E65C / PC 53CC70
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(CarTunePauseScreen) == 0x158 && offsetof(CarTunePauseScreen, mbSaving) == 0x154);
#endif
