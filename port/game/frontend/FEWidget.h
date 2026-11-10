// Game-side frontend widgets (option screens): FEWidget base + FEButtonWidget. Original: zFe2.cpp. PC layouts
// (GC + 4: PC has two list words at +4/+8). Virtual calls go through the game's vtables (bridge).
#pragma once
#include <cstddef>
#include <cstdint>

struct FEObject;

// GC FEWidget (PC 0x2C bytes, vtable 0x790B98). Its one-instruction members (Show/Hide/Enable/Disable/SetPos*)
// are identical-code-folded on PC with 100+ other classes, so they are rewritten here but never hooked.
struct FEWidget {
    void *vtable;                    // +0x00
    void *_unknown04, *_unknown08;   // +0x04 (not set by the constructor)
    float mPosX, mPosY;              // +0x0C (GC +0x08) top left
    float mSizeX, mSizeY;            // +0x14
    float mLabelOffsetX, mLabelOffsetY;   // +0x1C -276, -10: label at position - offset
    FEObject *mpLabel;               // +0x24 (GC +0x20) optional label object
    uint8_t mbEnabled;               // +0x28 (GC +0x24) Enable/Disable (PC 50D7C0 / 50D7D0)
    uint8_t mbHidden;                // +0x29 (GC +0x28) Hide/Show (PC 4AE980 / 4AE970)
    uint8_t _pad2A[2];

    void Init(const float *pos, const float *size, bool enabled);   // GC 801536F4 (PC: inlined into every ctor)
    void Show() { mbHidden = 0; }
    void Hide() { mbHidden = 1; }
    void Enable() { mbEnabled = 1; }
    void Disable() { mbEnabled = 0; }
    void SetPosX(float x) { mPosX = x; }   // PC 4047F0 (folded)
    void SetPosY(float y) { mPosY = y; }   // PC 4AE9B0
};

// GC FEButtonWidget (PC 0x38 bytes, vtable 0x79B2F8).
struct FEButtonWidget {
    FEWidget base;
    FEObject *mpObject;              // +0x2C (GC +0x30) the button
    float mWidth, mHeight;           // +0x30 (GC +0x34) copy of the size

    FEButtonWidget *Construct(const float *pos, const float *size, bool enabled);   // GC 80153858 / PC 51DFC0 (ret 0xC)
    void Show();                                 // GC 80153A40 / PC 50D530: object + label visible
    void Hide();                                 // GC 80153A80 / PC 50D550
    void SetFocus(const char *pkg);              // GC 80153AC0 / PC 52D770 (ret 4): current button + "Highlight"
    void UnsetFocus();                           // GC 80153B20 / PC 51E020: "UnHighlight"
    void Position();                             // GC 80153960 / PC 52D6B0: place object (centred strings) + label
    void CheckMouse(const char *pkg, float x, float y);   // GC 801538D8 / PC 52D650 (ret 0xC)
};

// GC FELabelWidget (PC 0x38 bytes, vtable 0x79B338): same fields as the button, always enabled, no current button.
struct FELabelWidget {
    FEWidget base;
    FEObject *mpObject;              // +0x2C
    float mWidth, mHeight;           // +0x30

    FELabelWidget *Construct(const float *pos, const float *size);   // PC 51E090 (ret 8; GC ctor inlined)
    void Position();                 // GC 80153B7C / PC 51E0F0: object top-left at the position + label
    void SetFocus(const char *pkg);  // GC 80153C5C / PC 51E130 (ret 4, pkg unused): "Highlight"
};

// GC FEStatWidget (PC 0x4C bytes, vtable 0x79B378): name object + value object placed in its own box.
struct FEStatWidget {
    FEWidget base;
    FEObject *mpObject;              // +0x2C (GC +0x30)
    FEObject *mpValue;               // +0x30 (GC +0x34)
    float mWidth, mHeight;           // +0x34 copy of the size
    float mValueSizeX, mValueSizeY;  // +0x3C
    float mValuePosX, mValuePosY;    // +0x44

    // GC 80153D04 / PC 51E1B0 (ret 0x14)
    FEStatWidget *Construct(const float *pos, const float *size, const float *valueSize, const float *valuePos, bool enabled);
    void Show();                     // GC 80153ED4 / PC 50D570
    void Hide();                     // GC 80153F1C / PC 50D5A0
    void SetPosX(float x);           // GC 80153F64 / PC 52D930 (ret 4): value box moves along; label follows
    void SetPosY(float y);           // GC 80153FDC / PC 52D990 (ret 4)
    void Position();                 // GC 80153D94 / PC 52D800
};

// GC cSlider (PC 0x44, vtable 0x78F544; virtuals through bridge::cSlider_*). Embedded in FESliderWidget.
struct cSlider {
    void *vtable;              // +0x00
    uint8_t _unknown04[4];     // +0x04 (byte cleared)
    const char *_unknown08;    // +0x08 "" (0x783FE5)
    FEObject *mpThumb;         // +0x0C
    uint32_t _unknown10[5];    // +0x10
    float _unknown24;          // +0x24 (widget +0x78) compared with +0x2C for sound event 5
    float _unknown28;          // +0x28 (widget +0x7C) compared with +0x2C for sound event 4
    float _unknown2C;          // +0x2C (widget +0x80)
    uint32_t _unknown30[4];    // +0x30
    float _unknown40;          // +0x40 46.0
};

// GC FESliderWidget (PC 0x98, vtable 0x79B400; 7 option-screen subclasses override it). PC: FEStatWidget base
// (GC: FEToggleWidget, whose ctor PC inlines: just the two arrows).
struct FESliderWidget {
    FEStatWidget base;               // +0x00
    FEObject *mpLeftArrow;           // +0x4C (GC +0x50)
    FEObject *mpRightArrow;          // +0x50 (GC +0x54)
    cSlider mSlider;                 // +0x54 (GC +0x58)

    // PC 51E840 (ret 0x14; GC 801549D4): FEStatWidget ctor, arrows 0, cSlider ctor 4FF430 inlined.
    FESliderWidget *Construct(const float *pos, const float *size, const float *valueSize, const float *valuePos, bool enabled);
    // GC 80154A18 / PC 50D6F0 (ret 8): event 4 -> 4 if slider +0x2C != +0x28, 5 -> 5 if +0x2C != +0x24 (else 0); others kept.
    uint32_t DetermineSpecialSound(uint32_t msg, uint32_t event) const;
    void Position();                 // GC 80154A70 / PC 52DEF0: object, thumb centred on the value box, Draw, arrows
    void Show();                     // GC 80154B6C / PC 50D740
    void Hide();                     // GC 80154BC8 / PC 50D780
    void SetFocus(const char *pkg);  // GC 80154C3C / PC 52DFF0 (ret 4): current button + "Highlight" (inlined)
    void UnsetFocus();               // GC 80154CC4 / PC 51E8D0
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(cSlider) == 0x44 && sizeof(FESliderWidget) == 0x98 && offsetof(FESliderWidget, mSlider) + offsetof(cSlider, _unknown2C) == 0x80);
#endif
#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FELabelWidget) == 0x38 && sizeof(FEStatWidget) == 0x4C && offsetof(FEStatWidget, mValuePosY) == 0x48);
#endif
#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEWidget) == 0x2C && offsetof(FEWidget, mpLabel) == 0x24 && offsetof(FEWidget, mbHidden) == 0x29);
static_assert(sizeof(FEButtonWidget) == 0x38 && offsetof(FEButtonWidget, mWidth) == 0x30);
#endif
