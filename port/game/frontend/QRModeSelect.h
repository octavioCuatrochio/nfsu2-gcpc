// GC UIQRModeSelect (zFe.cpp): quick race mode icons (UI_QRModeSelect.fng) and the MS* icons.
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"

// MS* icons (IconOption 0x48), vtables / React: Circuit 790FD4 / 4B2BE0 (mode 0), Sprint 790FDC / 4B2C70 (1),
// Drag 790FE4 / 4B2C90 (2), Drift 790FEC / 4B2CB0 (3), PC LAN icon 790FF4 / 4B2CD0 (5028D0, mode 8), FreeRun 790FFC /
// 4B2CF0 (mode 9, track settings u16 4000 + int 0), ShortTrack 791004 / 4B2D20 (0xA), URL 79100C / 4B2D40 (4).
enum class MSKind { Circuit, Sprint, Drag, Drift, Lan, FreeRun, ShortTrack, URL };
uint32_t MSVTable(MSKind k);
void MSReact(MSKind k, uint32_t msg);
// PC _SetGameMode 4B2C00 (__cdecl): race mode -> *0x83AAB4 (or *0x850088 online / LAN), players and opponents.
void QR_SetGameMode(int32_t mode);

// PC 0x150, vtable 0x797734: 0 4E2C40, 1 40D080, 2 NotificationMessage 4B2B30, 3 49DFC0, 4 50DA50, 5 Setup 4B2D60, 6 50DAB0.
struct UIQRModeSelect {
    IconScrollerMenu base;
    UIQRModeSelect *Construct(const ScreenConstructorData *data);   // GC 800E16FC / PC 4E2A50 (ret 4)
    void Destruct();                                                // GC 80117CA8 / PC 4E2C60
    UIQRModeSelect *ScalarDeletingDestructor(uint32_t f);           // PC 4E2C40 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800E18A4 / PC 4B2B30
    void Setup();                                                   // GC 800E19E0 / PC 4B2D60
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(UIQRModeSelect) == 0x150);
#endif
