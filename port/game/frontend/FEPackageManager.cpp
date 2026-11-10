#include "FEPackageManager.h"
#include "FEHash.h"
#include "FEPackage.h"
#include "bWare/bString.h"
#include "bridge/Bridge.h"
#include "FENode.h"
#include "FEngine.h"
#include "cFEng.h"
#include "UIWidgetMenu.h"

uint32_t FEPackageData::GetNameHash() const
{
    const uint32_t id = mpChunk[0];
    if (id == 0x30203) {
        const uint32_t *h = reinterpret_cast<const uint32_t *>(mpData);
        if (h[0] == 0xE76E4546u && h[2] == 0x64486B50u && h[4] >= 0x20000u) return FEHashUpper(reinterpret_cast<const char *>(mpData + 0x28));
        return 0;
    }
    if (id == 0x30210) return mpChunk[2];
    return 0;
}

uint8_t *FEPackageData::GetDataChunk()
{
    const uint32_t id = mpChunk[0];
    if (id == 0x30203) return mpData;
    if (id != 0x30210) return nullptr;
    const uint8_t *lz = reinterpret_cast<const uint8_t *>(mpChunk) + 0xC;
    mpData = static_cast<uint8_t *>(bridge::bMalloc(reinterpret_cast<const uint32_t *>(lz)[2], 0));
    bridge::LZDecompress(lz, mpData);
    return mpData;
}

void FEPackageData::Construct(uint32_t *chunk)
{
    vtable = bridge::FEPackageDataVTable();
    mVisible = 1;
    mpChunk = chunk;
    mpScreen = nullptr;
    mpPackage = nullptr;
    mPackageArg = 0;
    _unknown50 = 0;
    mPermanent = 0;
    mScreenCreateData = 0;
    mRenderInfo = 0;
    mRenderInfoFlag = 0;
    if (chunk[0] == 0x30203) mpData = reinterpret_cast<uint8_t *>(chunk + 2);
    else if (chunk[0] == 0x30210) mpData = nullptr;
    // PC inlines GetNameHash and only calls FindScreenCreateData when a hash was read (GC always calls it, with 0).
    if (chunk[0] == 0x30203) {
        const uint32_t *h = reinterpret_cast<const uint32_t *>(mpData);
        if (h[0] != 0xE76E4546u || h[2] != 0x64486B50u || h[4] < 0x20000u) return;
    } else if (chunk[0] != 0x30210) return;
    mScreenCreateData = bridge::FindScreenCreateData(GetNameHash());
}

void FEPackageData::Destruct()
{
    vtable = bridge::FEPackageDataVTable();
    if (mpChunk[0] != 0x30203 && mpChunk[0] == 0x30210) bridge::bWareFree(mpData);
    mpData = nullptr;
}

FEPackageData *FEPackageData::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

void FEPackageData::Activate(FEPackage *pkg, int32_t arg)
{
    mpPackage = pkg;
    pkg->mpPackageData = this;
    int32_t *counter = bridge::FEngLoadCounterPtr();   // GC gInsideScreenConstructor
    ++*counter;
    mpScreen = bridge::ScreenFactory(GetNameHash(), pkg, arg);
    mPackageArg = pkg->mu32ControlMask;
    --*counter;
}

void FEPackageData::UnActivate()
{
    if (mpScreen) bridge::VirtualScalarDelete(mpScreen);
    mpScreen = nullptr;
    // "Un-Activate!!! %x %x\n" / "Deleting package [%s]\n" (debug print stub)
    if (mpPackage) mpPackage->mpPackageData = nullptr;
    mpPackage = nullptr;
    if (mpChunk[0] == 0x30210) {
        bridge::bWareFree(mpData);
        mpData = nullptr;
    }
}

void FEPackageData::Close()
{
    // "Calling close ...%x %x\n"
    if (!mpPackage) return;
    bridge::HackClearCache(mpPackage);
    if (cFEng *fe = bridge::cFEngInstance()) fe->ForceUnloadPackage(mpPackage->mpcName);
    UnActivate();
}

const char *FEPackageManager::GetBasePkgName(const char *name)
{
    const char *p = name + bStrLen(name);
    if (p == name) return p;
    for (;;) {
        if (*p == '\\') return p + 1;
        if (--p == name) return p;
    }
}

FEPackageData *FEPackageManager::FindFEPackageData(const char *name)
{
    const uint32_t hash = FEHashUpper(GetBasePkgName(name));
    for (FEPackageData *d = FEPackageData::FromNode(mList.next); d != Sentinel(); d = FEPackageData::FromNode(d->node.next)) {
        if (d->GetNameHash() != hash) continue;
        FEPkgNode *n = &d->node, *next = n->next, *prev = n->prev;   // move to front
        prev->next = next;
        next->prev = prev;
        FEPkgNode *head = mList.next;
        mList.next = n;
        head->prev = n;
        n->prev = &mList;
        n->next = head;
        return d;
    }
    return nullptr;
}

FEPackageData *FEPackageManager::FindFEPackageData(const void *chunk)
{
    for (FEPackageData *d = FEPackageData::FromNode(mList.next); d != Sentinel(); d = FEPackageData::FromNode(d->node.next))
        if (d->mpChunk == chunk) return d;
    return nullptr;
}

uint8_t *FEPackageManager::GetPackageData(const char *name)
{
    FEPackageData *d = FindFEPackageData(name);
    return d ? d->GetDataChunk() : nullptr;
}

uint32_t FEPackageManager::GetActiveScreensChecksum()
{
    uint32_t sum = 0;
    for (FEPackageData *d = FEPackageData::FromNode(mList.next); d != Sentinel(); d = FEPackageData::FromNode(d->node.next))
        if (d->mpPackage) sum += bStringHash(d->mpPackage->mpcName);
    return sum;
}

void FEPackageData::NotificationMessage(uint32_t msg, void *obj, uint32_t arg, FEPackage *pkg)
{
    if (mpScreen) bridge::MenuScreen_BaseNotify(mpScreen, msg, obj, arg, pkg);
}

void FEPackageData::NotifySoundMessage(uint32_t msg, void *obj, uint32_t arg, FEPackage *pkg)
{
    if (mpScreen) bridge::MenuScreen_BaseNotifySound(mpScreen, msg, obj, arg, pkg);
}

void FEPackageManager::Construct()
{
    vtable = bridge::FEPackageManagerVTable();
    mList.next = mList.prev = &mList;
}

void FEPackageManager::Destruct()
{
    vtable = bridge::FEPackageManagerVTable();
    DeleteAllNodeOwners(reinterpret_cast<WidgetListNode *>(&mList));   // bTList::DeleteAllElements (PC 4358E0)
}

FEPackageManager *FEPackageManager::ScalarDeletingDestructor(uint32_t flags)
{
    Destruct();
    if (flags & 1) bridge::FreeArray(this);
    return this;
}

void FEPackageManager::SendMessageToPackage(const char *name, uint32_t msg)
{
    FEPackageData *d = FindFEPackageData(name);
    if (d && d->mpPackage) d->NotificationMessage(msg, nullptr, 0, d->mpPackage);
}

void FEPackageManager::BroadcastMessage(uint32_t msg, uint32_t arg)
{
    FEPackageData *list[32];
    int n = 0;
    for (FEPackageData *d = FEPackageData::FromNode(mList.next); d != Sentinel(); d = FEPackageData::FromNode(d->node.next))
        if (d->mpPackage && n < 32) list[n++] = d;
    for (int i = 0; i < n; ++i)
        if (list[i]->mpPackage) list[i]->NotificationMessage(msg, nullptr, arg, list[i]->mpPackage);
}

void FEPackageManager::NotificationMessage(uint32_t msg, void *obj, uint32_t arg, FEPackage *pkg)
{
    for (FEPackageData *d = FEPackageData::FromNode(mList.next); d != Sentinel(); d = FEPackageData::FromNode(d->node.next))
        if (d->mpPackage && d->mpPackage == pkg) d->NotificationMessage(msg, obj, arg, pkg);
}

void FEPackageManager::NotifySoundMessage(uint32_t msg, void *obj, uint32_t arg, FEPackage *pkg)
{
    for (FEPackageData *d = FEPackageData::FromNode(mList.next); d != Sentinel(); d = FEPackageData::FromNode(d->node.next))
        if (d->mpPackage && d->mpPackage == pkg) d->NotifySoundMessage(msg, obj, arg, pkg);
}

void FEPackageManager::CloseAllPackages(int32_t all)
{
    FEPackageData *list[256] = {};
    int n = 0;
    for (FEPackageData *d = FEPackageData::FromNode(mList.next); d != Sentinel(); d = FEPackageData::FromNode(d->node.next)) {
        if (d->mPermanent == 0 || all != 0) {
            if (n < 256) list[n++] = d;   // the original has no bounds check
        } else if (d->mpPackage) {
            bridge::HackClearCache(d->mpPackage);
        }
    }
    for (int i = 0; i < n; ++i) list[i]->Close();   // PC inlines Close (and cFEng::ForceUnloadPackage)
}

void FEPackageManager::Loader(uint32_t *chunk, bool)
{
    auto *d = static_cast<FEPackageData *>(bridge::Alloc(sizeof(FEPackageData)));
    d->Construct(chunk);   // PC: a null allocation would crash on the link below
    FEPkgNode *n = &d->node, *tail = mList.prev;
    tail->next = n;
    mList.prev = n;
    n->prev = tail;
    n->next = &mList;
}

void FEPackageManager::UnLoader(uint32_t *chunk, bool)
{
    // "UNLOADER!!!\n"
    JustUpdateFeng();
    FEPackageData *d = FindFEPackageData(static_cast<const void *>(chunk));
    if (!d) return;
    // "UNLOAD PACKAGE(%x)\n" (PC still calls GetNameHash for it)
    d->_unknown50 = 0;
    d->Close();
    d->node.prev->next = d->node.next;
    d->node.next->prev = d->node.prev;
    bridge::VirtualScalarDelete(d);
    // "Done the unload\n"
}

void FEPackageManager::PackageWasLoaded(FEPackage *pkg, int32_t arg)
{
    if (FEPackageData *d = FindFEPackageData(pkg->mpcName)) d->Activate(pkg, arg);
}

void FEPackageManager::PackageWillBeUnloaded(FEPackage *pkg)
{
    // "Will be unloaded [%s]\n"
    if (FEPackageData *d = FindFEPackageData(pkg->mpcName)) d->UnActivate();
}

int32_t LoaderFEngPackage(uint32_t *chunk)
{
    if (chunk[0] != 0x30203 && chunk[0] != 0x30210) return 0;
    bridge::TheFEPackageManager()->Loader(chunk, bridge::IsCurrentlyHotChunking());
    return 1;
}

int32_t UnloaderFEngPackage(uint32_t *chunk)
{
    if (chunk[0] != 0x30203 && chunk[0] != 0x30210) return 0;
    bridge::TheFEPackageManager()->UnLoader(chunk, bridge::IsCurrentlyHotChunking());
    return 1;
}

void JustUpdateFeng()
{
    if (cFEng *fe = bridge::cFEngInstance()) bridge::FEngine_Update(fe->mpEngine, 0, 0);
}

FEPackage *FEngFindPackage(const char *name)
{
    cFEng *fe = bridge::cFEngInstance();
    if (bridge::FEngLoadCounter() == 0 && fe) {
        FEngine *e = fe->mpEngine;
        if (!e) return nullptr;
        if (FENode *n = FEListFindNode(name, reinterpret_cast<FENode *>(e->mpPackages))) return reinterpret_cast<FEPackage *>(n);
        return e->FindIdlePackage(name);
    }
    FEPackageData *d = bridge::TheFEPackageManager()->FindFEPackageData(name);
    return d ? d->mpPackage : nullptr;
}

uint32_t FEPkgMgr_GetActiveScreensChecksum() { return bridge::TheFEPackageManager()->GetActiveScreensChecksum(); }
