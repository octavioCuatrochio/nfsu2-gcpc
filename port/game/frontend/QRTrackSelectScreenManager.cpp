#include "QRTrackSelectScreenManager.h"
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "cFEng.h"
#include "FrontendDatabase.h"

namespace {
constexpr uint32_t kVTable = 0x79771C;
// PC name globals (const char *): 0x7F445C mode select, 0x7F4460 mode options, 0x7F4464 track select,
// 0x7F4468 UI_Main, 0x7F446C car select, 0x7F4470 PC_OL_GameRoom, 0x7F4478 PC_OL_Lobby
const char *ModeSelect() { return *bridge::GameStringSlot(0x7F445C); }
const char *ModeOptions() { return *bridge::GameStringSlot(0x7F4460); }
const char *TrackSelect() { return *bridge::GameStringSlot(0x7F4464); }
const char *MainMenu() { return *bridge::GameStringSlot(0x7F4468); }
const char *CarSelect() { return *bridge::GameStringSlot(0x7F446C); }
const char *GameRoom() { return *bridge::GameStringSlot(0x7F4470); }
const char *Lobby() { return *bridge::GameStringSlot(0x7F4478); }
bool Online(int32_t m) { return m == 8 || m == 0x200; }
int32_t RaceMode() { return bridge::FrontendDatabase()->mProfiles[0].mQuickRace.mRaceMode; }   // PC 0x83AAB4
int32_t OnlineRaceKind() { return *bridge::GameInt(0x850088); }   // GetOnlineSettings() +8
}  // namespace

QRTrackSelectScreenManager *&QRTrackSelectScreenManager::Instance()
{
    return *reinterpret_cast<QRTrackSelectScreenManager **>(bridge::GamePtrSlot(0x836340));
}

QRTrackSelectScreenManager *QRTrackSelectScreenManager::Construct(const char *front, const char *back)
{
    vtable = bridge::GameVTable(kVTable);
    mHead = mTail = reinterpret_cast<QRScreenNode *>(&mHead);
    const char *names[3] = {ModeSelect(), ModeOptions(), TrackSelect()};
    for (int32_t i = 0; i < 3; ++i) {
        if (FindScreen(names[i])) continue;
        auto *n = static_cast<QRScreenNode *>(bridge::Alloc(sizeof(QRScreenNode)));
        if (n) {
            n->mpName = names[i];
            n->mCalcNext = bridge::QRManager_CalcFn(i * 2);
            n->mCalcPrev = bridge::QRManager_CalcFn(i * 2 + 1);
        }
        QRScreenNode *tail = mTail;   // PC: no null check on the new node
        tail->next = n;
        mTail = n;
        n->prev = tail;
        n->next = reinterpret_cast<QRScreenNode *>(&mHead);
    }
    mpFront = front;
    mpBack = back;
    return this;
}

void QRTrackSelectScreenManager::Destruct()
{
    vtable = bridge::GameVTable(kVTable);
    auto *s = reinterpret_cast<QRScreenNode *>(&mHead);
    while (mHead != s) {
        QRScreenNode *n = mHead;
        n->prev->next = n->next;
        n->next->prev = n->prev;
        bridge::FreeArray(n);
    }
}

QRTrackSelectScreenManager *QRTrackSelectScreenManager::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

QRScreenNode *QRTrackSelectScreenManager::FindScreen(const char *name)
{
    auto *s = reinterpret_cast<QRScreenNode *>(&mHead);
    for (QRScreenNode *n = mHead; n != s; n = n->next)
        if (bStrICmp(n->mpName, name) == 0) return n;
    return nullptr;
}

void QRTrackSelectScreenManager::Start(const char *from, const char *to)
{
    mpCurrent = FindScreen(to);
    FEngSwitchPackages(from, to, 0);
}

void QRTrackSelectScreenManager::ChangeToNextScreen()
{
    const char *from = mpCurrent->mpName;
    const char *to = bridge::QRManager_CallCalc(mpCurrent->mCalcNext, this);
    mpCurrent = FindScreen(to);
    FEngSwitchPackages(from, to, 0);
    if (IsBackdoor(to)) CommitSuicide();
}

void QRTrackSelectScreenManager::ChangeToPrevScreen()
{
    const char *from = mpCurrent->mpName;
    const char *to = bridge::QRManager_CallCalc(mpCurrent->mCalcPrev, this);
    mpCurrent = FindScreen(to);
    FEngSwitchPackages(from, to, 0);
    if (!IsFrontdoor(to)) return;
    if (bStrCmp(MainMenu(), to) == 0 && bridge::GameModeFlags() == 4) bridge::FrontendDatabase()->RestorePreviousStable();
    CommitSuicide();
}

bool QRTrackSelectScreenManager::IsBackdoor(const char *name) const { return bStrICmp(name, mpBack) == 0; }
bool QRTrackSelectScreenManager::IsFrontdoor(const char *name) const { return bStrICmp(name, mpFront) == 0; }

void QRTrackSelectScreenManager::CommitSuicide()
{
    if (QRTrackSelectScreenManager *m = Instance()) bridge::VirtualScalarDelete(m);
    Instance() = nullptr;
}

void QRTrackSelectScreenManager::EnterMiddle()
{
    if (Instance()) return;
    const char *back = CarSelect(), *front = MainMenu();
    if (Online(bridge::GameModeFlags())) {
        back = GameRoom();
        front = Lobby();
    }
    auto *m = static_cast<QRTrackSelectScreenManager *>(bridge::Alloc(sizeof(QRTrackSelectScreenManager)));
    Instance() = m ? m->Construct(front, back) : nullptr;
    Instance()->mpCurrent = Instance()->FindScreen(ModeSelect());   // PC: no null check
}

void QRTrackSelectScreenManager::EnterFront(const char *from)
{
    if (Instance()) return;
    EnterMiddle();
    Instance()->Start(from, ModeSelect());
}

void QRTrackSelectScreenManager::EnterBack(const char *from)
{
    if (Instance()) return;
    EnterMiddle();
    const uint32_t r = static_cast<uint32_t>(RaceMode());
    const char *to;
    if (r <= 4 || r == 7 || r == 8 || r == 10) to = ModeOptions();
    else if (r == 9) to = ModeSelect();
    else to = TrackSelect();   // 5, 6, > 10 (unsigned: negatives too)
    Instance()->Start(from, to);
}

const char *QRTrackSelectScreenManager::CalcNextScreenFromModeSelect()
{
    const int32_t m = bridge::GameModeFlags();
    const int32_t r = RaceMode();
    if (m == 4) {
        if (r == 8) return ModeOptions();
    } else if (Online(m)) {
        if (OnlineRaceKind() == 8) {   // online getaway: random track
            *reinterpret_cast<int16_t *>(bridge::GameInt(0x8500B8)) =
                static_cast<int16_t>(bridge::GetRandomGetAwayTrack(bridge::bRandomInt(0x7FFFFFFF)));
            return ModeOptions();
        }
        return TrackSelect();
    }
    return r == 9 ? CarSelect() : TrackSelect();
}

const char *QRTrackSelectScreenManager::CalcNextScreenFromModeOptions()
{
    return Online(bridge::GameModeFlags()) ? GameRoom() : CarSelect();
}

const char *QRTrackSelectScreenManager::CalcNextScreenFromTrackSelect() { return ModeOptions(); }

const char *QRTrackSelectScreenManager::CalcPrevScreenFromModeSelect()
{
    return bridge::GameModeFlags() == 8 ? Lobby() : mpFront;
}

const char *QRTrackSelectScreenManager::CalcPrevScreenFromModeOptions()
{
    const int32_t m = bridge::GameModeFlags();
    if (m == 4) return RaceMode() == 8 ? ModeSelect() : TrackSelect();
    if (Online(m) && OnlineRaceKind() == 8) return ModeSelect();
    return TrackSelect();
}

const char *QRTrackSelectScreenManager::CalcPrevScreenFromTrackSelect() { return ModeSelect(); }
