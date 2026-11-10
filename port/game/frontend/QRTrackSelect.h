// GC UIQRTrackSelect / SelectableTrack / TrackMapScroller (zFe.cpp): quick race track select (track icons in a
// ThumbnailScroller, the track map image streamed from TRACKS\TRACKMAP%d.bin, best time / score, length, comment).
// PC differs from GC: TrackMapScroller and SetTrackMap are inlined into the screen ctor / handlers, the map texture
// is "TRACKMAP%d" (GC "TRACKMAPGC%d"), the zoom timer test divides by 0.5 (GC 1 - 2t), the screen has a scroll bar
// (mouse pages) and the online settings live at 0x8500B8 (PC 0x850078 == 8 / 0x200).
#pragma once
#include <cstddef>
#include <cstdint>
#include "MenuScreen.h"
#include "ThumbnailScroller.h"
#include "FEScrollBar.h"
struct FEObject;

// Game TrackInfo (PC TrackInfo::GetNthTrackInfo 5D3E80), only the fields the track select reads.
struct TrackInfo {
    uint8_t _unknown00[0x7C];
    int32_t mIconType;           // +0x7C CalcIconHash: 0..4 fixed icons, 5 = by race mode
    uint8_t _unknown80[8];
    uint8_t mbNotSelectable;     // +0x88 IsAvailable: never listed
    uint8_t _unknown89;
    int16_t mTrackNumber;        // +0x8A
    uint8_t _unknown8C[8];
    uint32_t mModeFlags;         // +0x94 IsAvailable: bit per race mode
    int32_t mLength;             // +0x98 metres (RefreshHeader: * 0.001 km, * 0.62137 miles)
    uint8_t _unknown9C[0x7C];
    float mMapX, mMapY;          // +0x118 map zoom centre (ZoomIn)
    float mMapZoom;              // +0x120 0: no zoom
    uint8_t mbMapZoomed;         // +0x124 zoom in at once (else after the 0.5 s timer)
};

// PC 0x30 (AddTrack 4B4C10: operator new 0x30, only +0x08 / +0x0C / +0x10 / +0x2C set). Fields +0x14..+0x28 are the
// fallback when there is no TrackInfo (never on PC).
struct SelectableTrack {
    SelectableTrack *next, *prev;   // +0x00 bNode (list head UIQRTrackSelect +0x4C)
    uint8_t mbLocked;               // +0x08 !UnlockSystem::IsTrackUnlocked
    uint8_t _pad09[3];
    int32_t _unknown0C;             // +0x0C = 0
    int32_t mDirection;             // +0x10 eTrackDirection = 0
    int32_t mTrackNumber;           // +0x14
    float mMapZoom;                 // +0x18
    uint8_t mbMapZoomed;            // +0x1C
    uint8_t _pad1D[3];
    float mMapX, mMapY;             // +0x20
    int32_t mLength;                // +0x28
    TrackInfo *mpInfo;              // +0x2C

    int32_t TrackNumber() const { return mpInfo ? mpInfo->mTrackNumber : mTrackNumber; }   // PC inlined (movsx)
    uint32_t CalcIconHash() const;  // GC 800E40FC / PC 4984B0
};

// PC 0x30, vtable 0x78F8C8 (0 scalar dtor 4B4B60). Embedded at UIQRTrackSelect +0x118; global *0x836344 = this.
struct TrackMapScroller {
    void *vtable;                // +0x00
    FEObject *mpImage;           // +0x04 the map image (0x90FE80F3)
    SelectableTrack *mpTrack;    // +0x08
    int32_t mWantedId;           // +0x0C track number to show
    int32_t mLoadingId;          // +0x10
    void *mpLoading;             // +0x14 ResourceFile being loaded
    uint8_t mbLoaded;            // +0x18 set by the load callback 4AEBF0
    uint8_t _pad19[3];
    int32_t mShownId;            // +0x1C
    void *mpShown;               // +0x20 ResourceFile of the shown map
    void *mpUnload;              // +0x24 previous map, unloaded next Update
    int32_t mZoomTimer;          // +0x28 Timer (ticks, 4000 / s)
    uint8_t mbZoomNow;           // +0x2C
    uint8_t _pad2D[3];

    void Destruct();                                     // GC 800E42E0 / PC 4985A0
    TrackMapScroller *ScalarDeletingDestructor(uint32_t f);   // PC 4B4B60 (ret 4)
    void SetTrackMap(SelectableTrack *t);                // GC 800E4380 / PC 498600 (ret 4; PC: no same-id check)
    void Update();                                       // GC 800E43DC / PC 4CDD40
    void InitZoom(float seconds);                        // GC 800E4624 / PC 4B4B80 (ret 4)
    void ZoomIn(float t);                                // GC 800E46F4 / PC 498630 (ret 4)
};
// GC 800E43D0 / PC 4AEBF0 (__cdecl): LoadResourceFile callback, param = the scroller.
void TrackMapScroller_TexturesLoadedCallback(void *scroller);

// PC 0x160 (factory 4EF9A0: operator new 0x160), vtable 0x7974F0: 0 4DF120, 1 40D080, 2 NotificationMessage 4EFA10,
// 3 NotifySoundMessage 4DF0C0.
struct UIQRTrackSelect {
    MenuScreen base;                 // +0x000
    SelectableTrack *mHead, *mTail;  // +0x04C list sentinel (next / prev)
    SelectableTrack *mpSelected;     // +0x054
    uint8_t _unknown58;              // +0x058 = 0
    uint8_t _pad59[3];
    ThumbnailScroller mScroller;     // +0x05C ("ICON_MASTER", "TRACK_ICON"; spacing 40)
    FEScrollBar mScrollBar;          // +0x0C0 ("ScrollBar")
    TrackMapScroller mMap;           // +0x118
    FEObject *_unknown148;           // +0x148 image 0x90FE80F3 (unused here)
    int32_t mScrollTime;             // +0x14C RealTimer of the last scroll (0 / 0x7FFFFFFF: none)
    int32_t mNumTracks;              // +0x150
    uint32_t mPrevMsg;               // +0x154
    uint8_t mbLeftArrow;             // +0x158 0x444969FD shown (not set by the ctor unless at the first track)
    uint8_t mbRightArrow;            // +0x159 0x444969FE shown
    uint8_t _pad15A[2];
    FEObject *mpBestLabel;           // +0x15C string 0x36227EE9

    SelectableTrack *Sentinel() { return reinterpret_cast<SelectableTrack *>(&mHead); }

    UIQRTrackSelect *Construct(const ScreenConstructorData *data);   // GC 800E4884 / PC 4E2F30 (ret 4)
    void Destruct();                                                // GC 80117560 / PC 4DF030
    UIQRTrackSelect *ScalarDeletingDestructor(uint32_t f);          // PC 4DF120 (ret 4)
    void NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg);   // GC 800E4B5C / PC 4EFA10
    uint32_t NotifySoundMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg, uint32_t event);   // GC 801175F8 / PC 4DF0C0
    void SetSelectedTrack(int32_t track, int32_t dir);              // GC 800E4DFC / PC 498740 (ret 8)
    void BuildPresetTrackList();                                    // GC 800E4E88 / PC 4CDED0
    void AddTrack(TrackInfo *info);                                 // GC 800E50F8 / PC 4B4C10 (ret 4)
    bool IsAvailable(const TrackInfo *info, int32_t dir) const;     // GC 800E51F8 / PC 4987C0 (ret 8)
    void Scroll(int32_t dir);                                       // GC 800E5334 / PC 4E3270 (ret 4)
    void RefreshHeader();                                           // GC 800E5404 / PC 4CE170
    void DisplayRaceType();                                         // GC 800E5854 / PC 4988C0
};
// PC 4987A0 (__cdecl, bList::Sort callback): 0 (b first) only when a is locked and b is not.
int32_t QRTrack_SortCompare(SelectableTrack *a, SelectableTrack *b);

#if UINTPTR_MAX == 0xFFFFFFFFu
static_assert(offsetof(TrackInfo, mTrackNumber) == 0x8A && offsetof(TrackInfo, mLength) == 0x98 &&
              offsetof(TrackInfo, mMapX) == 0x118 && offsetof(TrackInfo, mbMapZoomed) == 0x124);
static_assert(sizeof(SelectableTrack) == 0x30 && offsetof(SelectableTrack, mMapX) == 0x20 &&
              offsetof(SelectableTrack, mpInfo) == 0x2C);
static_assert(sizeof(TrackMapScroller) == 0x30 && offsetof(TrackMapScroller, mShownId) == 0x1C &&
              offsetof(TrackMapScroller, mZoomTimer) == 0x28);
static_assert(sizeof(UIQRTrackSelect) == 0x160 && offsetof(UIQRTrackSelect, mScroller) == 0x5C &&
              offsetof(UIQRTrackSelect, mScrollBar) == 0xC0 && offsetof(UIQRTrackSelect, mMap) == 0x118 &&
              offsetof(UIQRTrackSelect, mScrollTime) == 0x14C && offsetof(UIQRTrackSelect, mbLeftArrow) == 0x158 &&
              offsetof(UIQRTrackSelect, mpBestLabel) == 0x15C);
#endif
