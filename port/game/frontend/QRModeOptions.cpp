#include "QRModeOptions.h"
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "FEngUtil.h"
#include "QRTrackSelect.h"      // TrackInfo
#include "QuickRaceSettings.h"

namespace {
constexpr uint32_t kLeft = 0x9120409Eu, kRight = 0xB5971BF1u;
constexpr uint32_t kOutObject = 0x47FF4E7Cu, kOutScript = 0xDE6EFF34u;
QuickRaceSettings &QRS() { return *bridge::QuickRaceSettingsInstance(); }
bool Online()
{
    const int32_t f = bridge::GameModeFlags();
    return f == 8 || f == 0x200;
}
uint16_t &OnlineLaps() { return *reinterpret_cast<uint16_t *>(bridge::GameByte(0x8500BA)); }

// wrap v into [lo, hi] after a left / right step (other messages: unchanged)
int32_t Step(int32_t v, uint32_t msg, int32_t lo, int32_t hi)
{
    if (msg == kLeft) {
        if (--v < lo) v = hi;
    } else if (msg == kRight) {
        if (++v > hi) v = lo;
    }
    return v;
}

void BlinkAndDraw(QRToggleWidget &w, uint32_t msg)
{
    bridge::FEWidget_BlinkArrows(&w, msg);   // +0x40
    bridge::FEWidget_Draw(&w);               // +0x0C
}

// NumOpponents / NumLaps Draw tail: disabled while lap KO is on (virtual Enable +0x1C / Disable +0x20)
void SyncLapKOEnabled(QRToggleWidget &w)
{
    const bool enabled = w.base.base.base.mbEnabled != 0;
    if (QRS().GetCurrentTrackSettings()->mbLapKO) {
        if (enabled) bridge::FEWidget_Disable(&w);
    } else if (!enabled) {
        bridge::FEWidget_Enable(&w);
    }
}

// PC 4B3E40 / 4B3F10: minimum online laps by the online race mode
uint16_t OnlineMinLaps()
{
    const int32_t m = *bridge::GameInt(0x850088);
    if (m == 0 || m == 4) return 2;
    if (m == 3 || m == 5 || m == 10) return 4;
    return 1;
}

QRTrackSettings *DirectionSettings()
{
    return Online() ? reinterpret_cast<QRTrackSettings *>(bridge::GameByte(0x8500B8)) : QRS().GetCurrentTrackSettings();
}
}  // namespace

uint32_t QRToggleVTable(QRToggle k) { return 0x791380u + 0x48u * static_cast<uint32_t>(k); }

QRToggleWidget *QRToggleWidget::Construct(QRToggle k, const float *pos, const float *size, const float *valueSize,
                                          const float *valuePos, bool enabled)
{
    base.Construct(pos, size, valueSize, valuePos, enabled);
    base.base.base.vtable = bridge::GameVTable(QRToggleVTable(k));
    if (k == QRToggle::NumOpponents) {
        // PC 4B3750: game mode 2 drag on track 0x1132 allows 2 opponents at most
        QuickRaceSettings &s = QRS();
        if (bridge::GameModeFlags() == 2 && s.mRaceMode == 2 && s.GetCurrentTrackSettings()->mTrack == 0x1132 &&
            s.GetNumOpponents() > 2)
            s.SetNumOpponents(2);
    } else if (k == QRToggle::OnlineNumLaps) {
        mMinLaps = OnlineMinLaps();
        if (OnlineLaps() < mMinLaps) OnlineLaps() = mMinLaps = OnlineMinLaps();
    }
    return this;
}

void QRToggleWidget::Act(QRToggle k, const char *pkg, uint32_t msg)
{
    QuickRaceSettings &s = QRS();
    switch (k) {
    case QRToggle::NumOpponents: {
        if (msg != kRight && msg != kLeft) return;
        const int32_t players = s.GetNumPlayers();
        int32_t n = s.GetNumOpponents();
        int32_t maxCars = s.mRaceMode == 4 ? 5 : 3;
        if (bridge::GameModeFlags() == 2 && s.mRaceMode == 2 && s.GetCurrentTrackSettings()->mTrack == 0x1132) maxCars = 2;
        const int32_t lo = bridge::GameModeFlags() == 2 ? 0 : 1;
        if (msg == kLeft) {
            if (--n < lo) n = maxCars - players + 1;
        } else if (++n > maxCars - players + 1) {
            n = lo;
        }
        s.SetNumOpponents(n);
        BlinkAndDraw(*this, msg);
        return;
    }
    case QRToggle::AISkill: s.SetAISkill(Step(s.GetAISkill(), msg, 0, 2)); break;
    case QRToggle::CatchUp:
        if (msg == kLeft || msg == kRight) s.SetCatchUp(!s.IsCatchUpOn());
        break;
    case QRToggle::TrafficLevel: s.SetTrafficLevel(Step(s.GetTrafficLevel(), msg, 0, 3)); break;
    case QRToggle::PointsToWin: s.SetNumPointsToWinOutrun(Step(s.GetNumPointsToWinOutrun(), msg, 0, 6)); break;
    case QRToggle::TimeLimit: s.SetOutrunTimeLimit(Step(s.GetOutrunTimeLimit(), msg, 0, 4)); break;
    case QRToggle::NumLaps: {
        const int32_t laps = Step(s.GetCurrentTrackSettings()->mLaps, msg, 1, 10);
        s.GetCurrentTrackSettings()->mLaps = static_cast<uint16_t>(laps);
        break;
    }
    case QRToggle::OnlineNumLaps: {
        int32_t laps = OnlineLaps();
        if (msg == kLeft) {
            if (--laps < mMinLaps) laps = 10;
        } else if (msg == kRight) {
            if (++laps > 10) laps = mMinLaps;
        }
        OnlineLaps() = static_cast<uint16_t>(laps);
        break;
    }
    case QRToggle::LapKO:
        if (msg == kLeft || msg == kRight) {
            const bool on = s.GetCurrentTrackSettings()->mbLapKO == 0;
            s.GetCurrentTrackSettings()->mbLapKO = on;
        }
        UIQRModeOptions::SetupForLapKO();
        bridge::FEngSendMessageToPackage(0x92B703B5u, pkg);
        break;
    case QRToggle::TrackDirection: {
        if (msg != kLeft && msg != kRight) return;
        QRTrackSettings *t = DirectionSettings();
        const int32_t d = t->mDirection == 0 ? 1 : 0;
        t->mDirection = d;
        *bridge::GameInt(0x89E7A4) = d;
        break;
    }
    }
    BlinkAndDraw(*this, msg);
}

void QRToggleWidget::Draw(QRToggle k)
{
    FEObject *name = base.base.mpObject, *value = base.base.mpValue;
    QuickRaceSettings &s = QRS();
    switch (k) {
    case QRToggle::NumOpponents:
        FEngSetLanguageHash(name, 0x3384A679u);
        bridge::FEPrintfInt(value, "%d", s.GetNumOpponents());
        SyncLapKOEnabled(*this);
        return;
    case QRToggle::AISkill: {
        const uint8_t v = static_cast<uint8_t>(s.GetAISkill());
        const uint32_t h = v == 0 ? 0xDBBB4E8Fu : v == 1 ? 0xCAB262DEu : v == 2 ? 0xDBBCF37Cu : 0u;
        FEngSetLanguageHash(name, 0x4D156786u);
        FEngSetLanguageHash(value, h);
        return;
    }
    case QRToggle::CatchUp:
        FEngSetLanguageHash(value, s.IsCatchUpOn() ? 0x63Cu : 0xCCFAu);
        FEngSetLanguageHash(name, 0x8B8E913Au);
        return;
    case QRToggle::TrafficLevel: {
        static const uint32_t kHashes[4] = {0x896BCB4Du, 0xB6BD9C95u, 0x907C555Eu, 0xB672F15Au};
        const uint32_t v = static_cast<uint32_t>(s.GetTrafficLevel());
        FEngSetLanguageHash(value, v <= 3 ? kHashes[v] : 0u);
        FEngSetLanguageHash(name, 0xEB9DFC09u);
        return;
    }
    case QRToggle::PointsToWin:
        FEngSetLanguageHash(name, 0x81CAA8E6u);
        if (s.GetNumPointsToWinOutrun() == 6) {
            bridge::FEPrintfFormat(value, bridge::LocalizedString(0x2F8344EAu));   // the text is the format
            return;
        }
        bridge::FEPrintfInt(value, "%$d", s.GetNumPointsToWinOutrunValue());
        return;
    case QRToggle::TimeLimit:
        FEngSetLanguageHash(name, 0xFE407A18u);
        if (s.GetOutrunTimeLimit() == 0) {
            bridge::FEPrintfFormat(value, bridge::LocalizedString(0x53B157FCu));
            return;
        }
        bridge::FEPrintfIntString(value, "%d %s", s.GetOutrunTimeLimitValue(), bridge::LocalizedString(0x542110C3u));
        return;
    case QRToggle::NumLaps:
        bridge::FEPrintfInt(value, "%d", s.GetCurrentTrackSettings()->mLaps);
        FEngSetLanguageHash(name, 0x48494E83u);
        SyncLapKOEnabled(*this);
        return;
    case QRToggle::OnlineNumLaps:
        bridge::FEPrintfInt(value, "%d", OnlineLaps());
        FEngSetLanguageHash(name, 0x48494E83u);
        return;
    case QRToggle::LapKO:
        FEngSetLanguageHash(value, s.GetCurrentTrackSettings()->mbLapKO ? 0x63Cu : 0xCCFAu);
        FEngSetLanguageHash(name, 0x7FD73849u);
        return;
    case QRToggle::TrackDirection:
        FEngSetLanguageHash(name, 0xA88FFEB4u);
        FEngSetLanguageHash(value, DirectionSettings()->mDirection == 0 ? 0x9E4C4648u : 0x22F254AFu);
        return;
    }
}

// ---- UIQRModeOptions ----

namespace {
void Add(UIQRModeOptions &m, QRToggle k, bool enabled = true)
{
    auto *w = static_cast<QRToggleWidget *>(bridge::Alloc(sizeof(FEToggleWidget)));   // operator new 0x54
    UIWidgetMenu &b = m.base;
    if (w) w->Construct(k, &b.mStartX, &b.mTitleW, &b.mDataW, &b.mDataX, enabled);
    b.AddToggleOption(&w->base, true);
}

// TrackInfo of the current track; the direction option when it can be reversed (and, for most modes, unlocked)
const TrackInfo *CurrentTrackInfo() { return bridge::TrackInfo_GetTrackInfo(QRS().GetCurrentTrackSettings()->mTrack); }
void AddDirectionIfUnlocked(UIQRModeOptions &m, const TrackInfo *ti)
{
    if (ti->mbReversible == 1 && bridge::UnlockSystem_IsTrackUnlocked(1, ti->mTrackNumber, 1, 0))
        Add(m, QRToggle::TrackDirection);
}
bool Splitscreen() { return bridge::GameModeFlags() == 4; }
}  // namespace

UIQRModeOptions *UIQRModeOptions::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    MenuScreen &ms = base.base;
    ms.vtable = bridge::GameVTable(0x798014);
    mPasswordWidget = mConfirmWidget = _unknown154 = 0;
    mbAccepted = 0;
    mpDialog = nullptr;
    mOnlineLapsWidget = 0;
    Setup();
    const int32_t mode = bridge::GameModeFlags();
    if (mode == 8 || mode == 0x200) {
        ms.mHelpHash = 0xF4EA444Bu;
        *bridge::GameByte(0x8500C8) = 0;
        FEngSetLanguageHash(FEngFindString(ms.mpPackageName, 0x42ADB44Cu),
                            bridge::GameModeFlags() == 0x200 ? 0x245448DBu : 0x5D2B0B43u);
        if (bridge::GameModeFlags() == 8) FEngSetScript(ms.mpPackageName, "EA_MESSENGER_GROUP", "Show", true);
        FEngSetScript(ms.mpPackageName, 0x2BAC0CEEu, "Show", true);
        FEngSetVisible(FEngFindObject(ms.mpPackageName, 0x4FD28546u));
        FEngSetInvisible(FEngFindObject(ms.mpPackageName, 0xEB406FECu));
        bridge::HelpBar_SyncByMask(ms.mData.mHelpBarMask | 0x4000u, ms.mpPackageName);
        return this;
    }
    if (mode == 4) {
        if (QRS().mRaceMode == 3) ms.mHelpHash = 0x0D3F5F29u;
        FEngSetLanguageHash(FEngFindString(ms.mpPackageName, 0x42ADB44Cu), 0x7D7A6E0Eu);
    } else {
        FEngSetLanguageHash(FEngFindString(ms.mpPackageName, 0x42ADB44Cu), 0x47597606u);
    }
    FEngSetScript(ms.mpPackageName, 0x2BAC0CEEu, "Hide", true);
    FEngSetInvisible(FEngFindObject(ms.mpPackageName, 0x4FD28546u));
    return this;
}

void UIQRModeOptions::Destruct()
{
    base.base.vtable = bridge::GameVTable(0x798014);
    base.DestructBody();
}

UIQRModeOptions *UIQRModeOptions::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void UIQRModeOptions::Setup()
{
    const int32_t mode = Online() ? *bridge::GameInt(0x850088) : QRS().mRaceMode;
    switch (mode) {
    case 0: SetupCircuit(); break;
    case 1: SetupSprint(); break;
    case 2: SetupDrag(); break;
    case 3: SetupDrift(); break;
    case 4:
    case 5:
    case 10: SetupShortTrack(); break;
    case 7: if (!Online()) SetupGT(); break;
    case 8: SetupOutrun(); break;
    case 9: if (!Online()) SetupFreeRun(); break;
    default: break;
    }
    base.SetInitialOption(0);
}

void UIQRModeOptions::SetupCircuit()
{
    if (Online()) return bridge::QRModeOptions_BoilerPlateOnline(this);
    QuickRaceSettings &s = QRS();
    if (Splitscreen() && s.GetCurrentTrackSettings()->mbLapKO) s.GetCurrentTrackSettings()->mbLapKO = 0;
    if (CurrentTrackInfo()->mbReversible == 1) Add(*this, QRToggle::TrackDirection);   // no unlock test here
    if (!Splitscreen()) {
        const bool lapsEnabled = s.GetCurrentTrackSettings()->mbLapKO == 0;
        Add(*this, QRToggle::LapKO);
        Add(*this, QRToggle::NumLaps, lapsEnabled);
    } else {
        Add(*this, QRToggle::NumLaps);
    }
    Add(*this, QRToggle::TrafficLevel);
    if (!Splitscreen()) {
        Add(*this, QRToggle::NumOpponents);
        Add(*this, QRToggle::AISkill);
    }
    Add(*this, QRToggle::CatchUp);
}

void UIQRModeOptions::SetupSprint()
{
    if (Online()) return bridge::QRModeOptions_BoilerPlateOnline(this);
    AddDirectionIfUnlocked(*this, CurrentTrackInfo());
    Add(*this, QRToggle::TrafficLevel);
    if (!Splitscreen()) {
        Add(*this, QRToggle::NumOpponents);
        Add(*this, QRToggle::AISkill);
    }
    Add(*this, QRToggle::CatchUp);
}

void UIQRModeOptions::SetupDrag()
{
    if (Online()) return bridge::QRModeOptions_BoilerPlateOnline(this);
    const TrackInfo *ti = CurrentTrackInfo();
    AddDirectionIfUnlocked(*this, ti);
    if (ti->mTrackNumber < 0x1073 || ti->mTrackNumber > 0x107F) Add(*this, QRToggle::TrafficLevel);
    if (!Splitscreen()) {
        Add(*this, QRToggle::NumOpponents);
        Add(*this, QRToggle::AISkill);
    }
    Add(*this, QRToggle::CatchUp);
}

void UIQRModeOptions::SetupDrift()
{
    if (Online()) return bridge::QRModeOptions_BoilerPlateOnline(this);
    const TrackInfo *ti = CurrentTrackInfo();
    AddDirectionIfUnlocked(*this, ti);
    if (ti->mDriftKind == 0) Add(*this, QRToggle::NumLaps);
    if (!Splitscreen()) {
        if (ti->mDriftKind == 0) Add(*this, QRToggle::NumOpponents);
        Add(*this, QRToggle::AISkill);
    }
    if (ti->mDriftKind == 1) {
        Add(*this, QRToggle::TrafficLevel);
        *bridge::GameInt(0x89E7C4) = 0;
    }
}

void UIQRModeOptions::SetupOutrun()
{
    if (Online()) return bridge::QRModeOptions_BoilerPlateOnline(this);
    Add(*this, QRToggle::PointsToWin);
    Add(*this, QRToggle::TimeLimit);
}

void UIQRModeOptions::SetupGT()
{
    if (Online()) return bridge::QRModeOptions_BoilerPlateOnline(this);
    AddDirectionIfUnlocked(*this, CurrentTrackInfo());
    if (!Splitscreen()) {
        Add(*this, QRToggle::NumOpponents);
        Add(*this, QRToggle::AISkill);
    }
    Add(*this, QRToggle::CatchUp);
}

void UIQRModeOptions::SetupShortTrack()
{
    if (Online()) return bridge::QRModeOptions_BoilerPlateOnline(this);
    AddDirectionIfUnlocked(*this, CurrentTrackInfo());
    Add(*this, QRToggle::NumLaps);
    if (!Splitscreen()) {
        Add(*this, QRToggle::NumOpponents);
        Add(*this, QRToggle::AISkill);
    }
    Add(*this, QRToggle::CatchUp);
}

void UIQRModeOptions::SetupFreeRun()
{
    if (Online()) return bridge::QRModeOptions_BoilerPlateOnline(this);
    AddDirectionIfUnlocked(*this, CurrentTrackInfo());
    Add(*this, QRToggle::TrafficLevel);
}

void UIQRModeOptions::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    const char *pkg = base.base.mpPackageName;
    switch (msg) {
    case 0x0C407210u: {   // accept
        if (!Online()) {
            mbAccepted = 1;
            FEngSetScript(pkg, kOutObject, "FORWARD", true);
            return;
        }
        // online password checks (network code: helpers stubbed)
        if (base.mpCurrent == base.GetWidget(mConfirmWidget) || base.mpCurrent == base.GetWidget(mPasswordWidget)) return;
        char *password = reinterpret_cast<char *>(bridge::GameByte(0x85009C));
        const int len = bStrLen(password);
        if ((len < 3 || len > 12) && *bridge::GameByte(0x85009B) != 1) {
            bridge::DialogShowOk(pkg, "GenericDialog.fng", 0xD29A1A18u, 3, 12);
            return;
        }
        if (bStrCmp(reinterpret_cast<char *>(bridge::GameByte(0x8500A9)), password) != 0 && *bridge::GameByte(0x85009B) != 1) {
            mpDialog = bridge::DialogShowOk(pkg, "GenericDialog.fng", 0xFF99D845u);
            return;
        }
        bridge::QRModeOptions_OnlineAccept(this);   // 498440
        return;
    }
    case 0x2C0E485Au: bridge::QRModeOptions_Online4A0140(pkg); return;
    case 0x0153E234u: base.RefreshWidgets(); return;
    case 0x34DC1BECu:
    case 0xB4EDEB6Du:
        if (mpDialog) {
            bridge::DialogDismiss(mpDialog);
            mpDialog = nullptr;
        }
        [[fallthrough]];
    case 0xFAF20A02u: FEngSetScript(pkg, kOutObject, "UNDIM", true); return;
    case 0x59337816u:
        bridge::QRModeOptions_Online4E2DC0(this);
        mbAccepted = 1;
        FEngSetScript(pkg, kOutObject, "FORWARD", true);
        return;
    case 0x911AB364u:   // back
        base.mPrevMsg = 0x911AB364u;
        if (!FEngIsScriptRunning(pkg, kOutObject, kOutScript)) FEngSetScript(pkg, kOutObject, kOutScript, true);
        return;
    case 0xC98356BAu:   // tick
        if (Online()) {
            bridge::QRModeOptions_HandlePasswordEntryRestrictions(this);
            base.RefreshWidgets();
        }
        if (bridge::GameModeFlags() == 8 && mOnlineLapsWidget && *bridge::GameByte(0x850098) == 1 &&
            OnlineLaps() < bridge::OnlineNumLaps_MinLaps(base.GetWidget(mOnlineLapsWidget))) {
            OnlineLaps() = bridge::OnlineNumLaps_MinLaps(base.GetWidget(mOnlineLapsWidget));
            base.RefreshWidgets();
        }
        return;
    case 0xDA5B8712u: {   // text entered (online password / confirm)
        FEWidget *cur = base.mpCurrent;
        FEWidget *confirm = base.GetWidget(mConfirmWidget), *password = base.GetWidget(mPasswordWidget);
        const char *text = reinterpret_cast<const char *>(cur) + 0x4C;
        if (cur == confirm) bStrNCpy(reinterpret_cast<char *>(bridge::GameByte(0x85009C)), text, 12);
        else if (cur == password) bridge::QRModeOptions_Online495220(text);
        return;
    }
    case 0xE1FDE1D1u:   // out script finished
        if (base.mPrevMsg == 0x0C407210u) {
            if (mbAccepted) bridge::QRManager_ChangeToNextScreen();
        } else if (base.mPrevMsg == 0x911AB364u) {
            bridge::QRManager_RestorePreviousStable();
        }
        return;
    default: return;
    }
}

void UIQRModeOptions::SetupForLapKO()
{
    QuickRaceSettings &s = QRS();
    if (s.GetCurrentTrackSettings()->mbLapKO) {
        s.GetCurrentTrackSettings()->mLaps = 3;
        s.SetNumOpponents(3);
    }
}

void UIQRModeOptions::SetupStreetCross()
{
    if (Online()) return bridge::QRModeOptions_BoilerPlateOnline(this);
    AddDirectionIfUnlocked(*this, CurrentTrackInfo());
    Add(*this, QRToggle::NumLaps);
    if (!Splitscreen()) {
        Add(*this, QRToggle::NumOpponents);   // (the drag-track clamp is in its ctor)
        Add(*this, QRToggle::AISkill);
    }
    Add(*this, QRToggle::CatchUp);
}
