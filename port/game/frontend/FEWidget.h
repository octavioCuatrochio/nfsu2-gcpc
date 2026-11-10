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
    // GC 8015375C / PC 50D460 (ret 4): message -> sound event (0x911C0A4B 7, 0x911AB364 10, 0x72619778 6,
    // 0x9120409E 4, 0xB5971BF1 5, 0xB5AF2461 9), then DetermineSpecialSound (slot 14); other messages 0.
    uint32_t DetermineSound(uint32_t msg);
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

// GC FEToggleWidget (PC 0x54, vtable 0x79B3B8; 17 slots: +0x40 BlinkArrows). PC: FEStatWidget base + arrows.
// Vtable slots used here: +0x04 Act, +0x38 DetermineSpecialSound, +0x3C DetermineSoundTrigger, +0x40 BlinkArrows.
struct FEToggleWidget {
    FEStatWidget base;               // +0x00
    FEObject *mpLeftArrow;           // +0x4C (GC +0x50)
    FEObject *mpRightArrow;          // +0x50 (GC +0x54)

    // GC 80154054 / PC 51E230 (ret 0x14)
    FEToggleWidget *Construct(const float *pos, const float *size, const float *valueSize, const float *valuePos, bool enabled);
    // GC 8015409C / PC 52D9F0 (ret 0xC): left arrow -> 0x9120409E (event 4); right arrow or value -> 0xB5971BF1 (5):
    // Act, BlinkArrows, then the sound (PC: trigger fetched before the special sound, MakeFrontendSound inlined).
    void CheckMouse(const char *pkg, float x, float y);
    void Position();                 // GC 801542A4 / PC 52DAC0: stat widget + arrows around the value box
    void Enable();                   // GC 80154334 / PC 51E270: "UnHighlight" on all
    void Disable();                  // GC 801543C0 / PC 51E3E0: "Disable" on all
    void Show();                     // GC 8015444C / PC 50D5D0
    void Hide();                     // GC 801544A4 / PC 50D610
    void SetFocus(const char *pkg);  // GC 801544FC / PC 52DB40 (ret 4): current button + "Highlight"
    void UnsetFocus();               // GC 8015458C / PC 51E550
    void BlinkArrows(uint32_t msg);  // GC 80154610 / PC 51E6B0 (ret 4): "Blink" on the arrow for the message
    static uint32_t DetermineSoundTrigger() { return 2; }   // GC 8018BCA0 / PC 408CD0 (folded)
};

// GC FEToggleImageWidget (GC 0x5C -> PC 0x58): toggle + image above the value. PC never instantiates it directly:
// its ctor (GC 80154678) is inlined into BrandScrollerThing's 521E30 (vtable 0x79B6A0, +0x58 brand data, +0x5C index),
// whose vtable holds these members.
struct FEToggleImageWidget {
    FEToggleWidget base;             // +0x00
    FEObject *mpImage;               // +0x54 (GC +0x58)

    FEToggleImageWidget *Construct(const float *pos, const float *size, const float *valueSize, const float *valuePos, bool enabled);
    void Position();                 // GC 801546BC / PC 52DCC0: object, image and value centred in the value box, arrows
    void Enable();                   // GC 8015486C / PC 51E6F0
    void Disable();                  // GC 801548AC / PC 51E760
    void Show();                     // GC 801548EC / PC 50D650 (PC: image last, after the label)
    void Hide();                     // GC 80154920 / PC 50D6A0
    void SetFocus(const char *pkg);  // GC 80154954 / PC 52DE80 (ret 4)
    void UnsetFocus();               // GC 80154994 / PC 51E7D0
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEToggleWidget) == 0x54 && sizeof(FEToggleImageWidget) == 0x58);
#endif

// GC SliderInitParams (cSlider::Init): object names (null = keep the current object) and values.
struct SliderInitParams {
    const char *mpPackage;           // +0x00 null: keep
    const char *mpBacking, *mpHandle, *mpValue, *mpCurrent, *mpEndCap, *mpEnd;   // +0x04 .. +0x18
    float mMin, mMax, mValue, mStep, mMargin;   // +0x1C .. +0x2C
};

// GC cSlider (zFe2.cpp; nfsu2-re UIProgressBar). PC 0x44, vtable 0x78F544: +0 scalar dtor 4957B0, +4 Update,
// +8 Init, +0xC Toggle, +0x10 Highlight, +0x14 UnHighlight, +0x18 Draw, +0x1C DrawVertical, +0x20 DrawHorizontal.
// Two subclasses (0x79AE30 / 0x79AE54) override Toggle / Draw / Update. Own virtual calls go through the vtable.
struct cSlider {
    void *vtable;              // +0x00
    uint8_t mbVertical;        // +0x04
    uint8_t _pad05[3];
    const char *mpPackage;     // +0x08 "" (0x783FE5) until Init
    FEObject *mpBacking;       // +0x0C
    FEObject *mpHandle;        // +0x10
    FEObject *mpValue;         // +0x14 string: "%d" of the value
    FEObject *mpCurrent;       // +0x18 fill bar
    FEObject *mpEndCap;        // +0x1C
    FEObject *mpEnd;           // +0x20
    float mMax;                // +0x24
    float mMin;                // +0x28
    float mPrev;               // +0x2C value before the last change
    float mValue;              // +0x30
    float mInitial;            // +0x34 value at Init
    float mStep;               // +0x38
    float mLength;             // +0x3C backing width - 2 * margin
    float mMargin;             // +0x40 46.0 by default

    cSlider *Construct();                          // GC 8011F220 / PC 4FF430
    // GC 8011F290 / PC 4FF480 (ret 4): msg 0x9120409E: value -= step (>= min); 0xB5971BF1: += step (<= max); then
    // Draw (virtual, also for other messages). True if it moved: false when new and previous value are the limit.
    bool Update(uint32_t msg);
    void Init(const SliderInitParams *params, bool vertical);   // GC 8011F350 / PC 540C10 (ret 8)
    void Draw();                                   // GC 8011F790 / PC 4FF530: DrawVertical / DrawHorizontal (virtual)
    void DrawHorizontal();                         // GC 8011F7F0 / PC 540D20
    void DrawVertical();                           // GC 8011FA88 / PC 540E90
    void Toggle(bool on);                          // GC 8011FFE4 / PC 512240 (ret 4)
    void SetCurValue(float v);                     // GC 80120078 / PC 4FF550 (ret 4): clamp to [min, max]
    void Highlight();                              // GC 801200AC / PC 526D90 (FEngSetScript inlined x6)
    void UnHighlight();                            // GC 80120138 / PC 526F80
};

// GC PreviewSlider (PC 0x4C, vtable 0x79AE30; ctor inlined in its screen, e.g. 559CF8): cSlider + a bar showing
// the difference to a previewed value ("ToGreater" / "ToLesser" scripts).
struct PreviewSlider {
    cSlider base;              // +0x00
    FEObject *mpPreviewBar;    // +0x44
    float mPreview;            // +0x48 (ThreeStageSlider: upper stage limit)

    void Toggle(bool on);      // GC 801206AC / PC 5122B0 (ret 4): cSlider objects, handle again, preview bar
    void Draw();               // GC 8012046C / PC 527170
};

// GC ThreeStageSlider (PC 0x50, vtable 0x79AE54; ctor inlined in TuningSlider 522AF0): the value moves inside
// [mStageLow, base.mPreview]; Toggle / Init from PreviewSlider / cSlider.
struct ThreeStageSlider {
    PreviewSlider base;        // +0x00
    float mStageLow;           // +0x4C

    bool Update(uint32_t msg); // GC 801207C0 / PC 4FF590 (ret 4): like cSlider::Update with the stage limits
    void Draw();               // GC 8012087C / PC 5273A0
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(PreviewSlider) == 0x4C && sizeof(ThreeStageSlider) == 0x50);
#endif

// GC FESliderWidget (PC 0x98, vtable 0x79B400; 7 option-screen subclasses override it). PC: FEStatWidget base
// (GC: FEToggleWidget, whose ctor PC inlines: just the two arrows).
struct FESliderWidget {
    FEStatWidget base;               // +0x00
    FEObject *mpLeftArrow;           // +0x4C (GC +0x50)
    FEObject *mpRightArrow;          // +0x50 (GC +0x54)
    cSlider mSlider;                 // +0x54 (GC +0x58)

    // PC 51E840 (ret 0x14; GC 801549D4): FEStatWidget ctor, arrows 0, cSlider ctor 4FF430 inlined.
    FESliderWidget *Construct(const float *pos, const float *size, const float *valueSize, const float *valuePos, bool enabled);
    // GC 80154A18 / PC 50D6F0 (ret 8): event 4 -> 4 if slider mPrev != mMin, 5 -> 5 if mPrev != mMax (else 0); others kept.
    uint32_t DetermineSpecialSound(uint32_t msg, uint32_t event) const;
    void Position();                 // GC 80154A70 / PC 52DEF0: object, thumb centred on the value box, Draw, arrows
    void Show();                     // GC 80154B6C / PC 50D740
    void Hide();                     // GC 80154BC8 / PC 50D780
    void SetFocus(const char *pkg);  // GC 80154C3C / PC 52DFF0 (ret 4): current button + "Highlight" (inlined)
    void UnsetFocus();               // GC 80154CC4 / PC 51E8D0
    // PC 5275B0 (ret 0x18; GC: inlined in AddSliderOption): slider objects, length = backing width - 2 * margin
    void SetSliderObjects(FEObject *backing, FEObject *handle, FEObject *value, FEObject *current, FEObject *endCap, FEObject *end);
};

// GC TuningSlider (zFe2.cpp; PC 0xC0, vtable 0x79B7B0 = FEToggleWidget's 17 slots with 0 dtor 531010, 1 Act 564F10,
// 2 CheckMouse 565050, 3 Draw 53CD30, 4 Position, 5 Show, 6 Hide, 9 SetFocus, 10 UnsetFocus; Enable/Disable/BlinkArrows
// are FEToggleWidget's). One per tuning setting on the tuning sliders screen (built in 549xxx).
struct TuningSlider {
    FEToggleWidget base;             // +0x00
    uint32_t mMinHash, mMaxHash;     // +0x54 (GC +0x58) range label language hashes; both set -> range labels shown
    FEObject *mpMinLabel;            // +0x5C (GC +0x60) (not set by the ctor)
    FEObject *mpMaxLabel;            // +0x60 (GC +0x64)
    FEObject *mpExtra;               // +0x64 PC only, optional: placed under the bar, centred on it
    int32_t mTuningType;             // +0x68
    ThreeStageSlider mSlider;        // +0x6C
    float mYOffset;                  // +0xBC

    // PC 522AF0 (ret 0x24; GC ctor inlined in TuningSlidersMenu). Label offset X = 0 (not -276).
    TuningSlider *Construct(int32_t type, const float *pos, const float *size, const float *valueSize, const float *valuePos,
                            uint32_t minHash, uint32_t maxHash, bool enabled, float yOffset);
    // PC 530D20 (ret 0x1C, PC only): slider objects; length = backing width - 2 * margin
    void SetSliderObjects(FEObject *backing, FEObject *handle, FEObject *value, FEObject *current, FEObject *previewBar,
                          FEObject *endCap, FEObject *end);
    void Position();                 // GC 80310264 / PC 530AA0 (PC: mpExtra, no +0.0 terms; label offset Y -= y offset)
    void Hide();                     // GC 803104F0 / PC 522BC0 (FEToggleWidget::Hide inlined)
    void Show();                     // GC 80310538 / PC 522C30
    void SetFocus(const char *pkg);  // GC 80310580 / PC 530F30 (ret 4)
    void UnsetFocus();               // GC 803105D8 / PC 522CA0
    // PC 530D90 (PC only; called by the screen and its notifications): min / max (0 -> 1) / step / value (clamped,
    // previous kept) / stage limits from the car tuning object, then the bar length.
    void InitValues();
    // GC 8031014C / PC 53CD30: slider, name "name (units)" or plain, value "%$2.1f" (types 10-15) else "%$2.2f" (PC:
    // then mpExtra gets hash 0xCBBCB1C5 first), range labels.
    void Draw();
    // GC 80310054 / PC 564F10 (ret 8): left/right: slider Update, write the value, read it back clamped; types 9-15
    // notify pkg 0x5FE4BAE4; master screen 0xE15F62F8; BlinkArrows; Draw (both virtual).
    void Act(const char *pkg, uint32_t msg);
    // PC 565050 (ret 0xC; GC: FEToggleWidget's): toggle CheckMouse, then a click on mpExtra sends 0xC519BFC3 to pkg.
    void CheckMouse(const char *pkg, float x, float y);
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(TuningSlider) == 0xC0 && offsetof(TuningSlider, mSlider) == 0x6C && offsetof(TuningSlider, mTuningType) == 0x68);
#endif

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(cSlider) == 0x44 && sizeof(FESliderWidget) == 0x98 && offsetof(FESliderWidget, mSlider) + offsetof(cSlider, mPrev) == 0x80);
#endif
#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FELabelWidget) == 0x38 && sizeof(FEStatWidget) == 0x4C && offsetof(FEStatWidget, mValuePosY) == 0x48);
#endif
#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEWidget) == 0x2C && offsetof(FEWidget, mpLabel) == 0x24 && offsetof(FEWidget, mbHidden) == 0x29);
static_assert(sizeof(FEButtonWidget) == 0x38 && offsetof(FEButtonWidget, mWidth) == 0x30);
#endif
