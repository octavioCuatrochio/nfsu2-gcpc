#include "FEScript.h"
#include "FENode.h"
#include "FEPackageReader.h"
#include "FEngine.h"
#include "FECodeListBox.h"
#include "FEListBox.h"
#include "cFEng.h"
#include "FEHash.h"
#include <cstring>
#include "bridge/Bridge.h"
#include "FEPackage.h"
#include "FEMessageResponse.h"
#include "FEObject.h"

void FEngine::QueueMessage(uint32_t msg, FEObject *obj, FEPackage *pkg, FEObject *target, uint32_t arg)
{
    auto *n = static_cast<FEMessageNode *>(bridge::Alloc(sizeof(FEMessageNode)));
    if (n) {
        n->node.next = n->node.prev = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
        n->node.vtable = bridge::FEMessageNodeVTable();
    }
    n->mpPackage = pkg;
    n->mu32MsgId = msg;
    n->mpObject = obj;
    n->mpTarget = target;
    n->mu32Arg = arg;
    if (mbForwardQueuedMessages) bridge::FEUIData_OnQueueMessage(mpUIData, msg, obj, pkg, target, arg);
    mMessages.AddTail(&n->node);
}

namespace {
// Inlined FEPackageCommand constructor + field setup (PC: every QueuePackage* starts like this). Like the original,
// allocation failure crashes (writes through null).
FEPackageCommand *NewPackageCommand(uint32_t type, uint32_t mask, FEPackage *pkg)
{
    auto *c = static_cast<FEPackageCommand *>(bridge::Alloc(sizeof(FEPackageCommand)));
    if (c) {
        c->node.node.next = c->node.node.prev = reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED);
        c->node.mpcName = nullptr;
        c->node.mu32NameHash = 0;
        c->node.node.vtable = bridge::FEPackageCommandVTable();
    }
    c->mpPackage = pkg;
    c->mu32Mask = mask;
    c->mu32Type = type;
    return c;
}
FEPackage *NextPackage(const FEPackage *p) { return reinterpret_cast<FEPackage *>(reinterpret_cast<const FEMinNode *>(p)->next); }
}

// GC null-checks pkg before clearing its masks; PC reads pkg->mu32ControlMask first, so pkg must be valid.
void FEngine::QueuePackageSwitch(const char *name, FEPackage *pkg)
{
    FEPackageCommand *c = NewPackageCommand(3, pkg->mu32ControlMask, pkg);
    c->node.SetName(name);
    mPackageCommands.AddTail(&c->node.node);
    pkg->SetControlMask(0);
}

void FEngine::QueuePackagePush(const char *name, FEPackage *pkg, uint32_t mask)
{
    FEPackageCommand *c = NewPackageCommand(2, mask, pkg);
    c->node.SetName(name);
    mPackageCommands.AddTail(&c->node.node);
    if (pkg) pkg->SetControlMask(pkg->mu32ControlMask & ~mask);
}

void FEngine::QueuePackagePushForModal(const char *name, FEPackage *pkg, uint32_t mask)
{
    FEPackageCommand *c = NewPackageCommand(0x12, mask, pkg);
    c->node.SetName(name);
    mPackageCommands.AddTail(&c->node.node);
    for (FEPackage *p = mpPackages; p; p = NextPackage(p)) p->SetControlMaskOnly(p->mu32ControlMask & ~mask);
}

void FEngine::QueuePackagePop(FEPackage *pkg)
{
    FEPackageCommand *c = NewPackageCommand(1, pkg->mu32ControlMask, pkg);
    mPackageCommands.AddTail(&c->node.node);
    for (FEPackage *p = mpPackages; p; p = NextPackage(p)) p->RestoreControlMask();
    pkg->SetControlMask(0);
}

bool FEngine::PackagePopQueuedPushPackage(const char *name)
{
    if (!name) return false;
    const uint32_t hash = FEHashUpper(name);
    for (FEMinNode *n = mPackageCommands.tail; n; n = n->prev) {
        auto *c = reinterpret_cast<FEPackageCommand *>(n);
        if ((c->mu32Type & 2) && c->node.mu32NameHash == hash) {
            mPackageCommands.RemNode(n);
            for (FEPackage *p = mpPackages; p; p = NextPackage(p)) {
                p->RestoreControlMask();
                p->RestoreControlMaskUsedByInput();
            }
            return true;
        }
        if (n == mPackageCommands.head) break;
    }
    return false;
}

// Command type bits: 1 pop the command's package (unload; its parent gets the command mask back), 2 push the named
// package (if it is in cFEng's level sorting list) at priority+1, 0x10 (modal) limit every other package's input
// mask, 4 / 8 hand the masked controls back to the parent / to the first child. A switch (3) pops then pushes,
// and the new package takes over the old one's parent and children.
// PC differences from GC: the parent of a popped package must still be active (else it is dropped, with a debug
// print); the "replaced" package/parent of a switch persist across loop iterations (GC: reset per command).
void FEngine::ProcessPackageCommands()
{
    FEPackage *replaced = nullptr;       // PC [esp+0x1C]: package popped by the last switch
    FEPackage *replacedParent = nullptr; // PC [esp+0x18]: its parent
    while (FEMinNode *head = mPackageCommands.RemHead()) {
        auto *c = reinterpret_cast<FEPackageCommand *>(head);
        int32_t priority = c->mpPackage ? c->mpPackage->mi32Priority : -1;
        if ((c->mu32Type & 1) && priority >= 0) {
            FEPackage *pkg = c->mpPackage;
            if (FEPackage *parent = pkg->mpParent) {
                bool active = false;
                for (FEPackage *p = mpPackages; p; p = NextPackage(p))
                    if (p == parent) { active = true; break; }
                if (!active) pkg->mpParent = nullptr;   // "FEPC_Pop : Parent of package being popped ... invalid"
            }
            if (!(c->mu32Type & 2)) {
                FEPackage *grand = pkg->mpParent;
                for (FEPackage *p = mpPackages; p; p = NextPackage(p))
                    if (p->mpParent == pkg) p->mpParent = grand;
            } else {
                replaced = pkg;
                replacedParent = pkg->mpParent;
            }
            if (FEPackage *parent = c->mpPackage->mpParent) parent->SetControlMask(parent->mu32ControlMask | c->mu32Mask);
            if (!(c->mu32Type & 2))
                for (FEPackage *p = mpPackages; p; p = NextPackage(p)) p->RestoreControlMaskUsedByInput();
            bridge::FEngine_UnloadPackage(this, c->mpPackage);
            --priority;
        }
        if ((c->mu32Type & 2) && bridge::cFEngInstance()->FindInLevelSortingList(c->node.mpcName)) {
            FEPackage *np = bridge::FEngine_PushPackage(this, c->node.mpcName, static_cast<uint8_t>(priority + 1), c->mu32Mask);
            if (np) {
                if (!(c->mu32Type & 1) && priority >= 0) {
                    np->mpParent = c->mpPackage;
                    c->mpPackage->SetControlMaskOnly(c->mpPackage->mu32ControlMask & ~c->mu32Mask);
                } else if (c->mu32Type & 1) {
                    np->mpParent = replacedParent;
                    for (FEPackage *p = mpPackages; p; p = NextPackage(p))
                        if (p->mpParent == replaced) p->mpParent = np;
                }
            }
            if (c->mu32Type & 0x10)
                for (FEPackage *p = mpPackages; p; p = NextPackage(p))
                    if (p != np) p->SetControlMaskUsedByInputOnly(p->mu32ControlMask & ~c->mu32Mask);
        }
        // hand the masked controls from the command's package to `to` (and tell `to`)
        auto giveBack = [&](FEPackage *to) {
            FEPackage *pkg = c->mpPackage;
            const uint32_t m = pkg->mu32ControlMask & c->mu32Mask;
            pkg->SetControlMask(pkg->mu32ControlMask & ~m);
            to->SetControlMask(to->mu32ControlMask | m);
            QueueMessage(0x334C5493u, nullptr, to, reinterpret_cast<FEObject *>(uintptr_t(0xFFFFFFFCu)), c->mu32Mask);
        };
        if (c->mu32Type & 4)
            if (FEPackage *parent = c->mpPackage->mpParent) giveBack(parent);
        if (c->mu32Type & 8)
            for (FEPackage *p = mpPackages; p; p = NextPackage(p))
                if (p->mpParent == c->mpPackage) { giveBack(p); break; }
        bridge::VirtualScalarDelete(c);
    }
}

void FEngine::QueuePackageUserTransfer(FEPackage *pkg, bool flag, uint32_t mask)
{
    FEPackageCommand *c = NewPackageCommand(flag ? 8 : 4, pkg->mu32ControlMask & mask, pkg);
    mPackageCommands.AddTail(&c->node.node);
}

bool FEngine::RecordPackageMarker(const char *name)
{
    if (mu32NumPackageMarkers == 16) return false;
    std::strcpy(macPackageMarkers[mu32NumPackageMarkers++], name);
    return true;
}

void FEngine::ClearPackageMarkers()
{
    for (auto &m : macPackageMarkers) m[0] = 0;
    mu32NumPackageMarkers = 0;
}

void FEngine::RecordLastPackageButton(uint32_t pkgHash, uint32_t guid)
{
    for (auto &b : mLastButtons)
        if (b.mu32PackageHash == pkgHash) b.mu32PackageHash = 0;
    mLastButtons[mi32LastButtonIndex] = {pkgHash, guid};
    mi32LastButtonIndex = (mi32LastButtonIndex + 1) % 32;   // signed modulo like the original
}

uint32_t FEngine::RecallLastPackageButton(uint32_t pkgHash) const
{
    for (const auto &b : mLastButtons)
        if (b.mu32PackageHash == pkgHash) return b.mu32ButtonGUID;
    return 0;
}

// GC 801484AC: the package's own response to the message, if any.
void FEngine::ProcessGlobalMessage(FEPackage *pkg, uint32_t msg, uint32_t arg)
{
    if (FEMessageResponse *r = pkg->FindResponse(msg)) ProcessResponses(r, nullptr, pkg, arg);
}

// PC 541FC0. Targets: an object -> ProcessObjectMessage; 0 -> every package (global response + listed objects);
// -4 -> the sender package only (if active); -2 -> every package's global response; -3 -> the sender package's
// global response; -1 / -5 -> game callbacks (UI vtable +0x18 / +0x1C); -6 -> engine flag 1.
void FEngine::ProcessMessageQueue()
{
    while (FEMinNode *head = mMessages.RemHead()) {
        auto *n = reinterpret_cast<FEMessageNode *>(head);
        const uint32_t msg = n->mu32MsgId;
        if (mbForwardQueuedMessages)
            bridge::FEUIData_Call5(mpUIData, 0x58, msg, n->mpTarget, n->mpObject, n->mpPackage, n->mu32Arg);
        const uint32_t target = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(n->mpTarget));
        auto deliver = [&](FEPackage *p) {
            ProcessGlobalMessage(p, msg, n->mu32Arg);
            if (FEMessageTargets *t = p->GetMessageTargets(msg))
                for (uint32_t i = 0; i < t->mu32Count; ++i)
                    ProcessObjectMessage(t->mpObjects[i], p, msg, n->mu32Arg);
        };
        switch (target) {
        case 0:
            for (FEPackage *p = mpPackages; p; p = NextPackage(p)) deliver(p);
            break;
        case 0xFFFFFFFCu:
            for (FEPackage *p = mpPackages; p; p = NextPackage(p))
                if (p == n->mpPackage) { deliver(p); break; }
            break;
        case 0xFFFFFFFAu:
            if (msg == 0x59BED120u) mbFlag01 = 1;
            else if (msg == 0x5D4CE32Du) mbFlag01 = 0;
            break;
        case 0xFFFFFFFBu:
            bridge::FEUIData_Call4(mpUIData, 0x1C, msg, n->mpObject, n->mu32Arg, n->mpPackage);
            break;
        case 0xFFFFFFFFu:
            bridge::FEUIData_Call4(mpUIData, 0x18, msg, n->mpObject, n->mu32Arg, n->mpPackage);
            break;
        case 0xFFFFFFFEu:
            for (FEPackage *p = mpPackages; p; p = NextPackage(p)) ProcessGlobalMessage(p, msg, n->mu32Arg);
            break;
        case 0xFFFFFFFDu:
            ProcessGlobalMessage(n->mpPackage, msg, n->mu32Arg);   // no null check, like the original
            break;
        default:
            ProcessObjectMessage(n->mpTarget, n->mpPackage, msg, n->mu32Arg);
            break;
        }
        bridge::VirtualScalarDelete(n);
    }
}

namespace {
// Scroll messages (up/down/left/right as sent by the pad code) -> selection delta.
bool ScrollDelta(uint32_t msg, int32_t &dx, int32_t &dy)
{
    switch (msg) {
    case 0xE10814A6u: dx = 0; dy = 1; return true;
    case 0x030471ACu: dx = 1; dy = 0; return true;
    case 0xE10C4AF9u: dx = -1; dy = 0; return true;
    case 0xFB814F13u: dx = 0; dy = -1; return true;
    default: return false;
    }
}
}  // namespace

bool FEngine::ProcessListBoxResponses(FEObject *listBox, FEPackage *, uint32_t msg)
{
    int32_t dx, dy;
    if (!ScrollDelta(msg, dx, dy)) return false;
    reinterpret_cast<FEListBox *>(listBox)->ScrollSelection(dx, dy);
    return true;
}

bool FEngine::ProcessCodeListBoxResponses(FEObject *listBox, FEPackage *, uint32_t msg)
{
    int32_t dx, dy;
    if (!ScrollDelta(msg, dx, dy)) return false;
    reinterpret_cast<FECodeListBox *>(listBox)->ScrollSelection(dx, dy);
    return true;
}

void FEngine::ProcessObjectMessage(FEObject *obj, FEPackage *pkg, uint32_t msg, uint32_t arg)
{
    if (obj->mu32Type == 4 && ProcessListBoxResponses(obj, pkg, msg)) return;
    if (obj->mu32Type == 6 && ProcessCodeListBoxResponses(obj, pkg, msg)) return;
    if (FEMessageResponse *r = obj->FindResponse(msg)) ProcessResponses(r, obj, pkg, arg);
}

// PC 52C7C0. Response = {type, param, extra}. Opcodes:
//   0 run script `param` on obj (time 0)       1 queue message param to target `extra` (GUID, or -4/-1 as is)
//   2/3 queue message param to the game (-1) / special (-5)
//   0x100 make GUID `param` the current button (0: none, no notification; missing GUID: ignored)
//   0x101 engine flag 1 = (param == 1)          0x102 / 0x103 record / recall the package's last button
//   0x105..0x108 package user transfer           0x200 switch / 0x201 push (control mask) / 0x202 push per arg bit
//   0x203 pop / 0x204 push (mask 0)               0x2C0..0x2C2 package marker record / recall / clear
//   0x300 / 0x301 if the current script id is not / is `param`: continue, else jump to the matching branch;
//   0x500 (else) jumps to the end. The response count is read once (like the original).
void FEngine::ProcessResponses(FEMessageResponse *resp, FEObject *obj, FEPackage *pkg, uint32_t arg)
{
    const uint32_t count = resp->mu32Count;
    auto special = [](uint32_t v) { return reinterpret_cast<FEObject *>(static_cast<uintptr_t>(v)); };
    for (uint32_t i = 0; i < count; ++i) {
        const FEResponse &r = resp->mpResponses[i];
        const uint32_t param = static_cast<uint32_t>(r.mParam);
        switch (r.mu32Type) {
        case 0:
            if (obj)
                if (FEScript *s = obj->FindScript(param)) { obj->SetCurrentScript(s); s->mi32Time = 0; }
            break;
        case 1: {
            FEObject *target = special(r.mu32Extra);
            if (r.mu32Extra != 0xFFFFFFFCu && r.mu32Extra != 0xFFFFFFFFu) target = pkg->FindObjectByGUID(r.mu32Extra);
            QueueMessage(param, obj, pkg, target, arg);
            break;
        }
        case 2: QueueMessage(param, obj, pkg, special(0xFFFFFFFFu), arg); break;
        case 3: QueueMessage(param, obj, pkg, special(0xFFFFFFFBu), arg); break;
        case 0x100: {
            FEObject *b = param ? pkg->FindObjectByGUID(param) : nullptr;
            if (b || !param) pkg->SetCurrentButton(this, b, b != nullptr);
            break;
        }
        case 0x101: mbFlag01 = param == 1; break;
        case 0x102:
            RecordLastPackageButton(pkg->mu32NameHash, pkg->mpCurrentButton ? pkg->mpCurrentButton->mu32GUID : 0);
            break;
        case 0x103: {
            FEObject *b = nullptr;
            if (uint32_t guid = RecallLastPackageButton(pkg->mu32NameHash)) b = pkg->FindObjectByGUID(guid);
            if (!b && param) b = pkg->FindObjectByGUID(param);
            if (b || !param) pkg->SetCurrentButton(this, b, b != nullptr);
            break;
        }
        case 0x105: QueuePackageUserTransfer(pkg, true, arg); break;
        case 0x106: QueuePackageUserTransfer(pkg, true, 0xFF); break;
        case 0x107: QueuePackageUserTransfer(pkg, false, arg); break;
        case 0x108: QueuePackageUserTransfer(pkg, false, 0xFF); break;
        case 0x200: QueuePackageSwitch(reinterpret_cast<const char *>(r.mParam), pkg); break;
        case 0x201: QueuePackagePush(reinterpret_cast<const char *>(r.mParam), pkg, pkg->mu32ControlMask); break;
        case 0x202:
            for (uint32_t bit = 0; bit < 8; ++bit)
                if (arg & (1u << bit)) QueuePackagePush(reinterpret_cast<const char *>(r.mParam), pkg, 1u << bit);
            break;
        case 0x203: QueuePackagePop(pkg); break;
        case 0x204: QueuePackagePush(reinterpret_cast<const char *>(r.mParam), pkg, 0); break;
        case 0x2C0: RecordPackageMarker(pkg->mpcName); break;
        case 0x2C1:                                       // RecallPackageMarker (inlined): pop the last name
            if (mu32NumPackageMarkers) {
                --mu32NumPackageMarkers;
                QueuePackageSwitch(macPackageMarkers[0] + mu32NumPackageMarkers * 0x100, pkg);
            }
            break;
        case 0x2C2: ClearPackageMarkers(); break;
        case 0x300:
            if (obj->mpCurrentScript->mu32Id != param) i = resp->FindConditionBranchTarget(i);
            break;
        case 0x301:
            if (obj->mpCurrentScript->mu32Id == param) i = resp->FindConditionBranchTarget(i);
            break;
        case 0x500: i = resp->FindConditionBranchTarget(i); break;
        default: break;
        }
    }
}

namespace {
// unlink without a membership check (PC: inlined FEMinList::RemNode), then delete through the vtable
void UnlinkAndDelete(FEMinList &l, FEMinNode *n)
{
    l.RemNode(n);
    bridge::VirtualScalarDelete(n);
}

// PC callback 0x79AAEC (slot 1 517EE0): back to script 0x1744B3 at time 0, all track flags dirty.
void ResetToInitialScript(FEObject *o)
{
    FEScript *s = o->FindScript(0x1744B3);
    o->SetCurrentScript(s);
    s->mi32Time = 0;   // PC: no null check
    o->mu32Flags |= 0x3C00000;
}
template <typename F> void ForEachObject(FEMinNode *head, F f)
{
    for (FEMinNode *n = head; n; n = n->next) {
        auto *o = reinterpret_cast<FEObject *>(n);
        f(o);
        if (o->mu32Type == 5) ForEachObject(reinterpret_cast<FEGroup *>(o)->mChildren.head, f);
    }
}

bool IsSpecialTarget(const FEObject *t)
{
    const auto v = static_cast<int32_t>(reinterpret_cast<intptr_t>(t));
    return v == 0 || (v >= -6 && v <= -1);
}
}  // namespace

void FEngine::AddPackageByPriority(FEPackage *pkg)
{
    FEMinList &l = ActiveList();
    auto *after = reinterpret_cast<FEPackage *>(l.tail);
    while (after && after->mi32Priority > pkg->mi32Priority) after = reinterpret_cast<FEPackage *>(after->node.prev);
    l.AddNode(after ? &after->node : nullptr, &pkg->node);
}

void FEngine::AddToIdleList(FEPackage *pkg) { mIdlePackages.AddTail(&pkg->node); }

void FEngine::RemoveFromLibraryList(FEPackage *pkg) { mLibraries.RemNode(&pkg->node); }

FEPackage *FEngine::PushPackage(const char *name, uint8_t priority, uint32_t mask)
{
    auto *pkg = reinterpret_cast<FEPackage *>(FEListFindNode(name, reinterpret_cast<FENode *>(mIdlePackages.head)));
    if (!pkg) {   // try the base name (after the last '/' or '\\'; signed char length like the original)
        int8_t i = static_cast<int8_t>(std::strlen(name));
        const char *p = name + i - 1;
        for (;;) {   // PC 5558B0..5558D7 (may read the byte before the string, like the original)
            if (*p == '/' || *p == '\\') {
                if (i != 0) ++p;
                break;
            }
            if (i <= 0) {
                ++p;
                break;
            }
            --p;
            --i;
        }
        pkg = reinterpret_cast<FEPackage *>(FEListFindNode(p, reinterpret_cast<FENode *>(mIdlePackages.head)));
    }
    if (pkg) {
        pkg->_unknown15[0] = 1;   // +0x15
        ForEachObject(pkg->mObjects.head, ResetToInitialScript);
        mIdlePackages.RemNode(&pkg->node);
    } else {
        void *buffer;
        uint8_t freeBuffer;
        const void *data = bridge::FEGameInterface_GetPackageData(mpUIData, name, &buffer, &freeBuffer);
        if (!data) return nullptr;
        pkg = FEngine_LoadPackage(this, data, 0);
        if (freeBuffer) bridge::FreeArray(buffer);
        if (!pkg) return nullptr;
    }
    pkg->SetControlMask(mask);
    pkg->mi32Priority = priority;
    pkg->mbFastInterp = _unknown000;
    if (mpUIData) bridge::FEGameInterface_LibraryLoaded(mpUIData, pkg);   // interface +0xC: package activated
    AddPackageByPriority(pkg);
    return pkg;
}

bool FEngine::UnloadPackage(FEPackage *pkg)
{
    FEMinList &active = ActiveList();
    FEMinNode *n = active.head;
    while (n && n != &pkg->node) n = n->next;
    if (!n) return false;
    const bool remove = mpUIData ? bridge::FEGameInterface_PackageWillUnload(mpUIData, pkg) : true;
    active.RemNode(&pkg->node);
    for (FEMinNode *c = mPackageCommands.head, *next; c; c = next) {
        next = c->next;
        if (reinterpret_cast<FEPackageCommand *>(c)->mpPackage == pkg) UnlinkAndDelete(mPackageCommands, c);
    }
    for (FEMinNode *m = mMessages.head, *next; m; m = next) {
        next = m->next;
        auto *msg = reinterpret_cast<FEMessageNode *>(m);
        bool drop = msg->mpPackage == pkg;   // "Removing message from package" (debug print stub)
        if (!drop && !IsSpecialTarget(msg->mpTarget)) drop = pkg->FindObjectByGUID(msg->mpTarget->mu32GUID) != nullptr;
        if (drop) UnlinkAndDelete(mMessages, m);
    }
    if (pkg->_unknown15[0]) {   // +0x15: keep it loaded
        AddToIdleList(pkg);
        return true;
    }
    for (FEMinNode *r = pkg->mList74.head; r; r = r->next) {
        FEPackage *lib = FEngine_FindLibraryPackage(this, reinterpret_cast<FENode *>(r)->mu32NameHash);
        if (!lib) continue;
        if (lib->mi32Priority - 1 > 0) lib->mi32Priority -= 1;   // library reference count
        else UnloadLibraryPackage(lib);
    }
    if (mpUIData) bridge::FEGameInterface_ReleaseResources(mpUIData, pkg, pkg->mu32NumResources, pkg->mpResources);
    if (remove) bridge::VirtualScalarDelete(pkg);
    return true;
}

void FEngine::UnloadLibraryPackage(FEPackage *pkg)
{
    if (!bridge::FEGameInterface_CanUnloadLibrary(mpUIData, pkg)) return;   // PC: no null check here
    RemoveFromLibraryList(pkg);
    const bool remove = mpUIData ? bridge::FEGameInterface_PackageWillUnload(mpUIData, pkg) : true;
    if (mpUIData) bridge::FEGameInterface_ReleaseResources(mpUIData, pkg, pkg->mu32NumResources, pkg->mpResources);
    if (remove && pkg) bridge::VirtualScalarDelete(pkg);
}
