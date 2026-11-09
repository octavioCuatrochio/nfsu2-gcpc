#pragma once
#include <cstdint>
#include <vector>
namespace teststub {
extern bool useOldDirtyFlag;   // value returned by bridge::UseOldFEngDirtyFlag()
extern int liveAllocations;    // bridge::Alloc minus bridge::FreeArray
extern int feStringVTable;     // its address stands in for the FEString vtable
extern void *lastLabelCallback;
extern int feMinNodeVTable, feMinListVTable, virtualDeletes;
extern int feObjectVTable, feSlotNodeVTable, feSlotPoolVTable;
}
struct FEMultiPool;
namespace teststub {
extern FEMultiPool objDataPool;
}
struct FEMinList;
namespace teststub {
extern FEMinList msgRespPool, scriptPool;
extern int feMsgRespVTable, feMsgRespBlockVTable, virtualDtorCalls;
extern int feScriptBlockVTable, feScriptVTable, feRefListVTable, feKeyNodeVTable, keyNodeNews, feKeyNodeBlockVTable, linearCalls;
extern uint32_t holdDirtyFlags;
struct ScriptMessage { void *script; int32_t from, to; };
extern std::vector<ScriptMessage> messages;
extern void *lastTypeUpdate;
extern FEMinList keyNodePool;
extern uint32_t nextGUID;
extern void *lastDestructorCallback;
}  // namespace teststub
