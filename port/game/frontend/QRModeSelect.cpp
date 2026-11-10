#include "QRModeSelect.h"
#include "bridge/Bridge.h"
#include "FEngUtil.h"
#include "cFEng.h"

namespace {
const char *Pkg(const UIQRModeSelect &m) { return m.base.base.mpPackageName; }
constexpr uint32_t kOutObject = 0x47FF4E7Cu, kOutScript = 0xDE6EFF34u, kTitle = 0x42ADB44Cu;

void AddMS(UIQRModeSelect &m, MSKind k, uint32_t name, uint32_t header)
{
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    if (o) {
        o->Construct(name, header, 0);
        o->vtable = bridge::GameVTable(MSVTable(k));
    }
    bridge::IconScrollerMenu_AddOption(&m.base, o);
}
}  // namespace

uint32_t MSVTable(MSKind k) { return 0x790FD4u + 8u * static_cast<uint32_t>(k); }

void QR_SetGameMode(int32_t mode)
{
    const int32_t flags = bridge::GameModeFlags();
    if (flags == 8 || flags == 0x200) *bridge::GameInt(0x850088) = mode;
    else *bridge::GameInt(0x83AAB4) = mode;
    if (flags == 4) {
        bridge::QuickRaceSettings_SetNumPlayers(2);
        bridge::QR_SetNumOpponents(2);
        return;
    }
    bridge::QuickRaceSettings_SetNumPlayers(1);
    bridge::QR_SetNumOpponents(*bridge::GameInt(0x83AAB4) == 4 ? 5 : 3);
}

void MSReact(MSKind k, uint32_t msg)
{
    if (msg != 0x0C407210u) return;
    switch (k) {
    case MSKind::Circuit: QR_SetGameMode(0); return;
    case MSKind::Sprint: QR_SetGameMode(1); return;
    case MSKind::Drag: QR_SetGameMode(2); return;
    case MSKind::Drift: QR_SetGameMode(3); return;
    case MSKind::Lan: bridge::QR_5028D0(); QR_SetGameMode(8); return;
    case MSKind::FreeRun: {
        QR_SetGameMode(9);
        uint8_t *t = bridge::QR_CurrentTrackSettings();
        *reinterpret_cast<uint16_t *>(t) = 4000;
        *reinterpret_cast<int32_t *>(t + 4) = 0;
        return;
    }
    case MSKind::ShortTrack: QR_SetGameMode(0xA); return;
    case MSKind::URL: QR_SetGameMode(4); return;
    }
}

UIQRModeSelect *UIQRModeSelect::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    MenuScreen &ms = base.base;
    ms.vtable = bridge::UIQRModeSelectVTable();
    Setup();
    base.RefreshHeader();
    const int32_t mode = bridge::GameModeFlags();
    const char *pkg = ms.mpPackageName;
    if (mode == 8 || mode == 0x200) {
        ms.mHelpHash = 0x36BEC45Fu;
        FEngSetLanguageHash(FEngFindString(pkg, kTitle), bridge::GameModeFlags() == 0x200 ? 0x47B276EFu : 0xB6068C57u);
        if (bridge::GameModeFlags() == 8) FEngSetScript(ms.mpPackageName, "EA_MESSENGER_GROUP", "Show", true);
        FEngSetScript(ms.mpPackageName, 0x2BAC0CEEu, "Show", true);
        FEngSetVisible(FEngFindObject(ms.mpPackageName, 0x4FD28546u));
        FEngSetInvisible(FEngFindObject(ms.mpPackageName, 0xEB406FECu));
        bridge::HelpBar_SyncByMask(ms.mData.mHelpBarMask | 0x4000u, ms.mpPackageName);
    } else if (mode == 4) {
        FEngSetScript(pkg, 0x2BAC0CEEu, "Hide", true);
        FEngSetInvisible(FEngFindObject(ms.mpPackageName, 0x4FD28546u));
        FEngSetLanguageHash(FEngFindString(ms.mpPackageName, kTitle), 0xEF9F780Du);
    } else {
        FEngSetLanguageHash(FEngFindString(pkg, kTitle), 0x05416105u);
        FEngSetScript(ms.mpPackageName, 0x2BAC0CEEu, "Hide", true);
        FEngSetInvisible(FEngFindObject(ms.mpPackageName, 0x4FD28546u));
    }
    return this;
}

void UIQRModeSelect::Destruct()
{
    base.base.vtable = bridge::UIQRModeSelectVTable();
    base.Destruct();
}

UIQRModeSelect *UIQRModeSelect::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void UIQRModeSelect::Setup()
{
    bridge::QR_CurrentTrackSettings()[8] = 0;
    AddMS(*this, MSKind::Circuit, 0xE9638D3Eu, 0x93FDEE74u);
    AddMS(*this, MSKind::Sprint, 0x2521E5EBu, 0xDCB9F861u);
    AddMS(*this, MSKind::Drag, 0xAAAB31E9u, 0x35102EDFu);
    AddMS(*this, MSKind::Drift, 0x00119144u, 0xD7162CFAu);
    AddMS(*this, MSKind::ShortTrack, 0x91703AEFu, 0x30DA61E1u);
    AddMS(*this, MSKind::URL, 0x3F6495C5u, 0xC786C4E7u);
    int32_t mode = bridge::GameModeFlags();
    if (mode == 8 || mode == 0x200 || mode == 4) {
        AddMS(*this, MSKind::Lan, 0x086C74CBu, 0x613D3106u);
        mode = bridge::GameModeFlags();
    }
    if (mode != 8 && mode != 0x200 && mode != 4) AddMS(*this, MSKind::FreeRun, 0x713D99C1u, 0x6F2421B7u);
    bridge::IconScroller_SetInitialPos(&base.mScroller, FEngGetLastButton(Pkg(*this)));
}

void UIQRModeSelect::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    const char *pkg = Pkg(*this);
    if (msg == 0x0C407210u || msg == 0x911AB364u) {
        if (!FEngIsScriptRunning(pkg, kOutObject, kOutScript)) FEngSetScript(pkg, kOutObject, kOutScript, true);
        return;
    }
    if (msg != 0xE1FDE1D1u) return;
    if (base.mPrevMsg == 0x0C407210u) bridge::QRManager_ChangeToNextScreen();
    else if (base.mPrevMsg == 0x911AB364u) bridge::QRManager_RestorePreviousStable();
}
