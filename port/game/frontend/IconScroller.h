// GC icon menus (zFe2.cpp): IconOption (one icon), IconPanel (a list of icons laid out in a row / column),
// IconScroller (an IconPanel that scrolls its icons past a centre, with a scroll bar, fades and arrows) and
// IconScrollerMenu (a MenuScreen around an IconScroller: main menu, options / pause option menus...).
// PC layouts differ from GC (MSVC vptr at 0, bools packed); PC adds mouse handling and the wrap choice helpers.
#pragma once
#include <cstddef>
#include <cstdint>
#include "MenuScreen.h"
#include "FEScrollBar.h"
#include "UIWidgetMenu.h"   // WidgetListNode

struct FEObject;

// PC 0x4C. Subclasses (one vtable per menu entry) override React (slot 1); the base vtable 0x79B604 has the empty
// React 50C2F0 (ret 0x14). Base class ThumbnailNode (vtable 0x79AFDC): vtable, list node, name hash, object.
struct IconOption {
    void *vtable;                    // +0x00
    WidgetListNode node;             // +0x04 next / prev (lists store node pointers, option = node - 4)
    uint32_t mNameHash;              // +0x0C ctor arg 2
    FEObject *mpObject;              // +0x10 the "<prefix>N" image (set by AddOption)
    float mOffsetX, mOffsetY;        // +0x14 IconScroller: x of the icon on the strip (AddOption); y unused (0)
    uint32_t mColor;                 // +0x1C colour restored by StartDim / ClearList (IconScroller: 0xFFFFFFFF)
    uint8_t mbLocked;                // +0x20 can't be selected (unless the panel allows locked icons)
    uint8_t _unknown21;              // +0x21 = 1
    uint8_t mbShowLock;              // +0x22 ctor arg 5: lock overlay shown, accept only plays sound 0x13
    uint8_t mbFadeInButton;          // +0x23 = 1: button overlay fades in (UpdateIconOverlay)
    float mWidth, mHeight;           // +0x24 image size at AddOption
    uint32_t mHeaderHash;            // +0x2C ctor arg 3 (IconScrollerMenu title)
    uint32_t mDescHash;              // +0x30 ctor arg 4 (second header string)
    float mScaleTarget;              // +0x34 GC StartScale (PC inlined in IconPanel::SetSelection): 1.25 selected, 1
    float mScaleStart;               // +0x38 start time in seconds (RealTimer * 0.00025)
    float mScaleDuration;            // +0x3C 0.125
    float mScale;                    // +0x40 scale the animation starts from (written when it ends, see AnimateList)
    uint8_t mbScaleDone;             // +0x44
    uint8_t mbActsDirectly;          // +0x45 accept goes straight to React (no dim / input stop / out-script)
    uint8_t _pad46[2];
    uint32_t mId;                    // +0x48 ctor arg 1

    // GC 801570E0 / PC 521FF0 (ret 0x14): PC takes (id, name hash, header hash, desc hash, show lock).
    IconOption *Construct(uint32_t id, uint32_t nameHash, uint32_t header, uint32_t desc, bool showLock);
    void Destruct();                                  // GC 8030D3E0 / PC 527770: ThumbnailNode vtable
    IconOption *ScalarDeletingDestructor(uint32_t f);  // PC 4AEB70 (ret 4)
};

// PC 0x38, vtable 0x795CBC: 0 scalar dtor 543BF0, 1 Update 538910, 2 AddOption 5439B0, 3 Act 50D980,
// 4 SetSelection(int) 52F460, 5 SetSelection(IconOption*) 52F510, 6 HandleMouse 52F5A0 (PC), 7 SetInitialPos 51F870.
struct IconPanel {
    void *vtable;                    // +0x00
    WidgetListNode mOptions;         // +0x04 list head (GC +0x00)
    IconOption *mpCurrent;           // +0x0C
    FEObject *mpOptionMaster;        // +0x10 ctor name 1 ("OPTION_MASTER": centre / first position)
    FEObject *mpLockMaster;          // +0x14 ctor name 2 (lock overlay)
    FEObject *mpButtonMaster;        // +0x18 ctor name 3 (button overlay)
    const char *mpPackageName;       // +0x1C
    const char *mpPrefix;            // +0x20 image name prefix ("option_")
    float mSpacing;                  // +0x24 10 (IconScroller 40)
    int32_t mNextNumber;             // +0x28 1-based number of the next image (count + 1)
    uint8_t mbWrap;                  // +0x2C ctor arg 6: scrolling wraps around
    uint8_t mbHorizontal;            // +0x2D 1
    uint8_t mbDirty;                 // +0x2E selection / sizes changed (IconScroller::Update re-targets)
    uint8_t mbInputEnabled;          // +0x2F 1 (IconScrollerMenu StartInput / StopInput)
    uint8_t mbAllowLocked;           // +0x30 0: locked icons are skipped
    uint8_t _pad31[3];
    int32_t mLastDir;                // +0x34 -9 = none; last Scroll direction (arrows blink by it)

    // GC 80158484 / PC 543840 (ret 0x18)
    IconPanel *Construct(const char *pkg, const char *master, const char *lock, const char *button, const char *prefix, bool wrap);
    void Destruct();                               // GC 8018BEA8 / PC 4CBC20 (also IconScroller's, folded)
    IconPanel *ScalarDeletingDestructor(uint32_t f);   // PC 543BF0 (ret 4)
    void Update();                                 // GC 80158C54 / PC 538910
    FEObject *AddOption(IconOption *opt);          // GC 80158568 / PC 5439B0 (ret 4): "<prefix><n>" image or null
    void ClearList();                              // GC 80158648 / PC 52F390
    void Act(uint32_t msg, FEObject *obj, uint32_t a3, uint32_t a4);   // GC 8015870C / PC 50D980 (ret 0x10)
    int32_t GetOptionIndex() const;                // GC 80158774 / PC 495910: current, 1-based; none -> 0
    int32_t GetOptionIndex(const IconOption *opt) const;   // PC 51F820 (ret 4): 1-based, null / absent -> -1
    bool SetSelection(int32_t index);              // GC 801587BC / PC 52F460 (ret 4): 0-based node index
    bool SetSelection(IconOption *opt);            // GC 80158858 / PC 52F510 (ret 4)
    bool HandleMouse();                            // PC 52F5A0: icon under the mouse becomes current
    void SetInitialPos();                          // GC 801588E4 / PC 51F870
    void Scroll(int32_t dir);                      // GC 80158A74 / PC 51F9F0 (ret 4): -1 / 1, skipping locked
    void ScrollWrapped(int32_t dir);               // GC 80158B68 / PC 51FAC0 (ret 4)
    void ScrollNext();                             // PC 527730: ScrollWrapped / Scroll (+1) by mbWrap
    void ScrollPrev();                             // PC 527750 (-1)
    void AnimateList(float *w, float *h);          // GC 80158C74 (+ AnimateSelected 80158CAC inlined) / PC 51FB70 (ret 8)
    bool IsHead() const;                           // GC 8018BF40 / PC 5276D0
    bool IsTail() const;                           // GC 8018BF54 / PC 527700
};

// GC tCubic1D (cPoint::SplineSeek PC 574E30): value / velocity / target, cubic state.
struct IconCubic {
    float mVal, mdVal;               // +0x00 (+0x90 in IconScroller) current strip offset
    float mValDesired, mdValDesired; // +0x08
    float _unknown10[5];             // +0x10 coefficients / time (SplineSeek's)
    float mDuration;                 // +0x24 0.3
    float _unknown28;                // +0x28 1.0
    uint16_t mFlags;                 // +0x2C 2 = new target
    uint16_t mState;                 // +0x2E
};

// PC 0xE8, vtable 0x79C0F4: 0 543BF0, 1 Update 538940, 2 AddOption 543C10, 3 Act 50D980, 4 SetSelection(int) 52F7C0,
// 5 SetSelection(IconOption*) 52F680, 6 HandleMouse 538B80 (PC), 7 IconPanel::SetInitialPos 51F870,
// 8 SetInitialPos(int) 538A30.
struct IconScroller {
    IconPanel base;                  // +0x00
    FEScrollBar mScrollBar;          // +0x38 "ScrollBar"
    IconCubic mPos;                  // +0x90
    int32_t mAlign;                  // +0xC0 1: unselected icons keep 0 top / 1 centre / 2 bottom edge
    int32_t _unknownC4;
    int32_t mIndex;                  // +0xC8 1-based current index
    float mStripWidth;               // +0xCC visible width (ctor arg 6)
    float _unknownD0;                // +0xD0 0
    float mCenterX, mCenterY;        // +0xD4 centre of the master object
    float _unknownDC;
    float mNextX;                    // +0xE0 x of the next added icon
    uint8_t mbFade;                  // +0xE4 icons fade towards the strip edges (StopDim sets, StartDim clears)
    uint8_t _padE5[3];

    // GC 80158E44 / PC 543A90 (ret 0x18)
    IconScroller *Construct(const char *pkg, const char *master, const char *lock, const char *button, const char *prefix, float width);
    void Update();                                 // GC 80158F68 / PC 538940
    FEObject *AddOption(IconOption *opt);          // GC 80159064 / PC 543C10 (ret 4)
    void SetInitialPos(int32_t index);             // GC 801591A0 / PC 538A30 (ret 4): 1-based, 0 = first
    bool SetSelection(int32_t index);              // GC 80159384 / PC 52F7C0 (ret 4)
    bool SetSelection(IconOption *opt);            // GC 801592D0 / PC 52F680 (ret 4)
    void StartDim();                               // GC 8015944C / PC 51FD10
    void StopDim();                                // GC 801594A0 / PC 538B30
    void ClipEdges(IconOption *opt, float x);      // GC 801594F4 / PC 50D9C0 (ret 8)
    void PositionOption(IconOption *opt);          // GC 80159550 / PC 52F8C0 (ret 4)
    void UpdateFade(IconOption *opt, float fade);  // GC 801596F8 / PC 51FD60 (ret 8)
    void UpdateArrows(int32_t dir);                // GC 801597C4 / PC 52FA70 (ret 4)
    bool HandleMouse();                            // PC 538B80
};

// PC 0x150, vtable 0x7974D4: 0 scalar dtor 4DF010, 1 40D080, 2 NotificationMessage 543D40, 3 570A70,
// 4 RefreshHeader 50DA50, 5 Setup (pure), 6 AddOption 50DAB0.
struct IconScrollerMenu {
    MenuScreen base;                 // +0x000
    IconScroller mScroller;          // +0x04C
    FEObject *mpTitle;               // +0x134 "ICON_TITLE"
    FEObject *mpDesc;                // +0x138 FEngFindString(pkg, FEHashUpper(""))
    uint32_t mPrevMsg;               // +0x13C StorePrevNotification
    FEObject *mpPrevObj;             // +0x140
    uint32_t mPrevArg, mPrevPkg;     // +0x144
    uint8_t mbInputStopped;          // +0x14C StopInput sets, StartInput clears (and restores the package mask)
    uint8_t _pad14D[3];

    IconScrollerMenu *Construct(const ScreenConstructorData *data);   // GC 80159B84 / PC 552330 (ret 4)
    void Destruct();                                 // GC 8018C034 / PC 4DEFB0
    IconScrollerMenu *ScalarDeletingDestructor(uint32_t f);   // PC 4DF010 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 80159C60 / PC 543D40
    void StartDim();                                 // GC 8015A218 / PC 538C00
    void StopDim();                                  // GC 8015A274 / PC 538CA0
    void StartInput();                               // GC 8015A2D8 / PC 52FBA0
    void StopInput();                                // GC 8015A350 / PC 52FBF0
    void StorePrevNotification(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 8015A3C0 / PC 50DA20
    void RefreshHeader();                            // GC 8015A3D4 / PC 50DA50
    void AddOption(IconOption *opt);                 // GC 8015A43C / PC 50DAB0 (ret 4)
};

// GC 80157E7C / PC 50D8D0 (__cdecl, x87 result): 0 outside [centre - width/2, centre + width/2], ramps 0 -> 1 over
// `w` at both edges, else 1.
long double thumbnail_scale(float x, float center, float width, float w);
// GC 80153618 / PC 51DF00 (__cdecl): lock / button overlay scripts for the current option.
void UpdateIconOverlay(const IconOption *opt, FEObject *lock, FEObject *button);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(IconOption) == 0x4C && offsetof(IconOption, mWidth) == 0x24 && offsetof(IconOption, mId) == 0x48);
static_assert(sizeof(IconPanel) == 0x38 && offsetof(IconPanel, mpPackageName) == 0x1C && offsetof(IconPanel, mLastDir) == 0x34);
static_assert(sizeof(IconScroller) == 0xE8 && offsetof(IconScroller, mPos) == 0x90 && offsetof(IconScroller, mAlign) == 0xC0 &&
              offsetof(IconScroller, mCenterX) == 0xD4 && offsetof(IconScroller, mNextX) == 0xE0);
static_assert(sizeof(IconScrollerMenu) == 0x150 && offsetof(IconScrollerMenu, mpTitle) == 0x134 && offsetof(IconScrollerMenu, mbInputStopped) == 0x14C);
#endif
