#include "cFEng.h"
#include "bWare/bString.h"
#include "FEngine.h"
#include "FEPackage.h"
#include "FEObject.h"
#include "FEPackageManager.h"
#include "bridge/Bridge.h"
#include "bWare/bSlotPool.h"

cFEngLevelSorter *cFEng::FindInLevelSortingList(const char *name)
{
    for (cFEngLevelSorter *n = mpSortHead; n != SortSentinel(); n = n->next)
        if (bStrICmp(n->mpcPackageName, name) == 0) return n;
    return nullptr;
}

void cFEng::NotificationMessage(uint32_t msg, FEObject *obj, uint32_t arg, FEPackage *pkg)
{
    if (msg == 0x05922615u) { mpEngine->mbFlag01 = 0; return; }   // "Disabling inputs!!"
    if (msg == 0x7E4D1288u) { mpEngine->mbFlag01 = 1; return; }   // "Enabling inputs!!"
    bridge::TheFEPackageManager()->NotificationMessage(msg, obj, arg, pkg);
}

void cFEng::SwitchFEngPackages(const char *from, const char *to)
{
    FEPackageManager *m = bridge::TheFEPackageManager();
    FEPackageData *d = m->FindFEPackageData(from);
    if (!d || !d->mpPackage) return;
    FEPackage *pkg = d->mpPackage;
    m->SendMessageToPackage(from, 0x5ECA113Du);
    mpEngine->QueuePackageSwitch(to, pkg);   // "SwitchFEngPackages from %s to %s"
}

void cFEng::QueuePopPackage(const char *name)
{
    FEPackageManager *m = bridge::TheFEPackageManager();
    FEPackageData *d = m->FindFEPackageData(name);
    if (d && d->mpPackage) {
        FEPackage *pkg = d->mpPackage;
        m->SendMessageToPackage(name, 0x90CA113Du);
        mpEngine->QueuePackagePop(pkg);      // "Queue popping %s"
    } else {
        mpEngine->PackagePopQueuedPushPackage(name);
    }
}

void cFEng::SetButtonState(const char *pkgName, uint32_t buttonHash, bool enable)
{
    FEPackageData *d = bridge::TheFEPackageManager()->FindFEPackageData(pkgName);
    if (!d || !d->mpPackage) return;
    FEPackage *pkg = d->mpPackage;
    FEObject *found = nullptr;
    for (uint32_t i = 0; i < pkg->mu32NumButtons; ++i)
        if (pkg->mppButtons[i]->mu32NameHash == buttonHash) found = pkg->mppButtons[i];
    if (!found) return;
    if (enable) found->mu32Flags &= ~0x04000000u;
    else found->mu32Flags |= 0x04000000u;
}

void cFEng::QueuePushPackage(const char *name, const char *parent, uint32_t mask)
{
    FEPackage *p = parent ? bridge::TheFEPackageManager()->FindPackage(parent) : nullptr;
    mpEngine->QueuePackagePush(name, p, mask);
}

void cFEng::QueuePushModalPackage(const char *name, const char *parent, uint32_t mask)
{
    FEPackage *p = parent ? bridge::TheFEPackageManager()->FindPackage(parent) : nullptr;
    mpEngine->QueuePackagePushForModal(name, p, mask);
}

void cFEng::ResyncSortingLevels()
{
    int32_t level = 0;
    for (cFEngLevelSorter *n = mpSortHead; n != SortSentinel(); n = n->next, ++level) {
        FEPackage *p = FEngFindPackage(n->mpcPackageName);
        n->mi32Level = level;
        if (p) p->mi32Priority = level;
    }
}

void cFEng::Popper(const char *name)
{
    cFEngLevelSorter *n = FindInLevelSortingList(name);
    if (!n) return;
    if (n->mi32State == 1 || n->mi32State == 0) bridge::cFEng_PopPackage(this, name);
    else QueuePopPackage(name);
    n->prev->next = n->next;
    n->next->prev = n->prev;
    bFree(bridge::LevelSorterSlots(), n);
    ResyncSortingLevels();
}

void cFEng::Switcher(const char *from, const char *to, int32_t arg)
{
    SwitchFEngPackages(from, to);
    cFEngLevelSorter *old = FindInLevelSortingList(from);
    old->mi32Level = 0;
    auto *n = static_cast<cFEngLevelSorter *>(bOMalloc(bridge::LevelSorterSlots()));
    if (n) {
        n->mpcPackageName = to;
        n->mi32Level = 0;
        n->mi32State = 0;
        n->mi32Arg = 0;
        n->mu32Flags = 0;
    }
    n->mi32Arg = arg;
    cFEngLevelSorter *next = old->next;   // insert after the old entry, then unlink the old one
    old->next = n;
    next->prev = n;
    n->prev = old;
    n->next = next;
    old->prev->next = old->next;
    old->next->prev = old->prev;
    bFree(bridge::LevelSorterSlots(), old);
    ResyncSortingLevels();
}
