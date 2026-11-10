#include "PostRaceResults.h"
#include <cstdio>
#include <cstring>
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "cFEng.h"
#include "DialogInterface.h"
#include "FEngUtil.h"
#include "PostRaceMenu.h"
#include "PostRaceScreenManager.h"
#include "FEPackage.h"
#include "FrontendDatabase.h"

namespace {
constexpr uint32_t kVTable = 0x797A8C;
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";   // PC GetHelpBarFNGName 50CEA0
struct ResultNode {   // bPNode-like 0xC
    bNode links;
    uint8_t *entry;
};
int32_t I32(const uint8_t *p, uint32_t off) { int32_t v; std::memcpy(&v, p + off, 4); return v; }
float F32(const uint8_t *p, uint32_t off) { float v; std::memcpy(&v, p + off, 4); return v; }
bool Online() { const int32_t m = bridge::GameModeFlags(); return m == 8 || m == 0x200; }
bool DriftRace() { return *bridge::GameByte(0x89E7D9) || *bridge::GameInt(0x890104); }   // byte || int
bool DragDyno()
{
    return *bridge::GameByte(0x89E7E6) && (*bridge::GameByte(0x89E7D8) || *bridge::GameInt(0x890108));
}
int32_t DriverCount()
{
    return *bridge::GameInt(0x89E7C0) + *bridge::GameInt(0x89E7C4) + *bridge::GameInt(0x89E7C8) + *bridge::GameInt(0x89E7CC);
}
// FEngHashString (PC 50B990): bVSPrintf then bStringHash (case-sensitive)
uint32_t Hash(const char *fmt) { return bStringHash(fmt); }
uint32_t Hash(const char *fmt, int32_t i) { return bridge::FEngHashStringInt(fmt, i); }
void Hide(const char *pkg, uint32_t h) { FEngSetInvisible(FEngFindObject(pkg, h)); }
void Label(const char *pkg, uint32_t obj, uint32_t lang) { FEngSetLanguageHash(FEngFindString(pkg, obj), lang); }
}  // namespace

// PC 50D230 (__cdecl): events 4..7 silent
uint32_t FilterSoundForBrowser(uint32_t, uint32_t event) { return event - 4 <= 3 ? 0 : event; }

PostRaceResultsScreen *PostRaceResultsScreen::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.vtable = bridge::GameVTable(kVTable);
    mResults.mHead.next = mResults.mHead.prev = &mResults.mHead;
    mb54 = mb55 = mb56 = 0;
    mDialog = 0;
    mpMessagePkg = base.mpPackageName;
    mb5C = 0;
    _unknown64 = 0;
    mbSplits = 0;
    mFlipTimer = 0.0f;
    mpLeader = bridge::FrontendDatabase()->mFinishedRaceStats.mFinishers[0];
    mFlipPeriod = 4.5f;   // PC 0x78DFF0
    Setup();
    return this;
}

void PostRaceResultsScreen::Destruct()
{
    base.vtable = bridge::GameVTable(kVTable);
    const char **current = bridge::PostRaceManager_CurrentScreen();   // cPostRaceScreenManager +0x1C
    if (*current && bStrCmp(*current, base.mpPackageName) == 0) *current = nullptr;
    while (mResults.mHead.next != &mResults.mHead) {
        bNode *n = mResults.mHead.next;
        n->prev->next = n->next;
        n->next->prev = n->prev;
        bridge::FreeArray(n);
    }
    base.vtable = bridge::MenuScreenVTable();   // MenuScreen dtor 510D40
}

PostRaceResultsScreen *PostRaceResultsScreen::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void PostRaceResultsScreen::dismissDialog()
{
    if (mDialog) DialogInterface::DismissDialog(mDialog);
    mDialog = 0;
}

uint32_t PostRaceResultsScreen::NotifySoundMessage(uint32_t msg, uint32_t event) const
{
    if (msg == 0x911AB364u) return 0;
    if (msg == 0xC519BFC0u) return 8;
    return FilterSoundForBrowser(msg, event);
}

void PostRaceResultsScreen::Setup()
{
    const char *pkg = base.mpPackageName;
    *bridge::PostRaceManager_CurrentScreen() = pkg;
    PostRace_SetControlMask(pkg);
    SetRaceSpecificHeader();
    if (DragDyno()) {
        PrintDragDynoResults();
        SetUnusedEntriesInvisible(6, 5);
    } else {
        OrganizeResults();
        if (Online()) {
            bridge::FEngPrintfString(pkg, 0x0019DE3Eu, "%s",
                                     bridge::LocalizedString(*bridge::GameInt(0x850088) == 8 ? 0x93447EE3u : 0xFE4B4CFAu));
        } else {
            bridge::FEngPrintfString(pkg, bStringHash("RESTART"), "%s", bridge::LocalizedString(0xF82DF37Eu));
            FEngSetScript(pkg, 0xAD685CB8u, bStringHash("SHOW"), true);
            FEngSetScript(pkg, 0x9B8C9D7Au, bStringHash("HIDE"), true);
        }
    }
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, pkg, 0xFF, 0);
    bridge::HelpBar_SetFlag6C(1);   // PC 54E6A0
    bridge::HelpBar_542990(true);
    if (Online() && bridge::Online_5FE500()) bridge::HelpBar_SyncByMask(base.mData.mHelpBarMask | 0x1000000u, pkg);
    else bridge::HelpBar_SyncByMask(base.mData.mHelpBarMask, pkg);
}

void PostRaceResultsScreen::ResultsUpdate()
{
    const char *pkg = base.mpPackageName;
    Label(pkg, 0xA4EC03BEu, mbSplits ? 0xE2F9334Du : 0xC97A2D22u);
    for (int32_t i = 0; i < DriverCount();) {
        auto *n = reinterpret_cast<ResultNode *>(bridge::bList_GetNode(&mResults, i));   // past the end: head (PC)
        ++i;
        if (mbSplits) PrintSplitTime(n->entry, i);
        else PrintFinishTime(n->entry, i);
    }
}

void PostRaceResultsScreen::SetRaceSpecificHeader()
{
    const char *pkg = base.mpPackageName;
    if (!DriftRace()) {
        if (Online() || !DragDyno()) return;
        bridge::FEngPrintfString(pkg, Hash("HEADER_TEXT_1"), bridge::LocalizedString(0xF77E6402u));
        FEngSetInvisible(pkg, Hash("HEADER_TEXT_2"));
        FEngSetInvisible(pkg, Hash("HEADER_TEXT_3"));
        return;
    }
    bridge::FEngPrintfString(pkg, Hash("HEADER_TEXT_3"), bridge::LocalizedString(0xC03F9F19u));
}

void PostRaceResultsScreen::OrganizeResults()
{
    const int32_t params = *bridge::GameInt(0x89E7A0);   // TheRaceParameters (race type id)
    if (params >= 0x104E && params <= 0x1053) {         // downhill drift
        const int32_t n = bridge::DriftScoreList()->TraversebList(nullptr);
        PrintDownhillResults();
        SetUnusedEntriesInvisible(6, n);
        return;
    }
    for (int32_t i = 0; i < bridge::CurrentRaceDriverCount(); ++i) {
        uint8_t *e = bridge::FrontendDatabase()->mFinishedRaceStats.mFinishers[i];
        auto *n = static_cast<ResultNode *>(bridge::Alloc(sizeof(ResultNode)));
        if (n) n->entry = e;
        bNode *tail = mResults.mHead.prev;   // PC: no null check
        tail->next = &n->links;
        mResults.mHead.prev = &n->links;
        n->links.prev = tail;
        n->links.next = &mResults.mHead;
    }
    PrintResults();
    SetUnusedEntriesInvisible(6, bridge::CurrentRaceDriverCount());
}

void PostRaceResultsScreen::PrintDownhillResults()
{
    const char *pkg = base.mpPackageName;
    bList *scores = bridge::DriftScoreList();
    const int32_t n = scores->TraversebList(nullptr);
    for (int32_t i = 1; i <= n; ++i) {
        auto *s = reinterpret_cast<const uint8_t *>(bridge::bList_GetNode(scores, i - 1));
        if (s[0x14]) FEngSetScript(pkg, Hash("RANKING_DATA_%d", i), 0x249DB7B7u, true);
        bridge::FEngPrintfInt(pkg, Hash("RANKING_DATA_%d", i), "%2d", i);
        s = reinterpret_cast<const uint8_t *>(bridge::bList_GetNode(scores, i - 1));
        const char *name;
        std::memcpy(&name, s + 0x18, sizeof name);
        bridge::FEngPrintfString(pkg, Hash("PLAYER_DATA_%d", i), "%s", name);
        s = reinterpret_cast<const uint8_t *>(bridge::bList_GetNode(scores, i - 1));
        const uint32_t car = bridge::FEngHashStringArg("CAR_NAME_%s", reinterpret_cast<const char *>(bridge::CarTypeInfo(I32(s, 0x1C))));
        Label(pkg, Hash("CAR_DATA_%d", i), car);
        s = reinterpret_cast<const uint8_t *>(bridge::bList_GetNode(scores, i - 1));
        bridge::FEngPrintfDouble(pkg, Hash("TIME_DATA_%d", i), "%$.0f", static_cast<double>(F32(s, 0x10)));
    }
}

void PostRaceResultsScreen::PrintResults()
{
    const char *pkg = base.mpPackageName;
    for (int32_t i = 1; i - 1 < DriverCount(); ++i) {
        const uint8_t *e = reinterpret_cast<ResultNode *>(bridge::bList_GetNode(&mResults, i - 1))->entry;
        const uint8_t *racer = bridge::RacerInfo(I32(e, 4));
        const uint8_t *car;
        std::memcpy(&car, racer + 0x14, sizeof car);
        const uint8_t *car2;
        std::memcpy(&car2, bridge::RacerInfo(I32(e, 4)) + 0x14, sizeof car2);
        if (I32(car2, 4) == 1 && !car2[0x900]) FEngSetScript(pkg, Hash("RANKING_DATA_%d", i), Hash("HIGHLIGHT"), true);
        bridge::FEngPrintfInt(pkg, Hash("RANKING_DATA_%d", i), "%2d", i);
        const uint8_t *drv;
        std::memcpy(&drv, racer + 0x14, sizeof drv);
        if (I32(drv, 4) == 1 && !drv[0x900]) {   // local human
            uint8_t *player;
            std::memcpy(&player, racer + 0x18, sizeof player);
            if (I32(player, 0x10) == 1) Label(pkg, Hash("PLAYER_DATA_%d", i), 0xD87133BDu);   // "Player 2"
            else bridge::FEngPrintfString(pkg, Hash("PLAYER_DATA_%d", i), "%s", bridge::DriverName(player));
        } else {
            const char *name = bridge::OnlineBusy() ? bridge::OnlinePlayerName(I32(e, 4))
                                                    : reinterpret_cast<const char *>(car + 0x90C);
            bridge::FEngPrintfString(pkg, Hash("PLAYER_DATA_%d", i), "%s", name);
        }
        const char *carType;   // racer +0xC: the car type info (starts with its name)
        std::memcpy(&carType, racer + 0xC, sizeof carType);
        const uint32_t carName = bridge::FEngHashStringArg("CAR_NAME_%s", carType);
        Label(pkg, Hash("CAR_DATA_%d", i), carName);
        if (DriftRace())
            bridge::FEngPrintfInt(pkg, Hash("TIME_DATA_%d", i), "%$d", I32(e, 0x198 + *bridge::GameInt(0x89E7BC) * 4));
        else PrintFinishTime(e, i);
    }
}

void PostRaceResultsScreen::PrintFinishTime(const uint8_t *entry, int32_t row)
{
    char buf[0x20];
    const int32_t kind = *bridge::GameInt(0x850088);   // online race kind
    const int32_t status = I32(entry, 8);
    if (bridge::OnlineBusy() && row == 0 && kind == 9) {
        std::memset(buf, 0, 4);
    } else if (bridge::OnlineBusy() && kind == 2 && (status == 8 || status == 9)) {
        bStrNCpy(buf, bridge::LocalizedString(status == 8 ? 0x01E66364u : 0xB7B75185u), 0x1F);
    } else if (bridge::OnlineBusy() && status == 0xC) {
        bStrNCpy(buf, bridge::LocalizedString(0xA7DB717Fu), 0x1F);
    } else {
        bridge::Timer_PrintToString(I32(entry, 0xC), buf, 0);
    }
    bridge::FEngPrintfString(base.mpPackageName, Hash("TIME_DATA_%d", row), "%s", buf);
}

void PostRaceResultsScreen::PrintSplitTime(const uint8_t *entry, int32_t row)
{
    char buf[0x20];
    if (row == 1) std::snprintf(buf, sizeof buf, "%s", bridge::LocalizedString(0x61C9B7A2u));   // PC bSNPrintf 50C960
    else bridge::Timer_PrintToString(I32(entry, 0xC) - I32(mpLeader, 0xC), buf, 2);
    bridge::FEngPrintfString(base.mpPackageName, Hash("TIME_DATA_%d", row), "%s", buf);
}

void PostRaceResultsScreen::PrintDragDynoResults()
{
    const char *pkg = base.mpPackageName;
    bridge::FEngPrintfString(pkg, bridge::FEngHashStringArg("HEADER_TEXT", bridge::LocalizedString(0x6950EC05u)),
                             bridge::LocalizedString(0x6950EC05u));
    uint32_t s[13];
    std::memcpy(s, bridge::GameInt(0x850478), sizeof s);   // dyno / drag sheet
    auto f = [&](int32_t i) { float v; std::memcpy(&v, &s[i], 4); return v; };
    const bool metric = *bridge::GameByte(0x83AA1A) == 1;
    char buf[0x20];
    auto row = [&](int32_t r, uint32_t title) {
        Hide(pkg, Hash("RANKING_DATA_%d", r));
        bridge::FEngPrintfString(pkg, Hash("PLAYER_DATA_%d", r), bridge::LocalizedString(title));
        Hide(pkg, Hash("CAR_DATA_%d", r));
    };
    row(1, metric ? 0x365DC8E2u : 0x1A09BEF9u);
    bridge::Timer_PrintToString(static_cast<int32_t>(s[2]), buf, 0);
    bridge::FEngPrintfString(pkg, Hash("TIME_DATA_%d", 1), "%s", buf);
    row(2, metric ? 0x1C6F2A82u : 0x49FD5DCBu);
    bridge::Timer_PrintToString(static_cast<int32_t>(s[5]), buf, 0);
    bridge::FEngPrintfString(pkg, Hash("TIME_DATA_%d", 2), "%s", buf);
    // speeds: x87 product rounded to float (fstp), then printed as double
    auto speed = [&](float v) { return static_cast<float>(static_cast<long double>(v) * (metric ? 3.6f : 2.237f)); };
    const uint32_t speedUnit = metric ? 0xE74B7E13u : 0xE74B86F8u;
    row(3, metric ? 0xAA461724u : 0xBCF1255Fu);
    bridge::FEngPrintfFloatString(pkg, Hash("TIME_DATA_%d", 3), "%$3.2f %s", speed(f(4)), bridge::LocalizedString(speedUnit));
    row(4, 0xC431600Cu);
    float accel = f(10);
    if (!metric) accel = static_cast<float>(static_cast<long double>(accel) * 100.0f * 0.03787878787878787);
    bridge::FEngPrintfFloatString(pkg, Hash("TIME_DATA_%d", 4), "%$3.2f %s", accel, bridge::LocalizedString(metric ? 0xF0AEB103u : 0xD0B86077u));
    row(5, 0x5D304669u);
    bridge::FEngPrintfFloatString(pkg, Hash("TIME_DATA_%d", 5), "%$3.2f %s", speed(f(0)), bridge::LocalizedString(speedUnit));
}

void PostRaceResultsScreen::SetUnusedEntriesInvisible(int32_t max, int32_t used)
{
    const char *pkg = base.mpPackageName;
    for (int32_t i = used + 1; i <= max; ++i) {
        Hide(pkg, Hash("RANKING_DATA_%d", i));
        Hide(pkg, Hash("PLAYER_DATA_%d", i));
        Hide(pkg, Hash("CAR_DATA_%d", i));
        Hide(pkg, Hash("TIME_DATA_%d", i));
    }
}

void PostRaceResultsScreen::NotificationMessage(uint32_t msg, void *, uint32_t, uint32_t)
{
    switch (msg) {
    case 0x406415E3u:   // accept
        if (*bridge::GameByte(0x89E7E6) && (*bridge::GameByte(0x89E7D8) || *bridge::GameInt(0x890108))) {
            bridge::Profile_5100B0();
            bridge::RaceCoordinator_PostMessage(9, 0);
        } else {
            bridge::PostRaceManager_ChangeToNextScreen();
        }
        dismissDialog();
        return;
    case 0x34DC1BECu:
        dismissDialog();
        bridge::Online_601D00();
        bridge::PostRaceManager_ChangeToNextScreen();
        return;
    case 0x1FAB5998u:
        if (mDialog) DialogInterface::DismissDialog(mDialog);
        mDialog = 0;
        bridge::Online_609750();
        return;
    case 0xC519BFC0u:   // stats
        if (!Online()) {
            *bridge::GameByte(0x7F45A1) = 0;
            FEngSwitchPackages(base.mpPackageName, "UI_PostRaceStats.fng", 0);
        } else if (bridge::Online_5FE500()) {
            bridge::Online_601D00();
            bridge::PostRaceManager_ChangeToNextScreen();
        }
        return;
    case 0xEAACFEEDu:
        bridge::FEngSetFirstFrontendScreen(bridge::FirstScreenAfterQuitRace());
        bridge::RaceCoordinator_PostMessage(0x15, 0);
        return;
    case 0xC98356BAu: {   // tick: flip finish / split times
        if (Online()) {
            bridge::PostRace_ManageOnlineUpdates(this);
            return;
        }
        if (DriftRace() || *bridge::GameByte(0x89E7E6)) return;
        const long double t = mFlipTimer, p = mFlipPeriod;
        if (!(t > p)) {   // fcomp: <=, or unordered -> keep counting
            mFlipTimer = static_cast<float>(static_cast<long double>(bridge::FrameSeconds()) + t);
            return;
        }
        mbSplits = !mbSplits;
        mFlipTimer = 0.0f;
        mFlipPeriod = mbSplits ? 1.75f : 4.5f;
        ResultsUpdate();
        return;
    }
    default: return;
    }
}

// ---- PostRaceRewardsScreen ----
PostRaceRewardsScreen *PostRaceRewardsScreen::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.vtable = bridge::GameVTable(0x7950D8);
    PostRace_SetControlMask(base.mpPackageName);
    data->mpPackage->mbFlagsAC[0] = 0;   // GC Init: PC 504820(0) on the constructor data's package (+0xAC)
    Setup();
    return this;
}

void PostRaceRewardsScreen::Destruct() { base.vtable = bridge::MenuScreenVTable(); }   // PC 4BF970: vtable, MenuScreen dtor

PostRaceRewardsScreen *PostRaceRewardsScreen::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void PostRaceRewardsScreen::Setup()
{
    const char *pkg = base.mpPackageName;
    // cash: x87 GetSponsorPointsEarned (float) + 0.5, _ftol
    const auto cash = static_cast<int32_t>(static_cast<long double>(bridge::Career_GetSponsorPointsEarned()) + 0.5f);
    bridge::FEngPrintfInt(pkg, 0x18D97A23u, "%d", cash);
    bridge::FEngPrintfInt(pkg, 0x18D97A24u, "%d", bridge::Career_GetLastPurse());
    bridge::FEngPrintfString(pkg, 0xEB406FECu, "%s", bridge::DriverName(*bridge::GamePtrSlot(0x8900AC)));
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, pkg, 0xFF, 0);
    bridge::HelpBar_SetFlag6C(1);   // PC 54E6A0
    bridge::HelpBar_542990(true);
    bridge::HelpBar_SyncByMask(base.mData.mHelpBarMask, pkg);
}

void PostRaceRewardsScreen::NotificationMessage(uint32_t msg, void *, uint32_t, uint32_t)
{
    if (msg == 0x406415E3u) cPostRaceScreenManager::Instance()->ChangeToNextScreen();
    else if (msg == 0x911AB364u) cPostRaceScreenManager::Instance()->ChangeToPrevScreen();   // PC: inlined
}
