#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "frontend/FERender.h"
#include "frontend/cFEng.h"
struct LanguageStringRecord;
namespace teststub {
extern bool useOldDirtyFlag;   // value returned by bridge::UseOldFEngDirtyFlag()
extern int liveAllocations;    // bridge::Alloc minus bridge::FreeArray
extern int feStringVTable;     // its address stands in for the FEString vtable
extern void *lastLabelCallback;
extern int feMinNodeVTable, feMinListVTable, virtualDeletes;
extern int feFieldNodeVTable, feTypeNodeVTable;
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
extern int32_t profileState, gameModeFlags, realTimer;
extern uint8_t textInputFlag, eamIconState, resetMasksFlag;
extern uint32_t textInputHandle;
extern bool helpBarHandles;
extern bool showEAMIcons, callingScreenFrozen;
extern char callingScreen[64];
extern const LanguageStringRecord *languageTable;   // LanguageStrings (native builds)
extern uint32_t languageCount;
extern int32_t tuningUnitType, tuningUnavailableType;
extern long double tuningMin, tuningMax, tuningValue, tuningUpper, tuningLower;
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
extern int fakeFont;
extern int32_t joyPluggedPort;
extern int32_t tuningCategory, allPortsFlag, activePort, lastButton;   // TuningSlidersMenu globals
extern bool speedMetric, hasLastButton, drawFEng, cursorEnabled;
extern int32_t playerToEdit;
extern uint32_t chosenBrands[8];
extern uint8_t audioSettings[0x40];
extern uint8_t gameBytes[0x100];
extern void *jumpCamObject;
extern int32_t optionsPage;
extern bool wheelPlugged;
extern bool autoSaveAllowed;
extern bool browsingParts;
extern const uint8_t *perfPackage, *perfPart;
extern uint8_t controllerOptions[0x28 * 2];
extern int32_t discError;
extern const char *singlePackage;
extern uint8_t mouseGlobals[0x24];
extern int32_t language;
extern uint8_t fontMetrics[48];
extern const void *realFontData;
extern int giVTable, initializerVTable, movieStarterVTable, movieStopperVTable, dirtyMakerVTable;
extern int32_t insideScreenCtor, gameFlowState;
extern float worldTime, realTime;
extern uint32_t lastJoyTypes[2];
extern uint8_t joyTypes[2];
extern void *memoryCard;
extern uint8_t moviePlayer[0x10];
extern uint8_t movieTexture[0x50];
extern int views[4];
extern const int32_t *renderStatsMode;
extern uint32_t renderStats[4];
extern uint8_t joyPortConfig[8];
extern std::string lastJoyTextureName;
extern uint32_t lastTextureHash;
extern int feWidgetVTable, feButtonWidgetVTable;
extern bool hitResult;
extern std::vector<void *> renderedObjects;   // objects of the last FEGameInterface_RenderObjectList, in order
extern float viewMatrix[16];
extern FESortGlobals sortGlobals;
extern int16_t mouseInfo[4];
   // FEGameInterface_GetPackageData result
extern void *screenResult;   // ScreenFactory result
extern bool hotChunking;
extern int renderDisconnectVTable, renderDisconnects, registerJoys;   // registerJoys: Init/CloseDebugLanguageChange balance
extern FEngJoyMapEntry joyMap[];
extern uint32_t joyRegistered;
extern bool joyPlugged;
extern ::SlotPool *levelSorterSlots;                               // FEngine_PushPackage return value
extern void *currentButton;
struct TestCallback { void *vtable; std::vector<void *> visited; void *stopAt; };   // stub FEObjectCallback
extern void *lastTypeUpdate;
extern FEMinList keyNodePool;
extern uint32_t nextGUID;
extern void *lastDestructorCallback;
}  // namespace teststub
