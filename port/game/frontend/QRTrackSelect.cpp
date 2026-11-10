#include "QRTrackSelect.h"
#include <cstdio>
#include "bridge/Bridge.h"
#include "bWare/bMath.h"
#include "bWare/bString.h"
#include "FEngUtil.h"
#include "FEMath.h"
#include "FEHash.h"
#include "FEObject.h"
#include "FEPackageManager.h"
#include "MainMenu.h"      // FEngMapJoyParamToJoyport
#include "OptionsMain.h"   // FilterSoundForHorizontalMenu

namespace {
constexpr uint32_t kMapImage = 0x90FE80F3u, kLeftArrow = 0x444969FDu, kRightArrow = 0x444969FEu;
constexpr uint32_t kOutObject = 0x47FF4E7Cu, kOutScript = 0xDE6EFF34u;
constexpr uint32_t kTrackMapVTable = 0x78F8C8u, kScreenVTable = 0x7974F0u;
const char kViewTrackPkg[] = "UI_OL_ViewTrack.fng";   // PC 0x78F8D0: online "view track" reuses the screen

bool Online()
{
    const int32_t f = bridge::GameModeFlags();
    return f == 8 || f == 0x200;
}
// PC inlined everywhere: online race mode *0x850088, else the quick race mode *0x83AAB4.
int32_t RaceMode() { return Online() ? *bridge::GameInt(0x850088) : *bridge::GameInt(0x83AAB4); }
uint16_t *OnlineTrack() { return reinterpret_cast<uint16_t *>(bridge::GameByte(0x8500B8)); }
int32_t *OnlineDirection() { return bridge::GameInt(0x8500BC); }

// The track settings write that SetSelectedTrack does (inlined by BuildPresetTrackList with the id as a word).
void StoreTrack(uint16_t track, int32_t dir)
{
    if (FEngFindPackage(kViewTrackPkg)) return;
    if (!Online()) {
        uint8_t *s = bridge::QR_CurrentTrackSettings();
        *reinterpret_cast<uint16_t *>(s) = track;
        *reinterpret_cast<int32_t *>(s + 4) = dir;
        return;
    }
    *OnlineDirection() = dir;
    *OnlineTrack() = track;
}

const float *MapPos(const SelectableTrack *t) { return t->mpInfo ? &t->mpInfo->mMapX : &t->mMapX; }
float MapZoom(const SelectableTrack *t) { return t->mpInfo ? t->mpInfo->mMapZoom : t->mMapZoom; }
uint8_t MapZoomed(const SelectableTrack *t) { return t->mpInfo ? t->mpInfo->mbMapZoomed : t->mbMapZoomed; }
}  // namespace

// ---- SelectableTrack ----

uint32_t SelectableTrack::CalcIconHash() const
{
    if (mbLocked) return 0xF05704ACu;
    switch (static_cast<uint32_t>(mpInfo->mIconType)) {
    case 0: return 0xB5A633A0u;
    case 1: return 0x79CEAD40u;
    case 2: return 0x4B938E3Cu;
    case 3: return 0x4FDE6FEBu;
    case 4: return 0x6BA31820u;
    case 5:
        switch (RaceMode()) {
        case 2: return bridge::FEngHashStringUpper("Track_Icon_Drag");
        case 3: return bridge::FEngHashStringUpper("Track_Icon_Drift");
        case 4: return bridge::FEngHashStringUpper("Track_Icon_U_Racing_League");
        case 10: return bridge::FEngHashStringUpper("Track_Icon_Short_Track");
        default: break;
        }
        break;
    default: break;
    }
    return 0x74CE8C0Bu;
}

int32_t QRTrack_SortCompare(SelectableTrack *a, SelectableTrack *b)
{
    return (a->mbLocked && !b->mbLocked) ? 0 : 1;
}

// ---- TrackMapScroller ----

void TrackMapScroller_TexturesLoadedCallback(void *scroller) { static_cast<TrackMapScroller *>(scroller)->mbLoaded = 1; }

void TrackMapScroller::Destruct()
{
    vtable = bridge::GameVTable(kTrackMapVTable);
    *bridge::TrackMapScrollerInstance() = nullptr;
    if (mpLoading)
        while (!mbLoaded) bridge::ServiceResourceLoading();
    if (mpLoading) bridge::UnloadResourceFile(mpLoading);
    if (mpShown) bridge::UnloadResourceFile(mpShown);
    if (mpUnload) bridge::UnloadResourceFile(mpUnload);
}

TrackMapScroller *TrackMapScroller::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void TrackMapScroller::SetTrackMap(SelectableTrack *t)
{
    if (!t) return;
    mpTrack = t;
    mWantedId = t->TrackNumber();
}

void TrackMapScroller::ZoomIn(float t)
{
    const SelectableTrack *tr = mpTrack;
    const float *pos = MapPos(tr);
    const float x = pos[0], y = pos[1];
    const float zoom = MapZoom(tr);
    const long double s = bSinF(static_cast<float>(static_cast<long double>(t) * 1.5707964f));   // PC 0x784254
    const float xs = static_cast<float>(x * s);
    const float ys = static_cast<float>(y * s);
    const float size = static_cast<float>(512.0L - (512.0L - zoom) * s);   // PC 0x78A18C
    constexpr float kInv512 = 0.001953125f;                                // PC 0x78F8CC
    FEVector2 v;
    v.x = static_cast<float>(static_cast<long double>(xs) * kInv512);
    v.y = static_cast<float>(static_cast<long double>(ys) * kInv512);
    mpImage->SetTrackValue(5, v, false);   // top-left UV
    mpImage->mu32Flags |= 0x400000u;
    v.x = static_cast<float>((static_cast<long double>(xs) + size) * kInv512);
    v.y = static_cast<float>((static_cast<long double>(ys) + size) * kInv512);
    mpImage->SetTrackValue(6, v, false);   // bottom-right UV
    mpImage->mu32Flags |= 0x400000u;
}

void TrackMapScroller::InitZoom(float seconds)
{
    const SelectableTrack *tr = mpTrack;
    if (MapZoom(tr) == 0.0f) {   // PC 0x7D7D3C
        mZoomTimer = 0;
        ZoomIn(0.0f);
        return;
    }
    if (!MapZoomed(tr)) {
        mZoomTimer = static_cast<int32_t>(static_cast<long double>(seconds) * 4000.0f + 0.5f);   // _ftol
        mbZoomNow = 0;
        ZoomIn(0.0f);
        return;
    }
    mbZoomNow = 1;
    mZoomTimer = 0;
}

void TrackMapScroller::Update()
{
    if (mpUnload) {
        bridge::UnloadResourceFile(mpUnload);
        mpUnload = nullptr;
    }
    if (mbLoaded) {
        mbLoaded = 0;
        if (mLoadingId == mWantedId) {
            if (mpShown) mpUnload = mpShown;
            mpShown = mpLoading;
            mShownId = mLoadingId;
            mpLoading = nullptr;
            mLoadingId = 0;
            FEngSetVisible(mpImage);
            const uint32_t tex = bridge::FEngHashStringInt("TRACKMAP%d", mShownId);
            bridge::GetTextureInfo(tex, 0, 0);
            FEngSetTextureHash(mpImage, tex);
            InitZoom(0.5f + 0.5f);   // PC 0x78DBCC + 0x78DBD0
            if (MapZoomed(mpTrack)) ZoomIn(1.0f);
        } else {
            bridge::UnloadResourceFile(mpLoading);
            mpLoading = nullptr;
            mLoadingId = 0;
        }
    }
    if (!mpLoading && mWantedId != 0 && mWantedId != mShownId) {
        char name[32];
        std::snprintf(name, sizeof name, "TRACKS\\TRACKMAP%d.bin", mWantedId);   // PC bSPrintf 4400D0
        mLoadingId = mWantedId;
        mpLoading = bridge::LoadResourceFile(name, 2, 0, &TrackMapScroller_TexturesLoadedCallback, this, 0, 0);
    }
    if (mShownId == 0 || mShownId != mWantedId) return;
    if (mbZoomNow) {
        ZoomIn(1.0f);
        mbZoomNow = 0;
        return;
    }
    if (!bridge::Timer_CountDown(&mZoomTimer, bridge::RealTimeElapsed())) return;
    const long double t = static_cast<long double>(mZoomTimer) * 0.00025f;   // PC 0x784268
    if (t <= 0.5f) ZoomIn(static_cast<float>(1.0f - t / 0.5f));             // PC 0x784250, 0x78DBCC
}

// ---- UIQRTrackSelect ----

UIQRTrackSelect *UIQRTrackSelect::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.vtable = bridge::GameVTable(kScreenVTable);
    mHead = mTail = Sentinel();
    mpSelected = nullptr;
    _unknown58 = 0;
    mScroller.Construct(base.mpPackageName, "ICON_MASTER", "TRACK_ICON", 1, "", "");
    mScrollBar.Construct(base.mpPackageName, "ScrollBar", false, false, false);

    mMap.Construct(FEngFindImage(base.mpPackageName, kMapImage));

    _unknown148 = FEngFindImage(base.mpPackageName, kMapImage);
    mScrollTime = 0;
    mNumTracks = 0;
    mPrevMsg = 0;
    bridge::QR_CurrentTrackSettings()[8] = 0;
    mScroller.mSpacing = 40.0f;
    BuildPresetTrackList();

    const int32_t mode = bridge::GameModeFlags();
    if (mode == 8 || mode == 0x200) {
        base.mHelpHash = 0x27329D35u;
        FEngSetLanguageHash(FEngFindString(base.mpPackageName, 0x42ADB44Cu),
                            bridge::GameModeFlags() == 0x200 ? 0x766A0EDFu : 0x3BC6066Du);
        if (bridge::GameModeFlags() == 8) FEngSetScript(base.mpPackageName, "EA_MESSENGER_GROUP", "Show", true);
        FEngSetScript(base.mpPackageName, 0x2BAC0CEEu, "Show", true);
        FEngSetVisible(FEngFindObject(base.mpPackageName, 0x4FD28546u));
        FEngSetInvisible(FEngFindObject(base.mpPackageName, 0xEB406FECu));
        if (bStrCmp(base.mpPackageName, kViewTrackPkg) != 0)
            bridge::HelpBar_SyncByMask(base.mData.mHelpBarMask | 0x4000u, base.mpPackageName);
    } else {
        FEngSetLanguageHash(FEngFindString(base.mpPackageName, 0x42ADB44Cu), mode == 4 ? 0x1BF733BDu : 0xE5D63BB5u);
        FEngSetScript(base.mpPackageName, 0x2BAC0CEEu, "Hide", true);
        FEngSetInvisible(FEngFindObject(base.mpPackageName, 0x4FD28546u));
    }

    if (mpSelected == mHead) {
        FEngSetInvisible(FEngFindObject(base.mpPackageName, kLeftArrow));
        mbLeftArrow = 0;
    } else if (mpSelected == mTail) {
        FEngSetInvisible(FEngFindObject(base.mpPackageName, kRightArrow));
        mbRightArrow = 0;
    }
    mpBestLabel = FEngFindString(base.mpPackageName, 0x36227EE9u);
    if (mpSelected) {   // TrackMapScroller::SetTrackMap (PC inlined)
        mMap.mpTrack = mpSelected;
        mMap.mWantedId = mpSelected->TrackNumber();
    }
    mScrollTime = 0;
    RefreshHeader();
    return this;
}

void UIQRTrackSelect::Destruct()
{
    base.vtable = bridge::GameVTable(kScreenVTable);
    mMap.Destruct();
    mScroller.Destruct();
    SelectableTrack *s = Sentinel();
    for (SelectableTrack *t = mHead; t != s; t = mHead) {   // bList dtor: unlink + delete each node
        t->prev->next = t->next;
        t->next->prev = t->prev;
        bridge::FreeArray(t);
    }
    base.vtable = bridge::MenuScreenVTable();   // MenuScreen dtor 510D40
}

UIQRTrackSelect *UIQRTrackSelect::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void UIQRTrackSelect::SetSelectedTrack(int32_t track, int32_t dir) { StoreTrack(static_cast<uint16_t>(track), dir); }

bool UIQRTrackSelect::IsAvailable(const TrackInfo *info, int32_t dir) const
{
    if (!info) return false;
    const uint32_t f = info->mModeFlags;
    int32_t mode = *bridge::GameInt(0x83AAB4);
    if (Online()) {
        if (info->mTrackNumber == 4000 || info->mTrackNumber == 0x12C1) return false;
        mode = *bridge::GameInt(0x850088);
    }
    if (info->mbNotSelectable) return false;
    switch (static_cast<uint32_t>(mode)) {
    case 0: return f & 1;
    case 1: return (f >> 2) & 1;
    case 2: return (f & 2) && dir == 0;
    case 3: return (f & 8) && dir == 0;
    case 4: return (f >> 13) & 1;
    case 5: return (f >> 12) & 1;
    case 8:
    case 9: return (f >> 4) & 1;
    case 10: return (f >> 11) & 1;
    case 12: return (f & 5) != 0;
    default: return false;
    }
}

void UIQRTrackSelect::AddTrack(TrackInfo *info)
{
    auto *t = static_cast<SelectableTrack *>(bridge::Alloc(sizeof(SelectableTrack)));
    if (t) {
        t->mbLocked = 0;
        t->mpInfo = info;
        t->_unknown0C = 0;
        t->mDirection = 0;
    }
    SelectableTrack *tail = mTail;   // bList::AddTail
    tail->next = t;
    mTail = t;
    t->prev = tail;
    t->next = Sentinel();
    const int32_t filter = Online() ? 0x1F8 : 1;   // eUnlockFilters
    t->mbLocked = !bridge::UnlockSystem_IsTrackUnlocked(filter, info->mTrackNumber, 0, 0);
    ++mNumTracks;
}

void UIQRTrackSelect::BuildPresetTrackList()
{
    for (int32_t i = 0; i < *bridge::GameInt(0x883DA4); ++i) {   // number of TrackInfos
        TrackInfo *info = bridge::TrackInfo_GetNth(i);
        if (IsAvailable(info, 0)) AddTrack(info);
    }
    SelectableTrack *const s = Sentinel();
    bridge::bList_Sort(&mHead, reinterpret_cast<int32_t (*)(void *, void *)>(&QRTrack_SortCompare));
    for (SelectableTrack *t = mHead; t != s; t = t->next) {
        FEObject *img = mScroller.AddNode(static_cast<uint32_t>(t->TrackNumber()), false, true, false, 0);
        uint32_t tex = t->CalcIconHash();
        if (!bridge::GetTextureInfo(tex, 0, 0)) tex = 0x74CE8C0Bu;
        FEngSetTextureHash(img, tex);
    }

    if (bridge::GameModeFlags() == 8) mpSelected = mHead;
    const uint16_t *settings = GetSelectedTrack();
    const uint16_t wanted = settings[0];
    const int32_t dir = *reinterpret_cast<const int32_t *>(settings + 2);
    if (bridge::GameModeFlags() != 8) {
        if (wanted == 0) {
            if (!mpSelected) mpSelected = mHead;
            StoreTrack(static_cast<uint16_t>(mpSelected->TrackNumber()), 0);
        } else {
            SelectableTrack *first = mHead, *found = FindTrack(wanted);
            mpSelected = found;
            if (!found || (found->mbLocked && !Online())) {
                mpSelected = first;
                StoreTrack(static_cast<uint16_t>(first->TrackNumber()), dir);
            }
        }
    }
    mScroller.StopDim();
    mScroller.SnapToItem(static_cast<uint32_t>(mpSelected->TrackNumber()));
    StoreTrack(static_cast<uint16_t>(mpSelected->TrackNumber()), dir);
}

void UIQRTrackSelect::Scroll(int32_t dir)
{
    SelectableTrack *t = mpSelected;
    if (dir == -1) {
        if (t != mHead) t = t->prev;
    } else if (dir == 1 && t != mTail) {
        t = t->next;
    }
    if (t == mpSelected) return;
    mpSelected = t;
    SetSelectedTrack(t->TrackNumber(), t->mDirection);
    mScroller.SetDesiredItem(static_cast<uint32_t>(mpSelected->TrackNumber()));
    RefreshHeader();
    mScrollTime = bridge::RealTimer();
}

void UIQRTrackSelect::DisplayRaceType()
{
    uint32_t hash;
    switch (static_cast<uint32_t>(RaceMode())) {
    case 1: hash = 0xDCB9F861u; break;
    case 2: hash = 0x35102EDFu; break;
    case 3: hash = 0xD7162CFAu; break;
    case 4: hash = 0x75F903F4u; break;
    case 9: hash = 0x6F2421B7u; break;
    case 10: hash = 0x30DA61E1u; break;
    default: hash = 0x93FDEE74u; break;   // 0 and everything else
    }
    char buf[0x20];
    bridge::GetLocalizedStringBuf(buf, sizeof buf, hash);
    bridge::FEngPrintfString(base.mpPackageName, 0x15E1351Bu, buf);   // the text is the format
}

void UIQRTrackSelect::RefreshHeader()
{
    const char *pkg = base.mpPackageName;
    SelectableTrack *cur = mpSelected;
    FEngSetLanguageHash(FEngFindString(pkg, 0xF601F2D4u), cur->CalcLanguageHash());
    FEngSetInvisible(FEngFindString(base.mpPackageName, 0x53C956C4u));

    mScrollBar.Update(1, mNumTracks, GetNodeIndex(mpSelected));

    FEObject *arrow1 = FEngFindObject(base.mpPackageName, FEHashUpper("ScrollBar_Arrow_1"));
    FEObject *arrow2 = FEngFindObject(base.mpPackageName, FEHashUpper("ScrollBar_Arrow_2"));
    if (mpSelected == mHead) {
        if (mbLeftArrow) {
            FEngSetInvisible(FEngFindObject(base.mpPackageName, kLeftArrow));
            mbLeftArrow = 0;
        }
    } else if (mpSelected == mTail) {
        if (mbLeftArrow) {   // PC tests the left flag here (GC too), then clears the right one
            FEngSetInvisible(FEngFindObject(base.mpPackageName, kRightArrow));
            mbRightArrow = 0;
        }
    } else {
        if (!mbLeftArrow) {
            FEngSetVisible(FEngFindObject(base.mpPackageName, kLeftArrow));
            mbLeftArrow = 1;
        } else if (!mbRightArrow) {
            FEngSetVisible(FEngFindObject(base.mpPackageName, kRightArrow));
            mbRightArrow = 1;
        }
        if (!FEngIsScriptRunning(arrow1, "Blink")) FEngSetScript(arrow1, "Static", true);
        if (!FEngIsScriptRunning(arrow2, "Blink")) FEngSetScript(arrow2, "Static", true);
    }
    DisplayRaceType();

    // length: metres * 0.001 (km), * 0.62137 more when the speed units byte 0x83AA1A is 0 (miles)
    const bool miles = *bridge::GameByte(0x83AA1A) == 0;
    const int32_t metres = mpSelected->mpInfo ? mpSelected->mpInfo->mLength : mpSelected->mLength;
    float length = static_cast<float>(static_cast<long double>(metres) * 0.001f);   // PC 0x78435C
    if (miles) length = static_cast<float>(static_cast<long double>(length) * 0.62137f);   // PC 0x78F754
    const char *unit = bridge::LocalizedString(miles ? 0xAC3A4F4Cu : 0xB5D640B1u);
    bridge::FEngPrintfFloatString(base.mpPackageName, 0x080C9DAAu, "%$.2f %s", length, unit);

    const int32_t mode = RaceMode();
    if (mode == 3) {
        FEngSetLanguageHash(mpBestLabel, 0x3B7B9567u);
        const int32_t score = bridge::HighScores_FindBestDriftScore(mpSelected->TrackNumber(), mpSelected->mDirection);
        bridge::FEngPrintfInt(base.mpPackageName, 0x0555EECAu, "%$d", score);
    } else {
        FEngSetLanguageHash(mpBestLabel, (mode == 1 || mode == 2) ? 0xDB0C5EFDu : 0x3B4A1ECBu);
        const int32_t best = bridge::HighScores_FindBestLapTime(mpSelected->TrackNumber(), mpSelected->mDirection);
        char time[0x20];
        bridge::Timer_PrintToString(&best, time, 0);
        bridge::FEngPrintfString(base.mpPackageName, 0x0555EECAu, "%s", time);
    }
    char fmt[0x200];
    bridge::GetLocalizedStringBuf(fmt, sizeof fmt, 0x8D9B9EC7u);
    const char *comment = bridge::LocalizedString(bridge::FEngHashStringInt("QR_TRACK_%d_COMMENT", mpSelected->TrackNumber()));
    bridge::FEngPrintfString(base.mpPackageName, 0x258E6A24u, fmt, comment);
}

void UIQRTrackSelect::NotificationMessage(uint32_t msg, FEObject *, uint32_t arg, uint32_t)
{
    const char *pkg = base.mpPackageName;
    switch (msg) {
    case 0x9120409Eu:   // left (PC inlines Scroll(-1))
        Scroll(-1);
        return;
    case 0xB5971BF1u:   // right
        Scroll(1);
        return;
    case 0x35F8620Bu:
        mScroller.mbFadeAlpha = 1;
        if (mpSelected == mHead) {
            FEngSetInvisible(pkg, kLeftArrow);
            mbLeftArrow = 0;
        }
        return;
    case 0x0C407210u:   // accept: sound 4, then as 0x406415E3
        bridge::PlayUISound(4);
        [[fallthrough]];
    case 0x406415E3u: {
        if (FEngMapJoyParamToJoyport(arg) >= 0) bridge::SetMenuJoyPort(FEngMapJoyParamToJoyport(arg));
        if (mHead == Sentinel() || mpSelected->mbLocked) return;
        FEngSetScript(pkg, kOutObject, "FORWARD", true);
        mPrevMsg = msg;
        return;
    }
    case 0x9803F6E2u:   // mouse
        mScrollBar.HandleMouse();
        switch (mScrollBar.mHit) {
        case -10:
            for (int32_t i = 0; i < *bridge::GameInt(0x7FA1A8); ++i) Scroll(1);
            return;
        case 10:
            for (int32_t i = 0; i < *bridge::GameInt(0x7FA1A8); ++i) Scroll(-1);
            return;
        case -1:
        case 1:
            Scroll(mScrollBar.mHit);
            RefreshHeader();
            return;
        default: return;
        }
    case 0xC98356BAu: {   // tick
        mScroller.Update();
        const int32_t t = mScrollTime;
        if (t == 0 || t == 0x7FFFFFFF) {
            mMap.Update();
            return;
        }
        if (static_cast<long double>(bridge::RealTimer() - t) * 0.00025f < 0.3f) return;   // PC 0x784268, 0x78758C
        mMap.SetTrackMap(mpSelected);
        mScrollTime = 0;
        return;
    }
    case 0xE1FDE1D1u:   // out script finished
        if (mPrevMsg == 0x0C407210u || mPrevMsg == 0x406415E3u) bridge::QRManager_ChangeToNextScreen();
        else if (mPrevMsg == 0x911AB364u) bridge::QRManager_RestorePreviousStable();
        break;
    case 0x911AB364u:   // back
        break;
    default: return;
    }
    // back / out finished: remember the message, play the out script once
    mPrevMsg = msg;
    if (!FEngIsScriptRunning(pkg, kOutObject, kOutScript)) {
        FEngSetScript(pkg, kOutObject, kOutScript, true);
        bridge::PlayUISound(5);
    }
}

uint32_t UIQRTrackSelect::NotifySoundMessage(uint32_t msg, FEObject *, uint32_t, uint32_t, uint32_t event)
{
    if (msg == 0x0C407210u) {
        if (mpSelected->mbLocked) return 0x13;
    } else if (msg == 0x9120409Eu) {
        if (mpSelected == mHead) return 0;
    } else if (msg == 0xB5971BF1u) {
        if (mpSelected == mTail) return 0;
    }
    return FilterSoundForHorizontalMenu(msg, event, true);
}

uint32_t SelectableTrack::CalcLanguageHash() const { return bridge::CalcTrackNameHash(TrackNumber(), mDirection); }

void TrackMapScroller::Construct(FEObject *image)
{
    mpImage = image;
    vtable = bridge::GameVTable(kTrackMapVTable);
    mpTrack = nullptr;
    mWantedId = 0;
    mZoomTimer = 0;
    mbZoomNow = 0;
    *bridge::TrackMapScrollerInstance() = this;
    FEngSetInvisible(mpImage);
    mLoadingId = 0;
    mpLoading = nullptr;
    mbLoaded = 0;
    mShownId = 0;
    mpShown = nullptr;
    mpUnload = nullptr;
}

uint16_t *UIQRTrackSelect::GetSelectedTrack()
{
    return Online() ? OnlineTrack() : reinterpret_cast<uint16_t *>(bridge::QR_CurrentTrackSettings());
}

SelectableTrack *UIQRTrackSelect::FindTrack(int32_t number)
{
    for (SelectableTrack *t = mHead; t != Sentinel(); t = t->next)
        if (t->TrackNumber() == number) return t;
    return nullptr;
}

int32_t UIQRTrackSelect::GetNodeIndex(const SelectableTrack *sel)
{
    int32_t index = 1;
    for (SelectableTrack *t = mHead; t != Sentinel(); t = t->next, ++index)
        if (t == sel) return index;
    return 0;
}
