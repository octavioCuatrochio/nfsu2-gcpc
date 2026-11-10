// GC ChooseBrandsScreen (zFe2.cpp): pick a brand per part of the selected performance package (or of the single
// part when browsing individual parts). PC = a plain UIWidgetMenu (0x14C, factory 554BB0), vtable 0x79CACC:
// 0 scalar dtor 55AD00 (folded with TuningSlidersMenu's: UIWidgetMenu body), 1 40D080, 2 NotificationMessage
// 55C930, 3 570A70, 4 Setup 550330.
#pragma once
#include <cstddef>
#include <cstdint>
#include "UIWidgetMenu.h"
#include "FEWidget.h"

struct ScreenConstructorData;
struct FEObject;

// Part record (PC; package parts start at package +0x0C, stride 0x5C): +0x04 name language hash, +0x0C brand
// count, +0x10 brand hashes.
// GC BrandScrollerThing (PC 0x60, vtable 0x79B6A0 = FEToggleImageWidget's slots + Act 521E90, Draw 53B960).
struct BrandScrollerThing {
    FEToggleImageWidget base;        // +0x00
    const uint8_t *mpPart;           // +0x58
    int32_t mIndex;                  // +0x5C selected brand

    // PC 521E30 (ret 0x18; GC: FEToggleImageWidget ctor + fields inline in Setup): stat widget, arrows / image 0,
    // label offset (-232, -64).
    BrandScrollerThing *Construct(const uint8_t *part, const float *pos, const float *size, const float *valueSize,
                                  const float *valuePos, bool enabled);
    // PC 521E90 (ret 8): left/right cycle the brand (wrapping), then BlinkArrows and Draw (virtual).
    void Act(const char *pkg, uint32_t msg);
    // PC 53B960: name = part name hash, value = brand display name (as the format), image = "DECAL_ICON_<brand>"
    // texture when it exists, else 0x74CE8C0B.
    void Draw();
    uint32_t Brand() const { return reinterpret_cast<const uint32_t *>(mpPart + 0x10)[mIndex]; }
};

struct ChooseBrandsScreen {
    UIWidgetMenu base;

    ChooseBrandsScreen *Construct(const ScreenConstructorData *data);   // GC 80174280 / PC 5532B0 (ret 4)
    // GC 801742CC / PC 55C930 (ret 0x10): select -> out-script (StorePrevNotification inline); its end buys and installs
    // the package / part and moves to the next customization screen; up/down show 0xB34A6C2B for multi-brand rows.
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);
    void Setup();                    // GC 80174438 / PC 550330 (at most 4 parts; < 2 rows hide 0x6D017BC5)
    // GC 801745E4 / PC 521EF0: each row's brand into the chosen-brands array (PC global 0x8389B4; GC
    // CarCustomizeManager::SetChosenBrand).
    void SetChosenBrands();
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(BrandScrollerThing) == 0x60 && sizeof(ChooseBrandsScreen) == 0x14C);
#endif
