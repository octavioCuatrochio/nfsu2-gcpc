#include "PostRaceStats.h"
#include <cstdio>
#include <cstring>
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "cFEng.h"
#include "FEHash.h"
#include "FEngUtil.h"
#include "FrontendDatabase.h"
#include "PostRaceMenu.h"
#include "PostRaceResults.h"

namespace {
constexpr uint32_t kVTable = 0x799108;
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";   // PC GetHelpBarFNGName 50CEA0
constexpr uint32_t kPrev = 0x9120409Eu, kNext = 0xB5971BF1u;
int32_t I32(const uint8_t *p, uint32_t off) { int32_t v; std::memcpy(&v, p + off, 4); return v; }
float F32(const uint8_t *p, uint32_t off) { float v; std::memcpy(&v, p + off, 4); return v; }
int32_t RaceDrivers() { return bridge::CurrentRaceDriverCount(); }
const uint8_t *Finisher(int32_t i) { return bridge::FrontendDatabase()->mFinishedRaceStats.mFinishers[i]; }
bool DragOrDyno() { return *bridge::GameByte(0x89E7D8) || *bridge::GameInt(0x890108); }
// PC 50D260 (__cdecl): 0x9120409E -> a, 0xB5971BF1 -> b, else 0
uint32_t HorizontalSound(uint32_t msg, uint32_t a, uint32_t b) { return msg == kPrev ? a : msg == kNext ? b : 0; }
void Sound(uint32_t event) { bridge::MakeFrontendSound(event, 0); }   // PC 51DE60(pkg, event): trigger 0
void SetLabel(FEObject *o, uint32_t lang) { FEngSetLanguageHash(o, lang); }
// x87 speed (float * 3.6 / 2.237, fstp to float) and its unit
float Speed(float v, uint32_t &unit)
{
    const bool metric = *bridge::GameByte(0x83AA1A) == 1;
    unit = metric ? 0xE74B7E13u : 0xE74B86F8u;
    return static_cast<float>(static_cast<long double>(v) * (metric ? 3.6f : 2.237f));
}
}  // namespace

// ---- PR* rows ----
PRStatWidget *PRStatWidget::Construct(uint32_t vt, const uint8_t *entry, const float *pos, const float *size,
                                      const float *valueSize, const float *valuePos, bool enabled)
{
    base.Construct(pos, size, valueSize, valuePos, enabled);
    base.base.vtable = bridge::GameVTable(vt);
    mpEntry = entry;
    return this;
}

void PRStatWidget::Position()
{
    base.Position();
    float x, y;
    FEngGetBottomRight(base.mpValue, &x, &y);
    FEngSetBottomRight(base.mpValue, static_cast<float>(static_cast<long double>(base.mValueSizeX) + base.mValuePosX), y);
    FEngGetTopLeft(base.mpValue, &x, &y);
    FEngSetTopLeft(base.mpValue, x, base.mValuePosY);
}

void PRStatWidget::DrawFinishTime()
{
    char buf[0x20];
    SetLabel(base.mpObject, 0xF9D7F4C2u);
    bridge::Timer_PrintToString(I32(mpEntry, 0xC), buf, 0);
    bridge::FEPrintfString(base.mpValue, "%s", buf);
}

void PRStatWidget::DrawTimeBehind()
{
    bridge::RacerInfo(I32(mpEntry, 4));   // PC: looked up, unused
    char buf[0x20];
    uint32_t lang = 0;
    switch (I32(mpEntry, 8)) {   // status
    case 1: {
        const uint8_t *first = bridge::FrontendDatabase()->mFinishedRaceStats.mEntries[0];
        if (mpEntry == first) lang = 0x61C9B7A2u;
        else bridge::Timer_PrintToString(I32(mpEntry, 0xC) - I32(first, 0xC), buf, 2);
        break;
    }
    case 3: lang = 0x5D82DBA2u; break;
    case 8: lang = 0x01E66364u; break;
    case 9: lang = 0xB7B75185u; break;
    case 11: lang = 0x1CE129E0u; break;
    case 12: lang = 0xA7DB717Fu; break;
    default: lang = 0xD4293449u; break;
    }
    SetLabel(base.mpObject, 0xAB44ED8Bu);
    if (lang) SetLabel(base.mpValue, lang);
    else bridge::FEPrintfString(base.mpValue, "%s", buf);
}

void PRStatWidget::DrawTopSpeed()
{
    uint32_t unit;
    const float v = Speed(F32(mpEntry, 0x10C), unit);
    SetLabel(base.mpObject, 0x5D304669u);
    bridge::FEPrintfDoubleString(base.mpValue, "%$3.1f %s", v, bridge::LocalizedString(unit));
}

void PRStatWidget::DrawAverageSpeed()
{
    uint32_t unit;
    const float v = Speed(F32(mpEntry, 0x110), unit);
    SetLabel(base.mpObject, 0x5F38E63Eu);
    bridge::FEPrintfDoubleString(base.mpValue, "%$3.1f %s", v, bridge::LocalizedString(unit));
}

void PRStatWidget::DrawPerfectShifts()
{
    SetLabel(base.mpObject, 0x680AC597u);
    bridge::FEPrintfInts2(base.mpValue, "%$d/%$d", I32(mpEntry, 0x128), I32(mpEntry, 0x124));
}

void PRStatWidget::DrawBestLapTime()
{
    char buf[0x20];
    SetLabel(base.mpObject, 0xAF6943CBu);
    bridge::Timer_PrintToString(I32(mpEntry, 0x10), buf, 0);
    bridge::FEPrintfString(base.mpValue, "%s", buf);
}

void PRStatWidget::DrawLapsLed()
{
    SetLabel(base.mpObject, 0xADBFD917u);
    bridge::FEPrintfInts2(base.mpValue, "%d/%d", I32(mpEntry, 0xF4), *bridge::GameInt(0x89E7BC));
}

void PRStatWidget::DrawBestDriftScore()
{
    SetLabel(base.mpObject, 0xD7D8CA79u);
    bridge::FEPrintfInt(base.mpValue, "%$d", I32(mpEntry, 0x214));
}

void PRStatWidget::DrawLongestDrift()
{
    char buf[0x20];
    SetLabel(base.mpObject, 0x5C17EC47u);
    // seconds -> ticks: x87 v * 4000 + 0.5, _ftol (truncation)
    const auto t = static_cast<int32_t>(static_cast<long double>(F32(mpEntry, 0x218)) * 4000.0f + 0.5f);
    bridge::Timer_PrintToString(t, buf, 0);
    bridge::FEPrintfString(base.mpValue, "%s", buf);
}

void PRStatWidget::DrawScore()
{
    const auto v = static_cast<int32_t>(static_cast<long double>(F32(mpEntry, 0x21C)));   // _ftol
    SetLabel(base.mpObject, 0xE8AEBFAFu);
    bridge::FEPrintfInt(base.mpValue, "%$d", v);
}

void PRLapResult::Draw()
{
    char buf[0x20];
    bridge::Timer_PrintToString(mTime, buf, 0);
    bridge::FEPrintfIntString(base.mpObject, "%d   %s", mLap, buf);
    bridge::FEPrintfInt(base.mpValue, "%d", mPosition);
}

// ---- screen ----
PostRaceStatsScreen *PostRaceStatsScreen::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::GameVTable(kVTable);
    _unknown14C = 0;
    mTimeX = mTimeY = mTimeW = mTimeH = 0.0f;
    mPosX = mPosY = mPosW = mPosH = 0.0f;
    mLapX = mLapY = 0.0f;
    mLaps.next = mLaps.prev = &mLaps;
    mLapRow = 1;
    mRacer = 0;
    mbLaterLaps = 0;
    mpEntry = nullptr;
    PostRace_SetControlMask(base.base.mpPackageName);
    for (int32_t idx = 0; idx < RaceDrivers(); ++idx) {
        if (I32(Finisher(idx), 4) != 0) continue;   // the player's (driver 0) sheet first
        if (idx == 0) {
            Init();
            SetNavigationArrows(0, kPrev);
        } else {
            for (int32_t i = 0; i < idx; ++i) {   // Scroll(next) inlined from each position
                mRacer = i;
                Scroll(kNext);
            }
        }
    }
    return this;
}

void PostRaceStatsScreen::Destruct()
{
    base.base.vtable = bridge::GameVTable(kVTable);
    DeleteAllNodeOwners(&mLaps);   // bTList dtor (PC 4358E0)
    base.DestructBody();
}

PostRaceStatsScreen *PostRaceStatsScreen::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void PostRaceStatsScreen::NotificationMessage(uint32_t msg, FEObject *, uint32_t, uint32_t)
{
    const char *pkg = base.base.mpPackageName;
    switch (msg) {
    case kPrev:
    case kNext: Scroll(msg); return;
    case 0x911AB364u:   // back
        FEngSwitchPackages(pkg, *bridge::GameByte(0x7F45A1) ? "UI_PostRace.fng" : "UI_PostRaceResults.fng", 0);
        Sound(0xA);
        return;
    case 0x9803F6E2u: {   // PC mouse: the arrows
        FEObject *left = FEngFindImage(pkg, 0x444969FDu);
        FEObject *right = FEngFindImage(pkg, 0x444969FEu);
        if (FEngIsClickedOn(left)) Scroll(kPrev);
        else if (FEngIsClickedOn(right)) Scroll(kNext);
        return;
    }
    default: return;
    }
}

void PostRaceStatsScreen::Init()
{
    const char *pkg = base.base.mpPackageName;
    FEObject *time = FEngFindImage(pkg, bStringHash("TIME_MASTER"));
    FEObject *pos = FEngFindImage(pkg, bStringHash("POSITION_MASTER"));
    float a, b;
    FEngGetTopLeft(time, &a, &b);
    mTimeX = a;
    FEngGetTopLeft(time, &a, &b);
    mTimeY = b;
    FEngGetSize(time, &a, &b);
    mTimeW = a;
    FEngGetSize(time, &a, &b);
    mTimeH = b;
    FEngGetTopLeft(pos, &a, &b);
    mPosX = a;
    FEngGetTopLeft(pos, &a, &b);
    mPosY = b;
    FEngGetSize(pos, &a, &b);
    mPosW = a;
    FEngGetSize(pos, &a, &b);
    mPosH = b;
    mLapY = mTimeY;
    mLapX = mTimeX;
    Setup(mRacer);
}

void PostRaceStatsScreen::SetLapList(int32_t first, int32_t last)
{
    for (int32_t i = first; i <= last; ++i) AddLapResult(i, I32(mpEntry, 0x1C + i * 4), I32(mpEntry, 0xC8 + i * 4));
}

void PostRaceStatsScreen::AddLapResult(int32_t lap, int32_t time, int32_t position)
{
    auto *w = static_cast<PRLapResult *>(bridge::Alloc(sizeof(PRLapResult)));
    if (w) {
        const float lapPos[2] = {mLapX, mLapY}, timeSize[2] = {mTimeW, mTimeH}, posSize[2] = {mPosW, mPosH},
                    posPos[2] = {mPosX, mPosY};
        w->base.Construct(lapPos, timeSize, posSize, posPos, true);
        w->base.base.vtable = bridge::GameVTable(0x7950E8);
        w->mLap = lap;
        w->mTime = time;
        w->mPosition = position;
    }
    const char *pkg = base.base.mpPackageName;
    char fmt[64], name[0x20];
    bStrCat(fmt, "TIME_DATA_", "%d");
    std::snprintf(name, sizeof name, fmt, mLapRow);   // PC FEngSNPrintf 50C960
    w->base.mpObject = FEngFindString(pkg, FEHashUpper(name));
    bStrCat(fmt, "POSITION_DATA_", "%d");
    std::snprintf(name, sizeof name, fmt, mLapRow);
    w->base.mpValue = FEngFindString(pkg, FEHashUpper(name));
    auto *node = reinterpret_cast<WidgetListNode *>(&w->base.base._unknown04);
    WidgetListNode *tail = mLaps.prev;
    tail->next = node;
    mLaps.prev = node;
    node->prev = tail;
    node->next = &mLaps;
    mLapY = static_cast<float>(static_cast<long double>(mTimeH) + mLapY + base.mSpacing);
    mPosY = static_cast<float>(static_cast<long double>(mPosH) + mPosY + base.mSpacing);
    if (!w->base.base.mbEnabled) bridge::FEWidget_Disable(w);
    bridge::FEWidget_Show(w);
    bridge::FEWidget_Position(w);
    bridge::FEWidget_Draw(w);
    ++mLapRow;
}

void PostRaceStatsScreen::ClearLapResultWidgets()
{
    for (WidgetListNode *n = mLaps.next; n != &mLaps;) {
        FEWidget *w = uiwm::FromNode(n);
        bridge::FEWidget_Hide(w);
        bridge::FEWidget_UnsetFocus(w);
        n = n->next;
    }
    DeleteAllNodeOwners(&mLaps);
    mLapRow = 1;
}

void PostRaceStatsScreen::Setup(int32_t racer)
{
    const char *pkg = base.base.mpPackageName;
    mpEntry = Finisher(racer);
    const uint8_t *info = bridge::RacerInfo(I32(mpEntry, 4));
    const uint8_t *car;
    std::memcpy(&car, info + 0x14, sizeof car);
    const char *name = reinterpret_cast<const char *>(car + 0x90C);
    if (I32(car, 4) == 1 && !car[0x900]) {
        void *player;
        std::memcpy(&player, info + 0x18, sizeof player);
        name = bridge::DriverName(player);
    }
    const uint8_t *ride;
    std::memcpy(&ride, info + 0x10, sizeof ride);
    bridge::FEngPrintfIntStrings(pkg, bStringHash("RACER_DATA"), "%2d   %s - %s", I32(mpEntry, 0), name,
                                 bridge::CarTypeDisplayName(I32(ride, 0)));
    if (!DragOrDyno() && !*bridge::GameInt(0x89E7B4)) {
        const int32_t laps = *bridge::GameInt(0x89E7BC);
        if (laps <= 10) SetLapList(1, laps);
        else if (mbLaterLaps == 1) SetLapList(11, laps);
        else SetLapList(1, 10);
    } else {
        FEngSetInvisible(FEngFindObject(pkg, bStringHash("HEAD_TEXT_2")));
        FEngSetInvisible(FEngFindObject(pkg, bStringHash("HEAD_TEXT_3")));
    }
    auto add = [&](uint32_t vt) {
        auto *w = static_cast<PRStatWidget *>(bridge::Alloc(sizeof(PRStatWidget)));
        if (w) w->Construct(vt, mpEntry, &base.mStartX, &base.mTitleW, &base.mDataW, &base.mDataX, true);
        base.AddStatOption(w ? &w->base : nullptr);
    };
    if (*bridge::GameByte(0x89E7D9) || *bridge::GameInt(0x890104)) {   // drift
        add(0x795388);   // PRScore
        add(0x795308);   // PRBestDriftScore
        add(0x795348);   // PRLongestDrift
        add(0x7951B0);   // PRTopSpeed
        add(0x795200);   // PRAverageSpeed
    } else {
        add(0x795130);   // PRFinishTime
        add(0x795170);   // PRTimeBehind
        add(0x7951B0);
        add(0x795200);
        if (DragOrDyno()) add(0x795240);   // PRPerfectShifts
        if (!DragOrDyno() && !*bridge::GameInt(0x89E7B4)) {
            add(0x795280);   // PRBestLapTime
            add(0x7952C0);   // PRLapsLed
        }
    }
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, pkg, 0xFF, 0);
    bridge::HelpBar_SetFlag6C(1);   // PC 54E6A0
    bridge::HelpBar_542990(true);
    bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, pkg);
}

void PostRaceStatsScreen::SetNavigationArrows(int32_t racer, uint32_t msg)
{
    const char *pkg = base.base.mpPackageName;
    const int32_t last = RaceDrivers() - 1;
    auto show = [&](uint32_t h, bool on) {
        if (on) FEngSetVisible(FEngFindObject(pkg, h));
        else FEngSetInvisible(FEngFindObject(pkg, h));
    };
    if (racer == 0) {
        show(0x444969FDu, false);
        show(0x444969FEu, true);
    } else if (racer == last) {
        show(0x444969FDu, true);
        show(0x444969FEu, false);
    } else {
        show(0x444969FDu, true);
        show(0x444969FEu, true);
    }
    const int32_t params = *bridge::GameInt(0x89E7A0);
    if (params >= 0x104E && params <= 0x1053) {   // downhill: no racer sheets to browse
        show(0x444969FDu, false);
        show(0x444969FEu, false);
        return;
    }
    uint32_t left = 0, right = 0;
    if (msg == kPrev) {
        left = bStringHash(mRacer == 0 ? "HIDE" : "BLINK");
        right = bStringHash("STATIC");
    } else if (msg == kNext) {
        right = bStringHash(mRacer == last ? "HIDE" : "BLINK");
        left = bStringHash("STATIC");
    }
    FEngSetScript(pkg, 0x444969FDu, left, true);
    FEngSetScript(pkg, 0x444969FEu, right, true);
}

void PostRaceStatsScreen::Scroll(uint32_t msg)
{
    const int32_t count = RaceDrivers();
    const int32_t old = mRacer;
    if (msg == kPrev) {
        mRacer = old - 1 < 0 ? 0 : old - 1;
    } else if (msg == kNext) {
        mRacer = old + 1 > count - 1 ? count - 1 : old + 1;
    }
    if (old != mRacer) {
        Sound(HorizontalSound(msg, 4, 5));
        base.mCurrentIndex = 1;
        mLapRow = 1;
        mLapY = 0.0f;
        base.ClearWidgets();
        ClearLapResultWidgets();
        mbLaterLaps = 0;
        Init();
    }
    SetNavigationArrows(mRacer, msg);
}
