#include "cFEng.h"
#include "bWare/bString.h"
#include "FEngine.h"
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
