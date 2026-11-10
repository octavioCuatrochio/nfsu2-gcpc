#include "StartCareer.h"
#include "bridge/Bridge.h"
#include "FELocalize.h"
#include "FEngUtil.h"
#include "cFEng.h"

namespace {
IconOption *NewIcon(SCKind k, uint32_t name, uint32_t header)
{
    auto *o = static_cast<IconOption *>(bridge::Alloc(sizeof(IconOption)));
    if (o) {
        o->Construct(name, header, 0);
        o->vtable = bridge::GameVTable(0x790CB8u + 8u * static_cast<uint32_t>(k));
    }
    return o;
}
}  // namespace

void SCReact(SCKind k, const char *pkg, uint32_t msg, uint32_t a3)
{
    if (msg != 0x0C407210u) return;
    switch (k) {
    case SCKind::NewCareerMemcard: bridge::EnterMemcard(pkg, pkg, 0x21961, 0, 0, 0, 0); return;
    case SCKind::Start: bridge::StartCareerHelper(static_cast<int32_t>(a3)); return;
    case SCKind::Load: bridge::EnterMemcard(pkg, pkg, 0xC11, 0, 0, 0x461A18EEu, 0xF35D144Eu); return;
    }
}

UIStartCareer *UIStartCareer::Create(const ScreenConstructorData *data)
{
    auto *s = static_cast<UIStartCareer *>(bridge::Alloc(sizeof(UIStartCareer)));
    if (s) s->Construct(data);
    s->Setup();   // PC: virtual slot 5, no null check
    return s;
}

UIStartCareer *UIStartCareer::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::UIStartCareerVTable();
    return this;
}

void UIStartCareer::Destruct()
{
    base.base.vtable = bridge::UIStartCareerVTable();
    base.Destruct();
}

UIStartCareer *UIStartCareer::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void UIStartCareer::Setup()
{
    const char *pkg = base.base.mpPackageName;
    bridge::FEngPrintfString(pkg, 0x42ADB44Cu, GetLocalizedString(0x5E7E1192u));
    bool startAdded = false;
    mpStartIcon = nullptr;
    if (bridge::AutoSaveSecondStateAllowed()) {   // PC 502FE0 IsProfileNamed
        IconOption *o;
        if (*bridge::GameByte(0x86347C)) {
            o = NewIcon(SCKind::Start, 0xA230CD88u, 0xE072DB21u);   // resume
        } else {
            startAdded = true;
            o = NewIcon(SCKind::Start, 0x3CB92E26u, 0x97B1DCDAu);   // new career straight away
        }
        mpStartIcon = o;
        bridge::IconScrollerMenu_AddOption(&base, o);
    }
    if (!startAdded) bridge::IconScrollerMenu_AddOption(&base, NewIcon(SCKind::NewCareerMemcard, 0x3CB92E26u, 0x97B1DCDAu));
    bridge::IconScrollerMenu_AddOption(&base, NewIcon(SCKind::Load, 0x72C87F3Cu, 0xAA3F09D8u));
    bridge::IconScroller_SetInitialPos(&base.mScroller, FEngGetLastButton(base.base.mpPackageName));
    bridge::IconScrollerMenu_RefreshHeader(&base);
}

void UIStartCareer::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    switch (msg) {
    case 0x911AB364u: FEngSwitchPackages(base.base.mpPackageName, bridge::CareerReturnScreen(), 0); return;
    case 0x461A18EEu:   // profile loaded
        if (bridge::AutoSaveSecondStateAllowed()) bridge::StartCareerHelper(bridge::CareerSlot());
        return;
    case 0x251BF83Eu:
        if (!bridge::AutoSaveSecondStateAllowed() && mpStartIcon) mpStartIcon->mbLocked = 1;
        return;
    default: return;
    }
}
