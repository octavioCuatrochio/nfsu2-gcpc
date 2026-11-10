#include "cFEng.h"
#include "bWare/bString.h"
#include "FEngine.h"
#include "FENode.h"
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
    if (n->mi32State == 1 || n->mi32State == 0) PopPackage(name);
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

void cFEng::PopPackage(const char *name)
{
    FEPackageManager *m = bridge::TheFEPackageManager();
    FEPackageData *d = m->FindFEPackageData(name);
    if (!d || !d->mpPackage) return;
    FEPackage *pkg = d->mpPackage;
    m->SendMessageToPackage(name, 0xD9CA113Du);
    struct { void *vtable; void *render; void *renderInfo; } cb;   // RenderObjectDisconnect (PC 12 bytes)
    cb.vtable = bridge::RenderObjectDisconnectVTable();
    cb.render = mpRender;
    cb.renderInfo = pkg->mpPackageData ? reinterpret_cast<uint8_t *>(pkg->mpPackageData) + 0x24 : nullptr;
    pkg->ForAllObjects(reinterpret_cast<FEObjectCallback *>(&cb));
    bridge::FEngine_UnloadPackage(mpEngine, pkg);   // "Direct Pop---------------------%s------------------"
}

void cFEng::ForceUnloadPackage(const char *name)
{
    cFEngLevelSorter *n = FindInLevelSortingList(name);
    if (!n) return;
    PopPackage(name);   // "CAlling the pop in [%s]"
    n->prev->next = n->next;
    n->next->prev = n->prev;
    bFree(bridge::LevelSorterSlots(), n);
}

void cFEng::Pusher(uint32_t flags, const char *name, const char *parent, uint32_t mask, int32_t arg)
{
    auto *n = static_cast<cFEngLevelSorter *>(bOMalloc(bridge::LevelSorterSlots()));
    if (n) {
        n->mpcPackageName = name;
        n->mi32Level = 0;
        n->mi32State = 0;
        n->mi32Arg = 0;
        n->mu32Flags = 0;
    }
    n->mi32Arg = arg;
    n->mu32Flags = flags;
    cFEngLevelSorter *const s = SortSentinel();
    auto insertAfter = [&](cFEngLevelSorter *p) { cFEngLevelSorter *next = p->next; p->next = n; next->prev = n; n->next = next; n->prev = p; };
    auto insertBefore = [&](cFEngLevelSorter *p) { cFEngLevelSorter *prev = p->prev; prev->next = n; p->prev = n; n->prev = prev; n->next = p; };
    switch (flags & 6) {
    case 0: insertAfter(s); break;
    case 2: { cFEngLevelSorter *p = FindInLevelSortingList(parent); if (p) insertBefore(p); else insertAfter(s); break; }
    case 4: { cFEngLevelSorter *p = FindInLevelSortingList(parent); if (p) insertAfter(p); else insertBefore(s); break; }
    default: {   // 6
        cFEngLevelSorter *found = nullptr;
        for (cFEngLevelSorter *p = mpSortTail; p != s; p = p->prev)
            if (p->mu32Flags & 8) found = p;
        if (found) insertBefore(found); else insertBefore(s);
        break;
    }
    }
    cFEng *inst = bridge::cFEngInstance();
    if (flags & 0x11) {
        n->mi32State = 2;
        if (inst) {
            if (flags & 1) inst->QueuePushPackage(name, parent, mask);
            else inst->QueuePushModalPackage(name, parent, mask);
        }
    } else {
        n->mi32State = 1;
        if (inst) {
            FEngine *e = inst->mpEngine;
            if (!FEListFindNode(name, reinterpret_cast<FENode *>(e->mpPackages)))   // cFEng::PushPackage + IsPackageLoaded
                bridge::FEngine_PushPackage(e, name, 0, mask);
        }
    }
    ResyncSortingLevels();
}

void FEngStartInteraction(const char *name, int32_t arg)
{
    if (cFEng *fe = bridge::cFEngInstance()) {
        fe->Pusher(0, name, nullptr, 0xFF, arg);
        bridge::cFEngInstance()->RegisterJoy();
    }
}
void FEngShowPackageModal(const char *name, const char *parent, uint32_t mask, int32_t arg)
{
    if (cFEng *fe = bridge::cFEngInstance()) fe->Pusher(5, name, parent, mask, arg);
}
void FEngShowPackageTrueModal(const char *name, const char *parent, uint32_t mask, int32_t arg)
{
    if (cFEng *fe = bridge::cFEngInstance()) fe->Pusher(0x16, name, parent, mask, arg);
}
void FEngShowPackage(const char *name, const char *parent, uint32_t mask, int32_t arg)
{
    if (cFEng *fe = bridge::cFEngInstance()) {
        if (parent) fe->Pusher(4, name, parent, mask, arg);
        else fe->Pusher(0, name, nullptr, mask, arg);
    }
}
void FEngPushPackageSpecial(uint32_t flags, const char *name, const char *parent, uint32_t mask, int32_t arg)
{
    if (cFEng *fe = bridge::cFEngInstance()) fe->Pusher(flags, name, parent, mask, arg);
}
void FEngSetControlMask(const char *name, uint32_t mask)
{
    if (FEPackage *p = FEngFindPackage(name)) p->SetControlMask(mask);
}
void FEngSwitchPackages(const char *from, const char *to, int32_t arg)
{
    if (cFEng *fe = bridge::cFEngInstance()) fe->Switcher(from, to, arg);
}
void FEngPopPackage(const char *name)
{
    if (cFEng *fe = bridge::cFEngInstance()) fe->Popper(name);
}

void cFEng::HandleJoy(uint32_t port, uint32_t event, uint8_t state, void *)
{
    FEngJoyMapEntry *map = bridge::FEngJoyMap();
    for (int i = 0; i < kFEngJoyMapCount; ++i)
        if (map[i].mu32Event == event) map[i].mState[port] = state;   // (debug print "Joy Event: %s[%d]" is a no-op)
}

void cFEng::RegisterJoy()
{
    if (!*bridge::FEngJoyRegistered()) {
        FEngJoyMapEntry *map = bridge::FEngJoyMap();
        for (int i = 0; i < kFEngJoyMapCount; ++i) {
            bridge::AddJoyHandlerToAllPorts(map[i].mu32Event, bridge::FEngHandleJoyFunction(), this);
            map[i].mState[0] = map[i].mState[1] = map[i].mState[2] = map[i].mState[3] = 0;
        }
        bridge::InitDebugLanguageChange();   // PC: AddJoyHandler(0, 0x34, 0x527520, 0), inlined
    }
    *bridge::FEngJoyRegistered() = 1;
}

void cFEng::UnRegisterJoy()
{
    if (*bridge::FEngJoyRegistered()) {
        FEngJoyMapEntry *map = bridge::FEngJoyMap();
        for (int i = 0; i < kFEngJoyMapCount; ++i) {
            bridge::RemoveJoyHandlerFromAllPorts(map[i].mu32Event);
            map[i].mState[0] = map[i].mState[1] = map[i].mState[2] = map[i].mState[3] = 0;
        }
        bridge::CloseDebugLanguageChange();   // PC: RemoveJoyHandler(0, 0x34), inlined
    }
    *bridge::FEngJoyRegistered() = 0;
}

uint32_t cFEng::GetJoyPadMask(uint8_t pad)
{
    const int32_t port = pad <= 3 ? pad : -1;
    FEngJoyMapEntry *map = bridge::FEngJoyMap();
    if (!bridge::IsLiveJoystickPluggedIn(port)) {
        // like the original, pads >= 4 index past mState into the following fields/entries
        for (int i = 0; i < kFEngJoyMapCount; ++i) reinterpret_cast<uint32_t *>(&map[i].mState)[pad] = 0;
        return 0;
    }
    uint32_t mask = 0;
    for (int i = 0; i < kFEngJoyMapCount; ++i)
        if (reinterpret_cast<const uint32_t *>(&map[i].mState)[pad]) mask |= map[i].mu32Mask;
    return mask;
}
