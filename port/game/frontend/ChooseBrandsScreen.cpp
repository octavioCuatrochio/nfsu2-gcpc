#include "ChooseBrandsScreen.h"
#include "bridge/Bridge.h"
#include "FEngUtil.h"
#include "FEObject.h"
#include "FEPackage.h"
#include "FEPackageManager.h"
#include "MenuScreen.h"

BrandScrollerThing *BrandScrollerThing::Construct(const uint8_t *part, const float *pos, const float *size,
                                                  const float *valueSize, const float *valuePos, bool enabled)
{
    FEStatWidget &s = base.base.base;
    s.Construct(pos, size, valueSize, valuePos, enabled);
    base.base.mpLeftArrow = nullptr;
    base.base.mpRightArrow = nullptr;
    base.mpImage = nullptr;
    mIndex = 0;
    s.base.vtable = bridge::BrandScrollerThingVTable();
    mpPart = part;
    s.base.mLabelOffsetX = -232.0f;   // 0xC3680000
    s.base.mLabelOffsetY = -64.0f;    // 0xC2800000
    return this;
}

void BrandScrollerThing::Act(const char *, uint32_t msg)
{
    const int32_t count = *reinterpret_cast<const int32_t *>(mpPart + 0x0C);
    if (msg == 0x9120409Eu) {
        if (++mIndex > count - 1) mIndex = 0;
    } else if (msg == 0xB5971BF1u) {
        if (--mIndex < 0) mIndex = count - 1;
    }
    bridge::FEWidget_BlinkArrows(this, msg);   // +0x40
    bridge::FEWidget_Draw(this);               // +0x0C
}

void BrandScrollerThing::Draw()
{
    FEngSetLanguageHash(base.base.base.mpObject, *reinterpret_cast<const uint32_t *>(mpPart + 4));
    bridge::FEPrintfFormat(base.base.base.mpValue, bridge::BrandDisplayName(Brand()));
    const uint32_t tex = bridge::FEngHashStringArg("DECAL_ICON_%s", bridge::BrandStringFromHash(Brand()));
    FEngSetTextureHash(base.mpImage, bridge::GetTextureInfo(tex, 0, 0) ? tex : 0x74CE8C0Bu);
}

ChooseBrandsScreen *ChooseBrandsScreen::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::ChooseBrandsScreenVTable();
    data->mpPackage->mbFlagsAC[0] = 0;   // package +0xAC: no package-level click messages
    bridge::SetShowEAMIcons(false);
    Setup();
    return this;
}

void ChooseBrandsScreen::SetChosenBrands()
{
    uint32_t *out = bridge::ChosenBrands();
    FEWidget *end = uiwm::FromNode(&base.mWidgets);
    for (FEWidget *w = uiwm::FromNode(base.mWidgets.next); w != end; w = uiwm::FromNode(uiwm::NextOf(w)))
        *out++ = reinterpret_cast<BrandScrollerThing *>(w)->Brand();
}

void ChooseBrandsScreen::Setup()
{
    UIWidgetMenu &m = base;
    const char *pkg = m.base.mpPackageName;
    auto add = [&](const uint8_t *part, bool enabled) {
        auto *w = static_cast<BrandScrollerThing *>(bridge::Alloc(sizeof(BrandScrollerThing)));
        if (w) w->Construct(part, &m.mStartX, &m.mTitleW, &m.mDataW, &m.mDataX, enabled);
        m.AddToggleImageOption(w ? &w->base : nullptr, 1);
    };
    if (bridge::BrowsingIndividualParts()) {
        FEngSetInvisible(pkg, 0x6D017BC5u);
        add(bridge::SelectedPerfPart(), true);
    } else {
        const uint8_t *package = bridge::SelectedPerfPackage();
        int32_t rows = 0;
        for (uint32_t i = 0; i < *reinterpret_cast<const uint32_t *>(package + 8) && i < 4; ++i) {
            const uint8_t *part = package + 0x0C + i * 0x5C;
            const uint32_t brands = *reinterpret_cast<const uint32_t *>(part + 0x0C);
            if (brands == 0) continue;
            add(part, brands > 1);
            ++rows;
        }
        if (rows < 2) FEngSetInvisible(pkg, 0x6D017BC5u);
    }
    m.SetInitialOption(0);
}

void ChooseBrandsScreen::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkg)
{
    base.NotificationMessage(msg, obj, arg, pkg);
    const char *name = base.base.mpPackageName;
    switch (msg) {
    case 0x35F8620Bu: {
        FEWidget *cur = base.mpCurrent;
        if (!cur->mbEnabled) bridge::FEWidget_Disable(cur);   // slot 8
        return;
    }
    case 0x0C407210u:
        if (!FEngIsScriptRunning(name, 0x47FF4E7Cu, 0xDE6EFF34u)) {
            base.StorePrevNotification(0x0C407210u, obj, arg, pkg);
            FEngSetScript(name, 0x47FF4E7Cu, 0xDE6EFF34u, true);
        }
        return;
    case 0xE1FDE1D1u:
        if (base.mPrevMsg != 0x0C407210u) return;
        SetChosenBrands();
        if (const uint8_t *p = bridge::SelectedPerfPackage()) {
            bridge::CCM_BuyPerfPackage(p);
            bridge::CCM_InstallPerfPackage(p);
        } else if (const uint8_t *part = bridge::SelectedPerfPart()) {
            bridge::CCM_BuyPerfPart(part);
            bridge::CCM_InstallPerfPart(part);
        }
        bridge::CustomizationScreenManager_ChangeToNextScreen();
        return;
    case 0x72619778u:
    case 0x911C0A4Bu: {
        const auto *cur = reinterpret_cast<const BrandScrollerThing *>(base.mpCurrent);
        if (*reinterpret_cast<const int32_t *>(cur->mpPart + 0x0C) > 1) FEngSetVisible(name, 0xB34A6C2Bu);
        else FEngSetInvisible(name, 0xB34A6C2Bu);
        return;
    }
    default:
        return;
    }
}
