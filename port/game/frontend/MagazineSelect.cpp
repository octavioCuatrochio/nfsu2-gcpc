#include "MagazineSelect.h"
#include <cstdio>
#include "bridge/Bridge.h"
#include "bWare/bString.h"
#include "cFEng.h"
#include "FEHash.h"
#include "FEngUtil.h"

namespace {
constexpr const char *kHelpBar = "UI_PC_Help_Bar.fng";
constexpr const char *kThumb = "MagThumb";        // PC 0x79C724
int32_t &Mode() { return *bridge::GameInt(0x838578); }
uint32_t &CurrentHash() { return *reinterpret_cast<uint32_t *>(bridge::GameInt(0x837FD8)); }
char *ReturnPackage() { return bridge::GameChars(0x837CA0); }   // 0x80 bytes
const char *Pkg(const UIMagazineSelect &m) { return m.base.base.mpPackageName; }
// PC `dec; neg; sbb; and K; add base`: mode != 1 ? base + K : base
uint32_t ByMode(uint32_t base, uint32_t k) { return Mode() != 1 ? base + k : base; }
uint32_t RecordHash(const uint8_t *r) { return *reinterpret_cast<const uint32_t *>(r + 0x28); }

// mode, remember the caller and switch (PC: cFEng null check around the switch)
void Enter(int32_t mode, const char *from, const char *to)
{
    Mode() = mode;
    bStrNCpy(ReturnPackage(), from, 0x80);
    if (bridge::cFEngInstance()) FEngSwitchPackages(from, to, 0);
}
}  // namespace

// ---- SCMagazineDatabase ----

namespace {
void *FindByHash(void *db, uint32_t count, uint32_t first, uint32_t stride, uint32_t hash)
{
    uint8_t *p = static_cast<uint8_t *>(db) + first;
    for (uint32_t i = 0; i < count; ++i, p += stride)
        if (*reinterpret_cast<uint32_t *>(p) == hash) return p;
    return nullptr;
}
}  // namespace
void *SCMagazineDatabase::GetCarSetupByHash(uint32_t h) { return FindByHash(this, mNumCarSetups, 0xC, 0x2C, h); }
void *SCMagazineDatabase::GetRenderSetupByHash(uint32_t h) { return FindByHash(this, mNumRenderSetups, 0xB0C, 0xC, h); }
void *SCMagazineDatabase::GetMagazineSetupByHash(uint32_t h) { return FindByHash(this, mNumMagazineSetups, 0xC8C, 0x38, h); }
void *SCMagazineDatabase::GetMagazineSetupByIndex(uint32_t i) { return reinterpret_cast<uint8_t *>(this) + 0xC8C + i * 0x38; }

// ---- MagazineViewer ----

const uint8_t *MagazineViewer::ShowcaseRecord(int32_t i)
{
    if (i < 0 || i >= *bridge::GameInt(0x838408)) return nullptr;
    return bridge::ShowcaseRecords() + 0x40 * i;
}

void MagazineViewer::GetImageNameFromShowcaseDataIndex(int32_t index, char *buf, int32_t)
{
    // count magazines / DVDs up to and including `index`; the last record visited decides the kind
    int32_t magazines = 0, dvds = 0;
    const uint8_t *r = nullptr;
    for (int32_t i = 0; i <= index; ++i) {
        r = ShowcaseRecord(i);
        const uint8_t kind = r[0x24];
        if (kind == 1 || kind == 3) ++magazines;
        else if (kind == 2 || kind == 4) ++dvds;
    }
    const uint8_t kind = r[0x24];   // PC: index < 0 reads a null record too
    if (kind == 1 || kind == 3) std::sprintf(buf, "Magazine %d", magazines);   // FEngSNPrintf 50C960 ignores len
    else if (kind == 2 || kind == 4) std::sprintf(buf, "DVD %d", dvds);
}

void MagazineViewer::GetImageNameFromFileName(const char *file, char *buf, int32_t len)
{
    bStrLen(file);   // PC: result unused
    const int32_t c = static_cast<signed char>(file[7]);
    int32_t index = 0;
    if (c >= '0' && c <= '9') index = c - '0';
    else if (c >= 'A' && c <= 'Z') index = c - 0x37;
    GetImageNameFromShowcaseDataIndex(index, buf, len);
}

const char *MagazineViewer::MakeProfileImageName(int32_t index)
{
    char *out = bridge::GameChars(0x837B80);
    const char *profile = bridge::GameChars(0x83A9E0);
    const int len = bStrLen(profile);
    for (int i = 0; i < len; ++i) out[i] = profile[i];   // no terminator copied
    for (int i = len; i < 7; ++i) out[i] = '_';
    out[8] = 0;
    out[7] = static_cast<char>(index < 10 ? index + '0' : index + 0x37);
    return out;
}

const char *MagazineViewer::GetCurrentImageName()
{
    const int32_t n = *bridge::GameInt(0x838408);
    int32_t found = -1;
    for (int32_t i = 0; i < n; ++i)
        if (RecordHash(bridge::ShowcaseRecords() + 0x40 * i) == CurrentHash()) {
            found = i;
            break;
        }
    return MakeProfileImageName(found);
}

int32_t MagazineViewer::GetCurrentMode() { return Mode(); }
uint32_t MagazineViewer::GetMagSetupHash() { return CurrentHash(); }
void MagazineViewer::EnterIngameMagazineReward() { Mode() = 1; }
void MagazineViewer::EnterIngameDvdReward() { Mode() = 2; }
void MagazineViewer::ReturnFromIngameReward() { Mode() = 0; }

void MagazineViewer::EnterFrontendMagazineReward(const char *from)
{
    Mode() = 1;
    if (from) Enter(1, from, "UI_MagazineReward.fng");
}
void MagazineViewer::EnterFrontendDvdReward(const char *from)
{
    Mode() = 2;
    if (from) Enter(2, from, "UI_MagazineReward.fng");
}
void MagazineViewer::EnterFrontendMagazineSelect(const char *from) { Enter(1, from, "UI_MagazineSelect.fng"); }
void MagazineViewer::EnterFrontendDvdSelect(const char *from) { Enter(2, from, "UI_MagazineSelect.fng"); }

void MagazineViewer::ReturnToFrontend(const char *pkg)
{
    Mode() = 0;
    if (bridge::cFEngInstance()) FEngSwitchPackages(pkg, ReturnPackage(), 0);
}

// ---- MagazineThing ----

void MagazineThing::React(const char *pkg, uint32_t msg)
{
    if (msg != 0x0C407210u) return;
    CurrentHash() = RecordHash(MagazineViewer::ShowcaseRecord(mIndex));
    if (bridge::cFEngInstance()) FEngSwitchPackages(pkg, bridge::GameStringPointer(0x7FA2A0), 0);
}

// ---- UIMagazineSelect ----

UIMagazineSelect *UIMagazineSelect::Construct(const ScreenConstructorData *data)
{
    base.Construct(data);
    base.base.vtable = bridge::GameVTable(0x79CE04);
    bridge::MagazineTextureHandler_Reset();   // 488AE0 on 0x832DAC
    mbScreenShotLoaded = 0;
    _unknown155 = 0;
    Setup();
    if (!FEngIsPackagePushed(kHelpBar)) FEngShowPackage(kHelpBar, Pkg(*this), 0xFF, 0);
    bridge::HelpBar_SetFlag6C(1);
    bridge::HelpBar_542990(true);
    bridge::HelpBar_SyncByMask(base.base.mData.mHelpBarMask, Pkg(*this));
    return this;
}

void UIMagazineSelect::Destruct()
{
    base.base.vtable = bridge::GameVTable(0x79CE04);
    base.Destruct();
}

UIMagazineSelect *UIMagazineSelect::ScalarDeletingDestructor(uint32_t f)
{
    Destruct();
    if (f & 1) bridge::FreeArray(this);
    return this;
}

void UIMagazineSelect::DeallocateScreenShot()
{
    if (!mbScreenShotLoaded) return;
    bridge::MagazineTextureHandler_Unload(mScreenShot);   // 488AA0 on 0x832DAC
    mbScreenShotLoaded = 0;
    _unknown155 = 0;
}

void UIMagazineSelect::BuildShowcaseList()
{
    const uint32_t name = ByMode(0xFF293517u, 0x5A54BD90u);
    const int32_t kindA = Mode() == 1 ? 1 : 2, kindB = Mode() == 1 ? 3 : 4;
    for (int32_t i = 0; i < *bridge::GameInt(0x838408); ++i) {
        const uint8_t *r = MagazineViewer::ShowcaseRecord(i);
        const uint8_t kind = r[0x24];
        if (kind != kindA && kind != kindB) continue;
        const int32_t key = *reinterpret_cast<const int32_t *>(r + 0x20);
        const uint8_t *unlock = nullptr;
        const int32_t n = *bridge::GameInt(0x86283C);
        for (int32_t u = 0; u < n; ++u)
            if (*reinterpret_cast<const int32_t *>(bridge::UnlockEntries() + 8 * u) == key) {
                unlock = bridge::UnlockEntries() + 8 * u;
                break;
            }
        if (!unlock || unlock[6] != 1) continue;
        auto *t = static_cast<MagazineThing *>(bridge::Alloc(sizeof(MagazineThing)));
        if (t) {
            const uint32_t title = bStringHash("_TITLE", RecordHash(r));   // 43DB80
            t->base.Construct(name, title, 0);   // PC inlined (same fields)
            t->base.vtable = bridge::GameVTable(0x79B840);
            t->mIndex = i;
        }
        bridge::IconScrollerMenu_AddOption(&base, t ? &t->base : nullptr);   // vtable +0x18
    }
}

void UIMagazineSelect::PopulateDataInfoStuff()
{
    const auto *cur = reinterpret_cast<const MagazineThing *>(base.mScroller.base.mpCurrent);
    const uint8_t *r = MagazineViewer::ShowcaseRecord(cur->mIndex);
    FEngSetLanguageHash(Pkg(*this), 0x42ADB44Cu, ByMode(0xF7333312u, 0x6E07F3B2u));
    FEngSetLanguageHash(Pkg(*this), 0x010CBA23u, ByMode(0x246536EFu, 0x96A425B2u));
    FEngSetLanguageHash(Pkg(*this), 0x9E302291u, bStringHash("_TITLE", RecordHash(r)));
    FEngSetLanguageHash(Pkg(*this), 0x3554DEB1u, bStringHash("_BODY", RecordHash(r)));
}

void UIMagazineSelect::Setup()
{
    FEngSetInvisible(Pkg(*this), kThumb);
    BuildShowcaseList();
    uint8_t &initialized = *bridge::GameByte(0x838581);
    if (!initialized) {
        bridge::IconScroller_SetInitialPos(&base.mScroller, 1);
        initialized = 1;
    } else {
        const int32_t *slot = bridge::ScreenLastButtonSlot(FEHashUpper(Pkg(*this)));
        bridge::IconScroller_SetInitialPos(&base.mScroller, slot ? *slot : 0);
    }
    base.base.mHelpHash = ByMode(0x82F20772u, 0x07AC07D2u);
    PopulateDataInfoStuff();
    bridge::IconScrollerMenu_RefreshHeader(&base);   // vtable +0x10 (tail call)
}

void UIMagazineSelect::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, uint32_t pkgArg)
{
    base.NotificationMessage(msg, obj, arg, pkgArg);
    switch (msg) {
    case 0xB5971BF1u:
    case 0x9120409Eu: PopulateDataInfoStuff(); return;
    case 0x911AB364u:   // back (PC: both mode branches call the same return)
        DeallocateScreenShot();
        *bridge::GameByte(0x838581) = 0;
        MagazineViewer::ReturnToFrontend(Pkg(*this));
        return;
    case 0xC98356BAu: {   // tick: thumbnail of the selected record
        const auto *cur = reinterpret_cast<const MagazineThing *>(base.mScroller.base.mpCurrent);
        if (!cur) return;
        const uint32_t hash = RecordHash(MagazineViewer::ShowcaseRecord(cur->mIndex));
        if (FEObject *img = FEngFindImage(Pkg(*this), kThumb)) FEngSetTextureHash(img, hash);
        FEngSetVisible(Pkg(*this), kThumb);
        return;
    }
    default: return;
    }
}
