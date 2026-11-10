// Bridge: everything rewritten game code needs from parts of the game that are NOT rewritten yet.
//
// Rewritten code calls only these functions for such things, never addresses or platform APIs directly.
// Implementations:
//   host/win32-hook/bridge_win32.cpp  -> calls the original SPEED2.EXE functions (hooked build)
//   tests/bridge_stub.cpp             -> simple stand-ins for native unit tests
//   (later) the native port          -> rewritten implementations
// When the code behind an entry gets rewritten, the entry is removed and callers use the rewrite directly.
#pragma once
#include <cstdint>

struct FEMultiPool;   // frontend/FEMultiPool.h
struct FEMinList;     // frontend/FEMinList.h
struct FEPackage;     // frontend/FEPackage.h
struct cFEng;         // frontend/cFEng.h
struct SlotPoolManager;   // bWare/bSlotPool.h
struct SlotPool;
struct FEPackageManager;   // frontend/FEPackageManager.h
struct FEObject;
struct LanguageStringRecord;

struct bASinTableEntry;
struct FESortGlobals;
struct FEngFont;
struct FEngine;
struct FERenderBucket;
struct ePoly;
struct cFEngRender;
struct cFEngRenderContext;
struct FERenderObject;
struct bMatrix4;
struct FEngJoyMapEntry;
namespace bridge {

// --- memory: buffers are shared with original game code, so they must come from the game's heap ---
void *Alloc(uint32_t bytes);           // PC 0x575620 -> CRT operator new (NOT zeroed)
void FreeArray(void *p);               // PC 0x575630 -> CRT operator delete[] (null is fine)

// --- frontend text ---
// Global character histogram used to unpack the game's compressed (packed) 8-bit strings into wide
// strings. Null until language data is loaded.            PC: pWideCharHistogram @ 0x8383DC
void *WideCharHistogram();
// WideCharHistogram::UnpackString(dst, dstBytes, packed)  PC 0x50D3C0 (__thiscall, ret 0xC)
void WideCharHistogram_UnpackString(void *histogram, uint16_t *dst, int dstBytes, const char *packed);
// FEng global UseOldFEngDirtyFlag: when set, string changes always mark objects dirty.  PC @ 0x8384D4
bool UseOldFEngDirtyFlag();

// --- FEng objects not rewritten yet ---
void *FEStringVTable();                                                // PC 0x78F53C (hooked: the game's vtable)
// FEString::pLabelCallback (PC 0x838478): if set, calls its vtable[0](callback, string)
void CallLabelCallback(void *feString);

void *FEMinNodeVTable();                                               // PC 0x79A9E8
void *FEFieldNodeVTable();                                             // PC 0x79B018
void *FETypeNodeVTable();                                              // PC 0x79B90C
void *FEMinListVTable();                                               // PC 0x79A9EC
// delete an object through its own vtable: slot 0 = MSVC scalar deleting destructor, called with flag 1
void VirtualScalarDelete(void *object);
void VirtualScalarDtor(void *object, uint32_t flags);                 // vtable slot 0 with explicit flags

void *FEObjectVTable();                                                // PC 0x79B8E0
uint32_t NextFEObjectGUID();                                           // FEngine::SysGUID++ (PC 0x7F7CF8)
::FEMultiPool *ObjDataPool();                                            // global FEMultiPool, PC 0x863770
void *FESlotNodeVTable();                                              // PC 0x79B13C
void *FESlotPoolVTable();                                              // PC 0x79B8DC
// FEObject::pDestructorCallback (PC 0x838474): if set, calls its vtable[0](callback, object)
void CallDestructorCallback(void *object);

// object pools (FEPool.h): pool lists and block vtables
::FEMinList *FEMessageResponsePool();                             // PC 0x8637A0
::FEMinList *FEScriptPool();                                      // PC 0x839C30
void *FEMessageResponseBlockVTable();                                  // PC 0x79BF50 (slot 0 = 53EB20)
void *FEScriptBlockVTable();                                           // PC 0x79BF54 (slot 0 = 53EC20)
void *FEScriptVTable();                                                // PC 0x79AD28 (slot 0 = 53EE30)
void *FEKeyNodeVTable();                                               // PC 0x79AA34
::FEMinList *FEKeyNodePool();                                          // PC 0x839BD0
void *FEKeyNodeBlockVTable();                                          // PC 0x79B9EC (slot 0 = 531BC0)
void *bWareMalloc(uint32_t size, const char *name, int32_t line, uint32_t flags);   // PC 0x440390 (__cdecl)
::SlotPoolManager *TheSlotPoolManager();                                   // PC 0x828530 (game global)
::SlotPool **BPNodeSlotPool();          // PC 0x8284F0 (game global)
int32_t *BPListWantToClose();          // PC 0x8284F4 (game global)
::FEngJoyMapEntry *FEngJoyMap();                 // PC 0x7F91C8 (21 entries)
uint32_t *FEngJoyRegistered();                   // PC 0x8384B8
void *FEngHandleJoyFunction();                   // handler passed to AddJoyHandlerToAllPorts (PC 0x50C630, rewritten)
void AddJoyHandlerToAllPorts(uint32_t event, void *handler, void *user);   // PC 5794B0 (__cdecl)
void RemoveJoyHandlerFromAllPorts(uint32_t event);                         // PC 57D590 (__cdecl)
void InitDebugLanguageChange();                  // PC: AddJoyHandler 579410(0, 0x34, 0x527520, 0)
void CloseDebugLanguageChange();                 // PC: RemoveJoyHandler 57C330(0, 0x34)
int32_t IsLiveJoystickPluggedIn(int32_t port);   // PC 571A80 (__cdecl)
void *RenderObjectDisconnectVTable();                                              // PC 0x79AB9C
void bWareFree(void *p);
int32_t FEngLoadCounter();                                                         // PC 0x8384D0 (game global)
int32_t *FEngLoadCounterPtr();                                                     // same global (GC gInsideScreenConstructor)
// FEPackageManager callees (game-side screens; PC __cdecl)
// FEPackageReader chunk readers not rewritten yet (PC __thiscall on the reader, bool in AL)
// FEGameInterface (engine +0x10C) vtable slot 0: package data by name; *freeBuffer -> caller frees *buffer
const void *FEGameInterface_GetPackageData(void *gi, const char *name, void **buffer, uint8_t *freeBuffer);
void FEGameInterface_LibraryLoaded(void *gi, ::FEPackage *pkg);   // vtable slot 3 (+0xC): library / package activated
void FEGameInterface_ReleaseResources(void *gi, ::FEPackage *pkg, uint32_t count, void *resources);   // slot 2 (+0x8)
bool FEGameInterface_PackageWillUnload(void *gi, ::FEPackage *pkg);   // slot 4 (+0x10): true -> delete it
bool FEGameInterface_CanUnloadLibrary(void *gi, ::FEPackage *pkg);    // slot 5 (+0x14)
uint32_t FEGameInterface_GetJoyPadBits(void *gi, uint32_t pad);   // +0x40 (one argument)
void FEGameInterface_GetMouseInfo(void *gi, void *info);              // +0x44 (8 bytes: x, y, wheel, buttons)
void FEGameInterface_BeginUpdate(void *gi);                           // +0x5C
void FEGameInterface_EndUpdate(void *gi);                             // +0x60
uint8_t FEGameInterface_GetContextTransform(void *gi, uint32_t context, float *matrix16);   // +0x2C (GC +0x64), bool in AL
uint8_t FEGameInterface_IsPointInObject(void *gi, float x, float y, ::FEObject *obj);   // +0x48 (PC; GC +0x9C), bool in AL
// game interface render slots (PC cFEng vtable 0x79BCD8)
void FEGameInterface_BeginPackageRendering(void *gi, ::FEPackage *pkg, uint32_t flag);       // +0x20
void FEGameInterface_EndPackageRendering(void *gi, ::FEPackage *pkg);                        // +0x24
void FEGameInterface_GenerateRenderContext(void *gi, uint32_t context, ::FEObject *group);   // +0x28
void FEGameInterface_RenderObjectList(void *gi, void *entries, uint32_t count);              // +0x30 ({obj, key} x count)
// --- cFEngRender (engine side; see frontend/cFEngRender.h) ---
::SlotPool **FERenderObjectPool();               // PC 0x83847C
::SlotPool **FERenderEPolyPool();                // PC 0x838480
::SlotPool **FERenderEPolyOverflowPool();        // PC 0x838484 (created on demand, deleted when empty)
::FERenderBucket *FERenderBuckets();             // PC 0x8639A0 (256)
void *eGetView(int32_t index);                   // PC 48B1E0: 0x832DE0 + 0x70 * index
void eView_Render(void *view, ::ePoly *poly, void *texture, int32_t flag);   // PC 5C7EA0 (__thiscall, ret 0xC)
const int32_t *FERenderStatsMode();              // PC *(0x8900D8): stats counted when it points at 4
uint32_t *FERenderStats();                       // PC 0x837D20: objects, cached, cached+built, clipped
void *cFEngRenderVTable();                       // PC 0x79BBA0
int32_t CurrentLanguage();                       // PC 0x7F70D0 (GetCurrentLanguage inlined)
int32_t MovieActive();                           // PC 0x8383AC
const uint8_t *MoviePlayer();                    // PC *(0x8383A4): +0x0C byte = flip vertically
void *MovieTexture();                            // PC 0x8385C0 (TextureInfo of the playing movie)
// PC 51C110 (FEngFont::RenderString, __thiscall, ret 0x18)
void FEngFont_RenderString(void *font, const int32_t *colour, const uint16_t *text, ::FEObject *obj, const ::bMatrix4 *m,
                           ::FERenderObject *cached, void *renderInfo);
// cFEng unit
// FEngFont unit
struct FEngFontList { ::FEngFont *mpHead, *mpTail; };
FEngFontList *FontList();                        // PC 0x863768
const uint32_t *FontAlias();                     // PC 0x7F9184: {hash, alias}
const uint8_t *FontMetricsTable();               // PC 0x7F918C: 4 x {hash, baseline, leading scale}
const char *LanguageSuffix(int32_t lang);         // PC table 0x7F7020 (11 x 16 {id, name, ...}); else "UNKNOWN" (0x79AA54)
void *RealFont_Create(const void *data);         // PC 6F4690 (RealFontOld::Font::Create)
void RealFont_Destroy(void *font);               // PC 6F46C0
void *FixupTextureInfoNull(void *tex, uint32_t hash, uint32_t a, uint32_t b);   // PC 4902D0
void *FEGameInterfaceVTable();                   // PC 0x79BCD8 (cFEng)
void *FEObjectInitializerVTable();               // PC 0x79AAEC
void *FEngMovieStarterVTable();                  // PC 0x79AB84
void *FEngMovieStopperVTable();                  // PC 0x79AB94
void *FEDirtyMakerVTable();                      // PC 0x79ABF0
::SlotPool **LevelSorterSlotsPtr();              // PC 0x8384B0
int32_t InsideScreenConstructor();               // PC 0x8384D0 gInsideScreenConstructor
int32_t GameFlowState();                         // PC 0x8654A4 (6 = racing)
float WorldTimeElapsed();                        // PC 0x865154
float RealTimeElapsed();                         // PC 0x865198
uint32_t *LastKnownJoystickTypes();              // PC 0x7F9414 (2 ports)
uint8_t GetJoystickType(int32_t port);           // PC 571AC0 (byte table 0x864788)
void *MemoryCardInstance();                      // PC 0x836498 MemoryCard::s_pThis
void MemoryCard_Tick(void *card, int32_t ms);    // PC 4F4A20 (__thiscall)
void MovieUpdate();                              // PC 5660F0
void MovieStop();                                // PC 511BC0
int32_t *SummonChyron();                         // PC 0x8384BC
int32_t *StopChyron();                           // PC 0x8384C0
void FakeCallFromSound();                        // PC 4ACA40
void CurrentSongStopped(void *song);             // PC 4AC9B0
int32_t GetMovieNameEnum(const char *name);      // PC 51C580
void CalculateMovieFilename(char *out, int32_t size, const char *name);   // PC 51C740
bool FILE_exists(const char *path);              // PC 6F909D
void MovieStart(const char *path, bool loop, int32_t which);              // PC 566020
void FEngine_InitObjects(::FEngine *engine);     // PC 51A7E0 (FEngine, not rewritten)
int32_t *DiscErrorOccurred();                    // PC 0x864F2C g_discErrorOccured
const char **FEngPleaseRenderSinglePackage();    // PC 0x8384B4
const uint8_t *MouseGlobals();                   // PC 0x8763D4: +0 x, +4 y (int), +0x1C left-handed, +0x1D / +0x20 buttons
void FEGameInterface_RenderObject(void *gi, ::FEObject *obj);                                // +0x34
void cFEngRender_GenerateRenderContext(void *render, uint32_t context, ::FEObject *group);  // PC 51B7F0 (__thiscall, ret 8)
void cFEngRender_AddToRenderList(void *render, ::FEObject *obj);                           // PC 51B850 (ret 4)
void cFEngRender_PackageFinished(void *render, ::FEPackage *pkg);                          // PC 5371F0 (ret 4)
void FEPackageManager_NotifySoundMessage(::FEPackageManager *m, uint32_t a, uint32_t b, uint32_t c, uint32_t d);   // PC 52CC90 (ret 0x10)
void FEGameInterface_GetViewTransformation(void *gi, void *matrix);                          // +0x3C (FEMatrix4)
::FESortGlobals *FEngSortGlobals();   // PC 0x838488
uint32_t FEngHashStringUpper(const char *fmt);   // PC 50B9C0 (__cdecl, varargs): bVSPrintf into 0x100 bytes, then FEHashUpper
// FEngFont (not rewritten): FindFont PC 51BD70 (__cdecl hash); members __thiscall, results in st0.
void *FEngFindFont(uint32_t hash);
long double FEngFont_GetTextWidth(void *font, const uint16_t *text, uint32_t flags);     // PC 50BEC0 (ret 8)
long double FEngFont_GetTextHeight(void *font, const uint16_t *text, uint32_t arg);      // PC 50BF40 (ret 8)
long double FEngFont_CalculateXOffset(void *font, uint32_t justify, float width);       // PC 50C020 (ret 8)
long double FEngFont_CalculateYOffset(void *font, uint32_t justify, float height);      // PC 50C050 (ret 8)
bool FEngGetLocalizedString(uint16_t *buffer, int32_t bytes, uint32_t labelHash);        // PC 512570 (__cdecl)
void *FEWidgetVTable();                          // PC 0x790B98
void *FEButtonWidgetVTable();                    // PC 0x79B2F8
void *FELabelWidgetVTable();                     // PC 0x79B338
void *FEStatWidgetVTable();                      // PC 0x79B378
void *FESliderWidgetVTable();                    // PC 0x79B400
void *FEToggleWidgetVTable();                    // PC 0x79B3B8
void *MenuScreenVTable();                        // PC 0x79AC00
int32_t ProfileState();                           // PC *(int*)0x8500D0 (>= 0xC: profile loaded)
int32_t GameModeFlags();                          // PC *(int*)0x850078 (0x200 / 0x20 / 0x100: no EAM icons)
const char *ProfileName(bool alternate);          // PC 0x850128 (alternate, mode 0x200) / 0x8500EC
void SetShowEAMIcons(bool on);                    // PC byte 0x838528
bool CallingScreenFrozen();                       // PC byte 0x838520
char *CallingScreenPackageName();                 // PC char[] 0x8384E0 (gCallingScreenPackageName)
void FEngPrintfString(const char *pkg, uint32_t objHash, const char *fmt, const char *s);   // PC 537B80 (varargs)
void HelpBar_SyncByMask(uint32_t mask, const char *pkg);   // PC 54E6E0 __thiscall on *(0x8384D8) (PCHelpBarFNGObject)
void *TuningSliderVTable();                      // PC 0x79B7B0
void *ThreeStageSliderVTable();                  // PC 0x79AE54
void *cSliderVTable();                           // PC 0x78F544
const char *GameEmptyString();                   // PC 0x783FE5 ("")
// cSlider virtuals (thiscall through the object's vtable): +0x0C Toggle(bool) 512240, +0x10 Highlight 526D90,
// +0x14 UnHighlight 526F80, +0x18 Draw 4FF530.
void cSlider_Toggle(void *slider, bool on);
void cSlider_Highlight(void *slider);
void cSlider_UnHighlight(void *slider);
void cSlider_Draw(void *slider);
void cSlider_DrawVertical(void *slider);     // vtable +0x1C
void cSlider_DrawHorizontal(void *slider);   // vtable +0x20
void FEPrintfStrings(FEObject *str, const char *fmt, const char *a, const char *b);   // PC 537BE0 (varargs)
void FEPrintfDouble(FEObject *str, const char *fmt, double v);                         // PC 537BE0 (varargs)
const LanguageStringRecord *LanguageStrings(uint32_t *count);   // PC count *(0x8383D0), table *(0x8383D8)
const char *TuningMasterScreenName();                           // PC *(0x7FA214) (GC gTuningMasterScreenName)
// PC car tuning object *(0x83A540) (GC: CarTuneManager / CustomPhysicsSpec calls). All __thiscall on it.
int32_t TuningUnitType();                                       // *(*(0x83A540) + 4) (GC eUnitType, 1 = metric)
bool Tuning_IsAvailable(int32_t type);                          // 59B080 (type)
long double Tuning_GetMin(int32_t type);                        // 5B0BD0 (type, 0, 1)
long double Tuning_GetMax(int32_t type);                        // 5B0C00 (type, 0, 1)
long double Tuning_GetValue(int32_t type);                      // 5B3E40 (type, 0, 1)
long double Tuning_GetUpper(int32_t type);                      // 5B2BD0 (type, 0, 1): ThreeStageSlider upper stage
long double Tuning_GetLower(int32_t type);                      // 5B2BA0 (type, 0, 1): lower stage
void Tuning_SetValue(float v, int32_t type);                    // 5B4670 (v, type, 0, 1)
long double CarTuneManager_GetStep(int32_t type);               // 522560 (__thiscall on 0x839C40)
bool cSlider_Update(void *slider, uint32_t msg);                // slider vtable +0x04
void FEWidget_Draw(void *widget);                               // widget vtable +0x0C
void FEWidget_Position(void *widget);                           // widget vtable +0x10
void FEWidget_Show(void *widget);                               // widget vtable +0x14
void FEWidget_Hide(void *widget);                               // widget vtable +0x18
void FEWidget_SetFocus(void *widget, const char *pkg);          // widget vtable +0x24
void FEWidget_UnsetFocus(void *widget);                         // widget vtable +0x28
void FEWidget_SetPosY(void *widget, float y);                   // widget vtable +0x34
void *UIWidgetMenuVTable();                                     // PC 0x795CDC
void FEPrintfInt(FEObject *str, const char *fmt, int32_t v);   // PC 537BE0 (__cdecl varargs): format into a string object
uint32_t FEWidget_DetermineSoundTrigger(void *widget);             // widget vtable +0x3C
void FEWidget_Act(void *widget, const char *pkg, uint32_t msg);   // widget vtable +0x04
uint32_t FEWidget_DetermineSpecialSound(void *widget, uint32_t msg, uint32_t event);   // widget vtable +0x38
void FEWidget_BlinkArrows(void *widget, uint32_t msg);             // toggle widget vtable +0x40
// GC MakeFrontendSound (PC inlined): idx = (int8) table 0x837FE0[event * 0x1B + trigger]; != -1 -> PlayUISoundFX 458520
void MakeFrontendSound(uint32_t event, uint32_t trigger);
void PlayFrontendSound(uint32_t trigger);       // PC: idx = (int8) table 0x8380B8[trigger]; != -1 -> PlayUISoundFX 458520
int32_t IsJoystickPluggedIn(int32_t port);              // PC 571A60 (__cdecl)
const uint8_t *JoyPortConfig();                         // PC 0x7F9414: 4 bytes per port, 0xFF = unused
uint32_t GetJoyEventTexture(const char *name, int32_t port);   // PC 572190 (__cdecl): icon texture hash
void *GetTextureInfo(uint32_t hash, int32_t a, int32_t b);     // PC 4901D0 (__cdecl)
bool TextInputFocused();   // PC globals 0x83A9C0 (byte, FocusedTextbox active) && 0x83A9C4 (dword) both nonzero
void SetHoldDirtyFlags(uint32_t v);                                   // uHoldDirtyFlags, PC 0x7F7C8C
// constructors of object types not rewritten yet (types 1, 4, 5, 6, 7, 9, 10, 11; PC 532640/5349C0/52B080/52B9E0/52B0B0/534B90/534DB0/52B480)
::FEObject *FEObjectTypeConstruct(void *mem, uint32_t type);
::FEObject *FEObject_VirtualClone(::FEObject *obj, bool reference);       // vtable slot 1
::FEObject *FEGroup_CopyConstruct(void *mem, ::FEObject *src, bool a, bool b);   // PC 5548A0 (__thiscall, 0x6C bytes)
void FEKeyInterpScript(void *script, uint8_t track, uint32_t time, void *obj);   // PC 541F60 (__cdecl; GC FEKeyInterp 4 args)
// FEGameInterface vtable +0x4C (__thiscall): fill one code list box cell (virtual column, row)
void FEGameInterface_FillCodeListBoxCell(void *gi, void *listBox, int32_t column, int32_t row);
// FEGameInterface vtable +0x18 (__thiscall): message to the game (msg hash, object, arg, extra)
void FEGameInterface_Message(void *gi, uint32_t msg, void *obj, uint32_t arg, uint32_t extra);
void *FECodeListBoxVTable();                                       // PC 0x79B904 (slot 0 = 5352B0)
void *FECodeListBoxDefaultCallback();                              // FECodeListBox::mpDefaultCallback (PC *0x7F7CC4)
void *FEListBoxVTable();                                           // PC 0x79BAE4 (slot 0 = 541D90)
void *FEPackageVTable();                                           // PC 0x79B8F8 (slot 0 = 534C50)
void *FENodeVTable();                                              // PC 0x79AB0C
void *FEPackageList74VTable();                                     // PC 0x79B878 (FEPackage +0x74 list)
void *ResourceConnectorVTable();                                   // PC 0x79AB04 (game-side FEObjectCallback)
// FEGameInterface vtable slot 1 (__thiscall): load the package's resources        (GC slot +0x14)
bool FEGameInterface_LoadResources(void *gi, ::FEPackage *pkg, uint32_t count, void *resources);
void *FEPackageDataVTable();                                       // PC 0x79B15C (slot 0 = 52CC20)
void *ScreenFactory(uint32_t nameHash, ::FEPackage *pkg, int32_t arg);   // PC 0x50B790 (GC 8014A2CC)
uint32_t FindScreenCreateData(uint32_t nameHash);                  // PC 0x50B840 (GC 8014A3A8)
void HackClearCache(::FEPackage *pkg);                             // PC 0x50C550 (GC 8014DFBC)
void FEngine_Update(void *engine, uint32_t a, uint32_t b);         // PC 0x558C00 (__thiscall, GC 80146028)
bool IsCurrentlyHotChunking();                                     // PC 0x571090 (GC 80195514)
::SlotPool *LevelSorterSlots();                                                    // PC *(0x8384B0) (game global)
// MenuScreen::BaseNotify(msg, obj, arg, pkg): game-side screen notification            PC 0x55B5C0 (__thiscall)
void MenuScreen_BaseNotify(void *screen, uint32_t msg, void *obj, uint32_t arg, ::FEPackage *pkg);
void *bMalloc(uint32_t size, uint32_t flags);                                      // PC 0x440820 (__cdecl; GC bMalloc(int,int))
void LZDecompress(const uint8_t *src, uint8_t *dst);                               // PC 0x574670 (__cdecl)
::FEPackageManager *TheFEPackageManager();                                         // PC 0x8637B0 (game global)                                                       // PC 0x440540 (__cdecl)
uint16_t BaselineFovNoise();           // PC 0x7EC1F0 aBaselineFovNoise (game global)
uint32_t *BRandomSeed();              // PC 0x7EA624 (game global)
const uint8_t *BFastATanTable();      // PC 0x7EACB0 (game data, 257 bytes)
const uint16_t *BFixATanTable1();     // PC 0x7EB1C0 (game data, 129 entries: atan(i / 32))
const uint16_t *BFixATanTable2();     // PC 0x7EB2C8 (game data, 129 entries: atan(i / 4))
const ::bASinTableEntry *BASinTable();                            // PC 0x7EA628 (game data)
// FEPackage callees not rewritten yet (PC __thiscall)
// FEngine package loading (not rewritten; __thiscall)
void FEngine_UnloadPackage(void *engine, FEPackage *pkg);                                    // 5362E0 (GC 80145A94)
FEPackage *FEngine_PushPackage(void *engine, const char *name, uint8_t priority, uint32_t mask);   // 555860 (GC 80145CF8)
cFEng *cFEngInstance();                                 // cFEng::pInstance (PC 0x8384C4)
void *FEPackageCommandVTable();                                                   // PC 0x79AB64
void *FEMessageNodeVTable();                                                      // PC 0x79AB60
// game UI handler virtual calls (__thiscall), slot given as a byte offset
void FEUIData_Call4(void *ui, uint32_t slotOffset, uint32_t a, const void *b, uint32_t c, const void *d);
void FEUIData_Call5(void *ui, uint32_t slotOffset, uint32_t a, const void *b, const void *c, const void *d, uint32_t e);
void FEUIData_OnQueueMessage(void *ui, uint32_t msg, void *obj, void *pkg, void *target, uint32_t arg);   // vtable +0x54
void FEngine_SendMessageToGame(void *engine, uint32_t msg, void *obj, void *pkg);   // engine+0x10C vtable slot 6 (msg, obj, 0, pkg)
bool FEObjectCallback_Call(void *callback, void *obj);                               // vtable slot 1, __thiscall
void FEListBox_Update(void *listBox, float delta);                                                        // 517230
void FECodeListBox_Update(void *listBox, float delta);                                                    // 504DC0
uint32_t HoldDirtyFlags();                                             // uHoldDirtyFlags, PC 0x7F7C8C
void *FERefListVTable();                                               // PC 0x79AA30
void *FEMessageResponseVTable();                                       // PC 0x79AA38

}  // namespace bridge
