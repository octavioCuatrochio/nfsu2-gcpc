#include "cFEng.h"
#include "bWare/bString.h"
#include "FEngine.h"
#include "FEPackage.h"
#include "FEObject.h"
#include "FEPackageManager.h"
#include "bridge/Bridge.h"

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
