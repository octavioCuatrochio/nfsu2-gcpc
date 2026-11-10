#include "PostRaceScreenManager.h"
#include "bridge/Bridge.h"
#include "bWare/bSlotPool.h"
#include "cFEng.h"
#include "FEngUtil.h"
#include "FrontendDatabase.h"

namespace {
constexpr uint32_t kVTable = 0x797B7C;
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";   // PC GetHelpBarFNGName 50CEA0
// PC screen name globals: 0x7F4570 results, 74 tourn results, 78 tourn standings, 7C reward, 84 sponsor popup,
// 88 sponsorship new, 90 UI_PostRace.fng
const char *Name(uint32_t a) { return *bridge::GameStringSlot(a); }
int32_t Mode() { return bridge::GameModeFlags(); }
bool Online(int32_t m) { return m == 8 || m == 0x200; }
int32_t RaceMode() { return *bridge::GameInt(0x83AAB4); }   // QuickRaceSettings race mode
bool DragDynoSheet() { return *bridge::GameByte(0x89E7E6) != 0; }
bool CareerMultiRace() { return *bridge::GameInt(0x863470) && *bridge::GameInt(0x85EF4C) > 1; }   // tournament / multi-event
int32_t CareerStage() { return *bridge::GameInt(0x862838); }
}  // namespace

cPostRaceScreenManager *&cPostRaceScreenManager::Instance()
{
    return *reinterpret_cast<cPostRaceScreenManager **>(bridge::GamePtrSlot(0x8363DC));
}

void cPostRaceScreenManager::DestroyInstance()
{
    if (cPostRaceScreenManager *m = Instance()) bridge::VirtualScalarDelete(m);
    Instance() = nullptr;
}

void cPostRaceScreenManager::Add(const char *name)
{
    auto *n = static_cast<Node *>(bPNode_Malloc());
    if (n) n->mpName = name;
    Node *tail = mTail;   // PC: no null check
    tail->next = n;
    mTail = n;
    n->prev = tail;
    n->next = reinterpret_cast<Node *>(&mHead);
}

bool cPostRaceScreenManager::NeedToSeePostRaceResults()
{
    const int32_t m = Mode();
    if (m == 1) return !CareerMultiRace();
    if (m == 8) return *bridge::GameInt(0x850098) != 1;
    if (m == 4) return RaceMode() != 8;
    return true;
}

bool cPostRaceScreenManager::NeedToSeePostRaceTournamentResults()
{
    if (DragDynoSheet()) return false;
    const int32_t m = Mode();
    if (m == 1) return CareerMultiRace();
    if (Online(m)) {
        if (*bridge::GameInt(0x850088) == 8) return true;
        return m == 8 && *bridge::GameInt(0x850098) == 1;
    }
    return m == 4 && RaceMode() == 8;
}

bool cPostRaceScreenManager::NeedToSeePostRaceTournamentStandings()
{
    if (DragDynoSheet()) return false;
    const int32_t m = Mode();
    if (m == 1) return CareerMultiRace();
    if (Online(m)) return *bridge::GameInt(0x850088) == 8;
    return m == 4 && RaceMode() == 8;
}

bool cPostRaceScreenManager::NeedToSeePostRaceRewards()
{
    return !DragDynoSheet() && Mode() == 1 && bridge::Career_5287A0(0) && bridge::Career_500DB0() && !bridge::Career_500DD0();
}

bool cPostRaceScreenManager::NeedToAskForSponsor()
{
    if (Mode() != 1 || CareerMultiRace()) return false;
    const int32_t stage = CareerStage();
    if (!bridge::CareerStage_500610(stage)) return false;
    if (stage <= 1) return false;
    return bridge::CareerStage_500670(stage, bridge::CareerStage_5005E0(stage)) > 0;
}

bool cPostRaceScreenManager::NeedToSeePostRaceMenu()
{
    if (DragDynoSheet()) return false;
    const int32_t m = Mode();
    if (Online(m)) return false;
    return !(m == 4 && RaceMode() == 8);
}

bool cPostRaceScreenManager::NeedToRemoveTheSponsorScreens()
{
    return Mode() == 1 && *bridge::GameInt(0x862100 + CareerStage() * 0x104) != 0;
}

cPostRaceScreenManager *cPostRaceScreenManager::Construct()
{
    vtable = bridge::GameVTable(kVTable);
    _unknown10 = 0;
    mMsgHead = mMsgTail = &mMsgHead;
    _unknown28 = 0;
    mb24 = 0;
    _unknown1C_unused = 0;
    _unknown0C = 0;
    mCountdownSteps = 5;
    mCountdownTime = 2.0f;
    mpTimerName = "OL_Countdown_Timer";   // PC 0x7964A4
    _unknown2C = 1;
    mHead = mTail = reinterpret_cast<Node *>(&mHead);
    if (NeedToSeePostRaceResults()) Add(Name(0x7F4570));
    if (NeedToSeePostRaceTournamentResults()) Add(Name(0x7F4574));
    if (NeedToSeePostRaceTournamentStandings()) Add(Name(0x7F4578));
    if (NeedToSeePostRaceRewards()) Add(Name(0x7F457C));
    if (NeedToAskForSponsor()) Add(Name(0x7F4584));
    if (NeedToAskForSponsor()) Add(Name(0x7F4588));
    if (NeedToSeePostRaceMenu()) Add(Name(0x7F4590));
    if (mHead == reinterpret_cast<Node *>(&mHead)) {   // nothing to show: straight back
        *bridge::GameByte(0x7F45A0) = 0;
        bridge::FEngSetFirstFrontendScreen(bridge::FirstScreenAfterQuitRace());
        bridge::FrontendDatabase()->NotifyExitRaceToFrontend(1);
        bridge::RaceCoordinator_PostMessage(0xB, 0);
    }
    mpCurrent = mHead;
    for (Node *n = mHead; n != reinterpret_cast<Node *>(&mHead); n = n->next) FEngSetUseIdleList(n->mpName, true);
    if (!FEngIsPackagePushed(kHelpBar)) FEngPushPackageSpecial(0xE, kHelpBar, "", 0xFF, 0);
    return this;
}

void cPostRaceScreenManager::Destruct()
{
    vtable = bridge::GameVTable(kVTable);
    auto *s = reinterpret_cast<Node *>(&mHead);
    for (Node *n = mHead; n != s; n = n->next) {
        FEngSetUseIdleList(n->mpName, false);
        FEngPopPackage(n->mpName);
    }
    if (FEngIsPackagePushed(kHelpBar)) FEngPopPackage(kHelpBar);
    while (mHead != s) {
        Node *n = mHead;
        n->prev->next = n->next;
        n->next->prev = n->prev;
        bPNode_Free(n);
    }
    bridge::PostRaceMessageCenter_Destruct(&mCountdownTime);   // +0x04 member dtor (network)
}

cPostRaceScreenManager *cPostRaceScreenManager::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void cPostRaceScreenManager::ChangeToNextScreen()
{
    if (mTail != mpCurrent) {
        const char *from = mpCurrent->mpName;
        mpCurrent = mpCurrent->next;
        FEngSwitchPackages(from, mpCurrent->mpName, 0);
        return;
    }
    const int32_t m = Mode();
    if (Online(m)) {
        if (bridge::Online_5FE4C0()) {
            bridge::RaceCoordinator_PostMessage(9, 0);
            return;
        }
    } else if (m == 4 && RaceMode() == 8) {
        if (!bridge::FrontendDatabase()->mProfiles[0].mQuickRace.HasAPlayerWonTheOutrunTourney()) {
            bridge::Call526830(1);
            bridge::RaceCoordinator_PostMessage(9, 0);
            return;
        }
    } else {
        return;
    }
    bridge::FEngSetFirstFrontendScreen(bridge::FirstScreenAfterQuitRace());
    bridge::RaceCoordinator_PostMessage(0x15, 0);
}

void cPostRaceScreenManager::ChangeToPrevScreen()
{
    if (mHead == mpCurrent) return;
    const char *from = mpCurrent->mpName;
    mpCurrent = mpCurrent->prev;
    FEngSwitchPackages(from, mpCurrent->mpName, 0);
}

void *cPostRaceScreenMgrInterface::Construct(void *self)
{
    auto *m = static_cast<cPostRaceScreenManager *>(bridge::Alloc(sizeof(cPostRaceScreenManager)));
    cPostRaceScreenManager::Instance() = m ? m->Construct() : nullptr;
    return self;
}

void cPostRaceScreenMgrInterface::Destruct() { cPostRaceScreenManager::DestroyInstance(); }

void cPostRaceScreenMgrInterface::Suspend()
{
    cPostRaceScreenManager *m = cPostRaceScreenManager::Instance();
    if (!m) return;
    bridge::HelpBar_Off();   // PC 54E6B0
    bridge::FEngStopInteraction(m->mpCurrent->mpName);
}

void cPostRaceScreenMgrInterface::Resume()
{
    cPostRaceScreenManager *m = cPostRaceScreenManager::Instance();
    if (!m) return;
    FEngStartInteraction(m->mpCurrent->mpName, 0);
    bridge::HelpBar_SetFlag6C(1);   // PC 54E6A0 (tail call)
    bridge::HelpBar_542990(true);
}
