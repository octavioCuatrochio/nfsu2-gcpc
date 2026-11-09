#include "FEPackageManager.h"
#include "FEHash.h"
#include "FEPackage.h"
#include "bWare/bString.h"
#include "bridge/Bridge.h"

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

uint32_t FEPkgMgr_GetActiveScreensChecksum() { return bridge::TheFEPackageManager()->GetActiveScreensChecksum(); }
