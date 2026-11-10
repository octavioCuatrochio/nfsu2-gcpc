// GC UIWidgetMenu (zFe2.cpp): a MenuScreen holding a scrolling list of FEWidgets (options screens). PC 0x14C bytes,
// vtable 0x795CDC: 0 scalar dtor 4CBC90, 1 MaybeOverrideActiveButtons 40D080, 2 NotificationMessage 556020,
// 3 NotifySoundMessage 570A70, 4 Setup (empty 4022C0). Fields: GC - 4 up to the scroll bar, GC - 0x10 after it.
#pragma once
#include <cstddef>
#include <cstdint>
#include "MenuScreen.h"
#include "FEScrollBar.h"

struct FEObject;
struct FEWidget;
struct FEButtonWidget;
struct FEStatWidget;
struct FEToggleWidget;
struct FEToggleImageWidget;
struct FESliderWidget;

// Intrusive list node: FEWidget +0x04 / +0x08 (after the vtable); the list stores node pointers.
struct WidgetListNode {
    WidgetListNode *next, *prev;
};

struct UIWidgetMenu {
    MenuScreen base;                 // +0x000
    WidgetListNode mWidgets;         // +0x04C list head (GC +0x50)
    FEWidget *mpCurrent;             // +0x054 selected widget (GC +0x58)
    FEWidget *mpViewTop;             // +0x058 first visible widget (GC +0x5C)
    FEObject *mpTitleMaster;         // +0x05C "TITLE_MASTER"
    FEObject *mpDataMaster;          // +0x060 "DATA_MASTER"
    FEObject *mpPrevObject;          // +0x064 StorePrevNotification object (GC +0x68)
    FEObject *mpDoneButton;          // +0x068 "DONE_BUTTON" (GC: FEngFindString; PC: FEngFindObject)
    FEScrollBar mScrollBar;          // +0x06C "scrollbar" (GC +0x70, 0xC bytes bigger)
    const char *mOptionName, *mOptionData, *mOptionImage, *mOptionBacking;   // +0x0C4 object name prefixes
    const char *mLeftArrow, *mRightArrow, *mSliderBase, *mSliderKnob;      // +0x0D4
    const char *mSliderValue, *mSliderHandle, *mSliderBeg, *mSliderEnd;    // +0x0E4
    float mTitleX, mTitleY;          // +0x0F4 TITLE_MASTER top-left (GC +0x104)
    float mStartX, mStartY;          // +0x0FC copies (IncrementStartPos moves Y)
    float mDataOffsetX;              // +0x104 |titleX - (dataW + dataX)|
    float mRowHeight;                // +0x108 = title height
    float mTitleW, mTitleH;          // +0x10C 175, 24 until SetInitialPositions
    float mDataW, mDataH;            // +0x114 175, 24
    float mDataX, mDataY;            // +0x11C DATA_MASTER top-left
    float _unknown124;               // +0x124 4.0 (GC +0x134)
    float mSpacing;                  // +0x128 extra row gap (GC +0x138)
    uint32_t mCurrentIndex;          // +0x12C 1-based (GC +0x13C)
    uint32_t mScrollPos;             // +0x130 1-based, kept in [1, mCurrentIndex - 1] by Scroll (GC +0x140)
    uint32_t mVisibleCount;          // +0x134 7
    uint32_t mPrevMsg;               // +0x138 StorePrevNotification (GC +0x148)
    uint32_t mPrevArg;               // +0x13C
    uint32_t mPrevPkg;               // +0x140
    uint8_t _unknown144, _unknown145, _unknown146;   // +0x144 0, 0, 1 (GC int fields)
    uint8_t mbViewDirty;             // +0x147 cleared by Reset / Sync (GC +0x160)
    uint8_t _unknown148, _unknown149;   // +0x148 1, 1
    uint8_t _pad14A[2];

    UIWidgetMenu *Construct(const ScreenConstructorData *data);   // GC 8015AB90 / PC 552780 (ret 4)
    void DestructBody();                                          // GC 8018C11C / PC 4CBC30 (non-deleting)
    UIWidgetMenu *ScalarDeletingDestructor(uint32_t flags);       // PC 4CBC90 (ret 4)

    void RefreshWidgets();                 // GC 8015C5AC / PC 51FF40: Draw (slot 3) on every widget
    void SetInitialPositions();            // GC 8015C7E8 / PC 51FF80
    uint32_t GetWidgetIndex(const FEWidget *w) const;   // GC 8015CAFC / PC 520020 (ret 4): 1-based, 0 = absent
    void ClearWidgets();                   // GC 8015C4D4 / PC 52FD40: hide + unfocus all, delete all, SetInitialPositions
    void Reposition();                     // GC 8015C888 / PC 52FDB0: visible window placed from mTitleY, others at y 6969
    void Reset();                          // GC 8015C998 / PC 52FE60: select + view the first widget
    void SyncViewToSelection();            // GC 8015CA2C / PC 52FEA0
    // GC 8015B430 / PC 5390E0 (ret 4): dir 1 = next enabled widget (after the last: DONE_BUTTON if any), else
    // previous; the view follows one row at a time (Reposition each step). PC differs from GC (no lookahead).
    void Scroll(int32_t dir);
    void ScrollWrapped(int32_t dir);       // GC 8015B6B0 / PC 539370 (ret 4): like Scroll but wraps around
    // Option rows: objects are named <prefix><mCurrentIndex> ("OPTION_NAME_3"). Each Add appends the widget, advances the
    // start position, Disable (if not enabled) / Show / Position / Draw, and returns the new row's index (0-based).
    FEObject *GetCurrentFEObject(const char *prefix);   // GC 8015C468 / PC 5445F0 (ret 4)
    FEObject *GetCurrentFEString(const char *prefix);   // GC 8015C390 (PC: inlined, 4953C0)
    FEObject *GetCurrentFEImage(const char *prefix);    // GC 8015C3FC (PC: inlined, 4953E0)
    uint32_t AddButtonOption(FEButtonWidget *w);        // GC 8015B9B8 / PC 54EE40 (ret 4)
    uint32_t AddStatOption(FEStatWidget *w);            // GC 8015BAA8 / PC 54F820 (ret 4)
    // style: 1 -> arrows 0x9943D584, 2/3 -> 0x374F6465 / 0x760BDC9D, else 0x367B3E04 (texture hashes)
    uint32_t AddToggleOption(FEToggleWidget *w, int32_t style);            // GC 8015BBE4 / PC 54EF30 (ret 8)
    uint32_t AddToggleOption(FEToggleWidget *w, bool plain);               // GC 8015BDB0 / PC 54F170: style plain ? 0 : 2
    uint32_t AddToggleImageOption(FEToggleImageWidget *w, int32_t style);  // GC 8015BDDC / PC 54F190 (ret 8)
    uint32_t AddSliderOption(FESliderWidget *w, int32_t style);            // GC 8015BFB8 / PC 54F410 (ret 8)
    uint32_t AddSliderOption(FESliderWidget *w, bool plain);               // GC 8015C228 / PC 54F800
    FEWidget *GetWidget(uint32_t index1);  // GC 8015B3F8 (PC: inlined): 0 -> null, else bList::GetNode(index - 1)
    void IncrementStartPos();              // GC 8015CA04 (PC: inlined)
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(UIWidgetMenu) == 0x14C && offsetof(UIWidgetMenu, mScrollBar) == 0x6C && offsetof(UIWidgetMenu, mOptionName) == 0xC4);
static_assert(offsetof(UIWidgetMenu, mTitleX) == 0xF4 && offsetof(UIWidgetMenu, mCurrentIndex) == 0x12C && offsetof(UIWidgetMenu, mbViewDirty) == 0x147);
#endif

// bTList<T>::DeleteAllElements (PC 4358E0, __thiscall on the head): unlink each node and delete its owner (node at
// owner + 4, virtual scalar dtor slot 0 with 1). Used for widget lists and others with the same shape.
void DeleteAllNodeOwners(WidgetListNode *head);
// bList::GetNode (PC 43BE60, __thiscall, ret 4): the index-th node (0-based); past the end -> the head itself.
WidgetListNode *ListGetNode(WidgetListNode *head, uint32_t index);
