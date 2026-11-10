// ThumbnailScroller (zFe2.cpp): a horizontal / vertical strip of thumbnail images (quick-race car select,
// magazine / part browsers). Each ThumbnailNode carries one image and its offset along the strip; the strip
// scrolls so that the selected node sits at the centre, scaling / fading the nodes by distance.
// PC layout is the GC one with the vptr moved to the front (all fields +4) plus the packed bools; the PC ctor
// also inlines the object lookups and adds the volume-fade flag.
#pragma once
#include <cstddef>
#include <cstdint>
#include "IconScroller.h"   // IconCubic (tCubic1D), thumbnail_scale, UpdateIconOverlay
#include "UIWidgetMenu.h"   // WidgetListNode
struct FEObject;

// PC 0x2C, vtable 0x79AFDC (the same base object that IconOption builds on).
struct ThumbnailNode {
    void *vtable;                    // +0x00 0x79AFDC
    WidgetListNode node;             // +0x04 list links (list sentinel is the scroller + 0x04)
    uint32_t mu32Id;                 // +0x0C item id (AddNode arg 1; FindNode key)
    FEObject *mpObject;              // +0x10 the thumbnail image
    float mOffsetX;                  // +0x14 offset along a horizontal strip
    float mOffsetY;                  // +0x18 offset along a vertical strip
    uint32_t mu32Color;              // +0x1C colour to restore (alpha is scaled while fading)
    uint8_t mbDimmed;                // +0x20 alpha forced to 70 when fading
    uint8_t mbAnimate;               // +0x21 pulse-scale the image while near the centre
    uint8_t mbShowLock;              // +0x22 lock overlay (UpdateIconOverlay)
    uint8_t mbFadeInButton;          // +0x23 button overlay fades in
    float mWidth, mHeight;           // +0x24 image size captured at AddNode
};

// PC 0x64 (embedded 0x64 apart at 55E86F: +0x4C / +0xB0; the ctor never writes past +0x60), vtable 0x79B948 (0 scalar deleting dtor 0x5385A0, base dtor 0x52EA90).
struct ThumbnailScroller {
    void *vtable;                    // +0x00
    WidgetListNode mNodes;           // +0x04 list head (sentinel)
    ThumbnailNode *mpCurrent;        // +0x0C selected node
    IconCubic *mpCubic;              // +0x10 owned 0x30-byte tCubic1D (0.3 s scroll)
    const char *mpPackage;           // +0x14 package name
    const char *mpMasterName;        // +0x18 name whose objects are the strip slots / are hidden ("car_thumb")
    FEObject *mpMaster;              // +0x1C object found by ctor arg 2 ("car_thumb3")
    FEObject *mpLock;                // +0x20 lock-overlay image (ctor arg 5)
    FEObject *mpButton;              // +0x24 button-overlay object (ctor arg 6)
    FEObject *mpNext;                // +0x28 next strip object (consumed by AddNode)
    uint8_t mbSmooth;                // +0x2C Scroll animates (SetDesiredItem) instead of snapping
    uint8_t mbVertical;              // +0x2D unused by the rewritten functions
    uint8_t mbScale;                 // +0x2E scale the images by distance from the centre
    uint8_t mbFadeAlpha;             // +0x2F fade the alpha by scale^3 (StartDim / StopDim)
    uint8_t mbHorizontal;            // +0x30 strip axis: 1 = x, 0 = y
    uint8_t mbVisible;               // +0x31 nodes inside the strip are shown
    uint8_t _pad32[2];
    int32_t mAlignment;              // +0x34 eScrollerAlignment (0 / 1 centre / 2 far edge)
    float mScroll;                   // +0x38 strip offset (target of the cubic)
    float mPulse;                    // +0x3C selected image pulse time [0, 2]
    float mOriginX, mOriginY;        // +0x40 / +0x44 strip centre used by MoveObjectsTo
    float mMasterW, mMasterH;        // +0x48 / +0x4C master size restored by ClearAll / dtor
    float mStripW, mStripH;          // +0x50 480 / +0x54 100
    float mCenterX, mCenterY;        // +0x58 / +0x5C master centre used by the scale / fade tests
    float mSpacing;                  // +0x60 gap added after every node

    // GC 80157208 / PC 5435D0 (ret 0x18): pkg, master object name, strip slot name, alignment, lock image, button.
    ThumbnailScroller *Construct(const char *pkg, const char *master, const char *slot, int32_t alignment,
                                 const char *lockImage, const char *button);
    void Destruct();                                   // GC 801573A0 / PC 52EA90
    ThumbnailScroller *ScalarDeletingDestructor(uint32_t flags);   // PC 5385A0 (ret 4)

    void ClearAll();                                   // GC 801574A0 / PC 5385C0
    void ResetNodes();                                 // GC 80157578 / PC 51F6C0
    void MoveMeOffscreen();                            // GC 801575D4 / PC 51F750
    void SetVisibility(bool visible);                  // GC 8015762C / PC 52EB90 (ret 4)
    void Scroll(int32_t dir);                          // GC 801576BC / PC 5437C0 (ret 4)
    FEObject *GetNextNode(FEObject *from);             // GC 80157744 / PC 52EC20 (ret 4)
    FEObject *AddNode(uint32_t id, bool dimmed, bool animate, bool useColor, uint32_t color);   // GC 801577BC / PC 52EC90 (ret 0x14)
    FEObject *AddNode(ThumbnailNode *node, bool useColor, uint32_t color);   // GC 80157910 / PC 52ED80 (ret 0xC)
    void SetDesiredItem(uint32_t id);                  // GC 80157A58 / PC 52EE40 (ret 4)
    void MoveObjectsTo(float scroll);                  // GC 80157EFC / PC 52EEA0 (ret 4)
    void SnapToNode(ThumbnailNode *node);              // GC 80157AF4 / PC 5386A0 (ret 4)
    void SnapToItem(uint32_t id);                      // GC 80157B8C / PC 538700 (ret 4)
    void SnapToHead();                                 // GC 80157BD8 / PC 538740
    void Update();                                     // GC 80157BFC / PC 538760
    void StartDim();                                   // GC 80157E24 / PC 5388F0
    void StopDim();                                    // GC 80157E50 / PC 538900
    ThumbnailNode *FindNode(uint32_t id);              // GC 801583E8 / PC 51F7A0 (ret 4)
    int32_t GetCurrenNodeIndex() const;                // GC 80158418 / PC 51F7E0 (1-based; 0 = none)
};

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(sizeof(ThumbnailNode) == 0x2C && offsetof(ThumbnailNode, mWidth) == 0x24);
static_assert(sizeof(ThumbnailScroller) == 0x64 && offsetof(ThumbnailScroller, mpCurrent) == 0x0C &&
              offsetof(ThumbnailScroller, mpNext) == 0x28 && offsetof(ThumbnailScroller, mbVertical) == 0x2D &&
              offsetof(ThumbnailScroller, mbHorizontal) == 0x30 && offsetof(ThumbnailScroller, mAlignment) == 0x34 &&
              offsetof(ThumbnailScroller, mOriginX) == 0x40 && offsetof(ThumbnailScroller, mStripW) == 0x50 &&
              offsetof(ThumbnailScroller, mCenterX) == 0x58 && offsetof(ThumbnailScroller, mSpacing) == 0x60);
#endif
