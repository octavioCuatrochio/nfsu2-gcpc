#include "FEngine.h"
#include "bridge/Bridge.h"

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
