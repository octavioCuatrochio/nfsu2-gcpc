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
#include <cmath>
#include "bWare/bMath.h"

namespace teststub {
bool useOldDirtyFlag = false;
int liveAllocations = 0;
int feStringVTable = 0;
void *lastLabelCallback = nullptr;
int feMinNodeVTable = 0, feMinListVTable = 0, virtualDeletes = 0;
int feObjectVTable = 0, feSlotNodeVTable = 0, feSlotPoolVTable = 0;
FEMultiPool objDataPool{{&feMinListVTable, 0, nullptr, nullptr}};
int feMsgRespVTable = 0, feMsgRespBlockVTable = 0, virtualDtorCalls = 0;
int feScriptBlockVTable = 0, feScriptVTable = 0, feRefListVTable = 0, feKeyNodeVTable = 0, keyNodeNews = 0, feKeyNodeBlockVTable = 0, linearCalls = 0;
uint32_t holdDirtyFlags = 0;
std::vector<ScriptMessage> messages;
std::vector<QueuedMessage> queued, forwarded;
int feMessageNodeVTable = 0;
void *currentButton = nullptr;
void *lastTypeUpdate = nullptr;
FEMinList keyNodePool{&feMinListVTable, 0, nullptr, nullptr};
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
    else if (vt == &teststub::feScriptVTable) static_cast<FEScript *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feMsgRespVTable) static_cast<FEMessageResponse *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feObjectVTable) static_cast<FEObject *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feKeyNodeVTable) static_cast<FEKeyNode *>(o)->ScalarDeletingDestructor(1);
    else if (vt == &teststub::feKeyNodeBlockVTable) FEKeyNodeBlock_ScalarDtor(static_cast<FEKeyNodeBlock *>(o), 1);
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
void *FEKeyNodeVTable() { return &teststub::feKeyNodeVTable; }
::FEMinList *FEKeyNodePool() { ++teststub::keyNodeNews; return &teststub::keyNodePool; }
void *FEKeyNodeBlockVTable() { return &teststub::feKeyNodeBlockVTable; }
const bASinTableEntry *BASinTable()
{
#ifdef NFSU2_REFEREE
    return reinterpret_cast<const bASinTableEntry *>(0x7EA628);   // runs inside SPEED2.EXE
#else
    static bASinTableEntry t[12 * 16 + 1];                          // formula approximation (not the game's data)
    static bool init = false;
    if (!init) {
        for (int seg = 0, start = 0, step = 0x8000; seg < 12; ++seg, start += step, step >>= 1)
            for (int k = 0; k < 16; ++k) {
                double x = (start + (step >> 4) * k) / 65536.0;
                t[seg * 16 + k].angle = static_cast<uint16_t>(std::asin(x) * 65536.0 / 6.283185307179586 + 0.5);
                t[seg * 16 + k].slope = static_cast<float>(1.0 / (6.283185307179586 * std::sqrt(1.0 - x * x)));
            }
        init = true;
    }
    return t;
#endif
}
void *FEMessageNodeVTable() { return &teststub::feMessageNodeVTable; }
void FEUIData_OnQueueMessage(void *, uint32_t msg, void *obj, void *, void *target, uint32_t) { teststub::forwarded.push_back({msg, obj, target}); }
void FEngine_SendMessageToGame(void *, uint32_t msg, void *obj, void *) { teststub::queued.push_back({msg, obj, reinterpret_cast<void *>(-1)}); }
bool FEObjectCallback_Call(void *cb, void *obj)
{
#ifdef NFSU2_REFEREE
    return (*reinterpret_cast<bool(__attribute__((thiscall)) ***)(void *, void *)>(cb))[1](cb, obj);
#endif
    auto *t = static_cast<teststub::TestCallback *>(cb);
    t->visited.push_back(obj);
    return obj != t->stopAt;
}
void FEListBox_Update(void *lb, float) { teststub::lastTypeUpdate = lb; }
void FECodeListBox_Update(void *lb, float) { teststub::lastTypeUpdate = lb; }
uint32_t HoldDirtyFlags()
{
#ifdef NFSU2_REFEREE
    return *reinterpret_cast<const uint32_t *>(0x7F7C8C);
#else
    return teststub::holdDirtyFlags;
#endif
}
void *FERefListVTable() { return &teststub::feRefListVTable; }
void *FEMessageResponseVTable() { return &teststub::feMsgRespVTable; }
void *FEObjectVTable() { return &teststub::feObjectVTable; }
uint32_t NextFEObjectGUID() { return teststub::nextGUID++; }
FEMultiPool *ObjDataPool() { return &teststub::objDataPool; }
void *FESlotNodeVTable() { return &teststub::feSlotNodeVTable; }
void *FESlotPoolVTable() { return &teststub::feSlotPoolVTable; }
void CallDestructorCallback(void *o) { teststub::lastDestructorCallback = o; }
}  // namespace bridge
