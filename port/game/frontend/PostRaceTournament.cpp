#include "PostRaceTournament.h"
#include <cstring>
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "cFEng.h"
#include "DialogInterface.h"
#include "FEHash.h"
#include "FEngUtil.h"
#include "FrontendDatabase.h"
#include "PostRaceScreenManager.h"

namespace {
constexpr uint32_t kVTable = 0x797B80;
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";   // PC GetHelpBarFNGName 50CEA0
struct Node {   // 0x18
    bNode links;
    const uint8_t *entry;
    int32_t driver, points, total;
};
int32_t I32(const uint8_t *p, uint32_t off) { int32_t v; std::memcpy(&v, p + off, 4); return v; }
int32_t Mode() { return bridge::GameModeFlags(); }
bool OnlineGetaway() { const int32_t m = Mode(); return (m == 0x200 || m == 8) && *bridge::GameInt(0x850088) == 8; }
bool SplitOutrun() { return Mode() == 4 && *bridge::GameInt(0x83AAB4) == 8; }
bool OutrunTimed() { return *bridge::GameInt(0x83AAB4 + 0x10C) != 0 && *bridge::GameInt(0x83AAB4 + 0x110) == 1; }
int32_t CareerRacers() { return *bridge::GameInt(0x85BAB4); }
// PC byte table 0x78DAA7 (1-based position -> points): 10, 6, 3, 0..
int32_t Points(int32_t pos) { static const int8_t t[] = {0, 10, 6, 3, 0, 0, 0, 0}; return pos >= 0 && pos < 8 ? t[pos] : 0; }
uint32_t Hash(const char *fmt, int32_t i) { return bridge::FEngHashStringInt(fmt, i); }
const uint8_t *Entry(int32_t i) { return bridge::FrontendDatabase()->mFinishedRaceStats.mEntries[i]; }
void Append(bList &l, Node *n)
{
    bNode *tail = l.mHead.prev;   // PC: no null check
    tail->next = &n->links;
    l.mHead.prev = &n->links;
    n->links.prev = tail;
    n->links.next = &l.mHead;
}
cPostRaceScreenManager *Manager() { return cPostRaceScreenManager::Instance(); }
}  // namespace

int32_t SortTournamentResults(bNode *a, bNode *b)
{
    return I32(reinterpret_cast<Node *>(b)->entry, 0) >= I32(reinterpret_cast<Node *>(a)->entry, 0);
}

cPostRaceTournamentResults *cPostRaceTournamentResults::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.vtable = bridge::GameVTable(kVTable);
    mResults.mHead.next = mResults.mHead.prev = &mResults.mHead;
    mDialog1 = mDialog2 = 0;
    mb5C = 0;
    _unknown60 = 0;
    mb64 = 0;
    mpPkg = base.mpPackageName;
    _unknown6C = 0;
    mOnlineDialog = 0;
    mb74 = 0;
    _unknown78 = 0;
    mb7C = mb7D = mb7E = mb7F = mb80 = 0;
    _unknown84 = _unknown88 = 0;
    *bridge::PostRaceManager_CurrentScreen() = base.mpPackageName;
    if (bridge::OnlineBusy() && *bridge::GameInt(0x850098) == 1) {   // network: lobby callback, countdown helper
        bridge::TournResults_OnlineRegister(this, true);
        SetRaceSpecificHeader();
        SetUnusedEntriesInvisible(6, 0);
        bridge::TournResults_OnlineHelperInit(&mOnlineDialog, this);
    } else {
        Setup();
    }
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, base.mpPackageName, 0xFF, 0);
    bridge::HelpBar_SetFlag6C(1);   // PC 54E6A0
    bridge::HelpBar_542990(true);
    if (Mode() == 8 && bridge::Online_5FE500()) bridge::HelpBar_SyncByMask(base.mData.mHelpBarMask | 0x1000000u, base.mpPackageName);
    else bridge::HelpBar_SyncByMask(base.mData.mHelpBarMask, base.mpPackageName);
    return this;
}

void cPostRaceTournamentResults::Destruct()
{
    base.vtable = bridge::GameVTable(kVTable);
    bridge::TournResults_OnlineTeardown(this);   // network: state 3, lobby callback off, dialogs +0x54 / +0x58
    const char **current = bridge::PostRaceManager_CurrentScreen();
    if (*current && bStrCmp(*current, base.mpPackageName) == 0) *current = nullptr;
    if (mOnlineDialog) {
        DialogInterface::DismissDialog(mOnlineDialog);
        mOnlineDialog = 0;
    }
    while (mResults.mHead.next != &mResults.mHead) {
        bNode *n = mResults.mHead.next;
        n->prev->next = n->next;
        n->next->prev = n->prev;
        bridge::FreeArray(n);
    }
    base.vtable = bridge::MenuScreenVTable();   // MenuScreen dtor 510D40
}

cPostRaceTournamentResults *cPostRaceTournamentResults::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void cPostRaceTournamentResults::NotificationMessage(uint32_t msg, void *obj, uint32_t arg, uint32_t pkg)
{
    switch (msg) {
    case 0x911AB364u: Manager()->ChangeToPrevScreen(); break;
    case 0x406415E3u: Manager()->ChangeToNextScreen(); break;
    case 0x2C0E485Au:   // network: leave the online tournament
        bridge::TournResults_OnlineLeave(this);
        Manager()->ChangeToNextScreen();
        break;
    case 0xA8DE4CD2u:
        bridge::Call5B7C40();
        *bridge::GameByte(0x864F4C) = 1;
        break;
    case 0xEAACFEEDu:   // quit
        bridge::FEngSetFirstFrontendScreen(bridge::FirstScreenAfterQuitRace());
        bridge::RaceCoordinator_PostMessage(0x15, 0);
        break;
    case 0xC98356BAu:   // tick
        if (bridge::OnlineBusy() && mb64) {
            bridge::TournResults_OnlineTick(this);
        } else {
            bridge::PostRaceMessageCenter_Update(reinterpret_cast<uint8_t *>(Manager()) + 4, *bridge::GameInt(0x865198));
            if (OnlineGetaway() && bridge::Online_5FE4C0() && *bridge::GameInt(0x89D05C) < bridge::RealTimer())
                Manager()->ChangeToNextScreen();
        }
        break;
    default: break;
    }
    bridge::TournResults_OnlineMessages(this, msg, obj, arg, pkg);   // network: lobby helper messages (gated inside)
}

void cPostRaceTournamentResults::SetUnusedEntriesInvisible(int32_t max, int32_t used)
{
    const char *pkg = base.mpPackageName;
    static const char *const kRows[] = {"RANKING_DATA_%d", "PLAYER_DATA_%d", "CAR_DATA_%d", "TIME_DATA_%d", "PLAYER_POINT_DATA_%d"};
    for (int32_t i = 1; i <= used; ++i)
        for (const char *r : kRows) FEngSetVisible(FEngFindObject(pkg, Hash(r, i)));
    if (used > max) return;
    for (int32_t i = used + 1; i <= max + 1; ++i)
        for (const char *r : kRows) FEngSetInvisible(FEngFindObject(pkg, Hash(r, i)));
}

void cPostRaceTournamentResults::Setup()
{
    SetRaceSpecificHeader();
    OrganizeRaceResults();
    PrintRaceResults();
    const char *pkg = base.mpPackageName;
    if (Mode() == 4) {
        if (*bridge::GameInt(0x83AAB4) == 8) {
            SetUnusedEntriesInvisible(6, 2);
            if (OutrunTimed()) FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("Header_text_3")), 0xA40518E9u);
            return;
        }
    } else if (OnlineGetaway()) {
        SetUnusedEntriesInvisible(6, bridge::OnlineRacerCount());
        FEngSetLanguageHash(FEngFindString(pkg, FEHashUpper("Header_text_3")), 0xA40518E9u);
        return;
    }
    SetUnusedEntriesInvisible(6, CareerRacers());
}

void cPostRaceTournamentResults::OrganizeRaceResults()
{
    if (OnlineGetaway()) {
        for (int32_t i = 0; i < bridge::OnlineRacerCount(); ++i) {
            const uint8_t *e = Entry(i);
            auto *n = static_cast<Node *>(bridge::Alloc(sizeof(Node)));
            if (n) {
                n->entry = e;
                n->driver = I32(e, 4);
                n->points = Points(I32(e, 0));
                n->total = 0;
            }
            Append(mResults, n);
        }
    } else if (Mode() == 1) {
        for (int32_t i = 0; i < CareerRacers(); ++i) {
            const int32_t driver = bridge::CareerRacerDriver(i);
            const uint8_t *e = bridge::FrontendDatabase()->mFinishedRaceStats.GetStatsForDriver(driver);
            const int32_t points = bridge::Career_GetPointsThisRace(i);
            const int32_t total = bridge::Career_GetPointsTotal(i);
            auto *n = static_cast<Node *>(bridge::Alloc(sizeof(Node)));
            if (n) {
                n->entry = e;
                n->driver = driver;
                n->points = points;
                n->total = total;
            }
            Append(mResults, n);
        }
    } else {
        return;
    }
    mResults.Sort(&SortTournamentResults);
}

void cPostRaceTournamentResults::PrintRacePosition(int32_t row, const uint8_t *entry)
{
    const char *pkg = base.mpPackageName;
    const int32_t r = row + 1;
    const uint8_t *racer = bridge::RacerInfo(I32(entry, 4));
    const uint8_t *car;
    std::memcpy(&car, racer + 0x14, sizeof car);
    if (I32(car, 4) == 1 && !car[0x900] && !SplitOutrun())
        FEngSetScript(pkg, Hash("RANKING_DATA_%d", r), bStringHash("HIGHLIGHT"), true);
    bridge::FEngPrintfInt(pkg, Hash("RANKING_DATA_%d", r), "%2d", I32(entry, 0));
}

void cPostRaceTournamentResults::PrintDriverName(int32_t row, const uint8_t *entry)
{
    const uint8_t *racer = bridge::RacerInfo(I32(entry, 4));
    const char *name;
    if (bridge::OnlineBusy()) {
        name = bridge::OnlinePlayerName(I32(entry, 4));
    } else {
        const uint8_t *car;
        std::memcpy(&car, racer + 0x14, sizeof car);
        if (I32(car, 4) == 1 && !car[0x900]) {
            void *player;
            std::memcpy(&player, racer + 0x18, sizeof player);
            name = bridge::DriverName(player);
        } else {
            name = reinterpret_cast<const char *>(car + 0x90C);
        }
    }
    bridge::FEngPrintfString(base.mpPackageName, Hash("PLAYER_DATA_%d", row + 1), "%s", name);
}

void cPostRaceTournamentResults::PrintCarName(int32_t row, const uint8_t *entry)
{
    const char *carType;   // racer +0xC: car type info (name first)
    std::memcpy(&carType, bridge::RacerInfo(I32(entry, 4)) + 0xC, sizeof carType);
    const uint32_t name = bridge::FEngHashStringArg("CAR_NAME_%s", carType);
    FEngSetLanguageHash(FEngFindString(base.mpPackageName, Hash("CAR_DATA_%d", row + 1)), name);
}

void cPostRaceTournamentResults::PrintRaceTime(int32_t row, int32_t ticks)
{
    char buf[0x20];
    bridge::Timer_PrintToString(ticks, buf, 0);
    bridge::FEngPrintfString(base.mpPackageName, Hash("TIME_DATA_%d", row + 1), "%s", buf);
}

void cPostRaceTournamentResults::PrintOutrunLeaderTime(int32_t row, const uint8_t *entry) { PrintRaceTime(row, I32(entry, 0x14)); }

void cPostRaceTournamentResults::PrintRacePoints(int32_t row, int32_t points)
{
    bridge::FEngPrintfInt(base.mpPackageName, Hash("PLAYER_POINT_DATA_%d", row + 1), "%$d", points);
}

void cPostRaceTournamentResults::PrintRaceResults()
{
    if (Mode() == 4) {
        if (*bridge::GameInt(0x83AAB4) != 8) goto career;
        for (int32_t row = 0; row < 2; ++row) {
            const uint8_t *e = Entry(row);
            PrintRacePosition(row, e);
            PrintDriverName(row, e);
            PrintCarName(row, e);
            if (OutrunTimed()) PrintOutrunLeaderTime(row, e);
            else PrintRaceTime(row, I32(e, 0xC));
            PrintRacePoints(row, Points(row + 1));
        }
        return;
    }
    if (OnlineGetaway()) {
        for (int32_t row = 0; row < bridge::OnlineRacerCount(); ++row) {
            const Node *n = reinterpret_cast<const Node *>(bridge::bList_GetNode(&mResults, row));
            PrintRacePosition(row, n->entry);
            PrintDriverName(row, n->entry);
            PrintCarName(row, n->entry);
            PrintRacePoints(row, Points(I32(n->entry, 0)));
            PrintRaceTime(row, I32(n->entry, 0x14));
        }
        return;
    }
career:
    for (int32_t row = 0; row < CareerRacers(); ++row) {
        const Node *n = reinterpret_cast<const Node *>(bridge::bList_GetNode(&mResults, row));
        PrintRacePosition(row, n->entry);
        PrintDriverName(row, n->entry);
        PrintCarName(row, n->entry);
        PrintRaceTime(row, I32(n->entry, 0xC));
        PrintRacePoints(row, n->points);
    }
}

void cPostRaceTournamentResults::SetRaceSpecificHeader()
{
    const char *pkg = base.mpPackageName;
    if (bridge::OnlineBusy() && !OnlineGetaway()) bridge::FEngPrintfByName(pkg, "Header_Text", bridge::LocalizedString(0xFF115FFFu));
    if (*bridge::GameByte(0x89E7D9) || *bridge::GameInt(0x890104))
        bridge::FEngPrintfString(pkg, bStringHash("HEADER_TEXT_3"), bridge::LocalizedString(0xC03F9F19u));
}

// ---- cPostRaceTournamentStandings ----
int32_t SortTournamentStandings(bNode *a, bNode *b)
{
    return reinterpret_cast<Node *>(b)->total <= reinterpret_cast<Node *>(a)->total;
}

namespace {
int32_t OutrunScore(int32_t driver) { return *bridge::GameByte(0x83ABC8 + driver); }   // QuickRaceSettings::mOutrunScores
void FreeList(bList &l)
{
    while (l.mHead.next != &l.mHead) {
        bNode *n = l.mHead.next;
        n->prev->next = n->next;
        n->next->prev = n->prev;
        bridge::FreeArray(n);
    }
}
}  // namespace

cPostRaceTournamentStandings *cPostRaceTournamentStandings::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::GameVTable(0x798390);
    mStandings.mHead.next = mStandings.mHead.prev = &mStandings.mHead;
    Setup();
    return this;
}

void cPostRaceTournamentStandings::Destruct()
{
    base.base.vtable = bridge::GameVTable(0x798390);
    const char **current = bridge::PostRaceManager_CurrentScreen();
    if (*current && bStrCmp(*current, base.base.mpPackageName) == 0) *current = nullptr;
    FreeList(mStandings);
    base.Destruct();
}

cPostRaceTournamentStandings *cPostRaceTournamentStandings::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void cPostRaceTournamentStandings::NotificationMessage(uint32_t msg, void *, uint32_t, uint32_t)
{
    switch (msg) {
    case 0x911AB364u: Manager()->ChangeToPrevScreen(); return;
    case 0x406415E3u: Manager()->ChangeToNextScreen(); return;
    case 0xC98356BAu:
        if (bridge::OnlineBusy() && base.mb64) {
            bridge::PostRaceMessageCenter_4A3DA0();   // network
            return;
        }
        bridge::PostRaceMessageCenter_Update(reinterpret_cast<uint8_t *>(Manager()) + 4, *bridge::GameInt(0x865198));
        if (OnlineGetaway() && bridge::Online_5FE4C0() && *bridge::GameInt(0x89D05C) < bridge::RealTimer())
            Manager()->ChangeToNextScreen();
        return;
    case 0xEAACFEEDu:
        bridge::FEngSetFirstFrontendScreen(bridge::FirstScreenAfterQuitRace());
        bridge::RaceCoordinator_PostMessage(0x15, 0);
        return;
    default: return;
    }
}

void cPostRaceTournamentStandings::Setup()
{
    const char *pkg = base.base.mpPackageName;
    OrganizeStandings();
    PrintRaceStandings();
    if ((OnlineGetaway() || SplitOutrun()) && bridge::FrontendDatabase()->mProfiles[0].mQuickRace.IsOutrunOvertime())
        FEngSetScript(pkg, "OverTime", "Show", true);
    *bridge::PostRaceManager_CurrentScreen() = pkg;
    int32_t used;
    if (Mode() == 4) used = *bridge::GameInt(0x83AAB4) == 8 ? 2 : bridge::CurrentRaceDriverCount();
    else used = OnlineGetaway() ? bridge::OnlineRacerCount() : bridge::CurrentRaceDriverCount();
    base.SetUnusedEntriesInvisible(6, used);
    uint32_t next = 0;
    if (OnlineGetaway()) next = bridge::Online_5FE4C0() ? 0x23F359B8u : 0x224936F4u;
    else if (bridge::FrontendDatabase()->mProfiles[0].mQuickRace.HasAPlayerWonTheOutrunTourney()) next = 0x224936F4u;
    else if (bridge::FrontendDatabase()->IsThereANextRace()) next = 0x23F359B8u;
    if (next) bridge::FEngPrintfByName(pkg, "NEXT", bridge::LocalizedString(next));
}

void cPostRaceTournamentStandings::OrganizeStandings()
{
    if (Mode() == 4) {
        if (*bridge::GameInt(0x83AAB4) != 8) goto career;
        for (int32_t row = 0; row < 2; ++row) {
            const uint8_t *e = Entry(row);
            const int32_t score = OutrunScore(I32(e, 4));
            auto *n = static_cast<Node *>(bridge::Alloc(sizeof(Node)));
            if (n) {
                n->entry = e;
                n->driver = row;
                n->points = 0;
                n->total = score;
            }
            Append(mStandings, n);
        }
        mStandings.Sort(&SortTournamentStandings);
        return;
    }
    if (OnlineGetaway()) {
        for (int32_t i = 0; i < bridge::OnlineRacerCount(); ++i) {
            const uint8_t *e = Entry(i);
            const int32_t score = OutrunScore(I32(e, 4));
            auto *n = static_cast<Node *>(bridge::Alloc(sizeof(Node)));
            if (n) {
                n->entry = e;
                n->driver = I32(e, 4);
                n->points = 0;
                n->total = score;
            }
            Append(mStandings, n);
        }
        mStandings.Sort(&SortTournamentStandings);
        int32_t prev = -1, rank = 0;
        for (int32_t i = 0; i < bridge::OnlineRacerCount(); ++i) {   // ties share a rank (scores read by entry order, PC)
            auto *n = reinterpret_cast<Node *>(bridge::bList_GetNode(&mStandings, i));
            const int32_t score = OutrunScore(I32(Entry(i), 4));
            if (prev != score) {
                rank = i;
                prev = score;
            }
            n->points = rank;
        }
        return;
    }
career:
    for (int32_t i = 0; i < CareerRacers(); ++i) {
        const int32_t driver = bridge::CareerRacerDriver(i);
        const uint8_t *e = bridge::FrontendDatabase()->mFinishedRaceStats.GetStatsForDriver(driver);
        const int32_t points = bridge::Career_GetPointsThisRace(i);
        const int32_t total = bridge::Career_GetPointsTotal(i);
        auto *n = static_cast<Node *>(bridge::Alloc(sizeof(Node)));
        if (n) {
            n->entry = e;
            n->driver = driver;
            n->points = points;
            n->total = total;
        }
        Append(mStandings, n);
        mStandings.Sort(&SortTournamentStandings);   // PC: after every insert
    }
}

void cPostRaceTournamentStandings::PrintOrdinalPlacing(int32_t row, const uint8_t *entry, int32_t rank)
{
    const char *pkg = base.base.mpPackageName;
    const int32_t r = row + 1;
    const uint8_t *racer = bridge::RacerInfo(I32(entry, 4));
    const uint8_t *car;
    std::memcpy(&car, racer + 0x14, sizeof car);
    if (I32(car, 4) == 1 && !car[0x900] && !SplitOutrun())
        FEngSetScript(pkg, Hash("RANKING_DATA_%d", r), bStringHash("HIGHLIGHT"), true);
    bridge::FEngPrintfInt(pkg, Hash("RANKING_DATA_%d", r), "%d", rank >= 0 ? rank + 1 : r);
}

void cPostRaceTournamentStandings::PrintPointsTotal(int32_t row, int32_t points)
{
    bridge::FEngPrintfInt(base.base.mpPackageName, Hash("PLAYER_POINT_DATA_%d", row + 1), "%$d", points);
}

void cPostRaceTournamentStandings::PrintRaceStandings()
{
    const char *pkg = base.base.mpPackageName;
    if (Mode() == 4) {
        if (*bridge::GameInt(0x83AAB4) != 8) goto career;
        for (int32_t r = 1; r - 1 < 2; ++r) {
            const Node *n = reinterpret_cast<const Node *>(bridge::bList_GetNode(&mStandings, r - 1));
            const uint8_t *racer = bridge::RacerInfo(I32(n->entry, 4));
            const uint8_t *car;
            std::memcpy(&car, racer + 0x14, sizeof car);
            if (I32(car, 4) == 1 && !car[0x900] && !SplitOutrun())   // never true here (split outrun)
                FEngSetScript(pkg, Hash("RANKING_DATA_%d", r), bStringHash("HIGHLIGHT"), true);
            bridge::FEngPrintfInt(pkg, Hash("RANKING_DATA_%d", r), "%d", r);
            base.PrintDriverName(r - 1, n->entry);
            base.PrintCarName(r - 1, n->entry);
            PrintPointsTotal(r - 1, n->total);
        }
        return;
    }
    if (OnlineGetaway()) {
        for (int32_t i = 0; i < bridge::OnlineRacerCount(); ++i) {
            const Node *n = reinterpret_cast<const Node *>(bridge::bList_GetNode(&mStandings, i));
            PrintOrdinalPlacing(i, n->entry, n->points);
            base.PrintDriverName(i, n->entry);
            base.PrintCarName(i, n->entry);
            PrintPointsTotal(i, n->total);
        }
        return;
    }
career:
    for (int32_t i = 0; i < CareerRacers(); ++i) {
        const Node *n = reinterpret_cast<const Node *>(bridge::bList_GetNode(&mStandings, i));
        PrintOrdinalPlacing(i, n->entry, -1);
        base.PrintDriverName(i, n->entry);
        base.PrintCarName(i, n->entry);
        PrintPointsTotal(i, n->total);
    }
}
