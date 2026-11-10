#include "MagazineScreens.h"
#include <initializer_list>
#include "bridge/Bridge.h"
#include "bWare/bMath.h"
#include "cFEng.h"
#include "FEngUtil.h"
#include "FEObject.h"
#include "FEPackage.h"
#include "MagazineSelect.h"   // MagazineViewer::GetCurrentImageName

namespace {
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";
const char *PackName(int32_t pack) { return bridge::GameStringPointer(0x7FA294 + 4u * static_cast<uint32_t>(pack)); }
void WaitForResources()
{
    while (!bridge::IsResourceLoadingComplete()) bridge::ServiceResourceLoading();
}
// PC: count <= 1 (or == 0 for the `zeroTest` fields) -> 0, else bRandom(count)
uint32_t Pick(uint32_t count, bool zeroTest)
{
    if (zeroTest ? count == 0 : count <= 1) return 0;
    return bRandom(static_cast<int32_t>(count));
}
uint32_t At(const uint8_t *d, uint32_t off) { return *reinterpret_cast<const uint32_t *>(d + off); }
}  // namespace

// ---- MagazineTextureHandler ----

void MagazineTextureHandler::Init(int32_t pack, bool allocMemory)
{
    mPack = pack;
    mbAllocMemory = allocMemory;
    mpMemory = nullptr;
    if (allocMemory) mpMemory = bridge::TrackStreamer_AllocateUserMemory(4, "MagStreamTexureLock", 0);
    if (bridge::eIsStreamingTexturePackLoaded(PackName(mPack))) return;
    bridge::eLoadStreamingTexturePack(PackName(mPack));
    bridge::eWaitForStreamingTexturePackLoading(PackName(mPack));
    WaitForResources();
}

void MagazineTextureHandler::TexturesLoaded()
{
    if (!mTextures[0] && !mTextures[1] && !mTextures[2] && !mTextures[3]) return;
    int32_t ok = 1;
    if (mTextures[0]) ok = bridge::GetTextureInfo(mTextures[0], 0, 0) != nullptr;
    for (int i = 1; i < 4; ++i)
        if (mTextures[i]) ok &= bridge::GetTextureInfo(mTextures[i], 0, 0) != nullptr;
    mbLoaded = ok;
}

void MagazineTextureHandler_LoadedCallback(void *h) { static_cast<MagazineTextureHandler *>(h)->TexturesLoaded(); }

void MagazineTextureHandler::UnloadTextures()
{
    for (uint32_t t : mTextures)
        if (t) bridge::eUnloadStreamingTexture(t);
    mTextures[0] = mTextures[1] = mTextures[2] = mTextures[3] = 0;
    mbLoaded = 0;
}

void MagazineTextureHandler::Close()
{
    UnloadTextures();
    bridge::eWaitForStreamingTexturePackLoading(PackName(mPack));
    if (bridge::eIsStreamingTexturePackLoaded(PackName(mPack))) {
        bridge::eUnloadStreamingTexturePack(PackName(mPack));
        WaitForResources();
    }
    if (mbAllocMemory && mpMemory) {
        bridge::TrackStreamer_FreeUserMemory(mpMemory);
        mpMemory = nullptr;
    }
}

void MagazineTextureHandler::LoadTextures(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    if (!a && !b && !c && !d) {
        mbLoaded = 1;
        return;
    }
    mTextures[3] = d;
    mTextures[0] = a;
    mTextures[1] = b;
    mTextures[2] = c;
    const int32_t flag = mbAllocMemory ? 1 : 0;
    for (uint32_t t : {a, b, c, d})
        if (t) bridge::eLoadStreamingTexture(t, &MagazineTextureHandler_LoadedCallback, this, flag);
}

// ---- SCMagazineSetup / FEMagazineSetup ----

void SCMagazineSetup::GenerateMagazineSelection(uint32_t *out) const
{
    out[0] = At(data, 0x18 + 4 * Pick(At(data, 0x8), false));
    out[1] = At(data, 0x20 + 4 * Pick(At(data, 0xC), false));
    const uint32_t car = Pick(At(data, 0x10), true);
    out[3] = car;
    out[2] = At(data, 0x24 + 4 * car);
    out[4] = At(data, 0x30 + 4 * Pick(At(data, 0x14), true));
}

void SCMagazineSetup::CycleCarSetups(uint32_t *out) const
{
    const int32_t last = static_cast<int32_t>(At(data, 0x10)) - 1;
    int32_t i = static_cast<int32_t>(out[3]);
    if (i < last) {
        out[3] = static_cast<uint32_t>(++i);
        out[2] = At(data, 0x24 + 4 * static_cast<uint32_t>(i));
        return;
    }
    if (i == last) out[3] = 0;
    out[2] = At(data, 0x24 + 4 * out[3]);
}

void FEMagazineSetup::GenerateMagazineSelection(uint32_t *out) const
{
    out[0] = At(data, 0x20 + 4 * Pick(At(data, 0x08), false));
    out[1] = At(data, 0x2C + 4 * Pick(At(data, 0x0C), false));
    out[2] = At(data, 0x34 + 4 * Pick(At(data, 0x10), false));
    out[3] = At(data, 0x48 + 4 * Pick(At(data, 0x14), false));
    out[4] = At(data, 0x54 + 4 * Pick(At(data, 0x18), true));
    out[5] = At(data, 0x60 + 4 * Pick(At(data, 0x1C), true));
}

// ---- MagazineBackScreen ----

MagazineBackScreen *MagazineBackScreen::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.vtable = bridge::GameVTable(0x79CB64);
    FEngSetInvisible(base.mpPackageName, "MagazineBack");
    return this;
}

MagazineBackScreen *MagazineBackScreen::ScalarDeletingDestructor(uint32_t f)
{
    base.vtable = bridge::MenuScreenVTable();   // MenuScreen dtor 510D40
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void MagazineBackScreen::NotificationMessage(uint32_t msg)
{
    if (msg != 0xC98356BAu) return;
    const char *pkg = base.mpPackageName;
    const auto *h = reinterpret_cast<const MagazineTextureHandler *>(bridge::GameByte(0x863780));
    const uint32_t tex = h->mTextures[0];   // *0x863790
    if (tex && h->mbLoaded) {               // *0x86378C
        if (FEObject *img = FEngFindImage(pkg, "MagazineBack")) FEngSetTextureHash(img, tex);
        FEngSetVisible(pkg, "MagazineBack");
        return;
    }
    FEngSetInvisible(pkg, "MagazineBack");
}

// ---- MagazineViewScreen ----

MagazineViewScreen *MagazineViewScreen::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.vtable = bridge::GameVTable(0x79ACE8);
    data->mpPackage->mbFlagsAC[0] = 0;   // PC: ScreenConstructorData +4 -> FEPackage +0xAC
    bridge::MagazineTextureHandler_Reset();
    mScreenShot = 1;
    mbLoaded = 0;
    mbReady = 0;
    bridge::HelpBar_SetFlag6C(0);
    bridge::HelpBar_542990(false);
    FEngSetInvisible(bridge::HelpBarPackageName(), 0x2BAC0CEEu);
    return this;
}

void MagazineViewScreen::Destruct()
{
    base.vtable = bridge::GameVTable(0x79ACE8);
    if (mbLoaded) {
        bridge::MagazineTextureHandler_Unload(mScreenShot);
        mbLoaded = 0;
        mbReady = 0;
    }
    base.vtable = bridge::MenuScreenVTable();
}

MagazineViewScreen *MagazineViewScreen::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void MagazineViewScreen::NotificationMessage(uint32_t msg)
{
    const char *pkg = base.mpPackageName;
    switch (msg) {
    case 0x42317A19u:   // memory card read done
        bridge::ScreenShot_488B10();
        bridge::ScreenShot_Get(mScreenShot);
        mbReady = 1;
        return;
    case 0x35F8620Bu:   // screen ready: allocate the 320x432 screen shot and read it from the card
        if (mbLoaded) return;
        if (!bridge::ScreenShot_Allocate(0x140, 0x1B0, mScreenShot, "SCREENSHOTHASH")) return;
        mbLoaded = 1;
        {
            const uint32_t buffer = bridge::ScreenShot_Buffer();       // *0x832DB4
            const uint32_t slot = bridge::ScreenShot_5BA030(mScreenShot);
            bridge::Memcard_4A95B0(2, MagazineViewer::GetCurrentImageName(), buffer, slot);
        }
        bridge::EnterMemcard(pkg, pkg, 0xCC1, 0, 0, 0x42317A19u, 0x2B46FE08u);
        return;
    case 0x2B46FE08u:   // read failed: back to the list
        if (mbLoaded) {
            bridge::MagazineTextureHandler_Unload(mScreenShot);
            mbLoaded = 0;
            mbReady = 0;
        }
        if (bridge::cFEngInstance()) FEngSwitchPackages(pkg, "UI_MagazineSelect.fng", 0);
        return;
    case 0x911AB364u:   // back
        if (FEngIsPackagePushed(kHelpBar)) {
            if (!bridge::cFEngInstance()) return;
            FEngPopPackage(kHelpBar);
        }
        if (bridge::cFEngInstance()) FEngSwitchPackages(pkg, "UI_MagazineSelect.fng", 0);
        return;
    case 0xC98356BAu:   // tick: show the screen shot
        if (!mbLoaded || !mbReady) return;
        if (FEObject *img = FEngFindImage(pkg, "MagImage"))   // PC 495400 (by name)
            FEngSetTextureHash(img, *reinterpret_cast<const uint32_t *>(bridge::ScreenShot_Get(mScreenShot) + 0x24));
        FEngSetVisible(pkg, "MagImage");
        return;
    case 0x9803F6E2u: {   // mouse: highlight the back button, click = back
        FEObject *back = FEngFindImage(pkg, 0x1C7FCF8Bu);
        const int32_t *mouse = reinterpret_cast<const int32_t *>(bridge::MouseGlobals());
        const float y = static_cast<float>(static_cast<long double>(mouse[1]) - 240.0f);   // PC 0x78DA30
        const float x = static_cast<float>(static_cast<long double>(mouse[0]) - 320.0f);   // PC 0x797D50
        const bool over = FEngTestForIntersection(x, y, back);
        FEngSetScript(back, over ? 0xA3E4657Bu : 0x5BAB0FC0u, false);   // FEObject::SetScript 52B030
        if (FEngIsClickedOn(back)) bridge::FEngSendMessageToPackage(0x911AB364u, pkg);
        return;
    }
    default: return;
    }
}
