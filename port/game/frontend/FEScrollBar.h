// GC FEScrollBar (zFe2.cpp; nfsu2-re "ScrollingThing"): list scroll bar built from the package images
// "<base>_Backing", "_Handle", "_Arrow_1/2", "_Backing_End_1/2". PC 0x58 bytes (GC 0x64: ints instead of bools at
// the start, so floats are GC - 0xC and objects GC - 0xC).
#pragma once
#include <cstdint>

struct FEObject;

struct FEScrollBar {
    uint8_t mbVertical;        // +0x00 (GC +0x00) handle moves in y (else x)
    uint8_t mbResized;         // +0x01 (GC +0x04) handle size follows visible / total
    uint8_t mbNoMove;          // +0x02 (GC +0x08) PC: the handle is being dragged (Update doesn't move it then)
    uint8_t mbMouseDisabled;   // +0x03 (GC +0x0C) 5th ctor argument: HandleMouse skips clicks
    float mDragStartX;         // +0x04 (GC +0x10) mouse position when the drag started (FEng coordinates)
    float mDragStartY;         // +0x08 (GC +0x14)
    float mHandleX, mHandleY;  // +0x0C (GC +0x18) last computed handle position (axis of travel only)
    float mGrabX, mGrabY;      // +0x14 (GC +0x20) |handle top-left - drag start| at the click
    float mBackingX, mBackingY;   // +0x1C (GC +0x28) backing top-left
    float mBackingW, mBackingH;   // +0x24 (GC +0x30) backing size
    float mHandleW, mHandleH;     // +0x2C (GC +0x38) handle size
    float mStep;               // +0x34 (GC +0x40) travel per item (not set by the ctor)
    int32_t mArrowTime;        // +0x38 (GC +0x44) RealTimer of the last arrow click (PC)
    int32_t mHit;              // +0x3C (GC +0x48) PC mouse result: -1/1 arrow or drag step, -10/10 page, -9 none
    FEObject *mpBacking;       // +0x40 (GC +0x4C)
    FEObject *mpHandle;        // +0x44
    FEObject *mpArrow1;        // +0x48
    FEObject *mpArrow2;        // +0x4C
    FEObject *mpBackingEnd1;   // +0x50
    FEObject *mpBackingEnd2;   // +0x54

    // GC 801554B4 / PC 542AC0 (ret 0x14). Null `base`: everything 0. Resized: handle size = backing width
    // (vertical) or height, both axes.
    FEScrollBar *Construct(const char *pkg, const char *base, bool vertical, bool resized, bool arg5);
    // GC 80155720 / PC 538080 (ret 0xC): `visible` < `total` (and total != 0): show the bar (handle only if
    // `index` != 0), then position the handle for the 1-based `index`; else hide handle and arrows.
    void Update(int32_t visible, int32_t total, int32_t index);
    // GC 801557FC / PC 52E200: handle length = visible/total of the backing, start = (index-1) * step.
    void SetPosResized(int32_t visible, int32_t total, int32_t index);
    // GC 80155A30 / PC 52E320: fixed handle centred at the ends for index 1 / total, else (index-1) * step.
    // PC computes mStep always and checks mbNoMove only for the move (GC returns early on it).
    void SetPosNonResized(int32_t visible, int32_t total, int32_t index);
    void SetArrowVisibility(int32_t arrow, bool visible);                    // GC 80155BB0 / PC 50D820: 1 or 2
    void SetBackingVisibility(bool visible);                                 // GC 80155C20 / PC 50D880
    // PC-only mouse handling (screens read mHit afterwards)
    void HandleMouse();        // PC 538130: handle click starts a drag, backing click pages; arrows repeat after 0.0625 s
    void ArrowClick();         // PC 52E470: arrow 1 -> mHit -1 (sound event 4), arrow 2 -> 1 (event 5)
    void Drag();               // PC 52E4E0: handle follows the mouse inside the backing; mHit +-1 per step moved
    void PageClick();          // PC 52E670: click above / left of the handle -> 10, below / right -> -10
    void BlinkArrow1();        // PC 495810: "Blink" on arrow 1 unless already blinking
    void BlinkArrow2();        // PC 495850
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(FEScrollBar) == 0x58, "FEScrollBar size");
static_assert(__builtin_offsetof(FEScrollBar, mStep) == 0x34 && __builtin_offsetof(FEScrollBar, mpBacking) == 0x40, "FEScrollBar layout");
#endif

// PC-only helpers (GC inlines them). All __cdecl.
FEObject *FEngFindImage(const char *pkg, const char *name);   // PC 4953E0: FEngFindImage(pkg, FEHashUpper(name))
void FEngSetWidth(FEObject *obj, float w);                    // PC 495590: FEngSetSize(obj, w, current height)
void FEngSetHeight(FEObject *obj, float h);                   // PC 525DB0: FEngSetSize(obj, current width, h)
