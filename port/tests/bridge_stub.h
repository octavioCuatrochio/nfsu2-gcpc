#pragma once
#include <cstdint>
#include <vector>
#include <string>
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
struct FEPackage;
struct SlotPool;
namespace teststub {
extern FEMinList msgRespPool, scriptPool;
extern int feMsgRespVTable, feMsgRespBlockVTable, virtualDtorCalls;
extern int feScriptBlockVTable, feScriptVTable, feRefListVTable, feKeyNodeVTable, keyNodeNews, feKeyNodeBlockVTable, linearCalls;
extern uint32_t holdDirtyFlags;
struct ScriptMessage { void *script; int32_t from, to; };
extern std::vector<ScriptMessage> messages;   // unused since IssueScriptMessages is rewritten
struct QueuedMessage { uint32_t msg; void *obj; void *target; };
extern std::vector<QueuedMessage> queued, forwarded;   // queued: SendMessageToGame calls; forwarded: UI hook
extern int feMessageNodeVTable, fePackageCommandVTable;
struct Delivered { const char *kind; void *a, *pkg; uint32_t msg, arg; };
extern std::vector<Delivered> delivered;
extern std::vector<std::string> levelSortingList;          // cFEng_FindInLevelSortingList (case-insensitive)
extern std::string lastPushName;                            // copy: the command (and its name) is freed
extern FEPackage *pushResult;
extern int32_t fengLoadCounter;
extern int feCodeListBoxVTable, feListBoxVTable, fePackageDataVTable, fePackageVTable, feNodeVTable, fePackageList74VTable, resourceConnectorVTable;
extern bool loadResourcesResult, readerChunkResult;
extern const void *libraryData;
extern bool willUnloadResult;
extern uint32_t padBits;
extern const float *contextTransform;   // FEGameInterface_GetContextTransform result (nullptr: fails)
struct HitTest { void *obj; float x, y; };
extern std::vector<void *> hitObjects;   // FEGameInterface_IsPointInObject: these are hit
extern std::vector<HitTest> hitTests;
extern bool textInputFocused;
extern int16_t mouseInfo[4];
   // FEGameInterface_GetPackageData result
extern void *screenResult;   // ScreenFactory result
extern bool hotChunking;
extern int renderDisconnectVTable, renderDisconnects, registerJoys;
extern ::SlotPool *levelSorterSlots;                               // FEngine_PushPackage return value
extern void *currentButton;
struct TestCallback { void *vtable; std::vector<void *> visited; void *stopAt; };   // stub FEObjectCallback
extern void *lastTypeUpdate;
extern FEMinList keyNodePool;
extern uint32_t nextGUID;
extern void *lastDestructorCallback;
}  // namespace teststub
