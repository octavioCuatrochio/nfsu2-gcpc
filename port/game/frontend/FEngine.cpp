#include "FEngine.h"
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

// GC 801484AC: the package's own response to the message, if any.
void FEngine::ProcessGlobalMessage(FEPackage *pkg, uint32_t msg, uint32_t arg)
{
    if (FEMessageResponse *r = pkg->FindResponse(msg)) bridge::FEngine_ProcessResponses(this, r, nullptr, pkg, arg);
}

namespace {
FEPackage *NextPackage(const FEPackage *p) { return reinterpret_cast<FEPackage *>(reinterpret_cast<const FEMinNode *>(p)->next); }
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
    bridge::FEListBox_ScrollSelection(listBox, dx, dy);
    return true;
}

bool FEngine::ProcessCodeListBoxResponses(FEObject *listBox, FEPackage *, uint32_t msg)
{
    int32_t dx, dy;
    if (!ScrollDelta(msg, dx, dy)) return false;
    bridge::FECodeListBox_ScrollSelection(listBox, dx, dy);
    return true;
}

void FEngine::ProcessObjectMessage(FEObject *obj, FEPackage *pkg, uint32_t msg, uint32_t arg)
{
    if (obj->mu32Type == 4 && ProcessListBoxResponses(obj, pkg, msg)) return;
    if (obj->mu32Type == 6 && ProcessCodeListBoxResponses(obj, pkg, msg)) return;
    if (FEMessageResponse *r = obj->FindResponse(msg)) bridge::FEngine_ProcessResponses(this, r, obj, pkg, arg);
}
