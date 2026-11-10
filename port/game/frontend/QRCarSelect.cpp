#include "QRCarSelect.h"
#include <cstring>
#include "bridge/Bridge.h"
#include "cFEng.h"
#include "DialogInterface.h"
#include "FEHash.h"
#include "FEngUtil.h"
#include "FrontendDatabase.h"
#include "CarViewer.h"
#include "OptionsMain.h"
#include "QRTrackSelectScreenManager.h"
#include "MainMenu.h"
#include "bWare/bString.h"

namespace {
uint32_t &CurrentViewFilter() { return *reinterpret_cast<uint32_t *>(bridge::GameInt(0x7F444C)); }
int32_t ActivePlayer() { return *bridge::GameInt(0x85AD28); }
// The active profile's view filter (UserProfile +0xE0 = QuickRaceSettings::mSelectedCar[2] slot).
int32_t &ProfileViewFilter() { return bridge::FrontendDatabase()->mProfiles[ActivePlayer()].mQuickRace.mSelectedCar[2]; }
}  // namespace

void UIQRCarSelect::SetViewFilter(uint32_t filter)
{
    ProfileViewFilter() = static_cast<int32_t>(filter);
    CurrentViewFilter() = filter;
}

void UIQRCarSelect::UpdateViewFilter() { CurrentViewFilter() = static_cast<uint32_t>(ProfileViewFilter()); }

uint32_t UIQRCarSelect::GetUnlockFilter()
{
    const int32_t m = bridge::GameModeFlags();
    return m == 8 || m == 0x100 ? 0x1F8u : 1u;
}

int32_t UIQRCarSelect::GetNumberOfCarsForFilter(uint32_t filter) const
{
    const uint32_t unlock = GetUnlockFilter();
    FEPlayerCarDB *db = mPlayer >= 0 && mPlayer < 2 ? &bridge::FrontendDatabase()->mProfiles[mPlayer].mCarDB : nullptr;
    return db->GetNumCarsForFilter(filter, static_cast<int32_t>(unlock));   // PC: no null check
}

void UIQRCarSelect::InitSlider()
{
    SliderInitParams p{};
    p.mpPackage = base.mpPackageName;
    p.mpBacking = "accel_backing";
    p.mpHandle = "accel_handle";
    p.mpCurrent = "accel_cur";
    p.mStep = 0.0f;
    p.mMin = 0.0f;
    p.mMax = 1.0f;
    p.mValue = 0.0f;
    p.mMargin = 46.0f;
    mAccel.Init(&p, false);
    p.mpBacking = "top_speed_backing";
    p.mpHandle = "top_speed_handle";
    p.mpCurrent = "top_speed_cur";
    mTopSpeed.Init(&p, false);
    p.mpBacking = "handling_backing";
    p.mpHandle = "handling_handle";
    p.mpCurrent = "handling_cur";
    mHandling.Init(&p, false);
}

void UIQRCarSelect::UpdateSliders()
{
    mAccel.SetCurValue(static_cast<float>(bridge::RideRating(bridge::SelectableCar_GetRide(mpCurrent), 0)));
    mAccel.Draw();
    mTopSpeed.SetCurValue(static_cast<float>(bridge::RideRating(bridge::SelectableCar_GetRide(mpCurrent), 1)));
    mTopSpeed.Draw();
    mHandling.SetCurValue(static_cast<float>(bridge::RideRating(bridge::SelectableCar_GetRide(mpCurrent), 2)));
    mHandling.Draw();
}

void UIQRCarSelect::ChooseTransmission()
{
    // the player's "automatic" flag (PC 0x83AA79 + player * 0x28: profile 0 player settings +0x15)
    const bool automatic = bridge::FrontendDatabase()->mProfiles[0].mPlayers[mPlayer * 0x28 + 0x15] != 0;
    mDialog = DialogInterface::ShowTwoButtonsHash(base.mpPackageName, "", 0x1B3A2485u, 0x61CB0720u, 0x5F5E3886u,
                                                  0x1A2826E1u, 0x1FAB5998u, automatic, 0x30003D27u);
}

void UIQRCarSelect::Setup()
{
    if (GetNumberOfCarsForFilter(CurrentViewFilter()) == 0) SetViewFilter(3);
    bridge::QRCarSelect_BuildCarList(this, CurrentViewFilter());
    mpLogo = FEngFindImage(base.mpPackageName, FEHashUpper("ManufacturerLogo"));
    mpBadge = FEngFindImage(base.mpPackageName, FEHashUpper("CarBadge"));
    const int32_t m = bridge::GameModeFlags();
    if (m == 0x20 || m == 0x100) {
        FEngSetInvisible(FEngFindObject(base.mpPackageName, 0x39B2B1C6u));
        FEngSetInvisible(FEngFindObject(base.mpPackageName, 0xE8CA2E25u));
    } else if (m == 4) {
        FEngSetControlMask(base.mpPackageName, 1);
    }
    bridge::QRCarSelect_RefreshHeader(this);
}

void UIQRCarSelect::RebuildCarListFromFilter()
{
    ClearCarList();
    _unknown60 = 0;
    _unknown64 = 1;
    if (GetNumberOfCarsForFilter(CurrentViewFilter()) == 0) SetViewFilter(3);
    bridge::QRCarSelect_BuildCarList(this, CurrentViewFilter());
}

void UIQRCarSelect::ClearCarList()
{
    auto *head = reinterpret_cast<void **>(&mCars);
    while (head[0] != head) {
        auto **n = static_cast<void **>(head[0]);
        auto **next = static_cast<void **>(n[0]), **prev = static_cast<void **>(n[1]);
        prev[0] = next;
        next[1] = prev;
        void *car = reinterpret_cast<uint8_t *>(n) - 0x10;
        if (car) bridge::VirtualScalarDelete(car);
    }
}

namespace {
uint8_t *CarFromNode(void *n) { return n ? static_cast<uint8_t *>(n) - 0x10 : nullptr; }
uint32_t CarHandle(const uint8_t *car) { uint32_t h; std::memcpy(&h, car + 0x920, 4); return h; }
}  // namespace

void UIQRCarSelect::BuildCarList(uint32_t filter)
{
    FEPlayerCarDB *db = mPlayer >= 0 && mPlayer < 2 ? &bridge::FrontendDatabase()->mProfiles[mPlayer].mCarDB : nullptr;
    uint8_t *rec = db->GetCarFiltered(filter, nullptr);
    const uint32_t unlock = GetUnlockFilter();
    for (; rec; rec = db->GetCarFiltered(filter, rec)) {
        const uint32_t type = bridge::CarRecord_GetType(rec);
        if (type == 0xFFFFFFFFu) continue;
        const uint8_t *ct = bridge::CarTypeInfo(static_cast<int32_t>(type));
        bool ok = false;
        if (ct) {
            int32_t v840;
            std::memcpy(&v840, ct + 0x840, 4);
            ok = v840 != -1 && bridge::CarTypeCheck840(v840) && bridge::CarTypeInfo_610180(ct) &&
                 bridge::UnlockSystem_IsCarUnlocked(static_cast<int32_t>(unlock), FEPlayerCarDB::Handle(rec), mPlayer != 0);
        }
        if (db->IsCarCareerCar(FEPlayerCarDB::Handle(rec)) && !rec[0x7F5]) continue;
        if (!ok) continue;
        uint32_t c;
        std::memcpy(&c, rec + 0xC, 4);
        uint8_t *car = static_cast<uint8_t *>(bridge::SelectableCar_New(ct, c, FEPlayerCarDB::Handle(rec), mPlayer, 1));
        auto **node = reinterpret_cast<void **>(car ? car + 0x10 : nullptr);
        auto **tail = static_cast<void **>(mCars.prev);
        tail[0] = node;
        mCars.prev = node;
        node[1] = tail;
        node[0] = &mCars;
        ++_unknown60;
    }
    bridge::QRCarSelect_SortCars(&mCars);   // bList::Sort with a comparator that always returns 1 (PC 40CB20)
    int32_t *selected = &bridge::FrontendDatabase()->mProfiles[0].mQuickRace.mSelectedCar[mPlayer];
    uint8_t *first = CarFromNode(mCars.next);
    if (*selected == 0) bridge::FrontendDatabase()->mProfiles[0].mQuickRace.SetSelectedCar(static_cast<int32_t>(CarHandle(first)), mPlayer);
    mpCurrent = first;
    const uint32_t want = static_cast<uint32_t>(*selected);
    uint8_t *const end = CarFromNode(&mCars);
    for (uint8_t *c = first; c != end; c = CarFromNode(*reinterpret_cast<void **>(c + 0x10))) {
        if (CarHandle(c) == want) {
            mpCurrent = c;
            break;
        }
        ++_unknown64;
    }
    if (mpCurrent == first) _unknown64 = 1;
    bridge::FrontendDatabase()->mProfiles[0].mQuickRace.SetSelectedCar(static_cast<int32_t>(CarHandle(static_cast<uint8_t *>(mpCurrent))), mPlayer);
    CarViewer::SetRideInfo(bridge::SelectableCar_GetRide(mpCurrent), 1, 0);
}

uint32_t UIQRCarSelect::NotifySoundMessage(uint32_t msg, uint32_t arg, uint32_t event) const
{
    if (bridge::GameModeFlags() == 4 && static_cast<uint32_t>(mPlayer + 1) != arg) return 0;
    switch (msg) {
    case 0x5073EF13u: return 6;
    case 0xD9FEEC59u: return 7;
    case 0x0C407210u:
    case 0x406415E3u: return 0;
    default: return FilterSoundForHorizontalMenu(msg, event, true);
    }
}

// ---- whole-class pass: ctor / dtor, header, scrolling, messages ----
namespace {
constexpr uint32_t kVTable = 0x79770C;
constexpr const char *kOLCarLot = "UI_OLCarLot.fng";
constexpr const char *kPressStart = "2P_PressStart.fng";
constexpr const char *kOLMain = "UI_OLMAIN.fng";
constexpr uint32_t kPrevCar = 0x9120409Eu, kNextCar = 0xB5971BF1u;
constexpr uint32_t kPrevList = 0x5073EF13u, kNextList = 0xD9FEEC59u;

FEPlayerCarDB &Db0() { return bridge::FrontendDatabase()->mProfiles[0].mCarDB; }   // PC 0x83AD90
// PC 495240 (cFrontendDatabase, ret 4): the player's car DB, null outside 0 / 1
FEPlayerCarDB *PlayerDb(int32_t p) { return p >= 0 && p < 2 ? &bridge::FrontendDatabase()->mProfiles[p].mCarDB : nullptr; }
QuickRaceSettings &QR() { return bridge::FrontendDatabase()->mProfiles[0].mQuickRace; }   // PC 0x83AAB4
uint8_t PlayerSetting(int32_t p, int32_t off) { return bridge::FrontendDatabase()->mProfiles[0].mPlayers[p * 0x28 + off]; }
void SetPlayerSetting(int32_t p, int32_t off, uint8_t v) { bridge::FrontendDatabase()->mProfiles[0].mPlayers[p * 0x28 + off] = v; }
int32_t *JoyPorts() { return &bridge::FrontendDatabase()->_unknown08; }   // PC 0x83A9D8 + player * 4
int32_t &FilterStore(int32_t p) { return bridge::FrontendDatabase()->mProfiles[p].mQuickRace.mSelectedCar[2]; }
void SetFilter(uint32_t f)   // inlined SetViewFilter on PC (profile of *0x85AD28)
{
    FilterStore(ActivePlayer()) = static_cast<int32_t>(f);
    CurrentViewFilter() = f;
}
uint32_t CarHandleOf(const void *car) { return CarHandle(static_cast<const uint8_t *>(car)); }
const uint8_t *CarTypeOf(const void *car)
{
    const uint8_t *ct;
    std::memcpy(&ct, static_cast<const uint8_t *>(car) + 0x918, sizeof ct);
    return ct;
}
int32_t CarTypeId(const void *car) { int32_t v; std::memcpy(&v, CarTypeOf(car) + 0x840, 4); return v; }
void Show(const char *pkg, uint32_t h) { FEngSetVisible(FEngFindObject(pkg, h)); }
void Hide(const char *pkg, uint32_t h) { FEngSetInvisible(FEngFindObject(pkg, h)); }
void Label(const char *pkg, uint32_t obj, uint32_t lang) { FEngSetLanguageHash(FEngFindString(pkg, obj), lang); }
void Blink(FEObject *o)
{
    if (!FEngIsScriptRunning(o, "Blink")) FEngSetScript(o, "Blink", true);
}
void PlaySound4() { bridge::PlayUISound(4); }   // PC 50D180(4)
}  // namespace

UIQRCarSelect *UIQRCarSelect::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.vtable = bridge::GameVTable(kVTable);
    mb4C = 0;
    mb4D = 1;
    mb4E = 1;
    mCars.next = mCars.prev = &mCars;
    mPlayer = 0;
    _unknown60 = 0;
    _unknown64 = 1;
    mDialog = 0;
    mScrollBar.Construct(base.mpPackageName, "ScrollBar", false, false, false);
    _unknownC4 = 0;
    mpLogo = mpBadge = nullptr;
    mAccel.Construct();
    mTopSpeed.Construct();
    mHandling.Construct();
    mb1A4 = 1;
    mbActive = 0;
    mOnlineHandle = static_cast<uint32_t>(QR().mSelectedCar[0]);
    _unknown1AC = 0;
    _unknown1B0 = -1;
    mbMemcard = 0;
    UpdateViewFilter();
    const char *pkg = base.mpPackageName;
    const uint32_t f = CurrentViewFilter();
    switch (bridge::GameModeFlags()) {
    case 2:
        if (f == 4 || f == 8) {
            Hide(pkg, 0xA936C3A2u);
            Hide(pkg, 0x89782734u);
        }
        base.mHelpHash = 0xE1385C1Eu;
        break;
    case 4: base.mHelpHash = 0x10E836C8u; break;
    case 0x20:
        if (f == 4 || f == 8) SetFilter(3);
        base.mHelpHash = 0xE1385C1Eu;
        break;
    case 0x100:
        if (f == 4 || f == 8) SetFilter(3);
        base.mHelpHash = 0xA7FA00B6u;
        break;
    case 8: base.mHelpHash = 0xA7FA00B6u; break;
    default: break;
    }
    *bridge::GameInt(0x85AD28) = mPlayer;
    Show(pkg, 0xF5A9D520u);
    _unknownC4 = 0;
    InitSlider();
    const int32_t m = bridge::GameModeFlags();
    bool showGroup;
    if (m == 8 || m == 0x100) {   // online / LAN (slot info stays the game's)
        if (bStrCmp(pkg, kOLCarLot) != 0) {
            bridge::SetGameModeFlags(8);
            if (*bridge::GameByte(0x7F4450)) {
                SetFilter(0x10);
                *bridge::GameByte(0x7F4450) = 0;
            }
            void *online = *bridge::GamePtrSlot(0x836338);
            if (online) bridge::QRCarSelect_SetOnlineSlotInfo(this, online);
            else if (CurrentViewFilter() == 0x10)
                bridge::QRCarSelect_SetOnlineSlotInfo(this, Db0().GetCarRecordByHandle(static_cast<uint32_t>(QR().mSelectedCar[0])));
        } else {
            base.mHelpHash = 0x01A11990u;
            SetFilter(1);
        }
        mOnlineHandle = static_cast<uint32_t>(QR().mSelectedCar[0]);
        showGroup = CurrentViewFilter() == 0x10 || bStrCmp(pkg, kOLCarLot) == 0;
    } else {
        if (CurrentViewFilter() == 0x10) SetFilter(3);
        showGroup = bStrCmp(pkg, kOLCarLot) == 0;
    }
    if (showGroup) FEngSetVisible(FEngFindObject(pkg, FEHashUpper("OL_CarMode_Group")));
    else FEngSetInvisible(FEngFindObject(pkg, FEHashUpper("OL_CarMode_Group")));
    Setup();
    base.mbMessengerOpen = 1;
    if (mpCurrent == CarFromNode(mCars.next)) {
        Hide(pkg, 0x444969FDu);
        mbArrow1Shown = 0;
    } else {
        Show(pkg, 0x444969FDu);
        mbArrow1Shown = 1;
    }
    if (mpCurrent == CarFromNode(mCars.prev)) {
        Hide(pkg, 0x444969FEu);
        mbArrow2Shown = 0;
    } else {
        Show(pkg, 0x444969FEu);
        mbArrow2Shown = 1;
    }
    mpStringD0 = FEngFindImage(pkg, 0xCF6F0439u);
    mpStringD4 = FEngFindImage(pkg, 0xABA93D3Fu);
    *bridge::GameByte(0x838528) = 0;
    return this;
}

void UIQRCarSelect::Destruct()
{
    base.vtable = bridge::GameVTable(kVTable);
    const int32_t m = bridge::GameModeFlags();
    if ((m == 8 && bStrCmp(base.mpPackageName, kOLCarLot) != 0) ||
        (m == 0x200 && bStrCmp(base.mpPackageName, "UI_QRCarSelect.fng") != 0))
        bridge::QRCarSelect_OnlineExitRide(mOnlineHandle);   // network: the kept car back into the garage view
    mHandling.vtable = mTopSpeed.vtable = mAccel.vtable = bridge::GameVTable(0x78F544);   // cSlider dtors
    ClearCarList();   // bList dtor (PC 479100: same loop)
    base.vtable = bridge::MenuScreenVTable();   // MenuScreen dtor 510D40
}

UIQRCarSelect *UIQRCarSelect::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void UIQRCarSelect::RefreshHeader()
{
    const char *pkg = base.mpPackageName;
    Show(pkg, 0xA936C3A2u);
    Show(pkg, 0x89782734u);
    uint32_t mask = base.mData.mHelpBarMask;
    const uint32_t f = CurrentViewFilter();
    switch (bridge::GameModeFlags()) {
    case 0x20: {
        mask &= ~0x1000u;
        const uint32_t h = CarHandleOf(mpCurrent);
        if (Db0().IsCarATunedCar(h)) {
            if (Db0().IsCarCareerCar(h)) {
                Label(pkg, 0xA936C3A2u, 0x5DA6131Bu);
            } else {
                Label(pkg, 0xA936C3A2u, 0xEFDDC126u);
                mask |= 0x40000u;
            }
        } else if (Db0().IsCarStock(h)) {
            Label(pkg, 0xA936C3A2u, 0xC8054CE0u);
            Hide(pkg, 0x89782734u);
            Hide(pkg, 0xA936C3A2u);
        }
        Label(pkg, 0x42ADB44Cu, 0x61032CE5u);
        break;
    }
    case 2:
        if (f == 4 || f == 8) {
            Hide(pkg, 0xA936C3A2u);
            Hide(pkg, 0x89782734u);
            mask &= ~0x1000u;
        }
        Label(pkg, 0x42ADB44Cu, 0x5AC1FB59u);
        break;
    case 4:
        Label(pkg, 0x42ADB44Cu, 0xF221D7EDu);
        Label(pkg, 0xA936C3A2u, 0xAA3CD907u);
        break;
    case 8: Label(pkg, 0x42ADB44Cu, 0x087FEA07u); break;
    case 0x100:
        if (f == 4 || f == 8) {
            Hide(pkg, 0xA936C3A2u);
            Hide(pkg, 0x89782734u);
        }
        Label(pkg, 0x42ADB44Cu, 0x61032CE5u);
        break;
    case 0x200: Label(pkg, 0x42ADB44Cu, 0x00D5F627u); break;
    default: break;
    }
    uint32_t logo = bridge::GetCarTypeLogoHash(CarTypeId(mpCurrent), true);
    FEngSetTextureHash(mpLogo, bridge::GetTextureInfo(logo, 0, 0) ? logo : FEHashUpper("GENERICPLACEHOLDER"));
    logo = bridge::GetCarTypeLogoHash(CarTypeId(mpCurrent), false);
    FEngSetTextureHash(mpBadge, bridge::GetTextureInfo(logo, 0, 0) ? logo : FEHashUpper("GENERICPLACEHOLDER"));
    mScrollBar.Update(1, _unknown60, _unknown64);
    FEObject *a1 = FEngFindObject(pkg, FEHashUpper("ScrollBar_Arrow_1"));
    FEObject *a2 = FEngFindObject(pkg, FEHashUpper("ScrollBar_Arrow_2"));
    if (mpCurrent == CarFromNode(mCars.next)) {
        if (mbArrow1Shown) {
            Hide(pkg, 0x444969FDu);
            mbArrow1Shown = 0;
        }
    } else if (mpCurrent == CarFromNode(mCars.prev)) {
        if (mbArrow2Shown) {
            Hide(pkg, 0x444969FEu);
            mbArrow2Shown = 0;
        }
    } else {
        if (!mbArrow1Shown) {
            Show(pkg, 0x444969FDu);
            mbArrow1Shown = 1;
        } else if (!mbArrow2Shown) {
            Show(pkg, 0x444969FEu);
            mbArrow2Shown = 1;
        }
        if (!FEngIsScriptRunning(a1, "Blink")) FEngSetScript(a1, "Static", true);
        if (!FEngIsScriptRunning(a2, "Blink")) FEngSetScript(a2, "Static", true);
    }
    uint32_t name;
    if (f == 3) {
        name = 0x6B68362Cu;
    } else {
        name = 0xE798A2F7u;
        if (f & 2) name = 0xE7AB43D3u;
        if (f & 4) name = 0xB4045365u;
        if (f & 8) name = 0x26FECEE7u;
        if (f & 0x10) name = 0x1A23D5E6u;
    }
    const int32_t m = bridge::GameModeFlags();
    if (((m == 8 || m == 0x200) && f == 4) || Db0().IsCarCareerCar(CarHandleOf(mpCurrent))) {
        Hide(pkg, 0x39B2B1C6u);
        Hide(pkg, 0xE8CA2E25u);
        mask &= ~0x1000u;
    } else {
        Show(pkg, 0x39B2B1C6u);
        Show(pkg, 0xE8CA2E25u);
    }
    Label(pkg, 0x3E81DE59u, name);
    bridge::HelpBar_SyncByMask(mask, pkg);
    UpdateSliders();
}

void UIQRCarSelect::SetupScreenForPlayer(int32_t player)
{
    mPlayer = player;
    *bridge::GameInt(0x85AD28) = player;
    CurrentViewFilter() = static_cast<uint32_t>(FilterStore(player));
    const char *pkg = base.mpPackageName;
    Label(pkg, FEHashUpper("Player_Name"), player == 0 ? 0xD87133BCu : 0xD87133BDu);
    FEngSetControlMask(pkg, player == 0 ? 1u : 2u);
    RebuildCarListFromFilter();
    mpCurrent = CarFromNode(mCars.next);
    _unknown64 = 1;
    uint8_t *const end = CarFromNode(&mCars);
    for (uint8_t *c = CarFromNode(mCars.next); c != end; c = CarFromNode(*reinterpret_cast<void **>(c + 0x10))) {
        if (CarHandle(c) == static_cast<uint32_t>(QR().mSelectedCar[mPlayer])) {
            mpCurrent = c;
            break;
        }
        ++_unknown64;
    }
    QR().SetSelectedCar(static_cast<int32_t>(CarHandleOf(mpCurrent)), mPlayer);
    CarViewer::SetRideInfo(bridge::SelectableCar_GetRide(mpCurrent), 1, 0);   // inlined (top only)
    RefreshHeader();
    if (bridge::GameModeFlags() == 4 && !mb4C && !FEngIsPackagePushed(kPressStart))
        FEngShowPackageModal(kPressStart, pkg, 0xFF, 0);
    mb4C = 0;
}

void UIQRCarSelect::ScrollCars(uint32_t msg)
{
    void *const old = mpCurrent;
    if (msg == kPrevCar) {
        if (mpCurrent != CarFromNode(mCars.next)) {
            FEngSetScript(base.mpPackageName, "ScrollBar_Arrow_1", "Blink", true);
            mpCurrent = CarFromNode(*reinterpret_cast<void **>(static_cast<uint8_t *>(mpCurrent) + 0x14));
            QR().SetSelectedCar(static_cast<int32_t>(CarHandleOf(mpCurrent)), mPlayer);
            --_unknown64;
            CarViewer::SendScrollDirectionToGarageScreen(0, msg);   // +0x148 = 1 (no null check)
        }
    } else if (msg == kNextCar) {
        if (mpCurrent != CarFromNode(mCars.prev)) {
            FEngSetScript(base.mpPackageName, "ScrollBar_Arrow_2", "Blink", true);
            mpCurrent = CarFromNode(*reinterpret_cast<void **>(static_cast<uint8_t *>(mpCurrent) + 0x10));
            QR().SetSelectedCar(static_cast<int32_t>(CarHandleOf(mpCurrent)), mPlayer);
            ++_unknown64;
            CarViewer::SendScrollDirectionToGarageScreen(0, msg);   // +0x148 = 0
        }
    }
    if (mpCurrent == old) return;
    const uint8_t *ct = CarTypeOf(mpCurrent);
    int32_t v844;
    std::memcpy(&v844, ct + 0x844, 4);
    if (v844 != 0) {
        // PC: a temporary RideInfo of the type (Init(type, 1, 0, 0), composite name, random parts + paint)
        bridge::WithRandomRide(CarTypeId(mpCurrent), [](void *ride) { CarViewer::SetRideInfo(ride, 1, 0); });
    } else {
        const int32_t type = CarTypeId(mpCurrent);
        if (uint8_t *s = CarViewer::FindWhichScreenToUpdate(0)) bridge::GarageMain_SetCarType(s, type);
        if (CurrentViewFilter() == 0x10)
            bridge::QRCarSelect_SetOnlineSlotInfo(this, Db0().GetCarRecordByHandle(CarHandleOf(mpCurrent)));
        _unknownC4 = bridge::RealTimer();
    }
    RefreshHeader();
}

void UIQRCarSelect::ScrollCarLists(uint32_t msg)
{
    const uint32_t f = CurrentViewFilter();
    uint32_t n = f;
    auto has = [this](uint32_t k) { return GetNumberOfCarsForFilter(k) != 0; };
    const bool prev = msg == kPrevList, next = msg == kNextList;
    switch (bridge::GameModeFlags()) {
    case 2:
    case 4:   // stock|tuned <-> stock <-> tuned <-> career <-> sponsor (empty kinds skipped)
        if (prev) {
            if (f == 3) n = has(8) ? 8 : has(4) ? 4 : has(2) ? 2 : 1;
            else if (f == 1) n = 3;
            else if (f == 2) n = 1;
            else if (f == 4) n = has(2) ? 2 : 1;
            else if (f == 8) n = has(4) ? 4 : has(2) ? 2 : 1;
        } else if (next) {
            if (f == 3) n = 1;
            else if (f == 1) n = has(2) ? 2 : has(4) ? 4 : has(8) ? 8 : 3;
            else if (f == 2) n = has(4) ? 4 : has(8) ? 8 : 3;
            else if (f == 4) n = has(8) ? 8 : 3;
            else if (f == 8) n = 3;
        }
        break;
    case 8:
    case 0x200:   // stock|tuned <-> stock <-> tuned <-> online (0x10) <-> career
        if (prev) {
            if (f == 3) n = has(4) ? 4 : 0x10;
            else if (f == 1) n = 3;
            else if (f == 2) n = 1;
            else if (f == 0x10) n = has(2) ? 2 : 1;
            else if (f == 4) n = 0x10;
        } else if (next) {
            if (f == 3) n = 1;
            else if (f == 1) n = has(2) ? 2 : 0x10;
            else if (f == 2) n = 0x10;
            else if (f == 0x10) n = has(4) ? 4 : 3;
            else if (f == 4) n = 3;
        }
        break;
    case 0x20:   // stock|tuned <-> stock <-> tuned
        if (prev) {
            if (f == 3) n = has(2) ? 2 : 1;
            else if (f == 1) n = 3;
            else if (f == 2) n = 1;
        } else if (next) {
            if (f == 3) n = 1;
            else if (f == 1) n = has(2) ? 2 : 3;
            else if (f == 2) n = 3;
        }
        break;
    default: break;
    }
    if (n == f) return;
    SetFilter(n);
    RebuildCarListFromFilter();
    RefreshHeader();
    const char *pkg = base.mpPackageName;
    if (CurrentViewFilter() == 0x10 || bStrCmp(pkg, kOLCarLot) == 0) {
        FEngSetVisible(FEngFindObject(pkg, FEHashUpper("OL_CarMode_Group")));
        bridge::QRCarSelect_SetOnlineSlotInfo(this, Db0().GetCarRecordByHandle(CarHandleOf(mpCurrent)));
    } else {
        FEngSetInvisible(FEngFindObject(pkg, FEHashUpper("OL_CarMode_Group")));
    }
    const int32_t m = bridge::GameModeFlags();
    if (m == 8 || m == 0x100) {
        if (CurrentViewFilter() == 4) {
            Hide(pkg, 0xA936C3A2u);
            Hide(pkg, 0x89782734u);
        } else {
            Show(pkg, 0xA936C3A2u);
            Show(pkg, 0x89782734u);
        }
    }
}

void UIQRCarSelect::DeleteCurrentCar()
{
    FEPlayerCarDB *db = PlayerDb(mPlayer);
    const uint32_t h = CarHandleOf(mpCurrent);
    if (!db->IsCarATunedCar(h) || db->IsCarCareerCar(h)) return;
    void *const car = mpCurrent;
    auto unlinkDelete = [](void *c) {
        auto **n = reinterpret_cast<void **>(static_cast<uint8_t *>(c) + 0x10);
        auto **next = static_cast<void **>(n[0]), **prev = static_cast<void **>(n[1]);
        prev[0] = next;
        next[1] = prev;
        bridge::VirtualScalarDelete(c);
    };
    if (_unknown60 > 1) {
        ScrollCars(car == CarFromNode(mCars.prev) ? kPrevCar : kNextCar);
        db->DeleteTunedCar(h);
        unlinkDelete(car);
        --_unknown60;
        RefreshHeader();
    } else {
        db->DeleteTunedCar(h);
        unlinkDelete(car);
        SetFilter(3);
        RebuildCarListFromFilter();
        RefreshHeader();
    }
}

void UIQRCarSelect::ExitToMain(const char *pkg)
{
    if (*bridge::GameByte(0x838540)) bridge::EnterMemcard(pkg, "UI_Main.fng", 0x42, 0, 0, 0, 0);
    else FEngSwitchPackages(pkg, "UI_Main.fng", 0);
    *bridge::GameByte(0x838540) = 0;   // CustomizationScreenManager::ClearHasBeenModified 50E2D0
}

void UIQRCarSelect::NotificationMessage(uint32_t msg, FEObject *, uint32_t, uint32_t port)
{
    const int32_t active = ActivePlayer();
    int32_t m = bridge::GameModeFlags();
    if (bridge::GameInt(0x7F4454)[active] != static_cast<int32_t>(port) && port != 0 && m == 4) return;
    const char *pkg = base.mpPackageName;
    auto sound = [this](uint32_t event) { bridge::MakeFrontendSound(event, base.mSoundTrigger); };   // PC 50D150
    auto toOnlineMain = [&] {
        mOnlineHandle = CarHandleOf(mpCurrent);
        FEngSwitchPackages(pkg, kOLMain, 0);
    };
    auto cannotAdd = [&] {
        DialogInterface::ShowOneButtonHash(pkg, "", 0x639u, 0x34DC1BECu, 0x34DC1BECu, 0x91921A8Du);
    };
    switch (msg) {
    case 0x1A2826E1u:   // transmission dialog: manual
    case 0x5F5E3886u:   // automatic
        SetPlayerSetting(mPlayer, 0x15, msg == 0x5F5E3886u ? 1 : 0);
        bridge::FEngSendMessageToPackage(0xBA5B5FCEu, pkg);
        return;
    case 0x251BF83Eu:
    case 0x34DC1BECu: FEngSetScript(pkg, 0x47FF4E7Cu, "UNDIM", true); return;
    case 0x96DDA9E6u: FEngSetScript(pkg, 0x47FF4E7Cu, "DIM", true); return;
    case 0x35F8620Bu: mbActive = 1; return;
    case 0x406415E3u:
    case 0x0C407210u: {   // accept
        if (!mbActive) return;
        FEPlayerCarDB *db = PlayerDb(mPlayer);
        if (m == 0x20 && !db->AbleToAddNewTunedCar()) {
            cannotAdd();
            PlaySound4();
            return;
        }
        m = bridge::GameModeFlags();
        if (m == 0x100) return;
        if (m == 0x20) {
            QR().SetSelectedCar(static_cast<int32_t>(bridge::BeginCarCustomize(3, pkg, mPlayer, CarHandleOf(mpCurrent))), mPlayer);
        } else if (m == 0x200) {
            bridge::QRCarSelect_ConfirmLAN(this);
        } else if (m == 8) {
            if (CurrentViewFilter() == 0x10) bridge::QRCarSelect_ConfirmOnline(this);
            else bridge::QRCarSelect_ConfirmOnlineUnranked(this);
        } else {
            if (QR().mRaceMode != 2 && PlayerSetting(mPlayer, 6)) {
                JoyPorts()[mPlayer] = FEngMapJoyParamToJoyport(port);
                ChooseTransmission();
                PlaySound4();
                return;
            }
            JoyPorts()[mPlayer] = FEngMapJoyParamToJoyport(port);
            if (bridge::GameModeFlags() == 4 && mPlayer == 0) SetupScreenForPlayer(1);
            else bridge::RaceStarter_StartRace();
        }
        PlaySound4();
        return;
    }
    case 0x43A743ABu:
        if (!PlayerSetting(mPlayer, 6)) return toOnlineMain();
        ChooseTransmission();
        return;
    case 0x43A743AAu:
        bridge::SetGameModeFlags(8);
        if (!PlayerSetting(mPlayer, 6)) return toOnlineMain();
        ChooseTransmission();
        return;
    case 0x461A18EEu:
        SetFilter(2);
        RebuildCarListFromFilter();
        CarViewer::SetRideInfo(bridge::SelectableCar_GetRide(mpCurrent), 1, 0);
        RefreshHeader();
        mbMemcard = 0;
        return;
    case 0x8867412Du: mbMemcard = 0; return;
    case kPrevList:
    case kNextList:
        if (mbActive) ScrollCarLists(msg);
        return;
    case 0x51123592u: DeleteCurrentCar(); return;
    case 0x911AB364u:
    case 0x93B596EDu: {   // back
        if (m != 8 && m != 0x200 && !mbActive && !FEngIsPackagePushed(kPressStart)) return;
        m = bridge::GameModeFlags();
        if (m == 0x20) {
            ExitToMain(pkg);
        } else if (m == 0x100 || m == 8) {
            bridge::SetGameModeFlags(8);
            FEngSwitchPackages(pkg, kOLMain, 0);
        } else if (m == 0x200) {
            FEngSwitchPackages(pkg, kOLMain, 0);
        } else if (mPlayer == 1) {
            if (m == 4) {
                if (!FEngIsPackagePushed(kPressStart)) {
                    FEngShowPackageModal(kPressStart, pkg, 0xFF, 0);
                    bridge::PlayUISound(5);
                    return;
                }
                FEngPopPackage(kPressStart);
            }
            SetupScreenForPlayer(0);
        } else if (m == 4 && !FEngIsPackagePushed(kPressStart)) {
            FEngShowPackageModal(kPressStart, pkg, 0xFF, 0);
        } else {
            QRTrackSelectScreenManager::EnterBack(pkg);
        }
        bridge::PlayUISound(5);
        return;
    }
    case kPrevCar:
        if (!mbActive) return;
        FEngSetScript(pkg, "Blink", "ScrollBar_Arrow_1", true);   // object / script swapped, as GC
        ScrollCars(kPrevCar);
        return;
    case kNextCar:
        if (!mbActive) return;
        FEngSetScript(pkg, "Blink", "ScrollBar_Arrow_2", true);
        ScrollCars(kNextCar);
        return;
    case 0x9803F6E2u: {   // PC mouse
        if (FEngIsClickedOn(mpStringD0)) {
            if (!mbActive) return;
            Blink(mpStringD0);
            ScrollCarLists(kPrevList);
        } else if (FEngIsClickedOn(mpStringD4)) {
            if (!mbActive) return;
            Blink(mpStringD4);
            ScrollCarLists(kNextList);
        }
        mScrollBar.HandleMouse();
        switch (mScrollBar.mHit) {
        case -10:
            for (int32_t i = 0; i < *bridge::GameInt(0x7FA1A8); ++i) ScrollCars(kNextCar);
            break;
        case -1: ScrollCars(kPrevCar); break;
        case 1: ScrollCars(kNextCar); break;
        case 10:
            for (int32_t i = 0; i < *bridge::GameInt(0x7FA1A8); ++i) ScrollCars(kPrevCar);
            break;
        default: break;
        }
        return;
    }
    case 0xBA5B5FCEu:
        if (m == 8 || m == 0x200) toOnlineMain();
        else if (m == 4 && mPlayer == 0) SetupScreenForPlayer(1);
        else bridge::RaceStarter_StartRace();
        return;
    case 0xC519BFC0u: {   // customize
        if ((m == 8 || m == 0x100) && CurrentViewFilter() == 4) return;
        if (!mbActive) return;
        const uint32_t h = CarHandleOf(mpCurrent);
        if (m == 0x20) {
            *bridge::GameByte(0x838540) = 0;
            FEPlayerCarDB *db = PlayerDb(mPlayer);
            if (!db->IsCarATunedCar(h) || db->IsCarCareerCar(h) || db->IsCarSponsorCar(h)) return;
            DialogInterface::ShowTwoButtonsHash(pkg, "", 0xF770u, 0x61Cu, 0x51123592u, 0x1FAB5998u, 0x1FAB5998u, 1u, 0x367161F5u);
            return;
        }
        if (m == 8) {   // online
            FEPlayerCarDB *db = PlayerDb(mPlayer);
            if (db->IsCarSponsorCar(h)) return;
            if (db->IsCarStock(h) && !db->AbleToAddNewTunedCar()) {
                cannotAdd();
                sound(8);
                return;
            }
            *bridge::GameByte(0x838540) = 0;
            bridge::SetGameModeFlags(0x100);
            uint8_t *rec = Db0().GetCarRecordByHandle(h);
            uint32_t r;
            if (CurrentViewFilter() == 0x10) {
                r = bridge::BeginCarCustomizeOnline(4, pkg, mPlayer, rec);
                *bridge::GamePtrSlot(0x836338) = rec;
            } else {
                *bridge::GamePtrSlot(0x836338) = nullptr;
                r = bridge::BeginCarCustomize(4, pkg, mPlayer, FEPlayerCarDB::Handle(rec));
            }
            QR().SetSelectedCar(static_cast<int32_t>(r), mPlayer);
            sound(8);
            return;
        }
        if (m == 4) {
            mbMemcard = 1;
            *bridge::GameInt(0x85AD28) = mPlayer;
            bridge::EnterMemcard(pkg, pkg, 0x971, 0, 0, 0, 0);
            sound(8);
            return;
        }
        FEPlayerCarDB *db = PlayerDb(mPlayer);
        if (db->IsCarSponsorCar(h) || db->IsCarCareerCar(h)) return;
        if (!db->AbleToAddNewTunedCar()) {
            cannotAdd();
            sound(8);
            return;
        }
        SetFilter(3);
        *bridge::GameByte(0x838540) = 0;
        QR().SetSelectedCar(static_cast<int32_t>(bridge::BeginCarCustomize(2, pkg, mPlayer, CarHandleOf(mpCurrent))), mPlayer);
        sound(8);
        return;
    }
    case 0xC98356BAu: {   // tick
        if (m == 4 && mb4D == 1) {
            FEngShowPackageModal(kPressStart, pkg, 0xFF, 0);
            mb4D = 0;
            return;
        }
        if (active == 1 && mb4E == 1) {
            mb4E = 0;
            return;
        }
        if (_unknownC4 == 0 || _unknownC4 == 0x7FFFFFFF) return;   // Timer::IsSet 401820
        // x87: fild, fmul 0.00025f, fcomp 0.3f
        const long double t = static_cast<long double>(bridge::RealTimer() - _unknownC4) * static_cast<long double>(0.00025f);
        if (t < static_cast<long double>(0.3f)) return;
        CarViewer::SetRideInfo(bridge::SelectableCar_GetRide(mpCurrent), 1, 0);
        _unknownC4 = 0;
        return;
    }
    default: return;
    }
}
