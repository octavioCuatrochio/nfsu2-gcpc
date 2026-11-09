// Bridge stand-ins for native unit tests: plain heap, no character histogram, settable dirty flag.
#include "bridge/Bridge.h"
#include "bridge_stub.h"
#include "frontend/FEObject.h"
#include "frontend/FEMultiPool.h"
#include "frontend/FEPool.h"
#include "frontend/FEMessageResponse.h"
#include <new>
#include <cstdlib>
#include <cstring>

namespace teststub {
bool useOldDirtyFlag = false;
int liveAllocations = 0;
int feStringVTable = 0;
void *lastLabelCallback = nullptr;
int feMinNodeVTable = 0, feMinListVTable = 0, virtualDeletes = 0;
int feObjectVTable = 0, feSlotNodeVTable = 0, feSlotPoolVTable = 0;
FEMultiPool objDataPool{{&feMinListVTable, 0, nullptr, nullptr}};
int feMsgRespVTable = 0, feMsgRespBlockVTable = 0, virtualDtorCalls = 0;
int feScriptBlockVTable = 0, feScriptVTable = 0, feRefListVTable = 0;
FEMinList msgRespPool{&feMinListVTable, 0, nullptr, nullptr}, scriptPool{&feMinListVTable, 0, nullptr, nullptr};
uint32_t nextGUID = 100;
void *lastDestructorCallback = nullptr;
}  // namespace teststub

namespace bridge {
void *Alloc(uint32_t bytes) { ++teststub::liveAllocations; return std::malloc(bytes); }
void FreeArray(void *p) { if (p) --teststub::liveAllocations; std::free(p); }
void *WideCharHistogram() { return nullptr; }
void WideCharHistogram_UnpackString(void *, uint16_t *, int, const char *) { std::abort(); }
bool UseOldFEngDirtyFlag() { return teststub::useOldDirtyFlag; }
// stand-in FEObject: zero the base, mark it constructed
// stand-in deep copy: same header fields, own (empty) name, data and lists (via the struct: tests are 64-bit)
void FEObject_CopyConstruct(void *self, const void *src, bool)
{
    auto *o = static_cast<FEObject *>(self);
    std::memcpy(static_cast<void *>(o), src, sizeof(FEObject));
    o->mu32NameHash = 0;
    o->mpcName = nullptr;
    o->mpData = nullptr;
    o->mu32DataSize = 0;
    o->mResponses.count = 0; o->mResponses.head = o->mResponses.tail = nullptr;
    o->mScripts.count = 0; o->mScripts.head = o->mScripts.tail = nullptr;
}
void *FEStringVTable() { return &teststub::feStringVTable; }
void CallLabelCallback(void *s) { teststub::lastLabelCallback = s; }
void *FEMinNodeVTable() { return &teststub::feMinNodeVTable; }
void *FEMinListVTable() { return &teststub::feMinListVTable; }
// dispatch on the stand-in vtables, like the game's real vtable slot 0 would
void VirtualScalarDelete(void *o)
{
    ++teststub::virtualDeletes;
    void *vt = *static_cast<void **>(o);
    if (vt == &teststub::feSlotNodeVTable) static_cast<FESlotNode *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feSlotPoolVTable) static_cast<FESlotPool *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feMsgRespBlockVTable)
        FEMessageResponseBlock_ScalarDtor(static_cast<FEMessageResponseBlock *>(o), 1);
    else if (vt == &teststub::feScriptBlockVTable) FEScriptBlock_ScalarDtor(static_cast<FEScriptBlock *>(o), 1);
    else FreeArray(o);
}
void VirtualScalarDtor(void *o, uint32_t flags)
{
    if (*static_cast<void **>(o) == &teststub::feMinNodeVTable && flags == 0) { ++teststub::virtualDtorCalls; return; }
    if (flags & 1) VirtualScalarDelete(o); else ++teststub::virtualDtorCalls;
}
::FEMinList *FEMessageResponsePool() { return &teststub::msgRespPool; }
::FEMinList *FEScriptPool() { return &teststub::scriptPool; }
void *FEMessageResponseBlockVTable() { return &teststub::feMsgRespBlockVTable; }
void *FEScriptBlockVTable() { return &teststub::feScriptBlockVTable; }
void *FEScriptVTable() { return &teststub::feScriptVTable; }
void *FERefListVTable() { return &teststub::feRefListVTable; }
void *FEMessageResponseVTable() { return &teststub::feMsgRespVTable; }
void *FEObjectVTable() { return &teststub::feObjectVTable; }
uint32_t NextFEObjectGUID() { return teststub::nextGUID++; }
FEMultiPool *ObjDataPool() { return &teststub::objDataPool; }
void *FESlotNodeVTable() { return &teststub::feSlotNodeVTable; }
void *FESlotPoolVTable() { return &teststub::feSlotPoolVTable; }
void CallDestructorCallback(void *o) { teststub::lastDestructorCallback = o; }
}  // namespace bridge
