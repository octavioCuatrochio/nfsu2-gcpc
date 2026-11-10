// Which original functions are replaced by rewritten code.
// One line per function: PC address, implementation, GameCube name, original first 6 bytes.
// Member functions are reached through thin __thiscall wrappers (MSVC passes `this` in ECX).
// Keep in sync with the `rewrite` table in knowledge.db.
#include "hooks.h"
#include "bWare/bCrc32.h"
#include "bWare/bString.h"
#include "bWare/bWideString.h"
#include "frontend/FEHash.h"
#include "frontend/FEMinList.h"
#include "frontend/FEObject.h"
#include "frontend/FEMultiPool.h"
#include "frontend/FEMessageResponse.h"
#include "frontend/FEPool.h"
#include "frontend/FEScript.h"
#include "frontend/FEMath.h"
#include "frontend/FEPackage.h"
#include "frontend/FEPackageReader.h"
#include "frontend/FEngine.h"
#include "frontend/FEButtonMap.h"
#include "frontend/FERender.h"
#include "frontend/FEngUtil.h"
#include "frontend/FEWidget.h"
#include "frontend/TuningSlidersMenu.h"
#include "frontend/UIWheelOptionsMenu.h"
#include "frontend/ChooseBrandsScreen.h"
#include "frontend/OptionWidgets.h"
#include "frontend/UIOptionsScreen.h"
#include "frontend/UIPauseOptions.h"
#include "frontend/IconScroller.h"
#include "frontend/ThumbnailScroller.h"
#include "frontend/QRTrackSelect.h"
#include "frontend/QRModeOptions.h"
#include "frontend/PostRaceMenu.h"
#include "frontend/CarTunePause.h"
#include "frontend/MagazineSelect.h"
#include "frontend/QuickRaceSettings.h"
#include "frontend/OptionsMain.h"
#include "frontend/PauseMenu.h"
#include "frontend/MainMenu.h"
#include "frontend/QRModeSelect.h"
#include "frontend/StartCareer.h"
#include "frontend/ProfileManager.h"
#include "frontend/CareerCrib.h"
#include "frontend/FEMath.h"
#include "frontend/TuningUtil.h"
#include "frontend/MenuScreen.h"
#include "frontend/UIWidgetMenu.h"
#include "frontend/FEScrollBar.h"
#include "frontend/cFEngRender.h"
#include "frontend/FEngFont.h"
#include "frontend/FENode.h"
#include "frontend/FEPackageManager.h"
#include "frontend/cFEng.h"
#include "frontend/FEListBox.h"
#include "frontend/FECodeListBox.h"
#include "bWare/bMath.h"
#include "bWare/bSlotPool.h"
#include "camera/CameraNoise.h"
#include "camera/Camera.h"
#include "frontend/FEString.h"
#include <new>
#include "frontend/FEWideString.h"

#define THISCALL __attribute__((thiscall))

namespace {

void THISCALL FEWideString_SetLength(FEWideString *self, uint32_t length) { self->SetLength(length); }
FEWideString *THISCALL FEWideString_AssignPacked(FEWideString *self, const char *s) { return &(*self = s); }
void THISCALL FEString_SetStringWide(FEString *self, const uint16_t *s) { self->SetString(s); }
void THISCALL FEString_SetStringPacked(FEString *self, const char *s) { self->SetString(s); }
FEString *THISCALL FEString_Ctor(FEString *self) { return FEString::Construct(self); }
FEString *THISCALL FEString_CopyCtor(FEString *self, const FEString *src, int deep) { return FEString::CopyConstruct(self, *src, deep != 0); }
void THISCALL FEString_Dtor(FEString *self) { self->Destruct(); }
FEString *THISCALL FEString_ScalarDtor(FEString *self, uint32_t flags) { return self->ScalarDeletingDestructor(flags); }
FEString *THISCALL FEString_Clone(const FEString *self, int deep) { return self->Clone(deep != 0); }
void THISCALL FEString_SetLabel(FEString *self, const char *label) { self->SetLabel(label); }
FEWideString *THISCALL FEWideString_Ctor(FEWideString *self) { return new (self) FEWideString(); }
FEWideString *THISCALL FEWideString_CopyCtor(FEWideString *self, const FEWideString *src) { return new (self) FEWideString(*src); }
void THISCALL FEWideString_Dtor(FEWideString *self) { self->~FEWideString(); }
void THISCALL FEMinList_AddNode(FEMinList *self, FEMinNode *after, FEMinNode *node) { self->AddNode(after, node); }
void THISCALL FEMinList_RemNode(FEMinList *self, FEMinNode *node) { self->RemNode(node); }
void THISCALL FEMinList_Purge(FEMinList *self) { self->Purge(); }
FEMinList *THISCALL FEMinList_VecDtor(FEMinList *self, uint32_t flags) { return self->VectorDeletingDestructor(flags); }
FEMinNode *THISCALL FEMinNode_ScalarDtor(FEMinNode *self, uint32_t flags) { return self->ScalarDeletingDestructor(flags); }
void THISCALL FEObject_SetName(FEObject *self, const char *name) { self->SetName(name); }
FEObject *THISCALL FEObject_Ctor(FEObject *self) { return FEObject::Construct(self); }
void THISCALL FEObject_Dtor(FEObject *self) { self->Destruct(); }
FEObject *THISCALL FEObject_ScalarDtor(FEObject *self, uint32_t flags) { return self->ScalarDeletingDestructor(flags); }
FESlotNode *THISCALL FESlotNode_Ctor(FESlotNode *self, uint32_t size) { return FESlotNode::Construct(self, static_cast<uint16_t>(size)); }
void THISCALL FESlotNode_Dtor(FESlotNode *self) { self->Destruct(); }
FESlotNode *THISCALL FESlotNode_ScalarDtor(FESlotNode *self, uint32_t flags) { return self->ScalarDeletingDestructor(flags); }
uint8_t *THISCALL FESlotNode_AllocBlock(FESlotNode *self) { return self->AllocBlock(); }
uint8_t *THISCALL FESlotPool_Alloc(FESlotPool *self) { return self->Alloc(); }
bool THISCALL FESlotPool_Free(FESlotPool *self, const uint8_t *b) { return self->Free(b); }
void THISCALL FESlotPool_Dtor(FESlotPool *self) { self->Destruct(); }
FESlotPool *THISCALL FESlotPool_ScalarDtor(FESlotPool *self, uint32_t flags) { return self->ScalarDeletingDestructor(flags); }
void *THISCALL FEMultiPool_Alloc(FEMultiPool *self, uint32_t size) { return self->Alloc(size); }
void THISCALL FEMultiPool_Free(FEMultiPool *self, void *b) { self->Free(b); }
FEResponse *THISCALL FEResponse_Ctor(FEResponse *self) { self->Construct(); return self; }
void THISCALL FEResponse_Release(FEResponse *self) { self->ReleaseParam(); }
void THISCALL FEResponse_SetParam(FEResponse *self, const char *t) { self->SetParam(t); }
FEResponse *THISCALL FEResponse_Assign(FEResponse *self, const FEResponse *src) { return &self->Assign(*src); }
void THISCALL FEMessageResponse_SetCount(FEMessageResponse *self, uint32_t n) { self->SetCount(n); }
void THISCALL FEMinList_AddTail(FEMinList *self, FEMinNode *n) { self->AddTail(n); }
void *THISCALL MsgRespPool_New(FEMinList *pool) { return FEPoolNew<FEMessageResponse, 64>(*pool, FEMessageResponseBlock_Construct); }
void THISCALL MsgRespPool_Delete(FEMinList *pool, void *p) { FEPoolDelete<FEMessageResponse, 64>(*pool, p); }
void *THISCALL ScriptPool_New(FEMinList *pool) { return FEPoolNew<FEScript, 32>(*pool, FEScriptBlock_Construct); }
void THISCALL ScriptPool_Delete(FEMinList *pool, void *p) { FEPoolDelete<FEScript, 32>(*pool, p); }
void *THISCALL MsgRespBlock_Ctor(void *self) { FEMessageResponseBlock_Construct(self); return self; }
void THISCALL MsgRespBlock_Dtor(FEMessageResponseBlock *self) { FEMessageResponseBlock_Destruct(self); }
void *THISCALL MsgRespBlock_ScalarDtor(FEMessageResponseBlock *self, uint32_t f) { return FEMessageResponseBlock_ScalarDtor(self, f); }
void *THISCALL ScriptBlock_Ctor(void *self) { FEScriptBlock_Construct(self); return self; }
void THISCALL ScriptBlock_Dtor(FEScriptBlock *self) { FEScriptBlock_Destruct(self); }
void *THISCALL ScriptBlock_ScalarDtor(FEScriptBlock *self, uint32_t f) { return FEScriptBlock_ScalarDtor(self, f); }
FEMinNode *THISCALL FERefList_RemNode(FERefList *self, FEMinNode *n) { return self->RemNode(n); }
void THISCALL FERefList_Purge(FERefList *self) { self->Purge(); }
void THISCALL FEKeyTrack_Dtor(FEKeyTrack *self) { self->Destruct(); }
void THISCALL FEScript_Dtor(FEScript *self) { self->Destruct(); }
FEScript *THISCALL FEScript_ScalarDtor(FEScript *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
FEKeyTrack *THISCALL FEKeyTrack_Ctor(FEKeyTrack *self) { return FEKeyTrack::Construct(self); }
void THISCALL FEKeyTrack_Assign(FEKeyTrack *self, const FEKeyTrack *src) { self->Assign(*src); }
void THISCALL FEScriptEvents_SetCount(FEScriptEventList *self, uint32_t n) { self->SetCount(n); }
void THISCALL FEScript_SetName(FEScript *self, const char *name) { self->SetName(name); }
void THISCALL FEScript_SetTrackCount(FEScript *self, uint32_t n) { self->SetTrackCount(n); }
FEScript *THISCALL FEScript_CopyCtor(FEScript *self, const FEScript *src, int reference)
{
    return FEScript::CopyConstruct(self, *src, (reference & 0xFF) != 0);   // bool arg: only the low byte is set
}
FEKeyNode *THISCALL FEKeyNode_Ctor(FEKeyNode *self) { return FEKeyNode::Construct(self); }
FEKeyNode *THISCALL FEKeyNode_ScalarDtor(FEKeyNode *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void *THISCALL KeyNodePool_New(FEMinList *pool) { return FEPoolNew<FEKeyNode, 256>(*pool, FEKeyNodeBlock_Construct); }
void THISCALL KeyNodePool_Delete(FEMinList *pool, void *p) { FEPoolDelete<FEKeyNode, 256>(*pool, p); }
void *THISCALL KeyNodeBlock_Ctor(void *self) { FEKeyNodeBlock_Construct(self); return self; }
void THISCALL KeyNodeBlock_Dtor(FEKeyNodeBlock *self) { FEKeyNodeBlock_Destruct(self); }
void *THISCALL KeyNodeBlock_ScalarDtor(FEKeyNodeBlock *self, uint32_t f) { return FEKeyNodeBlock_ScalarDtor(self, f); }
FEObject *THISCALL FEObject_CopyCtor(FEObject *self, const FEObject *src, int reference)
{
    return FEObject::CopyConstruct(self, *src, (reference & 0xFF) != 0);   // bool arg: low byte only
}
FEObject *THISCALL FEObject_Clone(const FEObject *self, int reference) { return self->Clone((reference & 0xFF) != 0); }
void THISCALL FEObject_SetupMoveToTracks(FEObject *self) { self->SetupMoveToTracks(); }
FEMinNode *THISCALL FERefList_GetHead(const FERefList *self) { return self->GetHead(); }
FEQuaternion *THISCALL FEQuat_Multiply(const FEQuaternion *a, FEQuaternion *out, const FEQuaternion *b)
{
    *out = FEQuatMultiply(*a, *b);
    return out;
}
FEColor *THISCALL FEColor_Sub(const FEColor *a, FEColor *out, const FEColor *b)
{
    *out = FEColorSub(*a, *b);
    return out;
}
FEKeyTrack *THISCALL FEScript_FindTrack(const FEScript *self, int track) { return self->FindTrack(track); }
void THISCALL FEObject_SetTrackVec3(FEObject *self, int track, const FEVector3 *v, int add) { self->SetTrackValue(track, *v, (add & 0xFF) != 0); }
void THISCALL FEObject_SetTrackVec2(FEObject *self, int track, const FEVector2 *v, int add) { self->SetTrackValue(track, *v, (add & 0xFF) != 0); }
void THISCALL FEObject_SetTrackColor(FEObject *self, int track, const FEColor *v, int add) { self->SetTrackValue(track, *v, (add & 0xFF) != 0); }
void THISCALL FEObject_SetPosition(FEObject *self, const FEVector3 *v, int add) { self->SetPosition(*v, (add & 0xFF) != 0); }
void THISCALL FEObject_SetRotation(FEObject *self, const FEQuaternion *q, int add) { self->SetRotation(*q, (add & 0xFF) != 0); }
void THISCALL FEObject_SetSize(FEObject *self, const FEVector3 *v, int add) { self->SetSize(*v, (add & 0xFF) != 0); }
void THISCALL FEObject_SetColor(FEObject *self, const FEColor *c, int add) { self->SetColor(*c, (add & 0xFF) != 0); }
bool __cdecl FEVector3_Close(const FEVector3 *a, const FEVector3 *b) { return FEVector3Close(*a, *b); }
bool __cdecl FEColor_Close(const FEColor *a, const FEColor *b) { return FEColorClose(*a, *b); }
FEKeyNode *THISCALL FEKeyTrack_GetKeyAt(FEKeyTrack *self, int32_t t) { return self->GetKeyAt(t); }
FEKeyNode *THISCALL FEKeyTrack_GetDeltaKeyAt(const FEKeyTrack *self, int32_t t) { return self->GetDeltaKeyAt(t); }
FEKeyNode *THISCALL FEKeyTrack_GetFirstKey(const FEKeyTrack *self) { return self->GetFirstKey(); }
void THISCALL FEKeyTrack_SetBaseValueAt(FEKeyTrack *self, int32_t t, const FEColor *c) { self->SetBaseValueAt(t, *c); }
void __cdecl FEInterpNone_Hook(FEKeyTrack *t, int32_t time, void *out) { FEInterpNone(t, time, out); }
void __cdecl FEKeyInterpFast_Hook(FEKeyTrack *t, int32_t time, void *out) { FEKeyInterpFast(t, time, out); }
void __cdecl FEInterpLinear_Hook(FEKeyTrack *t, int32_t time, void *out) { FEInterpLinear(t, time, out); }
long double __cdecl bSin_Hook(uint32_t angle) { return bSin(static_cast<uint16_t>(angle)); }
long double __cdecl bCos_Hook(uint32_t angle) { return bCos(static_cast<uint16_t>(angle)); }
long double __cdecl NoiseInterpolated_Hook(float x) { return NoiseInterpolated(x); }
long double __cdecl Noise_Hook(float x) { return Noise(x); }
uint32_t THISCALL Camera_FovRelativeAngle(const Camera *self, uint32_t a) { return self->FovRelativeAngle(static_cast<uint16_t>(a)); }
long double __cdecl bTan_Hook(uint32_t angle) { return bTan(static_cast<uint16_t>(angle)); }
long double __cdecl bLength_Hook(const bVector3 *v) { return bLength(v); }
uint16_t __cdecl bFastATan_Hook(float x, float y) { return bFastATan(x, y); }
void THISCALL SlotPool_Flush(SlotPool *self) { SlotPool::FlushSlotPool(self); }
int32_t THISCALL SlotPool_GetSlotNumber(SlotPool *self, const void *p) { return SlotPool::GetSlotNumber(self, p); }
void *THISCALL SlotPool_FastMalloc(SlotPool *self) { return self->FastMalloc(); }
void THISCALL SlotPool_Free(SlotPool *self, void *p) { self->Free(p); }
void *THISCALL SlotPool_Malloc(SlotPool *self) { return self->Malloc(); }
void THISCALL SlotPoolManager_Delete(SlotPoolManager *self, SlotPool *p) { self->DeleteSlotPool(p); }
void *THISCALL SlotPool_GetAllocatedSlot(SlotPool *self, int32_t n) { return self->GetAllocatedSlot(n); }
void THISCALL SlotPool_Expand(SlotPool *self, int32_t n) { self->ExpandSlotPool(n); }
SlotPool *__cdecl SlotPool_New(uint32_t size, int32_t n, const char *name, uint32_t flags) { return SlotPool::NewSlotPool(size, n, name, flags); }
int32_t __cdecl bEqual2_Hook(const bVector2 *a, const bVector2 *b, float e) { return bEqual(a, b, e); }
int32_t __cdecl bEqual3_Hook(const bVector3 *a, const bVector3 *b, float e) { return bEqual(a, b, e); }
void __cdecl bInitBB0_Hook(bVector2 *a, bVector2 *b) { bInitializeBoundingBox(a, b); }
void __cdecl bInitBB1_Hook(bVector2 *a, bVector2 *b, const bVector2 *p) { bInitializeBoundingBox(a, b, p); }
void __cdecl bExpandBB_Hook(bVector2 *a, bVector2 *b, const bVector2 *p) { bExpandBoundingBox(a, b, p); }
void __cdecl bExpandBBR_Hook(bVector2 *a, bVector2 *b, const bVector2 *p, float r) { bExpandBoundingBox(a, b, p, r); }
void __cdecl bExpandBB2_Hook(bVector2 *a, bVector2 *b, const bVector2 *p, const bVector2 *q) { bExpandBoundingBox(a, b, p, q); }
long double __cdecl bDistBetween_Hook(const bVector3 *a, const bVector3 *b) { return bDistBetween(a, b); }
void __cdecl bNormalize2_Hook(bVector2 *o, const bVector2 *i) { bNormalize(o, i); }
void __cdecl bNormalize2S_Hook(bVector2 *o, const bVector2 *i, float s) { bNormalize(o, i, s); }
void __cdecl bNormalize3_Hook(bVector3 *o, const bVector3 *i) { bNormalize(o, i); }
void __cdecl bNormalize3S_Hook(bVector3 *o, const bVector3 *i, float s) { bNormalize(o, i, s); }
void __cdecl bNormalize4_Hook(bVector4 *o, const bVector4 *i) { bNormalize(o, i); }
uint32_t __cdecl bRandomIS_Hook(int32_t r, uint32_t *s) { return bRandom(r, s); }
long double __cdecl bRandomFS_Hook(float r, uint32_t *s) { return bRandom(r, s); }
uint32_t __cdecl bRandomI_Hook(int32_t r) { return bRandom(r); }
long double __cdecl bRandomF_Hook(float r) { return bRandom(r); }
long double __cdecl bFMod_Hook(float a, float b) { return bFMod(a, b); }
int32_t __cdecl bSignedMod_Hook(int32_t a, int32_t b) { return bSignedMod(a, b); }
int32_t __cdecl bFixATan1_Hook(int32_t r) { return bFixATan(r); }
int32_t __cdecl bFixATan2_Hook(int32_t x, int32_t y) { return bFixATan(x, y); }
void __cdecl bSinCos_Hook(float *s, float *c, uint32_t angle) { bSinCos(s, c, static_cast<uint16_t>(angle)); }
long double __cdecl bSinF_Hook(float r) { return bSinF(r); }
uint32_t __cdecl bASin_Hook(float x) { return bASin(x); }
FEQuaternion *__cdecl FEQuatScale_Hook(FEQuaternion *o, const FEQuaternion *q, float s) { FEQuatScale(o, q, s); return o; }
FEQuaternion *__cdecl FEQuatAdd_Hook(FEQuaternion *o, const FEQuaternion *a, const FEQuaternion *b) { FEQuatAdd(o, a, b); return o; }
FEQuaternion *__cdecl FEQuatSub_Hook(FEQuaternion *o, const FEQuaternion *a, const FEQuaternion *b) { FEQuatSub(o, a, b); return o; }
void THISCALL FEPackage_UpdateObjectTracks(const FEPackage *self, FEObject *o, FEScript *s) { self->UpdateObjectTracks(o, s); }
void THISCALL FEPackage_UpdateObject(FEPackage *self, FEObject *o, int32_t delta) { self->UpdateObject(o, delta); }
void THISCALL FEPackage_UpdateGroup(FEPackage *self, FEGroup *g, int32_t delta) { self->UpdateGroup(g, delta); }
void THISCALL FEPackage_Update(FEPackage *self, void *engine, int32_t delta) { self->Update(engine, delta); }
void THISCALL FEPackage_IssueScriptMessages(FEPackage *self, void *engine, FEObject *o, FEScript *s, int32_t from, int32_t to)
{
    self->IssueScriptMessages(engine, o, s, from, to);
}
bool THISCALL FEPackage_ForAllObjects(FEPackage *self, FEObjectCallback *cb) { return self->ForAllObjects(cb); }
bool THISCALL FEPackage_ForAllChildren(FEPackage *self, FEGroup *g, FEObjectCallback *cb) { return self->ForAllChildren(g, cb); }
FEObject *THISCALL FEPackage_FindObjectByGUID(FEPackage *self, uint32_t guid) { return self->FindObjectByGUID(guid); }
FEObject *THISCALL FEPackage_FindObjectByHash(FEPackage *self, uint32_t hash) { return self->FindObjectByHash(hash); }
void THISCALL FEPackage_SetControlMask(FEPackage *self, uint32_t m) { self->SetControlMask(m); }
FELibraryRef *THISCALL FEPackage_FindLibraryReference(const FEPackage *self, uint32_t h) { return self->FindLibraryReference(h); }
void THISCALL FEPackage_SetCurrentButton(FEPackage *self, void *engine, FEObject *b, int notify) { self->SetCurrentButton(engine, b, (notify & 0xFF) != 0); }
void THISCALL FEngine_QueueMessage(FEngine *self, uint32_t msg, FEObject *obj, FEPackage *pkg, FEObject *target, uint32_t arg)
{
    self->QueueMessage(msg, obj, pkg, target, arg);
}
void THISCALL FEngine_ProcessMessageQueue(FEngine *self) { self->ProcessMessageQueue(); }
bool THISCALL FENode_SetName(FENode *self, const char *n) { return self->SetName(n); }
bool THISCALL FEngine_RecordPackageMarker(FEngine *self, const char *n) { return self->RecordPackageMarker(n); }
void THISCALL FEngine_ClearPackageMarkers(FEngine *self) { self->ClearPackageMarkers(); }
void THISCALL FEngine_ProcessPackageCommands(FEngine *self) { self->ProcessPackageCommands(); }
FEPackageData *THISCALL FEPkgMgr_FindByName(FEPackageManager *self, const char *n) { return self->FindFEPackageData(n); }
FEPackageData *THISCALL FEPkgMgr_FindByChunk(FEPackageManager *self, const void *c) { return self->FindFEPackageData(c); }
uint8_t *THISCALL FEPkgMgr_GetPackageData(FEPackageManager *self, const char *n) { return self->GetPackageData(n); }
uint32_t THISCALL FEPkgMgr_Checksum(FEPackageManager *self) { return self->GetActiveScreensChecksum(); }
uint32_t THISCALL FEPackageData_GetNameHash(const FEPackageData *self) { return self->GetNameHash(); }
void THISCALL FEPkgMgr_SendMessageToPackage(FEPackageManager *self, const char *n, uint32_t m) { self->SendMessageToPackage(n, m); }
void THISCALL FEPkgMgr_BroadcastMessage(FEPackageManager *self, uint32_t m, uint32_t a) { self->BroadcastMessage(m, a); }
void THISCALL FEPkgMgr_NotificationMessage(FEPackageManager *self, uint32_t m, void *o, uint32_t a, FEPackage *p) { self->NotificationMessage(m, o, a, p); }
FEPackageData *THISCALL FEPackageData_Ctor(FEPackageData *self, uint32_t *c) { self->Construct(c); return self; }
void THISCALL FEPackageData_Dtor(FEPackageData *self) { self->Destruct(); }
FEPackageData *THISCALL FEPackageData_ScalarDtor(FEPackageData *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL FEPackageData_Activate(FEPackageData *self, FEPackage *p, int32_t a) { self->Activate(p, a); }
void THISCALL FEPackageData_UnActivate(FEPackageData *self) { self->UnActivate(); }
void THISCALL FEPackageData_Close(FEPackageData *self) { self->Close(); }
void THISCALL FEPkgMgr_CloseAllPackages(FEPackageManager *self, int32_t all) { self->CloseAllPackages(all); }
void THISCALL FEPkgMgr_Loader(FEPackageManager *self, uint32_t *c, bool h) { self->Loader(c, h); }
void THISCALL FEPkgMgr_UnLoader(FEPackageManager *self, uint32_t *c, bool h) { self->UnLoader(c, h); }
FEPackage *THISCALL FEPackage_Ctor(FEPackage *self) { self->Construct(); return self; }
void THISCALL FEPackage_Dtor(FEPackage *self) { self->Destruct(); }
FEPackage *THISCALL FEPackage_ScalarDtor(FEPackage *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL FEPackage_SetFilename(FEPackage *self, const char *n) { self->SetFilename(n); }
bool THISCALL FEPackage_Startup(FEPackage *self, void *gi) { return self->Startup(gi); }
void THISCALL FEPackage_ConnectObjectResources(FEPackage *self) { self->ConnectObjectResources(); }
void THISCALL FENode_Dtor(FENode *self) { self->Destruct(); }
FEPackage *THISCALL FEPackageReader_Load(FEPackageReader *self, const void *d, void *gi, FEngine *e, uint8_t a, uint8_t b, uint8_t c) { return self->Load(d, gi, e, a, b, c); }
bool THISCALL FEPackageReader_ReadHeaderChunk(FEPackageReader *self) { return self->ReadHeaderChunk(); }
bool THISCALL FEPackageReader_ReadTypeSizes(FEPackageReader *self) { return self->ReadTypeSizes(); }
bool THISCALL FEPackageReader_ReadLibraryRefsChunk(FEPackageReader *self) { return self->ReadLibraryRefsChunk(); }
bool THISCALL FEPackageReader_ReadResourceChunk(FEPackageReader *self) { return self->ReadResourceChunk(); }
bool THISCALL FEPackageReader_ReadPackageResponseChunk(FEPackageReader *self) { return self->ReadPackageResponseChunk(); }
bool THISCALL FEPackageReader_ReadMessageResponseTags(FEPackageReader *self, const uint8_t *t, uint32_t n, uint8_t p) { return self->ReadMessageResponseTags(t, n, p != 0); }
void THISCALL FEPackage_SetNumLibraryRefs(FEPackage *self, uint32_t n) { self->SetNumLibraryRefs(n); }
bool THISCALL FEPackageReader_ReadMessageTargetListChunk(FEPackageReader *self) { return self->ReadMessageTargetListChunk(); }
bool THISCALL FEPackageReader_ReadReferencedPackagesChunk(FEPackageReader *self) { return self->ReadReferencedPackagesChunk(); }
bool THISCALL FEPackageReader_ReadObjectChunk(FEPackageReader *self) { return self->ReadObjectChunk(); }
bool THISCALL FEPackageReader_ReadObjectTags(FEPackageReader *self, const uint8_t *t, uint32_t n) { return self->ReadObjectTags(t, n); }
FEObject *THISCALL FEPackageReader_CreateObject(FEPackageReader *self, uint32_t t) { return self->CreateObject(t); }
void THISCALL FEPackageReader_ProcessStringTag(FEPackageReader *self, const uint8_t *t) { self->ProcessStringTag(t); }
bool THISCALL FEPackageReader_ReadScriptTags(FEPackageReader *self, const uint8_t *t, uint32_t n) { return self->ReadScriptTags(t, n); }
FEListBox *THISCALL FEListBox_Ctor(FEListBox *self) { return FEListBox::Construct(self); }
void THISCALL FEListBox_Dtor(FEListBox *self) { self->Destruct(); }
FEListBox *THISCALL FEListBox_ScalarDtor(FEListBox *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL FEListBox_Terminate(FEListBox *self) { self->Terminate(); }
void THISCALL FEListBox_SetNumColumns(FEListBox *self, uint32_t n) { self->SetNumColumns(n); }
void THISCALL FEListBox_SetNumRows(FEListBox *self, uint32_t n) { self->SetNumRows(n); }
void THISCALL FEListBox_CleanupCells(FEListBox *self) { self->CleanupCells(); }
void THISCALL FEListBox_SetSelColumn(FEListBox *self, uint32_t c) { self->SetSelColumn(c); }
void THISCALL FEListBox_SetSelRow(FEListBox *self, uint32_t r) { self->SetSelRow(r); }
void THISCALL FEListBox_SetCellType(FEListBox *self, uint32_t t) { self->SetCellType(t); }
void THISCALL FEListBox_IncrementCellByColumn(FEListBox *self) { self->IncrementCellByColumn(); }
void THISCALL FEListBox_SetAutoWrap(FEListBox *self, uint32_t w) { self->SetAutoWrap((w & 0xFF) != 0); }
void THISCALL FEListBox_RecalculateCummulative(FEListBox *self) { self->RecalculateCummulative(); }
void THISCALL FEListBox_SetCellColor(FEListBox *self, const FEColor *c) { self->SetCellColor(*c); }
void THISCALL FEListBox_SetCellScale(FEListBox *self, const float *xy) { self->SetCellScale(xy); }
void THISCALL FEListBox_SetCellJustification(FEListBox *self, uint32_t a, uint32_t b, uint32_t c) { self->SetCellJustification(a, b, c); }
void THISCALL FEListBox_SetCellData(FEListBox *self, const uint32_t *d) { self->SetCellData(d); }
void THISCALL FEListBox_SetCellString(FEListBox *self, const uint16_t *t) { self->SetCellString(t); }
void THISCALL FEPackageReader_ProcessListBoxTag(FEPackageReader *self, const uint8_t *t) { self->ProcessListBoxTag(t); }
FECodeListBox *THISCALL FECodeListBox_Ctor(FECodeListBox *self) { return FECodeListBox::Construct(self); }
void THISCALL FECodeListBox_Dtor(FECodeListBox *self) { self->Destruct(); }
FECodeListBox *THISCALL FECodeListBox_ScalarDtor(FECodeListBox *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL FECodeListBox_Initialize(FECodeListBox *self, uint32_t c, uint32_t r) { self->Initialize(c, r); }
void THISCALL FECodeListBox_SetTotalNumColumns(FECodeListBox *self, uint32_t n) { self->SetTotalNumColumns(n); }
void THISCALL FECodeListBox_FillAllCells(FECodeListBox *self) { self->FillAllCells(); }
void THISCALL FECodeListBox_CopyProperties(FECodeListBox *self, const FECodeListBox *s) { self->CopyProperties(*s); }
FECodeListBox *THISCALL FECodeListBox_CopyCtor(FECodeListBox *self, const FECodeListBox *s, uint32_t ref) { return FECodeListBox::CopyConstruct(self, *s, (ref & 0xFF) != 0); }
FECodeListBox *THISCALL FECodeListBox_Clone(FECodeListBox *self, uint32_t ref) { return self->Clone((ref & 0xFF) != 0); }
bool THISCALL FECodeListBox_CheckMovement(FECodeListBox *self, int32_t d, int32_t o, uint32_t s, int32_t t, int32_t v) { return self->CheckMovement(d, o, s, t, v); }
bool THISCALL FECodeListBox_MakeMove(FECodeListBox *self, int32_t d, int32_t *o, uint32_t *s, int32_t t, int32_t v) { return self->MakeMove(d, o, s, t, v); }
bool THISCALL FECodeListBox_ScrollAxis(FECodeListBox *self, int32_t d, int32_t *o, uint32_t *s, uint32_t t, uint32_t v, uint32_t cols) { return self->ScrollSelection(d, o, s, t, v, (cols & 0xFF) != 0); }
bool THISCALL FECodeListBox_ScrollSelection(FECodeListBox *self, int32_t dx, int32_t dy) { return self->ScrollSelection(dx, dy); }
void THISCALL FECodeListBox_Update(FECodeListBox *self, float dt) { self->Update(dt); }
void THISCALL FECodeListBox_SetTotalNumRows(FECodeListBox *self, uint32_t n) { self->SetTotalNumRows(n); }
void THISCALL FECodeListBox_AllocateStrings(FECodeListBox *self, uint32_t c, uint32_t l) { self->AllocateStrings(c, l); }
void THISCALL FECodeListBox_SetCellColor(FECodeListBox *self, uint32_t c0, uint32_t r0, uint32_t v, uint32_t nc, uint32_t nr) { self->SetCellColor(c0, r0, v, nc, nr); }
void THISCALL FECodeListBox_SetCellScale(FECodeListBox *self, uint32_t c0, uint32_t r0, const float *xy, uint32_t nc, uint32_t nr) { self->SetCellScale(c0, r0, xy, nc, nr); }
void THISCALL FECodeListBox_SetCellJustification(FECodeListBox *self, uint32_t c0, uint32_t r0, uint32_t v, uint32_t nc, uint32_t nr) { self->SetCellJustification(c0, r0, v, nc, nr); }
void THISCALL FEPackageReader_ProcessCodeListBoxTag(FEPackageReader *self, const uint8_t *t) { self->ProcessCodeListBoxTag(t); }
void THISCALL FEString_SetLabelHash(FEString *self, uint32_t h) { self->SetLabelHash(h); }
bool THISCALL FEPackageReader_FindReferencedObject(FEPackageReader *self, uint32_t h, FEObject **o, FEPackage **p) { return self->FindReferencedObject(h, o, p); }
FEPackage *THISCALL FEngine_FindLibraryPackageHook(FEngine *self, uint32_t h) { return FEngine_FindLibraryPackage(self, h); }
void THISCALL FEMessageTargets_SetCapacity(FEMessageTargets *self, uint32_t n) { self->SetCapacity(n); }
FEPackage *THISCALL FEngine_LoadPackageHook(FEngine *self, const void *d, uint8_t f) { return FEngine_LoadPackage(self, d, f); }
FEObject *THISCALL FEngine_FindObject(FEngine *self, uint32_t h, const char *p) { return self->FindObject(h, p); }
void THISCALL FEngine_UpdateHook(FEngine *self, uint32_t delta, uint32_t whole) { self->Update(delta, whole); }
void THISCALL FEngine_ProcessPadsForPackage(FEngine *self, FEPackage *p) { self->ProcessPadsForPackage(p); }
void THISCALL FEngine_ProcessMouseForPackage(FEngine *self, FEPackage *p) { self->ProcessMouseForPackage(p); }
void THISCALL FEQuaternion_GetMatrix(const FEQuaternion *q, FEMatrix4 *out) { FEQuatGetMatrix(q, out); }
void THISCALL FEObjectSorter_SortObjects(FEngine::ObjectSorter *self) { self->SortObjects(); }
void THISCALL FEngine_RenderObject(FEngine *self, FEObject *o, const FEMatrix4 *m, uint16_t c) { self->RenderObject(o, m, c); }
void THISCALL FEngine_RenderGroup(FEngine *self, FEGroup *g, const FEMatrix4 *p, const FEMatrix4 *v, uint16_t c) { self->RenderGroup(g, p, v, c); }
void THISCALL FEngine_Render(FEngine *self) { self->Render(); }
void THISCALL FETypeNode_AddField(FETypeNode *self, const char *n, uint32_t t) { self->AddField(n, t); }
void THISCALL FETypeNode_AddFieldNode(FETypeNode *self, FEFieldNode *f) { self->AddFieldNode(f); }
FEFieldNode *THISCALL FETypeNode_GetField(const FETypeNode *self, const char *n) { return self->GetField(n); }
FETypeNode *THISCALL FETypeLib_FindType(const FETypeLib *self, uint32_t t) { return self->FindType(t); }
FETypeNode *THISCALL FETypeLib_CreateBaseObjectType(FETypeLib *self, const char *n) { return self->CreateBaseObjectType(n); }
FETypeNode *THISCALL FETypeLib_CreateImageObjectType(FETypeLib *self, const char *n) { return self->CreateImageObjectType(n); }
bool THISCALL FETypeLib_Startup(FETypeLib *self) { return self->Startup(); }
FEngine *THISCALL FEngine_Ctor(void *self) { return FEngine::Construct(self); }
void THISCALL FEngine_ReleaseAll(FEngine *self) { self->ReleaseAll(); }
void THISCALL FEngine_SetNumJoyPads(FEngine *self, uint8_t n) { self->SetNumJoyPads(n); }
void THISCALL cFEng_RegisterJoy(cFEng *self) { self->RegisterJoy(); }
void THISCALL cFEng_UnRegisterJoy(cFEng *self) { self->UnRegisterJoy(); }
uint32_t THISCALL cFEng_GetJoyPadMask(cFEng *self, uint8_t pad) { return self->GetJoyPadMask(pad); }
void THISCALL cFEng_SetCurrentButton(cFEng *self, const char *p, uint32_t h) { self->SetCurrentButton(p, h); }
FEScrollBar *THISCALL FEScrollBar_Ctor(FEScrollBar *self, const char *pkg, const char *base, uint32_t v, uint32_t r, uint32_t a5) { return self->Construct(pkg, base, v & 0xFF, r & 0xFF, a5 & 0xFF); }
void THISCALL FEScrollBar_Update(FEScrollBar *self, int32_t v, int32_t t, int32_t i) { self->Update(v, t, i); }
void THISCALL FEScrollBar_SetPosResized(FEScrollBar *self, int32_t v, int32_t t, int32_t i) { self->SetPosResized(v, t, i); }
void THISCALL FEScrollBar_SetPosNonResized(FEScrollBar *self, int32_t v, int32_t t, int32_t i) { self->SetPosNonResized(v, t, i); }
void THISCALL FEScrollBar_SetArrowVisibility(FEScrollBar *self, int32_t a, uint32_t v) { self->SetArrowVisibility(a, v & 0xFF); }
void THISCALL FEScrollBar_SetBackingVisibility(FEScrollBar *self, uint32_t v) { self->SetBackingVisibility(v & 0xFF); }
FEObject *FEngFindStringHook(const char *pkg, uint32_t h) { return FEngFindString(pkg, h); }
FEObject *FEngFindImageByName(const char *pkg, const char *name) { return FEngFindImage(pkg, name); }
FESliderWidget *THISCALL FESliderWidget_Ctor(FESliderWidget *self, const float *p, const float *sz, const float *vs, const float *vp, uint32_t en) { return self->Construct(p, sz, vs, vp, en & 0xFF); }
uint32_t THISCALL FESliderWidget_DetermineSpecialSound(FESliderWidget *self, uint32_t m, uint32_t e) { return self->DetermineSpecialSound(m, e); }
void THISCALL FESliderWidget_Position(FESliderWidget *self) { self->Position(); }
void THISCALL FESliderWidget_Show(FESliderWidget *self) { self->Show(); }
void THISCALL FESliderWidget_Hide(FESliderWidget *self) { self->Hide(); }
void THISCALL FESliderWidget_SetFocus(FESliderWidget *self, const char *pkg) { self->SetFocus(pkg); }
void THISCALL FESliderWidget_UnsetFocus(FESliderWidget *self) { self->UnsetFocus(); }
FEToggleWidget *THISCALL FEToggleWidget_Ctor(FEToggleWidget *self, const float *p, const float *sz, const float *vs, const float *vp, uint32_t en) { return self->Construct(p, sz, vs, vp, en & 0xFF); }
void THISCALL FEToggleWidget_CheckMouse(FEToggleWidget *self, const char *pkg, float x, float y) { self->CheckMouse(pkg, x, y); }
void THISCALL FEToggleWidget_Position(FEToggleWidget *self) { self->Position(); }
void THISCALL FEToggleWidget_Enable(FEToggleWidget *self) { self->Enable(); }
void THISCALL FEToggleWidget_Disable(FEToggleWidget *self) { self->Disable(); }
void THISCALL FEToggleWidget_Show(FEToggleWidget *self) { self->Show(); }
void THISCALL FEToggleWidget_Hide(FEToggleWidget *self) { self->Hide(); }
void THISCALL FEToggleWidget_SetFocus(FEToggleWidget *self, const char *pkg) { self->SetFocus(pkg); }
void THISCALL FEToggleWidget_UnsetFocus(FEToggleWidget *self) { self->UnsetFocus(); }
void THISCALL FEToggleWidget_BlinkArrows(FEToggleWidget *self, uint32_t m) { self->BlinkArrows(m); }
void THISCALL FEToggleImageWidget_Position(FEToggleImageWidget *self) { self->Position(); }
void THISCALL FEToggleImageWidget_Enable(FEToggleImageWidget *self) { self->Enable(); }
void THISCALL FEToggleImageWidget_Disable(FEToggleImageWidget *self) { self->Disable(); }
void THISCALL FEToggleImageWidget_Show(FEToggleImageWidget *self) { self->Show(); }
void THISCALL FEToggleImageWidget_Hide(FEToggleImageWidget *self) { self->Hide(); }
void THISCALL FEToggleImageWidget_SetFocus(FEToggleImageWidget *self, const char *pkg) { self->SetFocus(pkg); }
void THISCALL FEToggleImageWidget_UnsetFocus(FEToggleImageWidget *self) { self->UnsetFocus(); }
TuningSlider *THISCALL TuningSlider_Ctor(TuningSlider *self, int32_t type, const float *p, const float *sz, const float *vs, const float *vp, uint32_t lo, uint32_t hi, uint32_t en, float yo) { return self->Construct(type, p, sz, vs, vp, lo, hi, en & 0xFF, yo); }
void THISCALL TuningSlider_SetSliderObjects(TuningSlider *self, FEObject *a, FEObject *b, FEObject *c, FEObject *d, FEObject *e, FEObject *f, FEObject *g) { self->SetSliderObjects(a, b, c, d, e, f, g); }
void THISCALL TuningSlider_Position(TuningSlider *self) { self->Position(); }
void THISCALL TuningSlider_Hide(TuningSlider *self) { self->Hide(); }
void THISCALL TuningSlider_Show(TuningSlider *self) { self->Show(); }
void THISCALL TuningSlider_SetFocus(TuningSlider *self, const char *pkg) { self->SetFocus(pkg); }
void THISCALL TuningSlider_UnsetFocus(TuningSlider *self) { self->UnsetFocus(); }
void THISCALL TuningSlider_InitValues(TuningSlider *self) { self->InitValues(); }
void THISCALL TuningSlider_Draw(TuningSlider *self) { self->Draw(); }
void THISCALL TuningSlider_Act(TuningSlider *self, const char *pkg, uint32_t m) { self->Act(pkg, m); }
void THISCALL TuningSlider_CheckMouse(TuningSlider *self, const char *pkg, float x, float y) { self->CheckMouse(pkg, x, y); }
void __cdecl FEngSetLanguageHash_Hook(FEObject *o, uint32_t h) { FEngSetLanguageHash(o, h); }
UIWidgetMenu *THISCALL UIWidgetMenu_Ctor(UIWidgetMenu *self, const ScreenConstructorData *d) { return self->Construct(d); }
void THISCALL UIWidgetMenu_DtorBody(UIWidgetMenu *self) { self->DestructBody(); }
UIWidgetMenu *THISCALL UIWidgetMenu_ScalarDtor(UIWidgetMenu *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL UIWidgetMenu_RefreshWidgets(UIWidgetMenu *self) { self->RefreshWidgets(); }
void THISCALL UIWidgetMenu_SetInitialPositions(UIWidgetMenu *self) { self->SetInitialPositions(); }
uint32_t THISCALL UIWidgetMenu_GetWidgetIndex(UIWidgetMenu *self, const FEWidget *w) { return self->GetWidgetIndex(w); }
void THISCALL UIWidgetMenu_ClearWidgets(UIWidgetMenu *self) { self->ClearWidgets(); }
void THISCALL UIWidgetMenu_Reposition(UIWidgetMenu *self) { self->Reposition(); }
void THISCALL UIWidgetMenu_Reset(UIWidgetMenu *self) { self->Reset(); }
void THISCALL UIWidgetMenu_SyncViewToSelection(UIWidgetMenu *self) { self->SyncViewToSelection(); }
void THISCALL UIWidgetMenu_Scroll(UIWidgetMenu *self, int32_t d) { self->Scroll(d); }
void THISCALL UIWidgetMenu_ScrollWrapped(UIWidgetMenu *self, int32_t d) { self->ScrollWrapped(d); }
void THISCALL DeleteAllNodeOwners_Hook(WidgetListNode *head) { DeleteAllNodeOwners(head); }
WidgetListNode *THISCALL ListGetNode_Hook(WidgetListNode *head, uint32_t i) { return ListGetNode(head, i); }
FEObject *THISCALL UIWidgetMenu_GetCurrentFEObject(UIWidgetMenu *self, const char *p) { return self->GetCurrentFEObject(p); }
uint32_t THISCALL UIWidgetMenu_AddButtonOption(UIWidgetMenu *self, FEButtonWidget *w) { return self->AddButtonOption(w); }
uint32_t THISCALL UIWidgetMenu_AddStatOption(UIWidgetMenu *self, FEStatWidget *w) { return self->AddStatOption(w); }
uint32_t THISCALL UIWidgetMenu_AddToggleOption(UIWidgetMenu *self, FEToggleWidget *w, int32_t st) { return self->AddToggleOption(w, st); }
uint32_t THISCALL UIWidgetMenu_AddToggleOptionB(UIWidgetMenu *self, FEToggleWidget *w, uint32_t b) { return self->AddToggleOption(w, (b & 0xFF) != 0); }
uint32_t THISCALL UIWidgetMenu_AddToggleImageOption(UIWidgetMenu *self, FEToggleImageWidget *w, int32_t st) { return self->AddToggleImageOption(w, st); }
uint32_t THISCALL UIWidgetMenu_AddSliderOption(UIWidgetMenu *self, FESliderWidget *w, int32_t st) { return self->AddSliderOption(w, st); }
uint32_t THISCALL UIWidgetMenu_AddSliderOptionB(UIWidgetMenu *self, FESliderWidget *w, uint32_t b) { return self->AddSliderOption(w, (b & 0xFF) != 0); }
void THISCALL FESliderWidget_SetSliderObjects(FESliderWidget *self, FEObject *a, FEObject *b, FEObject *c, FEObject *d, FEObject *e, FEObject *f) { self->SetSliderObjects(a, b, c, d, e, f); }
void THISCALL UIWidgetMenu_StorePrevNotification(UIWidgetMenu *self, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { self->StorePrevNotification(m, o, a, p); }
void THISCALL UIWidgetMenu_SetInitialOption(UIWidgetMenu *self, int32_t i) { self->SetInitialOption(i); }
void THISCALL UIWidgetMenu_NotificationMessage(UIWidgetMenu *self, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { self->NotificationMessage(m, o, a, p); }
FEWidget *THISCALL UIWidgetMenu_WidgetAtPoint(UIWidgetMenu *self, float x, float y) { return self->WidgetAtPoint(x, y); }
void THISCALL UIWidgetMenu_SelectWidget(UIWidgetMenu *self, FEWidget *w) { self->SelectWidget(w); }
void THISCALL UIWidgetMenu_HandleMouse(UIWidgetMenu *self) { self->HandleMouse(); }
void THISCALL UIWidgetMenu_HandleScrollBarMouse(UIWidgetMenu *self) { self->HandleScrollBarMouse(); }
FEObject *THISCALL UIWidgetMenu_GetCurrentFEString(UIWidgetMenu *self, const char *p) { return self->GetCurrentFEString(p); }
FEObject *THISCALL UIWidgetMenu_GetCurrentFEImage(UIWidgetMenu *self, const char *p) { return self->GetCurrentFEImage(p); }
uint32_t THISCALL FEWidget_DetermineSound_Hook(FEWidget *self, uint32_t m) { return self->DetermineSound(m); }
uint32_t __cdecl FEngTestForIntersectionRect_Hook(float x, float y, const float *p, const float *s) { return FEngTestForIntersection(x, y, p, s); }
void THISCALL MenuScreen_BaseNotify(MenuScreen *self, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { self->BaseNotify(m, o, a, p); }
uint32_t THISCALL MenuScreen_CheckKeyboard(MenuScreen *self, uint32_t m) { return self->CheckKeyboard(m); }
void THISCALL MenuScreen_EndTextInput(MenuScreen *self) { self->EndTextInput(); }
void THISCALL MenuScreen_BeginTextInput(MenuScreen *self, uint32_t h, uint32_t m, char *b, char *b2, uint32_t mode, uint32_t f) { self->FEngBeginTextInput(h, m, b, b2, mode, f); }
void THISCALL MenuScreen_BaseNotifySound(MenuScreen *self, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { self->BaseNotifySound(m, o, a, p); }
void THISCALL MenuScreen_SetAlternatingPlayerCash(MenuScreen *self) { self->SetAlternatingPlayerCash(); }
void THISCALL MenuScreen_SetAlternatingText(MenuScreen *self, FEObject *a, FEObject *b, float t1, float t2) { self->SetAlternatingText(a, b, t1, t2); }
void THISCALL MenuScreen_AlternateTheText(MenuScreen *self) { self->AlternateTheText(); }
void THISCALL MenuScreen_UpdateEAMIcons(MenuScreen *self) { self->UpdateEAMIcons(); }
void __cdecl FEngSetTextureHashPkg_Hook(const char *p, uint32_t h, uint32_t t) { FEngSetTextureHash(p, h, t); }
void __cdecl FEngSetLanguageHashPkg_Hook(const char *p, uint32_t h, uint32_t l) { FEngSetLanguageHash(p, h, l); }
void __cdecl FEngSetLanguageHashName_Hook(const char *p, const char *o, uint32_t l) { FEngSetLanguageHash(p, o, l); }
void __cdecl FEngSetInvisibleName_Hook(const char *p, const char *o) { FEngSetInvisible(p, o); }
void __cdecl FEngSetInvisiblePkg_Hook(const char *p, uint32_t h) { FEngSetInvisible(p, h); }
void __cdecl FEngSetVisibleName_Hook(const char *p, const char *o) { FEngSetVisible(p, o); }
void __cdecl FEngSetVisiblePkg_Hook(const char *p, uint32_t h) { FEngSetVisible(p, h); }
void THISCALL FEScrollBar_HandleMouse(FEScrollBar *self) { self->HandleMouse(); }
void THISCALL FEScrollBar_ArrowClick(FEScrollBar *self) { self->ArrowClick(); }
void THISCALL FEScrollBar_Drag(FEScrollBar *self) { self->Drag(); }
void THISCALL FEScrollBar_PageClick(FEScrollBar *self) { self->PageClick(); }
void THISCALL FEScrollBar_BlinkArrow1(FEScrollBar *self) { self->BlinkArrow1(); }
void THISCALL FEScrollBar_BlinkArrow2(FEScrollBar *self) { self->BlinkArrow2(); }
uint32_t __cdecl FEngIsClickedOn_Hook(const FEObject *o) { return FEngIsClickedOn(o); }
MenuScreen *THISCALL MenuScreen_Ctor(MenuScreen *self, const ScreenConstructorData *d) { return self->Construct(d); }
MenuScreen *THISCALL MenuScreen_ScalarDtor(MenuScreen *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
uint32_t __cdecl IsOverlayPackage_Hook(const char *p) { return IsOverlayPackage(p); }
uint32_t __cdecl IsGenericDialogPackage_Hook(const char *p) { return IsGenericDialogPackage(p); }
uint32_t __cdecl GetNameHashForTuningType_Hook(int32_t t) { return GetNameHashForTuningType(t); }
uint32_t __cdecl GetUnitsHashForTuningType_Hook(int32_t t, int32_t u) { return GetUnitsHashForTuningType(t, u); }
cSlider *THISCALL cSlider_Ctor(cSlider *self) { return self->Construct(); }
uint32_t THISCALL cSlider_Update(cSlider *self, uint32_t msg) { return self->Update(msg); }
void THISCALL cSlider_Init(cSlider *self, const SliderInitParams *p, uint32_t v) { self->Init(p, v & 0xFF); }
void THISCALL cSlider_Draw(cSlider *self) { self->Draw(); }
void THISCALL cSlider_DrawHorizontal(cSlider *self) { self->DrawHorizontal(); }
void THISCALL cSlider_DrawVertical(cSlider *self) { self->DrawVertical(); }
void THISCALL cSlider_Toggle(cSlider *self, uint32_t on) { self->Toggle(on & 0xFF); }
void THISCALL cSlider_SetCurValue(cSlider *self, float v) { self->SetCurValue(v); }
void THISCALL cSlider_Highlight(cSlider *self) { self->Highlight(); }
void THISCALL cSlider_UnHighlight(cSlider *self) { self->UnHighlight(); }
void THISCALL PreviewSlider_Toggle(PreviewSlider *self, uint32_t on) { self->Toggle(on & 0xFF); }
void THISCALL PreviewSlider_Draw(PreviewSlider *self) { self->Draw(); }
uint32_t THISCALL ThreeStageSlider_Update(ThreeStageSlider *self, uint32_t msg) { return self->Update(msg); }
void THISCALL ThreeStageSlider_Draw(ThreeStageSlider *self) { self->Draw(); }
uint32_t THISCALL cFEng_LoadResources(cFEng *self, FEPackage *p, int32_t n, FEResourceRequest *r) { return self->LoadResources(p, n, r); }
uint32_t THISCALL cFEng_UnloadResources(cFEng *self, FEPackage *p, int32_t n, FEResourceRequest *r) { return self->UnloadResources(p, n, r); }
void THISCALL cFEng_EndPackageRendering(cFEng *self, FEPackage *p) { self->EndPackageRendering(p); }
void THISCALL cFEng_GenerateRenderContext(cFEng *self, uint32_t c, FEObject *g) { self->GenerateRenderContext(c, g); }
void THISCALL cFEng_RenderObjectList(cFEng *self, void *e, uint32_t n) { self->RenderObjectList(e, n); }
void THISCALL cFEng_RenderObject(cFEng *self, FEObject *o) { self->RenderObject(o); }
void THISCALL cFEng_GetViewTransformation(cFEng *self, void *m) { self->GetViewTransformation(m); }
uint32_t THISCALL cFEng_DoesPointTouchObject(cFEng *self, float x, float y, FEObject *o) { return self->DoesPointTouchObject(x, y, o); }
void THISCALL cFEng_OutputWarning(cFEng *self, const char *t, int32_t l) { self->OutputWarning(t, l); }
void THISCALL cFEng_NotifySoundMessage(cFEng *self, uint32_t a, uint32_t b, uint32_t c, uint32_t d) { self->NotifySoundMessage(a, b, c, d); }
uint8_t *THISCALL cFEng_GetPackageData(cFEng *self, const char *n, uint32_t u, uint8_t *f) { return self->GetPackageData(n, u, f); }
void THISCALL cFEng_BeginPackageRendering(cFEng *self, FEPackage *p, uint32_t m) { self->BeginPackageRendering(p, m & 0xFF); }
uint32_t THISCALL cFEng_GetContextTransform(cFEng *self, uint32_t c, float *m) { return self->GetContextTransform(static_cast<uint16_t>(c), m); }
void THISCALL cFEng_GetMouseInfo(cFEng *self, void *i) { self->GetMouseInfo(i); }
// cFEngRender / FERenderObject / clipping
FERenderObject *THISCALL FERenderObject_Ctor(FERenderObject *self, FEObject *o, void *t) { return self->Construct(o, t); }
void THISCALL FERenderObject_FreePolys(FERenderObject *self) { self->FreePolys(); }
void THISCALL FERenderObject_Clear(FERenderObject *self, uint32_t a) { self->Clear(a); }
FERenderEPoly *THISCALL FERenderObject_AddPoly(FERenderObject *self, float x0, float y0, float x1, float y1, float z, float u0, float v0, float u1, float v1, const uint32_t *c, void *ri, uint32_t f) { return self->AddPoly(x0, y0, x1, y1, z, u0, v0, u1, v1, c, ri, f & 0xFF); }
void THISCALL FERenderObject_AddPolyClipped(FERenderObject *self, float x0, float y0, float x1, float y1, float z, float u0, float v0, float u1, float v1, const uint32_t *c, FEClipInfo *clip, void *ri) { self->AddPolyClipped(x0, y0, x1, y1, z, u0, v0, u1, v1, c, clip, ri); }
void THISCALL FERenderObject_Render(FERenderObject *self) { self->Render(); }
cFEngRender *THISCALL cFEngRender_Ctor(cFEngRender *self) { return self->Construct(); }
void THISCALL cFEngRender_Dtor(cFEngRender *self) { self->Destroy(); }
FERenderObject *THISCALL cFEngRender_CreateCachedRender(cFEngRender *self, FEObject *o, void *t) { return self->CreateCachedRender(o, t); }
void THISCALL cFEngRender_RemoveCachedRender(cFEngRender *self, FEObject *o, uint32_t a) { self->RemoveCachedRender(o, a); }
void THISCALL cFEngRender_GenerateRenderContext(cFEngRender *self, uint32_t c, FEObject *g) { self->GenerateRenderContext(static_cast<uint16_t>(c), g); }
void THISCALL cFEngRender_AddToRenderList(cFEngRender *self, FEObject *o) { self->AddToRenderList(o); }
void THISCALL cFEngRender_PackageFinished(cFEngRender *self, FEPackage *p) { self->PackageFinished(p); }
void THISCALL cFEngRender_RenderObject(cFEngRender *self, FEObject *o, void *ri) { self->RenderObject(o, ri); }
FEClipInfo *THISCALL cFEngRender_MakeRenderMatrix(cFEngRender *self, const void *d, bMatrix4 *m, int32_t *c, uint32_t id, float s) { return self->MakeRenderMatrix(d, m, c, id & 0xFFFF, s); }
void THISCALL cFEngRender_GenerateClipInfo(cFEngRender *self, cFEngRenderContext *c) { self->GenerateClipInfo(c); }
void THISCALL cFEngRender_RenderImage(cFEngRender *self, FEObject *o, FERenderObject *c, void *ri) { self->RenderImage(o, c, ri); }
void THISCALL cFEngRender_RenderCBVImage(cFEngRender *self, FEObject *o, FERenderObject *c, void *ri) { self->RenderCBVImage(o, c, ri); }
void THISCALL cFEngRender_RenderMovie(cFEngRender *self, FEObject *o, FERenderObject *c, void *ri) { self->RenderMovie(o, c, ri); }
void THISCALL cFEngRender_RenderString(cFEngRender *self, FEObject *o, FERenderObject *c, void *ri) { self->RenderString(o, c, ri); }
uint32_t FEClipEdgeXGe(float *ov, float *ouv, float *oc, const float *v, const float *uv, const float *c, uint32_t n, float d) { return FEClipEdge(0, true, ov, ouv, oc, v, uv, c, n, d); }
uint32_t FEClipEdgeYGe(float *ov, float *ouv, float *oc, const float *v, const float *uv, const float *c, uint32_t n, float d) { return FEClipEdge(1, true, ov, ouv, oc, v, uv, c, n, d); }
uint32_t FEClipEdgeXLe(float *ov, float *ouv, float *oc, const float *v, const float *uv, const float *c, uint32_t n, float d) { return FEClipEdge(0, false, ov, ouv, oc, v, uv, c, n, d); }
uint32_t FEClipEdgeYLe(float *ov, float *ouv, float *oc, const float *v, const float *uv, const float *c, uint32_t n, float d) { return FEClipEdge(1, false, ov, ouv, oc, v, uv, c, n, d); }
uint32_t THISCALL FEClipAligned_Hook(void *, const FEClipInfo *k, float *v, float *uv, float *c, float *tv, float *tuv, float *tc) { return FEClipAligned(k, v, uv, c, tv, tuv, tc); }
uint32_t THISCALL FEClipGeneral_Hook(void *, const FEClipInfo *k, float *v, float *uv, float *c, float *tv, float *tuv, float *tc) { return FEClipGeneral(k, v, uv, c, tv, tuv, tc); }
void THISCALL FEQuatToMatrix_Hook(const float *q, bMatrix4 *m) { FEQuatToMatrix(q, m); }
void THISCALL ePoly_Init(ePoly *self) { self->Init(); }
// cFEng unit
cFEng *THISCALL cFEng_Ctor(cFEng *self) { return self->Construct(); }
void THISCALL cFEng_Dtor(cFEng *self) { self->Destroy(); }
cFEng *THISCALL cFEng_ScalarDtor(cFEng *self, uint32_t flags) { self->Destroy(); if (flags & 1) bridge::FreeArray(self); return self; }
void THISCALL cFEng_InitPackage(cFEng *self, FEPackage *p) { self->InitPackage(p); }
uint32_t THISCALL cFEng_PackageWillUnload(cFEng *self, FEPackage *p) { return self->PackageWillUnload(p); }
void THISCALL cFEng_DrawBackground(cFEng *self) { self->DrawBackground(); }
void THISCALL cFEng_RenderPackageSpecial(cFEng *self, FEPackage *p, uint32_t f) { self->RenderPackageSpecial(p, f); }
void THISCALL cFEng_RenderObjectSpecial(cFEng *self, FEObject *o, const float *m, uint32_t c, void *s) { self->RenderObjectSpecial(o, m, static_cast<uint16_t>(c), s); }
void THISCALL cFEng_Service(cFEng *self) { self->Service(); }
void THISCALL cFEng_MakeLoadedPackagesDirty(cFEng *self) { self->MakeLoadedPackagesDirty(); }
uint32_t FEngIsPackagePushed_Hook(const char *n) { return FEngIsPackagePushed(n); }
uint32_t THISCALL FEObjectInitializer_Hook(void *self, FEObject *o) { return FEObjectInitializer_Function(self, o); }
uint32_t THISCALL FEngMovieStarter_Hook(void *self, FEObject *o) { return FEngMovieStarter_Function(self, o); }
uint32_t THISCALL FEngMovieStopper_Hook(void *self, FEObject *o) { return FEngMovieStopper_Function(self, o); }
uint32_t THISCALL RenderObjectDisconnect_Hook(void *self, FEObject *o) { return RenderObjectDisconnect_Function(self, o); }
uint32_t THISCALL FEDirtyMaker_Hook(void *self, FEObject *o) { return FEDirtyMaker_Function(self, o); }
// FEngFont unit
FEngFont *THISCALL FEngFont_Ctor(FEngFont *self, const uint8_t *chunk) { return self->Construct(chunk); }
void THISCALL FEngFont_Dtor(FEngFont *self) { self->Destroy(); }
void FEngFont_NotifyTextureLoading(uint32_t a, uint32_t b) { FEngFont::NotifyTextureLoading(a, b); }
void THISCALL FEngFont_RenderString(FEngFont *self, const int32_t *c, const uint16_t *t, FEObject *o, const bMatrix4 *m, FERenderObject *r, void *i) { self->RenderString(c, t, o, m, r, i); }
void THISCALL FEngFont_RenderCharacter(FEngFont *self, const FEngGlyph *g, float x, float y, const uint32_t *c, FERenderObject *r, void *i, uint32_t z) { self->RenderCharacter(g, x, y, c, r, i, z & 0xFF); }
FEngFont *FEngFindFont_Hook(uint32_t h) { return FEngFindFontNative(h); }
FEButtonWidget *THISCALL FEButtonWidget_Ctor(FEButtonWidget *self, const float *pos, const float *size, bool en) { return self->Construct(pos, size, en); }
void THISCALL FEButtonWidget_Show(FEButtonWidget *self) { self->Show(); }
void THISCALL FEButtonWidget_Hide(FEButtonWidget *self) { self->Hide(); }
void THISCALL FEButtonWidget_SetFocus(FEButtonWidget *self, const char *p) { self->SetFocus(p); }
void THISCALL FEButtonWidget_UnsetFocus(FEButtonWidget *self) { self->UnsetFocus(); }
void THISCALL FEButtonWidget_Position(FEButtonWidget *self) { self->Position(); }
void THISCALL FEButtonWidget_CheckMouse(FEButtonWidget *self, const char *p, float x, float y) { self->CheckMouse(p, x, y); }
FELabelWidget *THISCALL FELabelWidget_Ctor(FELabelWidget *self, const float *pos, const float *size) { return self->Construct(pos, size); }
void THISCALL FELabelWidget_Position(FELabelWidget *self) { self->Position(); }
void THISCALL FELabelWidget_SetFocus(FELabelWidget *self, const char *p) { self->SetFocus(p); }
void THISCALL FEWidget_SetPosVirtual(FEWidget *self, const float *v) { self->SetPosVirtual(v); }
void THISCALL FEWidget_Destruct(FEWidget *self) { self->Destruct(); }
FEWidget *THISCALL FEWidget_ScalarDtor(FEWidget *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL FESliderWidget_Destruct(FESliderWidget *self) { self->Destruct(); }
FESliderWidget *THISCALL FESliderWidget_ScalarDtor(FESliderWidget *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
uint32_t THISCALL FEButtonWidget_DetermineSpecialSound(void *, uint32_t m, uint32_t e) { return FEButtonWidget::DetermineSpecialSound(m, e); }
FEInputWidget *THISCALL FEInputWidget_Ctor(FEInputWidget *self, uint32_t a0, const float *p, const float *sz, const float *vs, const float *vp, const char *t, uint32_t a6, uint32_t en) { return self->Construct(a0, p, sz, vs, vp, t, a6, (en & 0xFF) != 0); }
void THISCALL FEInputWidget_Act(void *, const char *pkg, uint32_t msg) { FEInputWidget::Act(pkg, msg); }
void THISCALL FEInputWidget_CheckMouse(FEInputWidget *self, const char *pkg, float x, float y) { self->CheckMouse(pkg, x, y); }
void THISCALL FEInputWidget_Enable(FEInputWidget *self) { self->Enable(); }
void THISCALL FEInputWidget_Disable(FEInputWidget *self) { self->Disable(); }
void THISCALL FEInputWidget_SetFocus(FEInputWidget *self, const char *pkg) { self->SetFocus(pkg); }
void THISCALL FEInputWidget_UnsetFocus(FEInputWidget *self) { self->UnsetFocus(); }
void THISCALL FEInputWidget_SetInputFocus(FEInputWidget *self) { self->SetInputFocus(); }
FEDateWidget *THISCALL FEDateWidget_Ctor(FEDateWidget *self, const float *p, const float *sz, const float *vs, const float *vp, uint32_t en) { return self->Construct(p, sz, vs, vp, (en & 0xFF) != 0); }
void THISCALL FEDateWidget_SetDate(FEDateWidget *self) { self->SetDate(); }
TuningSlidersMenu *THISCALL TSM_Ctor(TuningSlidersMenu *self, const ScreenConstructorData *d) { return self->Construct(d); }
TuningSlidersMenu *THISCALL TSM_ScalarDtor(TuningSlidersMenu *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL TSM_Dtor(TuningSlidersMenu *self) { self->Destruct(); }
void THISCALL TSM_Notify(TuningSlidersMenu *self, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { self->NotificationMessage(m, o, a, p); }
uint32_t __cdecl TSM_GetTipHash(int32_t t) { return TuningSlidersMenu::GetTipHash(t); }
int32_t THISCALL TSM_AddTuningSlider(TuningSlidersMenu *self, int32_t t, uint32_t lo, uint32_t hi, uint32_t plain, float y) { return self->AddTuningSlider(t, lo, hi, (plain & 0xFF) != 0, y); }
void THISCALL TSM_Setup(TuningSlidersMenu *self) { self->Setup(); }
void THISCALL TSM_SetupSuspension(TuningSlidersMenu *self) { self->SetupSuspension(); }
void THISCALL TSM_SetupDrivetrain(TuningSlidersMenu *self) { self->SetupDrivetrain(); }
void THISCALL TSM_SetupAero(TuningSlidersMenu *self) { self->SetupAero(); }
void THISCALL TSM_SetupWheelBrakes(TuningSlidersMenu *self) { self->SetupWheelBrakes(); }
uint32_t __cdecl FEngMapPortToControlMask_Hook(int32_t p) { return FEngMapPortToControlMask(p); }
UIWheelOptionsMenu *THISCALL UWOM_Ctor(UIWheelOptionsMenu *self, const ScreenConstructorData *d) { return self->Construct(d); }
UIWheelOptionsMenu *THISCALL UWOM_ScalarDtor(UIWheelOptionsMenu *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL UWOM_Dtor(UIWheelOptionsMenu *self) { self->Destruct(); }
void THISCALL UWOM_Notify(UIWheelOptionsMenu *self, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { self->NotificationMessage(m, o, a, p); }
uint32_t THISCALL UWOM_NotifySound(void *, uint32_t m, FEObject *, uint32_t, uint32_t, uint32_t e) { return UIWheelOptionsMenu::NotifySoundMessage(m, e); }
void THISCALL UWOM_Setup(UIWheelOptionsMenu *self) { self->Setup(); }
uint32_t __cdecl FilterSoundForMenu_Hook(uint32_t m, uint32_t e) { return FilterSoundForMenu(m, e); }
bool __cdecl FEngIsScriptRunningId_Hook(const char *pkg, uint32_t h, uint32_t id) { return FEngIsScriptRunning(pkg, h, id); }
void THISCALL WCOWheelRange_Act(WCOWheelRange *self, const char *p, uint32_t m) { self->Act(p, m); }
void THISCALL WCOStrengthOfForce_Act(WCOStrengthOfForce *self, const char *p, uint32_t m) { self->Act(p, m); }
void THISCALL WCOOverall_Act(WCOOverallForceStrength *self, const char *p, uint32_t m) { self->Act(p, m); }
void THISCALL WCOWheelRange_Draw(WCOWheelRange *self) { self->Draw(); }
void THISCALL WCOStrengthOfForce_Draw(WCOStrengthOfForce *self) { self->Draw(); }
void THISCALL WCOOverall_Draw(WCOOverallForceStrength *self) { self->Draw(); }
void THISCALL WCOWheelRange_SetInit(WCOWheelRange *self) { self->SetInitialValues(); }
void THISCALL WCOStrengthOfForce_SetInit(WCOStrengthOfForce *self) { self->SetInitialValues(); }
void THISCALL WCOOverall_SetInit(WCOOverallForceStrength *self) { self->SetInitialValues(); }
long double THISCALL WCOStrengthOfForce_GetValue(const WCOStrengthOfForce *self) { return self->GetValue(); }
ChooseBrandsScreen *THISCALL CBS_Ctor(ChooseBrandsScreen *self, const ScreenConstructorData *d) { return self->Construct(d); }
void THISCALL CBS_Notify(ChooseBrandsScreen *self, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { self->NotificationMessage(m, o, a, p); }
void THISCALL CBS_Setup(ChooseBrandsScreen *self) { self->Setup(); }
void THISCALL CBS_SetChosenBrands(ChooseBrandsScreen *self) { self->SetChosenBrands(); }
BrandScrollerThing *THISCALL BST_Ctor(BrandScrollerThing *self, const uint8_t *part, const float *p, const float *sz, const float *vs, const float *vp, uint32_t en) { return self->Construct(part, p, sz, vs, vp, (en & 0xFF) != 0); }
void THISCALL BST_Act(BrandScrollerThing *self, const char *p, uint32_t m) { self->Act(p, m); }
void THISCALL BST_Draw(BrandScrollerThing *self) { self->Draw(); }
template <int I> void THISCALL AOVol_Act(AOVolumeSlider *self, const char *, uint32_t m) { self->Act(m, kAOVolumes[I].field); }
template <int I> void THISCALL AOVol_Draw(AOVolumeSlider *self) { self->Draw(kAOVolumes[I].hash); }
template <int I> void THISCALL AOVol_Init(AOVolumeSlider *self) { self->SetInitialValues(kAOVolumes[I].field); }
template <int I> void THISCALL OptToggle_Act(OptionToggle *self, const char *, uint32_t m) { OptionToggleAct(*self, kOptionToggles[I], m); }
template <int I> void THISCALL OptToggle_Draw(OptionToggle *self) { OptionToggleDraw(*self, kOptionToggles[I]); }
void THISCALL OptToggle_Enable(OptionToggle *self) { OptionToggleEnable(*self); }
void THISCALL OptToggle_Disable(OptionToggle *self) { OptionToggleDisable(*self); }
void THISCALL OC_AudioMode_Act(OptionCycle *self, const char *, uint32_t msg) { self->AudioModeAct(msg); }
void THISCALL OC_AudioMode_Draw(OptionCycle *self) { self->AudioModeDraw(); }
void THISCALL OC_SpeedoUnits_Act(OptionCycle *self, const char *, uint32_t msg) { self->SpeedoUnitsAct(msg); }
void THISCALL OC_SpeedoUnits_Draw(OptionCycle *self) { self->SpeedoUnitsDraw(); }
void THISCALL OC_RacingMiniMap_Act(OptionCycle *self, const char *, uint32_t msg) { self->RacingMiniMapAct(msg); }
void THISCALL OC_RacingMiniMap_Draw(OptionCycle *self) { self->RacingMiniMapDraw(); }
void THISCALL OC_Difficulty_Act(OptionCycle *self, const char *, uint32_t msg) { self->DifficultyAct(msg); }
void THISCALL OC_Difficulty_Draw(OptionCycle *self) { self->DifficultyDraw(); }
void THISCALL OC_ExploringMiniMap_Act(OptionCycle *self, const char *, uint32_t msg) { self->ExploringMiniMapAct(msg); }
void THISCALL OC_ExploringMiniMap_Draw(OptionCycle *self) { self->ExploringMiniMapDraw(); }
void THISCALL OC_Transmission_Act(OptionCycle *self, const char *, uint32_t msg) { self->TransmissionAct(msg); }
void THISCALL OC_Transmission_Draw(OptionCycle *self) { self->TransmissionDraw(); }
void THISCALL OC_Handling_Act(OptionCycle *self, const char *, uint32_t msg) { self->HandlingAct(msg); }
void THISCALL OC_Handling_Draw(OptionCycle *self) { self->HandlingDraw(); }
void THISCALL OC_DriveCam_Act(OptionCycle *self, const char *, uint32_t msg) { self->DriveCamAct(msg); }
void THISCALL OC_DriveCam_Draw(OptionCycle *self) { self->DriveCamDraw(); }
void THISCALL OC_SplitTime_Act(OptionCycle *self, const char *, uint32_t msg) { self->SplitTimeAct(msg); }
void THISCALL OC_SplitTime_Draw(OptionCycle *self) { self->SplitTimeDraw(); }
void THISCALL OC_JumpCams_Act(OptionCycle *self, const char *, uint32_t msg) { self->JumpCamsAct(msg); }
void THISCALL OC_JumpCams_Draw(OptionCycle *self) { self->JumpCamsDraw(); }
void THISCALL OC_AutoSave_Act(OptionCycle *self, const char *, uint32_t msg) { self->AutoSaveAct(msg); }
void THISCALL OC_AutoSave_Draw(OptionCycle *self) { self->AutoSaveDraw(); }
void THISCALL OC_Resolution_Act(OptionCycle *self, const char *, uint32_t msg) { self->ResolutionAct(msg); }
void THISCALL OC_Resolution_Draw(OptionCycle *self) { self->ResolutionDraw(); }
void THISCALL OC_PPOTransmission_Act(OptionCycle *self, const char *, uint32_t msg) { self->PPOTransmissionAct(msg); }
void THISCALL OC_PPOTransmission_Draw(OptionCycle *self) { self->PPOTransmissionDraw(); }
void THISCALL OC_PPOHandling_Act(OptionCycle *self, const char *, uint32_t msg) { self->PPOHandlingAct(msg); }
void THISCALL OC_PPOHandling_Draw(OptionCycle *self) { self->PPOHandlingDraw(); }
void THISCALL OC_PPODriveCam_Act(OptionCycle *self, const char *, uint32_t msg) { self->PPODriveCamAct(msg); }
void THISCALL OC_PPODriveCam_Draw(OptionCycle *self) { self->PPODriveCamDraw(); }
void THISCALL OC_PPOSplitTime_Act(OptionCycle *self, const char *, uint32_t msg) { self->PPOSplitTimeAct(msg); }
void THISCALL OC_PPOSplitTime_Draw(OptionCycle *self) { self->PPOSplitTimeDraw(); }
void THISCALL Detail_Act(DetailSlider *self, const char *, uint32_t msg) { self->Act(msg); }
void THISCALL Detail_Draw(DetailSlider *self) { self->Draw(); }
void THISCALL Detail_Init(DetailSlider *self) { self->SetInitialValues(); }
void THISCALL OB_WheelAct(OptionButton *self, const char *p, uint32_t m) { self->WheelOptionsAct(p, m); }
void THISCALL OB_WheelDraw(OptionButton *self) { self->WheelOptionsDraw(); }
void THISCALL OB_WheelEnable(OptionButton *self) { self->WheelOptionsEnable(); }
void THISCALL OB_WheelDisable(OptionButton *self) { self->WheelOptionsDisable(); }
void THISCALL OB_ForwardAct(void *, const char *p, uint32_t m) { OptionButton::ForwardAct(p, m); }
void THISCALL OB_792060Draw(OptionButton *self) { self->Button792060Draw(); }
void THISCALL OB_DefaultsAct(void *, const char *p, uint32_t m) { OptionButton::DefaultsAct(p, m); }
void THISCALL OB_DefaultsDraw(OptionButton *self) { self->DefaultsDraw(); }
void THISCALL OB_TrailerAct(void *, const char *p, uint32_t m) { OptionButton::TrailerAct(p, m); }
void THISCALL OB_TrailerDrawSSX(OptionButton *self) { self->TrailerDraw(bridge::TrailerNameSSX()); }
void THISCALL OB_TrailerDrawNFL(OptionButton *self) { self->TrailerDraw(reinterpret_cast<const char *>(0x794508)); }
void THISCALL OB_TrailerDrawNascar(OptionButton *self) { self->TrailerDraw(reinterpret_cast<const char *>(0x794558)); }
UIOptionsScreen *THISCALL UOS_Ctor(UIOptionsScreen *self, const ScreenConstructorData *d) { return self->Construct(d); }
UIOptionsScreen *THISCALL UOS_ScalarDtor(UIOptionsScreen *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL UOS_Dtor(UIOptionsScreen *self) { self->Destruct(); }
void THISCALL UOS_Notify(UIOptionsScreen *self, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { self->NotificationMessage(m, o, a, p); }
uint32_t THISCALL UOS_NotifySound(UIOptionsScreen *self, uint32_t, FEObject *, uint32_t, uint32_t, uint32_t) { return self->NotifySoundMessage(); }
void THISCALL UOS_CheckForWheels(UIOptionsScreen *self) { self->CheckForWheels(); }
void THISCALL UOS_Setup(UIOptionsScreen *self) { self->Setup(); }
void THISCALL UOS_SetupAudio(UIOptionsScreen *self) { self->SetupAudio(); }
void THISCALL UOS_SetupVideo(UIOptionsScreen *self) { self->SetupVideo(); }
void THISCALL UOS_SetupGameplay(UIOptionsScreen *self) { self->SetupGameplay(); }
void THISCALL UOS_SetupPlayer(UIOptionsScreen *self) { self->SetupPlayer(); }
void THISCALL UOS_SetupOnline(UIOptionsScreen *self) { self->SetupOnline(); }
void THISCALL UOS_SetupTrailers(UIOptionsScreen *self) { self->SetupTrailers(); }
IconOption *THISCALL IO_Ctor(IconOption *s, uint32_t n, uint32_t hd, uint32_t ds) { return s->Construct(n, hd, ds); }
CPPaintOption *THISCALL CPP_Ctor(CPPaintOption *s, uint32_t id, uint32_t n, uint32_t hd, uint32_t ds, uint32_t lock) { return s->Construct(id, n, hd, ds, (lock & 0xFF) != 0); }
void THISCALL IO_Dtor(IconOption *s) { s->Destruct(); }
IconOption *THISCALL IO_ScalarDtor(IconOption *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
IconPanel *THISCALL IP_Ctor(IconPanel *s, const char *p, const char *a, const char *b, const char *c, const char *pre, uint32_t wrap) { return s->Construct(p, a, b, c, pre, (wrap & 0xFF) != 0); }
void THISCALL IP_Dtor(IconPanel *s) { s->Destruct(); }
IconPanel *THISCALL IP_ScalarDtor(IconPanel *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL IP_Update(IconPanel *s) { s->Update(); }
FEObject *THISCALL IP_AddOption(IconPanel *s, IconOption *o) { return s->AddOption(o); }
void THISCALL IP_ClearList(IconPanel *s) { s->ClearList(); }
void THISCALL IP_Act(IconPanel *s, uint32_t m, FEObject *o, uint32_t a, uint32_t b) { s->Act(m, o, a, b); }
int32_t THISCALL IP_GetIndex(IconPanel *s) { return s->GetOptionIndex(); }
int32_t THISCALL IP_GetIndexOf(IconPanel *s, const IconOption *o) { return s->GetOptionIndex(o); }
bool THISCALL IP_SetSelIdx(IconPanel *s, int32_t i) { return s->SetSelection(i); }
bool THISCALL IP_SetSelOpt(IconPanel *s, IconOption *o) { return s->SetSelection(o); }
bool THISCALL IP_HandleMouse(IconPanel *s) { return s->HandleMouse(); }
void THISCALL IP_SetInitialPos(IconPanel *s) { s->SetInitialPos(); }
void THISCALL IP_Scroll(IconPanel *s, int32_t d) { s->Scroll(d); }
void THISCALL IP_ScrollWrapped(IconPanel *s, int32_t d) { s->ScrollWrapped(d); }
void THISCALL IP_ScrollNext(IconPanel *s) { s->ScrollNext(); }
void THISCALL IP_ScrollPrev(IconPanel *s) { s->ScrollPrev(); }
void THISCALL IP_AnimateList(IconPanel *s, float *x, float *y) { s->AnimateList(x, y); }
bool THISCALL IP_IsHead(IconPanel *s) { return s->IsHead(); }
bool THISCALL IP_IsTail(IconPanel *s) { return s->IsTail(); }
IconScroller *THISCALL IS_Ctor(IconScroller *s, const char *p, const char *a, const char *b, const char *c, const char *pre, float wd) { return s->Construct(p, a, b, c, pre, wd); }
void THISCALL IS_Update(IconScroller *s) { s->Update(); }
FEObject *THISCALL IS_AddOption(IconScroller *s, IconOption *o) { return s->AddOption(o); }
void THISCALL IS_SetInitialPos(IconScroller *s, int32_t i) { s->SetInitialPos(i); }
bool THISCALL IS_SetSelIdx(IconScroller *s, int32_t i) { return s->SetSelection(i); }
bool THISCALL IS_SetSelOpt(IconScroller *s, IconOption *o) { return s->SetSelection(o); }
void THISCALL IS_StartDim(IconScroller *s) { s->StartDim(); }
void THISCALL IS_StopDim(IconScroller *s) { s->StopDim(); }
void THISCALL IS_ClipEdges(IconScroller *s, IconOption *o, float x) { s->ClipEdges(o, x); }
void THISCALL IS_PositionOption(IconScroller *s, IconOption *o) { s->PositionOption(o); }
void THISCALL IS_UpdateFade(IconScroller *s, IconOption *o, float f) { s->UpdateFade(o, f); }
void THISCALL IS_UpdateArrows(IconScroller *s, int32_t d) { s->UpdateArrows(d); }
bool THISCALL IS_HandleMouse(IconScroller *s) { return s->HandleMouse(); }
IconScrollerMenu *THISCALL ISM_Ctor(IconScrollerMenu *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL ISM_Dtor(IconScrollerMenu *s) { s->Destruct(); }
IconScrollerMenu *THISCALL ISM_ScalarDtor(IconScrollerMenu *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL ISM_Notify(IconScrollerMenu *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
void THISCALL ISM_StartDim(IconScrollerMenu *s) { s->StartDim(); }
void THISCALL ISM_StopDim(IconScrollerMenu *s) { s->StopDim(); }
void THISCALL ISM_StartInput(IconScrollerMenu *s) { s->StartInput(); }
void THISCALL ISM_StopInput(IconScrollerMenu *s) { s->StopInput(); }
void THISCALL ISM_StorePrev(IconScrollerMenu *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->StorePrevNotification(m, o, a, p); }
void THISCALL ISM_RefreshHeader(IconScrollerMenu *s) { s->RefreshHeader(); }
void THISCALL ISM_AddOption(IconScrollerMenu *s, IconOption *o) { s->AddOption(o); }

// ---- ThumbnailScroller ----
ThumbnailScroller *THISCALL TS_Ctor(ThumbnailScroller *s, const char *pkg, const char *master, const char *slot, int32_t align,
                                    const char *lock, const char *button) { return s->Construct(pkg, master, slot, align, lock, button); }
void THISCALL TS_Dtor(ThumbnailScroller *s) { s->Destruct(); }
ThumbnailScroller *THISCALL TS_ScalarDtor(ThumbnailScroller *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL TS_ClearAll(ThumbnailScroller *s) { s->ClearAll(); }
void THISCALL TS_ResetNodes(ThumbnailScroller *s) { s->ResetNodes(); }
void THISCALL TS_MoveMeOffscreen(ThumbnailScroller *s) { s->MoveMeOffscreen(); }
void THISCALL TS_SetVisibility(ThumbnailScroller *s, bool v) { s->SetVisibility(v); }
void THISCALL TS_Scroll(ThumbnailScroller *s, int32_t d) { s->Scroll(d); }
FEObject *THISCALL TS_GetNextNode(ThumbnailScroller *s, FEObject *o) { return s->GetNextNode(o); }
FEObject *THISCALL TS_AddNodeId(ThumbnailScroller *s, uint32_t id, bool dim, bool anim, bool useColor, uint32_t color)
{ return s->AddNode(id, dim, anim, useColor, color); }
FEObject *THISCALL TS_AddNodeObj(ThumbnailScroller *s, ThumbnailNode *n, bool useColor, uint32_t color)
{ return s->AddNode(n, useColor, color); }
void THISCALL TS_SetDesiredItem(ThumbnailScroller *s, uint32_t id) { s->SetDesiredItem(id); }
void THISCALL TS_MoveObjectsTo(ThumbnailScroller *s, float f) { s->MoveObjectsTo(f); }
void THISCALL TS_SnapToNode(ThumbnailScroller *s, ThumbnailNode *n) { s->SnapToNode(n); }
void THISCALL TS_SnapToItem(ThumbnailScroller *s, uint32_t id) { s->SnapToItem(id); }
void THISCALL TS_SnapToHead(ThumbnailScroller *s) { s->SnapToHead(); }
void THISCALL TS_Update(ThumbnailScroller *s) { s->Update(); }
void THISCALL TS_StartDim(ThumbnailScroller *s) { s->StartDim(); }
void THISCALL TS_StopDim(ThumbnailScroller *s) { s->StopDim(); }
ThumbnailNode *THISCALL TS_FindNode(ThumbnailScroller *s, uint32_t id) { return s->FindNode(id); }
int32_t THISCALL TS_GetCurrenNodeIndex(ThumbnailScroller *s) { return s->GetCurrenNodeIndex(); }

// ---- UIQRTrackSelect / TrackMapScroller / SelectableTrack ----
uint32_t THISCALL QTS_CalcIconHash(SelectableTrack *t) { return t->CalcIconHash(); }
void THISCALL TMS_Dtor(TrackMapScroller *m) { m->Destruct(); }
TrackMapScroller *THISCALL TMS_ScalarDtor(TrackMapScroller *m, uint32_t f) { return m->ScalarDeletingDestructor(f); }
void THISCALL TMS_SetTrackMap(TrackMapScroller *m, SelectableTrack *t) { m->SetTrackMap(t); }
void THISCALL TMS_Update(TrackMapScroller *m) { m->Update(); }
void THISCALL TMS_InitZoom(TrackMapScroller *m, float f) { m->InitZoom(f); }
void THISCALL TMS_ZoomIn(TrackMapScroller *m, float f) { m->ZoomIn(f); }
UIQRTrackSelect *THISCALL QTS_Ctor(UIQRTrackSelect *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL QTS_Dtor(UIQRTrackSelect *s) { s->Destruct(); }
UIQRTrackSelect *THISCALL QTS_ScalarDtor(UIQRTrackSelect *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL QTS_Notify(UIQRTrackSelect *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
uint32_t THISCALL QTS_NotifySound(UIQRTrackSelect *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p, uint32_t e) { return s->NotifySoundMessage(m, o, a, p, e); }
void THISCALL QTS_SetSelectedTrack(UIQRTrackSelect *s, int32_t t, int32_t d) { s->SetSelectedTrack(t, d); }
void THISCALL QTS_Build(UIQRTrackSelect *s) { s->BuildPresetTrackList(); }
void THISCALL QTS_AddTrack(UIQRTrackSelect *s, TrackInfo *i) { s->AddTrack(i); }
bool THISCALL QTS_IsAvailable(UIQRTrackSelect *s, const TrackInfo *i, int32_t d) { return s->IsAvailable(i, d); }
void THISCALL QTS_Scroll(UIQRTrackSelect *s, int32_t d) { s->Scroll(d); }
void THISCALL QTS_RefreshHeader(UIQRTrackSelect *s) { s->RefreshHeader(); }
void THISCALL QTS_DisplayRaceType(UIQRTrackSelect *s) { s->DisplayRaceType(); }

// ---- QuickRaceSettings / QR toggles / UIQRModeOptions ----
int32_t THISCALL QRS_GetNumPlayers(QuickRaceSettings *s) { return s->GetNumPlayers(); }
int32_t THISCALL QRS_GetNumOpponents(QuickRaceSettings *s) { return s->GetNumOpponents(); }
int32_t THISCALL QRS_GetAISkill(QuickRaceSettings *s) { return s->GetAISkill(); }
int32_t THISCALL QRS_GetTrafficLevel(QuickRaceSettings *s) { return s->GetTrafficLevel(); }
int32_t THISCALL QRS_GetNumPointsToWinOutrun(QuickRaceSettings *s) { return s->GetNumPointsToWinOutrun(); }
int32_t THISCALL QRS_GetOutrunTimeLimit(QuickRaceSettings *s) { return s->GetOutrunTimeLimit(); }
int32_t THISCALL QRS_GetNumPointsToWinOutrunValue(QuickRaceSettings *s) { return s->GetNumPointsToWinOutrunValue(); }
int32_t THISCALL QRS_GetOutrunTimeLimitValue(QuickRaceSettings *s) { return s->GetOutrunTimeLimitValue(); }
bool THISCALL QRS_HasAPlayerWonTheOutrunTourney(QuickRaceSettings *s) { return s->HasAPlayerWonTheOutrunTourney(); }
bool THISCALL QRS_IsOutrunOvertime(QuickRaceSettings *s) { return s->IsOutrunOvertime(); }
void THISCALL QRS_ResetOutrunScores(QuickRaceSettings *s) { s->ResetOutrunScores(); }
uint8_t THISCALL QRS_IsCatchUpOn(QuickRaceSettings *s) { return s->IsCatchUpOn(); }
QRTrackSettings *THISCALL QRS_GetCurrentTrackSettings(QuickRaceSettings *s) { return s->GetCurrentTrackSettings(); }
void THISCALL QRS_SetNumPlayers(QuickRaceSettings *s, uint32_t n) { s->SetNumPlayers(static_cast<uint8_t>(n)); }
void THISCALL QRS_SetNumOpponents(QuickRaceSettings *s, uint32_t n) { s->SetNumOpponents(static_cast<uint8_t>(n)); }
void THISCALL QRS_SetAISkill(QuickRaceSettings *s, uint32_t n) { s->SetAISkill(static_cast<uint8_t>(n)); }
void THISCALL QRS_SetTrafficLevel(QuickRaceSettings *s, uint32_t n) { s->SetTrafficLevel(static_cast<uint8_t>(n)); }
void THISCALL QRS_SetCatchUp(QuickRaceSettings *s, uint32_t on) { s->SetCatchUp(static_cast<uint8_t>(on)); }
void THISCALL QRS_SetNumPointsToWinOutrun(QuickRaceSettings *s, int32_t i) { s->SetNumPointsToWinOutrun(i); }
void THISCALL QRS_SetOutrunTimeLimit(QuickRaceSettings *s, int32_t i) { s->SetOutrunTimeLimit(i); }
void THISCALL QRS_SetSelectedCar(QuickRaceSettings *s, int32_t car, int32_t player) { s->SetSelectedCar(car, player); }
template <QRToggle K> QRToggleWidget *THISCALL QRT_Ctor(QRToggleWidget *w, const float *p, const float *sz, const float *vs, const float *vp, bool en)
{ return w->Construct(K, p, sz, vs, vp, en); }
template <QRToggle K> void THISCALL QRT_Act(QRToggleWidget *w, const char *pkg, uint32_t m) { w->Act(K, pkg, m); }
template <QRToggle K> void THISCALL QRT_Draw(QRToggleWidget *w) { w->Draw(K); }
UIQRModeOptions *THISCALL QMO_Ctor(UIQRModeOptions *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL QMO_Dtor(UIQRModeOptions *s) { s->Destruct(); }
UIQRModeOptions *THISCALL QMO_ScalarDtor(UIQRModeOptions *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL QMO_Notify(UIQRModeOptions *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
void THISCALL QMO_Setup(UIQRModeOptions *s) { s->Setup(); }
void THISCALL QMO_SetupCircuit(UIQRModeOptions *s) { s->SetupCircuit(); }
void THISCALL QMO_SetupSprint(UIQRModeOptions *s) { s->SetupSprint(); }
void THISCALL QMO_SetupDrag(UIQRModeOptions *s) { s->SetupDrag(); }
void THISCALL QMO_SetupDrift(UIQRModeOptions *s) { s->SetupDrift(); }
void THISCALL QMO_SetupOutrun(UIQRModeOptions *s) { s->SetupOutrun(); }
void THISCALL QMO_SetupGT(UIQRModeOptions *s) { s->SetupGT(); }
void THISCALL QMO_SetupShortTrack(UIQRModeOptions *s) { s->SetupShortTrack(); }
void THISCALL QMO_SetupFreeRun(UIQRModeOptions *s) { s->SetupFreeRun(); }

// ---- PostRaceMenu + pr_* icons ----
PostRaceMenu *THISCALL PRM_Ctor(PostRaceMenu *m, const ScreenConstructorData *d) { return m->Construct(d); }
void THISCALL PRM_Dtor(PostRaceMenu *m) { m->Destruct(); }
PostRaceMenu *THISCALL PRM_ScalarDtor(PostRaceMenu *m, uint32_t f) { return m->ScalarDeletingDestructor(f); }
void THISCALL PRM_Notify(PostRaceMenu *m, uint32_t msg, FEObject *o, uint32_t a, uint32_t p) { m->NotificationMessage(msg, o, a, p); }
void THISCALL PRM_Setup(PostRaceMenu *m) { m->Setup(); }
void THISCALL PRM_SetupRace(PostRaceMenu *m) { m->SetupRace(); }
void PRM_QuitToWorldMap() { PostRaceMenu::QuitToWorldMap(); }   // called with ecx = menu, unused
void THISCALL PRM_QuitRaceToFreeRoam(PostRaceMenu *m) { m->QuitRaceToFreeRoam(); }
void THISCALL PRM_RaceDone(PostRaceMenu *m) { m->QuitRaceToFreeRoamRaceDone(); }
void THISCALL PRM_QuitGame(PostRaceMenu *m) { m->QuitGame(); }
template <PRKind K> void THISCALL PR_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PRReact(K, p, m); }

// ---- CarTunePauseScreen + icons ----
CarTunePauseScreen *THISCALL CTP_Ctor(CarTunePauseScreen *m, const ScreenConstructorData *d) { return m->Construct(d); }
void THISCALL CTP_Dtor(CarTunePauseScreen *m) { m->Destruct(); }
CarTunePauseScreen *THISCALL CTP_ScalarDtor(CarTunePauseScreen *m, uint32_t f) { return m->ScalarDeletingDestructor(f); }
void THISCALL CTP_Notify(CarTunePauseScreen *m, uint32_t msg, FEObject *o, uint32_t a, uint32_t p) { m->NotificationMessage(msg, o, a, p); }
uint32_t THISCALL CTP_NotifySound(void *, uint32_t, void *, uint32_t, uint32_t, uint32_t e) { return CarTunePauseScreen::NotifySoundMessage(e); }
bool THISCALL CTP_IsOkToResume(CarTunePauseScreen *m) { return m->IsOkToResume(); }
void THISCALL CTP_Setup(CarTunePauseScreen *m) { m->Setup(); }
void THISCALL CTP_SetupDyno(CarTunePauseScreen *m) { m->SetupDynoPause(); }
void THISCALL CTP_SetupTuning(CarTunePauseScreen *m) { m->SetupTuningPause(); }
void THISCALL CTP_RefreshHeader(CarTunePauseScreen *m) { m->RefreshHeader(); }
void THISCALL CTP_DoQuitTuning(CarTunePauseScreen *m) { m->DoQuitTuning(); }
template <CTKind K> void THISCALL CT_React(void *, const char *p, uint32_t m, void *, uint32_t a, uint32_t) { CTReact(K, p, m, a); }

// ---- UIMagazineSelect / MagazineThing / MagazineViewer / SCMagazineDatabase ----
void *THISCALL MDB_Car(SCMagazineDatabase *d, uint32_t h) { return d->GetCarSetupByHash(h); }
void *THISCALL MDB_Render(SCMagazineDatabase *d, uint32_t h) { return d->GetRenderSetupByHash(h); }
void *THISCALL MDB_Mag(SCMagazineDatabase *d, uint32_t h) { return d->GetMagazineSetupByHash(h); }
void *THISCALL MDB_MagIndex(SCMagazineDatabase *d, uint32_t i) { return d->GetMagazineSetupByIndex(i); }
void THISCALL MAG_ThingReact(MagazineThing *t, const char *p, uint32_t m, void *, uint32_t, uint32_t) { t->React(p, m); }
UIMagazineSelect *THISCALL MAG_Ctor(UIMagazineSelect *m, const ScreenConstructorData *d) { return m->Construct(d); }
void THISCALL MAG_Dtor(UIMagazineSelect *m) { m->Destruct(); }
UIMagazineSelect *THISCALL MAG_ScalarDtor(UIMagazineSelect *m, uint32_t f) { return m->ScalarDeletingDestructor(f); }
void THISCALL MAG_Notify(UIMagazineSelect *m, uint32_t msg, FEObject *o, uint32_t a, uint32_t p) { m->NotificationMessage(msg, o, a, p); }
void THISCALL MAG_Setup(UIMagazineSelect *m) { m->Setup(); }
void THISCALL MAG_Build(UIMagazineSelect *m) { m->BuildShowcaseList(); }
void THISCALL MAG_Populate(UIMagazineSelect *m) { m->PopulateDataInfoStuff(); }
void THISCALL MAG_Dealloc(UIMagazineSelect *m) { m->DeallocateScreenShot(); }

FEColor *ISX_GetObjectColor(FEColor *out, const FEObject *o) { *out = FEngGetObjectColor(o); return out; }
uint32_t THISCALL ISX_ColorPack(const FEColor *c) { return FEColorPack(*c); }
void THISCALL ISX_StaticArrow2(FEScrollBar *s) { FEScrollBar_StaticArrow2(s); }
UIOptionsMain *THISCALL UOM_Ctor(UIOptionsMain *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL UOM_Dtor(UIOptionsMain *s) { s->Destruct(); }
UIOptionsMain *THISCALL UOM_ScalarDtor(UIOptionsMain *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL UOM_Notify(UIOptionsMain *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
uint32_t THISCALL UOM_NotifySound(void *, uint32_t m, FEObject *, uint32_t, uint32_t, uint32_t e) { return UIOptionsMain::NotifySoundMessage(m, e); }
void THISCALL UOM_Setup(UIOptionsMain *s) { s->Setup(); }
UIPauseOptionsMain *THISCALL UPOM_Ctor(UIPauseOptionsMain *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL UPOM_Dtor(UIPauseOptionsMain *s) { s->Destruct(); }
UIPauseOptionsMain *THISCALL UPOM_ScalarDtor(UIPauseOptionsMain *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL UPOM_Notify(UIPauseOptionsMain *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
void THISCALL UPOM_Setup(UIPauseOptionsMain *s) { s->Setup(); }
void THISCALL OMAudio_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { OMReact(OMKind::Audio, m); }
void THISCALL OMVideo_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { OMReact(OMKind::Video, m); }
void THISCALL OMGameplay_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { OMReact(OMKind::Gameplay, m); }
void THISCALL OMPlayer_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { OMReact(OMKind::Player, m); }
void THISCALL OMController_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { OMReact(OMKind::Controller, m); }
void THISCALL OMEATrax_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { OMReact(OMKind::EATrax, m); }
void THISCALL OMInline_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { OMReact(OMKind::Inline, m); }
void THISCALL OMCredits_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { OMReact(OMKind::Credits, m); }
uint32_t OM_FilterSoundH(uint32_t m, uint32_t e, uint32_t back) { return FilterSoundForHorizontalMenu(m, e, (back & 0xFF) != 0); }
void OM_SetPlayerToEdit(int32_t p) { bridge::SetPlayerToEditForOptions(p); }
bool OM_IsScriptRunningName(const char *p, uint32_t h, const char *n) { return FEngIsScriptRunning(p, h, n); }
PauseMenu *THISCALL PM_Ctor(PauseMenu *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL PM_Dtor(PauseMenu *s) { s->Destruct(); }
PauseMenu *THISCALL PM_ScalarDtor(PauseMenu *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL PM_Notify(PauseMenu *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
void THISCALL PM_Setup(PauseMenu *s) { s->Setup(); }
void THISCALL PM_SetupExplore(PauseMenu *s) { s->SetupExplore(); }
void THISCALL PM_SetupRace(PauseMenu *s) { s->SetupRace(); }
bool THISCALL PM_OkToHandle(PauseMenu *s) { return s->OkToHandleInput(); }
bool THISCALL PM_NeedToSeeOptions(PauseMenu *s) { return s->NeedToSeeOptions(); }
void THISCALL PM_QuitToCrib(PauseMenu *s) { s->QuitToCrib(); }
void THISCALL PM_QuitToWorldMap(PauseMenu *s) { s->QuitToWorldMap(); }
void THISCALL PM_QuitGame(PauseMenu *s) { s->QuitGame(); }
IconOption *THISCALL PM_ReplayCtor(IconOption *o, uint32_t n, uint32_t h, uint32_t d) { return PMReplayConstruct(o, n, h, d); }
IconOption *THISCALL PM_QuitFECtor(IconOption *o, uint32_t n, uint32_t h, uint32_t d) { return PMQuitToFrontEndConstruct(o, n, h, d); }
void THISCALL pm_ResumeRace_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMReact(PMKind::ResumeRace, p, m); }
void THISCALL pm_ResumeFreeRoam_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMReact(PMKind::ResumeFreeRoam, p, m); }
void THISCALL pm_ReturnToWorldMap_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMReact(PMKind::ReturnToWorldMap, p, m); }
void THISCALL pm_RestartRace_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMReact(PMKind::RestartRace, p, m); }
void THISCALL pm_Replay_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMReact(PMKind::Replay, p, m); }
void THISCALL pm_SwitchToOptions_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMReact(PMKind::SwitchToOptions, p, m); }
void THISCALL pm_QuitToCrib_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMReact(PMKind::QuitToCrib, p, m); }
void THISCALL pm_QuitRaceToFreeRoam_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMReact(PMKind::QuitRaceToFreeRoam, p, m); }
void THISCALL pm_QuitToFrontEnd_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMReact(PMKind::QuitToFrontEnd, p, m); }
UIMain *THISCALL UIM_Ctor(UIMain *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL UIM_Dtor(UIMain *s) { s->Destruct(); }
UIMain *THISCALL UIM_ScalarDtor(UIMain *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL UIM_Notify(UIMain *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
void THISCALL UIM_Setup(UIMain *s) { s->Setup(); }
void THISCALL MMQuickRace_React(void *, const char *p, uint32_t m, void *, uint32_t a3, uint32_t) { MMReact(MMKind::QuickRace, p, m, a3); }
void THISCALL MMOnline_React(void *, const char *p, uint32_t m, void *, uint32_t a3, uint32_t) { MMReact(MMKind::Online, p, m, a3); }
void THISCALL MMOptions_React(void *, const char *p, uint32_t m, void *, uint32_t a3, uint32_t) { MMReact(MMKind::Options, p, m, a3); }
void THISCALL MMCarSelect_React(void *, const char *p, uint32_t m, void *, uint32_t a3, uint32_t) { MMReact(MMKind::CarSelect, p, m, a3); }
void THISCALL MMCareer_React(void *, const char *p, uint32_t m, void *, uint32_t a3, uint32_t) { MMReact(MMKind::Career, p, m, a3); }
void THISCALL MMSaveLoad_React(void *, const char *p, uint32_t m, void *, uint32_t a3, uint32_t) { MMReact(MMKind::SaveLoad, p, m, a3); }
void THISCALL MMSplitScreen_React(void *, const char *p, uint32_t m, void *, uint32_t a3, uint32_t) { MMReact(MMKind::SplitScreen, p, m, a3); }
int32_t MM_MapJoy(uint32_t p) { return FEngMapJoyParamToJoyport(p); }
void MM_JoyActive(int32_t p, uint32_t on) { FEngSetJoyPortActive(p, (on & 0xFF) != 0); }
UIQRModeSelect *THISCALL QRMS_Ctor(UIQRModeSelect *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL QRMS_Dtor(UIQRModeSelect *s) { s->Destruct(); }
UIQRModeSelect *THISCALL QRMS_ScalarDtor(UIQRModeSelect *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL QRMS_Notify(UIQRModeSelect *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
void THISCALL QRMS_Setup(UIQRModeSelect *s) { s->Setup(); }
void THISCALL MSCircuit_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { MSReact(MSKind::Circuit, m); }
void THISCALL MSSprint_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { MSReact(MSKind::Sprint, m); }
void THISCALL MSDrag_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { MSReact(MSKind::Drag, m); }
void THISCALL MSDrift_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { MSReact(MSKind::Drift, m); }
void THISCALL MSLan_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { MSReact(MSKind::Lan, m); }
void THISCALL MSFreeRun_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { MSReact(MSKind::FreeRun, m); }
void THISCALL MSShortTrack_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { MSReact(MSKind::ShortTrack, m); }
void THISCALL MSURL_React(void *, const char *, uint32_t m, void *, uint32_t, uint32_t) { MSReact(MSKind::URL, m); }
void QR_SetGameModeHook(int32_t m) { QR_SetGameMode(m); }
void THISCALL USC_Dtor(UIStartCareer *s) { s->Destruct(); }
UIStartCareer *THISCALL USC_ScalarDtor(UIStartCareer *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL USC_Notify(UIStartCareer *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
void THISCALL USC_Setup(UIStartCareer *s) { s->Setup(); }
UIStartCareer *USC_Create(const ScreenConstructorData *d) { return UIStartCareer::Create(d); }
void THISCALL SCNewCareerMemcard_React(void *, const char *p, uint32_t m, void *, uint32_t a3, uint32_t) { SCReact(SCKind::NewCareerMemcard, p, m, a3); }
void THISCALL SCStart_React(void *, const char *p, uint32_t m, void *, uint32_t a3, uint32_t) { SCReact(SCKind::Start, p, m, a3); }
void THISCALL SCLoad_React(void *, const char *p, uint32_t m, void *, uint32_t a3, uint32_t) { SCReact(SCKind::Load, p, m, a3); }
UIProfileManager *THISCALL UPM_Ctor(UIProfileManager *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL UPM_Dtor(UIProfileManager *s) { s->Destruct(); }
UIProfileManager *THISCALL UPM_ScalarDtor(UIProfileManager *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL UPM_Notify(UIProfileManager *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
void THISCALL UPM_Setup(UIProfileManager *s) { s->Setup(); }
UIDeleteProfile *THISCALL UDP_Ctor(UIDeleteProfile *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL UDP_Dtor(UIDeleteProfile *s) { s->Destruct(); }
UIDeleteProfile *THISCALL UDP_ScalarDtor(UIDeleteProfile *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL UDP_Notify(UIDeleteProfile *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
void THISCALL UDP_Setup(UIDeleteProfile *s) { s->Setup(); }
void THISCALL UPM_Refresh(UIProfileManager *s) { s->Refresh(); }
void THISCALL PMSave_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMIconReact(PMIcon::Save, p, m); }
void THISCALL PMLoad_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMIconReact(PMIcon::Load, p, m); }
void THISCALL PMDelete_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMIconReact(PMIcon::Delete, p, m); }
void THISCALL PMCreateNew_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { PMIconReact(PMIcon::CreateNew, p, m); }
uiCareerCrib *THISCALL CCR_Ctor(uiCareerCrib *s, const ScreenConstructorData *d) { return s->Construct(d); }
void THISCALL CCR_Dtor(uiCareerCrib *s) { s->Destruct(); }
uiCareerCrib *THISCALL CCR_ScalarDtor(uiCareerCrib *s, uint32_t f) { return s->ScalarDeletingDestructor(f); }
void THISCALL CCR_Notify(uiCareerCrib *s, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { s->NotificationMessage(m, o, a, p); }
void THISCALL CCR_Setup(uiCareerCrib *s) { s->Setup(); }
void THISCALL CCR_CheckMemory(uiCareerCrib *s) { s->CheckMemory(); }
void THISCALL CCR_DoStartTuning(uiCareerCrib *s) { s->DoStartTuning(); }
void THISCALL CCEnterGameplay_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { CCReact(CCKind::EnterGameplay, p, m); }
void THISCALL CCChangeParts_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { CCReact(CCKind::ChangeParts, p, m); }
void THISCALL CCCarSel_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { CCReact(CCKind::CarSel, p, m); }
void THISCALL CCViewSponsors_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { CCReact(CCKind::ViewSponsors, p, m); }
void THISCALL CCViewWorldMap_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { CCReact(CCKind::ViewWorldMap, p, m); }
void THISCALL CCSaveMemcard_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { CCReact(CCKind::SaveMemcard, p, m); }
void THISCALL CCTestTrack_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { CCReact(CCKind::TestTrack, p, m); }
void THISCALL CCBackToMain_React(void *, const char *p, uint32_t m, void *, uint32_t, uint32_t) { CCReact(CCKind::BackToMain, p, m); }
UIPauseOptions *THISCALL UPO_Ctor(UIPauseOptions *self, const ScreenConstructorData *d) { return self->Construct(d); }
UIPauseOptions *THISCALL UPO_ScalarDtor(UIPauseOptions *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL UPO_Dtor(UIPauseOptions *self) { self->Destruct(); }
bool THISCALL UPO_ReadyToExit(UIPauseOptions *self, uint32_t m) { return self->ReadyToExit(m); }
void THISCALL UPO_Notify(UIPauseOptions *self, uint32_t m, FEObject *o, uint32_t a, uint32_t p) { self->NotificationMessage(m, o, a, p); }
void THISCALL UPO_Setup(UIPauseOptions *self) { self->Setup(); }
void THISCALL UPO_SetupAudio(UIPauseOptions *self) { self->SetupAudio(); }
void THISCALL UPO_SetupVideo(UIPauseOptions *self) { self->SetupVideo(); }
void THISCALL UPO_SetupGameplay(UIPauseOptions *self) { self->SetupGameplay(); }
void THISCALL UPO_SetupPlayer(UIPauseOptions *self) { self->SetupPlayer(); }
FEWidget *THISCALL UIWidgetMenu_GetWidget(UIWidgetMenu *self, uint32_t i) { return self->GetWidget(i); }
FEStatWidget *THISCALL FEStatWidget_Ctor(FEStatWidget *self, const float *a, const float *b, const float *c, const float *d, bool e) { return self->Construct(a, b, c, d, e); }
void THISCALL FEStatWidget_Show(FEStatWidget *self) { self->Show(); }
void THISCALL FEStatWidget_Hide(FEStatWidget *self) { self->Hide(); }
void THISCALL FEStatWidget_SetPosX(FEStatWidget *self, float x) { self->SetPosX(x); }
void THISCALL FEStatWidget_SetPosY(FEStatWidget *self, float y) { self->SetPosY(y); }
void THISCALL FEStatWidget_Position(FEStatWidget *self) { self->Position(); }
long double THISCALL FEngFont_GetTextWidth(FEngFont *self, const uint16_t *t, uint32_t f) { return self->GetTextWidth(t, f); }
long double THISCALL FEngFont_GetTextHeight(FEngFont *self, const uint16_t *t, int32_t s) { return self->GetTextHeight(t, s); }
long double THISCALL FEngFont_GetLineWidth(FEngFont *self, const uint16_t *t, uint32_t f, uint32_t m, bool *tr) { return self->GetLineWidth(t, f, m, tr); }
long double THISCALL FEngFont_GetCharacterWidth(FEngFont *self, uint32_t c, uint32_t prev, uint32_t f) { return self->GetCharacterWidth(static_cast<uint16_t>(c), static_cast<uint16_t>(prev), f); }
uint32_t THISCALL FEngFont_GetNumWordWrappedLines(FEngFont *self, const uint16_t *t, uint32_t f, uint32_t m) { return self->GetNumWordWrappedLines(t, f, m); }
void *THISCALL FEngFont_GetJoyEventTextureInfo(FEngFont *self, const uint16_t *t) { return self->GetJoyEventTextureInfo(t); }
const FEngGlyph *THISCALL FEngFontData_FindCharacter(const FEngFontData *self, uint32_t c) { return self->FindCharacter(static_cast<uint16_t>(c)); }
int32_t THISCALL FEngFontData_GetKerning(const FEngFontData *self, const FEngGlyph *g, uint32_t prev) { return self->GetKerning(g, static_cast<uint16_t>(prev)); }
long double THISCALL FEngFont_CalculateXOffset(void *, uint32_t j, float w) { return FEngFont::CalculateXOffset(j & 0xFF, w); }
long double THISCALL FEngFont_CalculateYOffset(void *, uint32_t j, float h) { return FEngFont::CalculateYOffset(j & 0xFF, h); }
FEJoyPad *THISCALL FEJoyPad_Ctor(FEJoyPad *self) { return self->Construct(); }
void THISCALL FEngine_Dtor(FEngine *self) { self->Destruct(); }
void THISCALL FETypeNode_Dtor(FETypeNode *self) { self->Destruct(); }
FETypeNode *THISCALL FETypeNode_ScalarDtor(FETypeNode *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL FEFieldNode_Dtor(FEFieldNode *self) { self->Destruct(); }
FEFieldNode *THISCALL FEFieldNode_ScalarDtor(FEFieldNode *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
void THISCALL FEButtonMap_ComputeButtonLocation(const FEButtonMap *self, FEObject *o, void *gi, FEVector2 *out) { self->ComputeButtonLocation(o, gi, out); }
FEObject *THISCALL FEButtonMap_GetButtonFrom(const FEButtonMap *self, FEObject *f, uint32_t d, void *gi, uint32_t w) { return self->GetButtonFrom(f, d, gi, w); }
uint32_t THISCALL FEJoyPad_HeldFor(const FEJoyPad *self, uint32_t m) { return self->HeldFor(m); }
void THISCALL FEJoyPad_Update(FEJoyPad *self, uint32_t bits, uint32_t d) { self->Update(bits, d); }
void __cdecl FEngPopChildPackages_Hook(const FEPackage *p) { FEngPopChildPackages(p); }
uint32_t THISCALL cFEng_SetCellData_Hook(void *, void *lb, uint32_t a, uint32_t b) { return cFEng_SetCellData(lb, a, b); }
void __cdecl FEngRenderForeground_Hook() { FEngRenderForeground(); }
void __cdecl FEngRenderBackground_Hook() { FEngRenderBackground(); }
FERefList *THISCALL FERefList_ScalarDtor(FERefList *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
uint32_t THISCALL FEMouse_HeldFor(const FEMouse *self, uint32_t m) { return self->HeldFor(static_cast<uint16_t>(m)); }
void THISCALL FEMouse_Update(FEMouse *self, const FEMouse::Info *i, uint32_t d) { self->Update(*i, d); }
FEPackage *THISCALL FEngine_PushPackageHook(FEngine *self, const char *n, uint32_t prio, uint32_t mask) { return self->PushPackage(n, static_cast<uint8_t>(prio), mask); }
bool THISCALL FEngine_UnloadPackageHook(FEngine *self, FEPackage *p) { return self->UnloadPackage(p); }
void THISCALL FEngine_AddToIdleList(FEngine *self, FEPackage *p) { self->AddToIdleList(p); }
void THISCALL FEngine_RemoveFromLibraryList(FEngine *self, FEPackage *p) { self->RemoveFromLibraryList(p); }
void THISCALL FEngine_UnloadLibraryPackage(FEngine *self, FEPackage *p) { self->UnloadLibraryPackage(p); }
void THISCALL FEngine_AddPackageByPriority(FEMinList *list, FEPackage *p) { reinterpret_cast<FEngine *>(reinterpret_cast<uint8_t *>(list) - offsetof(FEngine, mpActiveListVTable))->AddPackageByPriority(p); }
void THISCALL cFEng_NotificationMessage(cFEng *self, uint32_t m, FEObject *o, uint32_t a, FEPackage *p) { self->NotificationMessage(m, o, a, p); }
void THISCALL cFEng_SwitchFEngPackages(cFEng *self, const char *f, const char *t) { self->SwitchFEngPackages(f, t); }
void THISCALL cFEng_QueuePopPackage(cFEng *self, const char *n) { self->QueuePopPackage(n); }
FENode *THISCALL FEList_FindNode(void *, const char *n, FENode *start) { return FEListFindNode(n, start); }
void THISCALL cFEng_ResyncSortingLevels(cFEng *self) { self->ResyncSortingLevels(); }
void THISCALL cFEng_Popper(cFEng *self, const char *n) { self->Popper(n); }
void THISCALL cFEng_Pusher(cFEng *self, uint32_t f, const char *n, const char *p, uint32_t m, int32_t a) { self->Pusher(f, n, p, m, a); }
void THISCALL cFEng_PopPackage(cFEng *self, const char *n) { self->PopPackage(n); }
void THISCALL cFEng_ForceUnloadPackage(cFEng *self, const char *n) { self->ForceUnloadPackage(n); }
void THISCALL cFEng_Switcher(cFEng *self, const char *f, const char *t, int32_t a) { self->Switcher(f, t, a); }
void THISCALL cFEng_QueuePushPackage(cFEng *self, const char *n, const char *p, uint32_t m) { self->QueuePushPackage(n, p, m); }
void THISCALL cFEng_QueuePushModalPackage(cFEng *self, const char *n, const char *p, uint32_t m) { self->QueuePushModalPackage(n, p, m); }
void __stdcall cFEng_SetButtonState(const char *p, uint32_t h, uint32_t en) { cFEng::SetButtonState(p, h, (en & 0xFF) == 1); }   // ecx unused
void THISCALL FEListBox_ScrollSelection(FEListBox *self, int32_t dx, int32_t dy) { self->ScrollSelection(dx, dy); }
void THISCALL FEListBox_Update(FEListBox *self, float dt) { self->Update(dt); }
void THISCALL FEListBox_CompleteScroll(FEListBox *self) { self->CompleteScroll(); }
cFEngLevelSorter *THISCALL cFEng_FindInLevelSortingList(cFEng *self, const char *n) { return self->FindInLevelSortingList(n); }
bool THISCALL FEngine_PackagePopQueuedPushPackage(FEngine *self, const char *n) { return self->PackagePopQueuedPushPackage(n); }
void THISCALL FEngine_RecordLastPackageButton(FEngine *self, uint32_t h, uint32_t g) { self->RecordLastPackageButton(h, g); }
uint32_t THISCALL FEngine_RecallLastPackageButton(FEngine *self, uint32_t h) { return self->RecallLastPackageButton(h); }
void THISCALL FEngine_QueuePackageSwitch(FEngine *self, const char *n, FEPackage *p) { self->QueuePackageSwitch(n, p); }
void THISCALL FEngine_QueuePackagePush(FEngine *self, const char *n, FEPackage *p, uint32_t m) { self->QueuePackagePush(n, p, m); }
void THISCALL FEngine_QueuePackagePushForModal(FEngine *self, const char *n, FEPackage *p, uint32_t m) { self->QueuePackagePushForModal(n, p, m); }
void THISCALL FEngine_QueuePackagePop(FEngine *self, FEPackage *p) { self->QueuePackagePop(p); }
void THISCALL FEngine_QueuePackageUserTransfer(FEngine *self, FEPackage *p, int f, uint32_t m) { self->QueuePackageUserTransfer(p, (f & 0xFF) != 0, m); }
void THISCALL FEngine_ProcessObjectMessage(FEngine *self, FEObject *o, FEPackage *p, uint32_t msg, uint32_t arg) { self->ProcessObjectMessage(o, p, msg, arg); }
bool THISCALL FEngine_ProcessListBoxResponses(FEngine *self, FEObject *o, FEPackage *p, uint32_t msg) { return self->ProcessListBoxResponses(o, p, msg); }
bool THISCALL FEngine_ProcessCodeListBoxResponses(FEngine *self, FEObject *o, FEPackage *p, uint32_t msg) { return self->ProcessCodeListBoxResponses(o, p, msg); }
void THISCALL FEngine_ProcessResponses(FEngine *self, FEMessageResponse *r, FEObject *o, FEPackage *p, uint32_t arg) { self->ProcessResponses(r, o, p, arg); }
uint32_t THISCALL FEMessageResponse_FindConditionBranchTarget(const FEMessageResponse *self, uint32_t i) { return self->FindConditionBranchTarget(i); }
void THISCALL FEObject_SetCurrentScript(FEObject *self, FEScript *s) { self->SetCurrentScript(s); }
FEScript *THISCALL FEScript_Ctor(FEScript *self) { return FEScript::Construct(self); }
FEMessageResponse *THISCALL FEMessageResponse_Ctor(FEMessageResponse *self) { return FEMessageResponse::Construct(self); }
void THISCALL FEMessageResponse_Dtor(FEMessageResponse *self) { self->Destruct(); }
FEMessageResponse *THISCALL FEMessageResponse_ScalarDtor(FEMessageResponse *self, uint32_t f) { return self->ScalarDeletingDestructor(f); }
FEScript *THISCALL FEObject_FindScript(const FEObject *self, uint32_t id) { return self->FindScript(id); }
FEMessageResponse *THISCALL FEObject_FindResponse(const FEObject *self, uint32_t msg) { return self->FindResponse(msg); }
uint32_t THISCALL FEObject_GetDataOffset(const FEObject *, int track) { return FEObject::GetDataOffset(track); }

}  // namespace

namespace hook {

const Replacement kReplacements[] = {
    // bWare
    {0x43DB50, (const void *)static_cast<uint32_t (*)(const char *)>(&bStringHash),
     "bStringHash(const char *)", {0x8B, 0x54, 0x24, 0x04, 0x8A, 0x0A}},
    {0x43DB80, (const void *)static_cast<uint32_t (*)(const char *, uint32_t)>(&bStringHash),
     "bStringHash(const char *, int)", {0x8B, 0x54, 0x24, 0x04, 0x8A, 0x0A}},
    {0x43DBB0, (const void *)&bStrLen, "bStrLen", {0x8B, 0x4C, 0x24, 0x04, 0x33, 0xC0}},
    {0x43DBD0, (const void *)static_cast<char *(*)(char *, const char *)>(&bStrCpy), "bStrCpy(char *, char const *)", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x8B}},
    {0x43DC00, (const void *)&bStrNCpy, "bStrNCpy", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x8B}},
    {0x43DC30, (const void *)&bStrCmp, "bStrCmp", {0x8B, 0x54, 0x24, 0x04, 0x56, 0x8B}},
    {0x43DC60, (const void *)&bStrNCmp, "bStrNCmp", {0x8B, 0x54, 0x24, 0x08, 0x56, 0x8B}},
    {0x43DCC0, (const void *)&bStrICmp, "bStrICmp", {0x8B, 0x54, 0x24, 0x08, 0x56, 0x8B}},
    {0x43DD10, (const void *)&bStrNICmp, "bStrNICmp", {0x8B, 0x54, 0x24, 0x04, 0x56, 0x8B}},
    {0x43DDA0, (const void *)&bStrCat, "bStrCat", {0x8B, 0x4C, 0x24, 0x08, 0x8A, 0x11}},
    {0x43DDF0, (const void *)&bStrChr, "bStrChr (PC-only)", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x43DE20, (const void *)&bToUpper, "bToUpper", {0x8B, 0x44, 0x24, 0x04, 0x80, 0x38}},
    {0x43DE50, (const void *)&bStrToLong, "bStrToLong", {0x8B, 0x54, 0x24, 0x04, 0x8A, 0x02}},
    {0x43DEF0, (const void *)&bStrToFloat, "bStrToFloat", {0x8B, 0x4C, 0x24, 0x04, 0x8A, 0x01}},
    {0x43BCD0, (const void *)&bCalculateCrc32, "bCalculateCrc32", {0x8B, 0x4C, 0x24, 0x08, 0x85, 0xC9}},
    {0x511630, (const void *)&CopyString, "CopyString<short>", {0x8B, 0x54, 0x24, 0x04, 0x85, 0xD2}},
    {0x43E000, (const void *)static_cast<uint16_t *(*)(uint16_t *, const char *)>(&bStrCpy), "bStrCpy(short *, char const *)", {0x8B, 0x54, 0x24, 0x08, 0x66, 0x0F}},
    // frontend
    {0x51DE90, (const void *)&FEWideString_SetLength, "FEWideString::SetLength", {0x56, 0x8B, 0xF1, 0x8B, 0x0E, 0x33}},
    {0x532670, (const void *)&FEWideString_AssignPacked, "FEWideString::operator=(char const *)", {0x8B, 0x44, 0x24, 0x04, 0x81, 0xEC}},
    {0x535F40, (const void *)&FEString_SetStringWide, "FEString::SetString(short *)", {0x53, 0x55, 0x8B, 0xD9, 0x8B, 0x43}},
    {0x535FE0, (const void *)&FEString_SetStringPacked, "FEString::SetString(char const *)", {0x53, 0x56, 0x57, 0x8B, 0xF9, 0x8B}},
    {0x505450, (const void *)&FEHashUpper, "FEHashUpper", {0x8B, 0x54, 0x24, 0x04, 0x83, 0xC8}},
    {0x51DE80, (const void *)&FEWideString_Ctor, "FEWideString::FEWideString()", {0x8B, 0xC1, 0xC7, 0x00, 0x00, 0x00}},
    {0x537FF0, (const void *)&FEWideString_CopyCtor, "FEWideString::FEWideString(FEWideString const &)", {0x56, 0x8B, 0xF1, 0x57, 0x8B, 0x7C}},
    {0x50D390, (const void *)&FEWideString_Dtor, "FEWideString::~FEWideString", {0x8B, 0x01, 0x85, 0xC0, 0x74, 0x07}},
    {0x495630, (const void *)&FEString_Ctor, "FEString::FEString()", {0x6A, 0xFF, 0x68, 0x88, 0x28, 0x77}},
    {0x554970, (const void *)&FEString_CopyCtor, "FEString::FEString(FEString const &, bool)", {0x6A, 0xFF, 0x68, 0xD3, 0x7E, 0x77}},
    {0x4956A0, (const void *)&FEString_Dtor, "FEString::~FEString", {0x6A, 0xFF, 0x68, 0xB3, 0x28, 0x77}},
    {0x495780, (const void *)&FEString_ScalarDtor, "FEString scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0xFF}},
    {0x495710, (const void *)&FEString_Clone, "FEString::Clone", {0x6A, 0xFF, 0x68, 0xCB, 0x28, 0x77}},
    {0x519F40, (const void *)&FEString_SetLabel, "FEString::SetLabel", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x5C}},
    {0x5053B0, (const void *)&FEMinList_AddNode, "FEMinList::AddNode", {0x8B, 0x44, 0x24, 0x08, 0x85, 0xC0}},
    {0x505400, (const void *)&FEMinList_RemNode, "FEMinList::RemNode", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x511670, (const void *)&FEMinList_Purge, "FEMinList::Purge", {0x56, 0x8B, 0xF1, 0x57, 0x8B, 0x7E}},
    {0x52C290, (const void *)&FEMinList_VecDtor, "FEMinList vector deleting destructor", {0x53, 0x8A, 0x5C, 0x24, 0x08, 0xF6}},
    {0x5176B0, (const void *)&FEObject_SetName, "FEObject::SetName", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x14}},
    {0x5046B0, (const void *)&FEObject_FindScript, "FEObject::FindScript", {0x8B, 0x41, 0x4C, 0x85, 0xC0, 0x74}},
    {0x5046D0, (const void *)&FEObject_FindResponse, "FEObject::FindResponse", {0x8B, 0x41, 0x3C, 0x85, 0xC0, 0x74}},
    // callers keep using ECX after this call (audit_hooks.py): preserve ECX/EDX; __thiscall, 1 stack arg, ret 4
    {0x5046F0, (const void *)&FEObject_GetDataOffset, "FEObject::GetDataOffset", {0x8B, 0x44, 0x24, 0x04, 0x83, 0xF8}, 1, 4, 1},
    {0x4FDB00, (const void *)&FEMinNode_ScalarDtor, "FEMinNode scalar deleting destructor", {0xF6, 0x44, 0x24, 0x04, 0x01, 0x56}},
    {0x517310, (const void *)&FESlotNode_Ctor, "FESlotNode::FESlotNode", {0x6A, 0xFF, 0x68, 0x68, 0x6F, 0x77}},
    {0x5173B0, (const void *)&FESlotNode_Dtor, "FESlotNode::~FESlotNode", {0x6A, 0xFF, 0x68, 0x68, 0x6F, 0x77}},
    {0x517390, (const void *)&FESlotNode_ScalarDtor, "FESlotNode scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x504400, (const void *)&FESlotNode_AllocBlock, "FESlotNode::AllocBlock", {0x56, 0x8B, 0xF1, 0x57, 0x66, 0x8B}},
    {0x517400, (const void *)&FESlotPool_Alloc, "FESlotPool::Alloc", {0x6A, 0xFF, 0x68, 0x6B, 0x80, 0x77}},
    {0x5174A0, (const void *)&FESlotPool_Free, "FESlotPool::Free", {0x56, 0x57, 0x8B, 0xF9, 0x8B, 0x77}},
    {0x534AC0, (const void *)&FESlotPool_Dtor, "FESlotPool::~FESlotPool", {0x6A, 0xFF, 0x68, 0x68, 0x6F, 0x77}},
    {0x534AA0, (const void *)&FESlotPool_ScalarDtor, "FESlotPool scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x52ACE0, (const void *)&FEMultiPool_Alloc, "FEMultiPool::Alloc", {0x53, 0x56, 0x57, 0x8B, 0x7C, 0x24}},
    {0x517520, (const void *)&FEMultiPool_Free, "FEMultiPool::Free", {0x53, 0x57, 0x8B, 0x7C, 0x24, 0x0C}},
    {0x4FE910, (const void *)&FEResponse_Ctor, "FEResponse::FEResponse", {0x8B, 0xC1, 0x33, 0xC9, 0x89, 0x08}},
    {0x517600, (const void *)&FEResponse_Release, "FEResponse::ReleaseParam (= ~FEResponse)", {0x56, 0x8B, 0xF1, 0x8B, 0x06, 0x3D}},
    {0x517630, (const void *)&FEResponse_SetParam, "FEResponse::SetParam", {0x53, 0x8B, 0xD9, 0x8B, 0x03, 0x3D}},
    {0x52AD80, (const void *)&FEResponse_Assign, "FEResponse::operator=", {0x56, 0x8B, 0xF1, 0x8B, 0x06, 0x3D}},
    {0x52AE00, (const void *)&FEMessageResponse_SetCount, "FEMessageResponse::SetCount", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x4951B0, (const void *)&FEMinList_AddTail, "FEMinList::AddTail", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x51}},
    {0x54BDF0, (const void *)&MsgRespPool_New, "FEMessageResponse::operator new (pool)", {0x6A, 0xFF, 0x68, 0x6B, 0x80, 0x77}},
    {0x525B30, (const void *)&MsgRespPool_Delete, "FEMessageResponse::operator delete (pool)", {0x56, 0x57, 0x8B, 0xF9, 0x8B, 0x77}},
    {0x54BEB0, (const void *)&ScriptPool_New, "FEScript::operator new (pool)", {0x6A, 0xFF, 0x68, 0x6B, 0x80, 0x77}},
    {0x525B90, (const void *)&ScriptPool_Delete, "FEScript::operator delete (pool)", {0x56, 0x57, 0x8B, 0xF9, 0x8B, 0x77}},
    {0x4FE920, (const void *)&FEMessageResponse_Ctor, "FEMessageResponse::FEMessageResponse", {0x8B, 0xC1, 0xB9, 0xFE, 0xCA, 0xAD}},
    {0x534B10, (const void *)&FEMessageResponse_Dtor, "FEMessageResponse::~FEMessageResponse", {0x6A, 0xFF, 0x68, 0x68, 0x6F, 0x77}},
    {0x53EE60, (const void *)&FEMessageResponse_ScalarDtor, "FEMessageResponse scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0xA8, 0x5C}},
    {0x53E710, (const void *)&MsgRespBlock_Ctor, "FEPoolNode<FEMessageResponse,64> ctor", {0x6A, 0xFF, 0x68, 0x68, 0x6F, 0x77}},
    {0x53EB40, (const void *)&MsgRespBlock_Dtor, "FEPoolNode<FEMessageResponse,64> dtor", {0x6A, 0xFF, 0x68, 0xCE, 0x78, 0x77}},
    {0x53EB20, (const void *)&MsgRespBlock_ScalarDtor, "FEPoolNode<FEMessageResponse,64> scalar deleting dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x53E910, (const void *)&ScriptBlock_Ctor, "FEPoolNode<FEScript,32> ctor", {0x6A, 0xFF, 0x68, 0x68, 0x6F, 0x77}},
    {0x53EC40, (const void *)&ScriptBlock_Dtor, "FEPoolNode<FEScript,32> dtor", {0x6A, 0xFF, 0x68, 0xFE, 0x78, 0x77}},
    {0x53EC20, (const void *)&ScriptBlock_ScalarDtor, "FEPoolNode<FEScript,32> scalar deleting dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x511D70, (const void *)&FEScript_Ctor, "FEScript::FEScript()", {0x8B, 0xC1, 0xB9, 0xFE, 0xCA, 0xAD}},
    {0x5055C0, (const void *)&FERefList_RemNode, "FERefList::RemNode", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x511D00, (const void *)&FERefList_Purge, "FERefList::Purge", {0x56, 0x57, 0x8B, 0xF9, 0x8B, 0x77}},
    {0x52B960, (const void *)&FEKeyTrack_Dtor, "FEKeyTrack::~FEKeyTrack", {0x6A, 0xFF, 0x68, 0xBB, 0x74, 0x77}},
    {0x535170, (const void *)&FEScript_Dtor, "FEScript::~FEScript", {0x6A, 0xFF, 0x68, 0xD3, 0x7B, 0x77}},
    {0x53EE30, (const void *)&FEScript_ScalarDtor, "FEScript scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x38, 0x63}},
    {0x525E80, (const void *)&FEKeyTrack_Ctor, "FEKeyTrack::FEKeyTrack()", {0x8B, 0xC1, 0x8B, 0x50, 0x04, 0x33}},
    {0x54D350, (const void *)&FEKeyTrack_Assign, "FEKeyTrack::operator=", {0x53, 0x55, 0x56, 0x8B, 0xF1, 0x57}},
    {0x519A10, (const void *)&FEScriptEvents_SetCount, "FEScript event list SetCount", {0x53, 0x8B, 0xD9, 0x8B, 0x03, 0x55}},
    {0x518730, (const void *)&FEScript_SetName, "FEScript::SetName", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x535210, (const void *)&FEScript_SetTrackCount, "FEScript::SetTrackCount", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x551BE0, (const void *)&FEScript_CopyCtor, "FEScript::FEScript(const FEScript&, bool)", {0x6A, 0xFF, 0x68, 0xD3, 0x7B, 0x77}},
    {0x4FE8B0, (const void *)&FEKeyNode_Ctor, "FEKeyNode::FEKeyNode()", {0x8B, 0xC1, 0xB9, 0xFE, 0xCA, 0xAD}},
    {0x531FE0, (const void *)&FEKeyNode_ScalarDtor, "FEKeyNode scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x28, 0x8B}},
    {0x53E650, (const void *)&KeyNodePool_New, "FEKeyNode::operator new (pool)", {0x6A, 0xFF, 0x68, 0x6B, 0x80, 0x77}},
    {0x525BF0, (const void *)&KeyNodePool_Delete, "FEKeyNode::operator delete (pool)", {0x56, 0x57, 0x8B, 0xF9, 0x8B, 0x77}},
    {0x5319C0, (const void *)&KeyNodeBlock_Ctor, "FEPoolNode<FEKeyNode,256> ctor", {0x6A, 0xFF, 0x68, 0x68, 0x6F, 0x77}},
    {0x531BE0, (const void *)&KeyNodeBlock_Dtor, "FEPoolNode<FEKeyNode,256> dtor", {0x6A, 0xFF, 0x68, 0x71, 0x75, 0x77}},
    {0x531BC0, (const void *)&KeyNodeBlock_ScalarDtor, "FEPoolNode<FEKeyNode,256> scalar deleting dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x554300, (const void *)&FEObject_CopyCtor, "FEObject::FEObject(const FEObject&, bool)", {0x6A, 0xFF, 0x68, 0x89, 0x7E, 0x77}},
    {0x5545D0, (const void *)&FEObject_Clone, "FEObject::Clone", {0x6A, 0xFF, 0x68, 0x6B, 0x80, 0x77}},
    {0x517720, (const void *)&FEObject_SetupMoveToTracks, "FEObject::SetupMoveToTracks", {0x83, 0xEC, 0x40, 0x53, 0x8B, 0xD9}, 1, 0, 1},   // 52749B: push ecx after the call (stack slot; thunk anyway)
    {0x4FE890, (const void *)&FERefList_GetHead, "FERefList::GetHead", {0x8A, 0x41, 0x04, 0x84, 0xC0, 0x74}},
    {0x4FE6B0, (const void *)&FEQuat_Multiply, "FEQuaternion multiply", {0x83, 0xEC, 0x10, 0x8B, 0x44, 0x24}},
    {0x505580, (const void *)&FEColor_Sub, "FEColor::operator-", {0x8B, 0x51, 0x08, 0x8B, 0x44, 0x24}},
    {0x504A00, (const void *)&FEScript_FindTrack, "FEScript::FindTrack", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x51}},
    {0x5178D0, (const void *)&FEObject_SetTrackVec3, "FEObject::SetTrackValue(FEVector3)", {0x51, 0x53, 0x8B, 0x5C, 0x24, 0x10}},
    {0x5179D0, (const void *)&FEObject_SetTrackVec2, "FEObject::SetTrackValue(FEVector2)", {0x51, 0x56, 0x8B, 0x74, 0x24, 0x10}},
    {0x517AD0, (const void *)&FEObject_SetTrackColor, "FEObject::SetTrackValue(FEColor)", {0x51, 0x53, 0x8B, 0x59, 0x4C, 0x85}},
    {0x517C10, (const void *)&FEObject_SetPosition, "FEObject::SetPosition", {0x83, 0xEC, 0x0C, 0x56, 0x8B, 0xF1}, 1, 8, 1},   // 4EC40C: push ecx = stack slot (thunk anyway)
    {0x517C80, (const void *)&FEObject_SetRotation, "FEObject::SetRotation", {0x83, 0xEC, 0x14, 0x81, 0x79, 0x18}},
    {0x517DB0, (const void *)&FEObject_SetSize, "FEObject::SetSize", {0x83, 0xEC, 0x0C, 0x56, 0x8B, 0xF1}},
    {0x517E20, (const void *)&FEObject_SetColor, "FEObject::SetColor", {0x83, 0xEC, 0x10, 0x56, 0x8B, 0xF1}},
    {0x504500, (const void *)&FEVector3_Close, "FEVector3 Close (copy 1)", {0x8B, 0x4C, 0x24, 0x08, 0xD9, 0x01}},
    {0x5045A0, (const void *)&FEVector3_Close, "FEVector3 Close (copy 2)", {0x8B, 0x4C, 0x24, 0x08, 0xD9, 0x01}},
    {0x504640, (const void *)&FEColor_Close, "FEColor Close", {0x8B, 0x4C, 0x24, 0x04, 0x8B, 0x44}},
    {0x505260, (const void *)&FEKeyTrack_GetKeyAt, "FEKeyTrack::GetKeyAt", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x0C}},
    {0x5052B0, (const void *)&FEKeyTrack_GetDeltaKeyAt, "FEKeyTrack::GetDeltaKeyAt", {0x8A, 0x41, 0x2C, 0x84, 0xC0, 0x74}},
    {0x4FE8D0, (const void *)&FEKeyTrack_GetFirstKey, "FEKeyTrack first key", {0x8A, 0x41, 0x2C, 0x84, 0xC0, 0x74}},
    {0x519D40, (const void *)&FEInterpNone_Hook, "FEInterpNone", {0x8B, 0x4C, 0x24, 0x04, 0x8B, 0x41}},
    {0x5354F0, (const void *)&FEKeyInterpFast_Hook, "FEKeyInterpFast", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x8A}},
    {0x535570, (const void *)&FEKeyTrack_SetBaseValueAt, "FEKeyTrack::SetBaseValueAt", {0x83, 0xEC, 0x10, 0x53, 0x56, 0x8B}},
    {0x52BDC0, (const void *)&FEInterpLinear_Hook, "FEInterpLinear", {0x83, 0xEC, 0x18, 0x53, 0x55, 0x8B}},
    {0x5050D0, (const void *)&FELerpInteger, "FELerpInteger", {0x8B, 0x44, 0x24, 0x08, 0x56, 0x8B}},
    {0x505110, (const void *)&FELerpVector2, "FELerpVector2", {0x8B, 0x4C, 0x24, 0x08, 0x8B, 0x44}},
    {0x505150, (const void *)&FELerpVector3, "FELerpVector3", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x4C}},
    {0x5051A0, (const void *)&FELerpColor, "FELerpColor", {0x53, 0x55, 0x56, 0x8B, 0x74, 0x24}},
    {0x43C140, (const void *)&bRandomIS_Hook, "bRandom(int, unsigned int *)", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x43C170, (const void *)&bRandomFS_Hook, "bRandom(float, unsigned int *)", {0x56, 0x8B, 0x74, 0x24, 0x0C, 0x8B}},
    {0x43C1C0, (const void *)&bRandomI_Hook, "bRandom(int)", {0x8B, 0x0D, 0x24, 0xA6, 0x7E, 0x00}},
    {0x43C1F0, (const void *)&bRandomF_Hook, "bRandom(float)", {0x51, 0x8B, 0x0D, 0x24, 0xA6, 0x7E}},
    {0x43C240, (const void *)&bFMod_Hook, "bFMod", {0xD9, 0x44, 0x24, 0x08, 0xD9, 0xE1}},   // audit 5D45CA: push ecx = stack slot (fstp [esp])
    {0x43C280, (const void *)&bSignedMod_Hook, "bSignedMod", {0x56, 0x8B, 0x74, 0x24, 0x0C, 0x85}},
    {0x43CBD0, (const void *)&bMatrixToQuaternion, "bMatrixToQuaternion", {0x8B, 0x4C, 0x24, 0x08, 0xD9, 0x41}},
    {0x43C7C0, (const void *)&bInvertMatrix, "bInvertMatrix", {0x83, 0xEC, 0x38, 0x8B, 0x44, 0x24}},
    {0x43C780, (const void *)&bIdentity, "bIdentity", {0x8B, 0x44, 0x24, 0x04, 0x33, 0xC9}},
    {0x4013F0, (const void *)&eMulMatrix, "eMulMatrix", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x43C990, (const void *)&bMulMatrix, "bMulMatrix", {0x8B, 0x44, 0x24, 0x08, 0x8B, 0x4C}},
    {0x43C9D0, (const void *)&bTransposeMatrix, "bTransposeMatrix", {0x83, 0xEC, 0x30, 0x8B, 0x44, 0x24}},
    {0x43CE10, (const void *)&bDistBetween_Hook, "bDistBetween", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x4C}},
    {0x43CE50, (const void *)&bNormalize2_Hook, "bNormalize(bVector2)", {0x8B, 0x4C, 0x24, 0x08, 0xD9, 0x01}},
    {0x43CEB0, (const void *)&bNormalize2S_Hook, "bNormalize(bVector2, scale)", {0x8B, 0x4C, 0x24, 0x08, 0xD9, 0x01}},
    {0x43CF10, (const void *)&bNormalize3_Hook, "bNormalize(bVector3)", {0x8B, 0x4C, 0x24, 0x08, 0xD9, 0x41}},
    {0x43CF90, (const void *)&bNormalize3S_Hook, "bNormalize(bVector3, scale)", {0x8B, 0x4C, 0x24, 0x08, 0xD9, 0x41}},
    {0x43D010, (const void *)&bNormalize4_Hook, "bNormalize(bVector4)", {0x8B, 0x4C, 0x24, 0x08, 0xD9, 0x41}},
    {0x43D090, (const void *)&bScaleAdd, "bScaleAdd(bVector2)", {0x8B, 0x44, 0x24, 0x0C, 0xD9, 0x44}},
    {0x43D150, (const void *)&bEqual2_Hook, "bEqual(bVector2)", {0x8B, 0x4C, 0x24, 0x04, 0xD9, 0x01}},
    {0x43D190, (const void *)&bEqual3_Hook, "bEqual(bVector3)", {0x8B, 0x4C, 0x24, 0x04, 0xD9, 0x01}},
    {0x43D1E0, (const void *)&bCross, "bCross", {0x8B, 0x4C, 0x24, 0x0C, 0x8B, 0x44}},
    {0x43D220, (const void *)&bInitBB0_Hook, "bInitializeBoundingBox", {0x8B, 0x44, 0x24, 0x04, 0xC7, 0x00}},
    {0x43D250, (const void *)&bInitBB1_Hook, "bInitializeBoundingBox(p)", {0x8B, 0x44, 0x24, 0x0C, 0xD9, 0x00}},
    {0x43D270, (const void *)&bExpandBB_Hook, "bExpandBoundingBox(p)", {0x8B, 0x54, 0x24, 0x0C, 0xD9, 0x02}},
    {0x43D2F0, (const void *)&bExpandBBR_Hook, "bExpandBoundingBox(p, r)", {0x8B, 0x54, 0x24, 0x0C, 0xD9, 0x02}},
    {0x43D380, (const void *)&bExpandBB2_Hook, "bExpandBoundingBox(p, q)", {0x8B, 0x4C, 0x24, 0x0C, 0xD9, 0x01}},
    {0x43CD10, (const void *)&bIsSlotPoolFull, "bIsSlotPoolFull", {0x8B, 0x4C, 0x24, 0x04, 0x8B, 0x41}},
    {0x43CD20, (const void *)&bCountFreeSlots, "bCountFreeSlots", {0x8B, 0x4C, 0x24, 0x04, 0x8B, 0x41}},
    {0x43CD30, (const void *)&bCountTotalSlots, "bCountTotalSlots", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x40}},
    {0x43CD40, (const void *)&SlotPool_Flush, "SlotPool::FlushSlotPool", {0x85, 0xC9, 0x74, 0x39, 0x56, 0x57}},
    {0x43CD80, (const void *)&SlotPool_GetSlotNumber, "SlotPool::GetSlotNumber", {0x53, 0x56, 0x33, 0xF6, 0x85, 0xC9}},
    {0x43CDD0, (const void *)&SlotPool_FastMalloc, "SlotPool::FastMalloc", {0x8B, 0x51, 0x18, 0x8B, 0x41, 0x10}},
    {0x43CDF0, (const void *)&SlotPool_Free, "SlotPool::Free", {0x8B, 0x41, 0x18, 0x8B, 0x54, 0x24}},
    {0x440C30, (const void *)&SlotPool_GetAllocatedSlot, "SlotPool::GetAllocatedSlot", {0x51, 0x53, 0x55, 0x56, 0x57, 0x6A}},
    {0x440DA0, (const void *)&SlotPoolManager_Delete, "SlotPoolManager::DeleteSlotPool", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x440DF0, (const void *)&bNewSlotPool, "bNewSlotPool", {0x8B, 0x44, 0x24, 0x10, 0x8B, 0x4C}},
    {0x440E40, (const void *)&bDeleteSlotPool, "bDeleteSlotPool", {0x8B, 0x44, 0x24, 0x04, 0x50, 0xB9}},
    {0x43F8F0, (const void *)&bOMalloc, "bOMalloc(SlotPool *)", {0x8B, 0x4C, 0x24, 0x04, 0x8B, 0x51}},
    {0x43F920, (const void *)static_cast<void (*)(SlotPool *, void *)>(&bFree), "bFree(SlotPool *, void *)", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x48}},
    {0x440E50, (const void *)&bMalloc, "bMalloc(SlotPool *)", {0x8B, 0x4C, 0x24, 0x04, 0xE9, 0xE7}},   // audit 423279: setne dl + byte store (false positive)
    {0x440E60, (const void *)&bPListInit, "bPListInit", {0xA1, 0xF0, 0x84, 0x82, 0x00, 0x85}},
    {0x440EC0, (const void *)&bPNode_Malloc, "bPNode::Malloc", {0xA1, 0xF0, 0x84, 0x82, 0x00, 0x85}},
    {0x440EF0, (const void *)&bPNode_Free, "bPNode::Free", {0x8B, 0x0D, 0xF0, 0x84, 0x82, 0x00}},
    {0x440B40, (const void *)&SlotPool_New, "SlotPool::NewSlotPool", {0x53, 0x8B, 0x5C, 0x24, 0x14, 0x55}},
    {0x440BB0, (const void *)&SlotPool_Expand, "SlotPool::ExpandSlotPool", {0x53, 0x56, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x440D40, (const void *)&SlotPool_Malloc, "SlotPool::Malloc", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x10}},
    {0x43C570, (const void *)&bFastATan_Hook, "bFastATan", {0xD9, 0x44, 0x24, 0x04, 0x56, 0xD8}},
    {0x43C650, (const void *)&bFixATan1_Hook, "bFixATan(int)", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x57}},
    {0x43C6F0, (const void *)&bFixATan2_Hook, "bFixATan(int, int)", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x57}},
    {0x440F50, (const void *)&bTan_Hook, "bTan (PC-only)", {0x51, 0x56, 0x8B, 0x74, 0x24, 0x0C}},
    {0x440F70, (const void *)&bLength_Hook, "bLength(bVector3) (PC-only)", {0x8B, 0x44, 0x24, 0x04, 0xD9, 0x40}},
    {0x4412E0, (const void *)&Camera_FovRelativeAngle, "Camera::FovRelativeAngle", {0x51, 0x33, 0xC0, 0x66, 0x8B, 0x81}},
    {0x4411B0, (const void *)&NoiseInterpolated_Hook, "NoiseInterpolated", {0x51, 0xD9, 0x44, 0x24, 0x08, 0xE8}},
    {0x441270, (const void *)&Noise_Hook, "Noise", {0x83, 0xEC, 0x0C, 0x56, 0xC7, 0x44}},
    {0x43C370, (const void *)&bCos_Hook, "bCos", {0x8B, 0x44, 0x24, 0x04, 0x05, 0x00}},
    {0x43C390, (const void *)&bSinCos_Hook, "bSinCos", {0x83, 0xEC, 0x0C, 0x0F, 0xB7, 0x44}},
    {0x43C2B0, (const void *)&bSin_Hook, "bSin(angle)", {0x0F, 0xB7, 0x44, 0x24, 0x04, 0x89}, 1, 4, 0},   // 43F70A reuses ECX after the call (real)
    {0x43C350, (const void *)&bSinF_Hook, "bSin(float)", {0xD9, 0x44, 0x24, 0x04, 0xD8, 0x0D}},
    {0x43C490, (const void *)&bASin_Hook, "bASin", {0x51, 0xD9, 0x44, 0x24, 0x08, 0x53}},
    {0x4FE770, (const void *)&FEQuatScale_Hook, "FEQuaternion scale", {0x83, 0xEC, 0x10, 0x8B, 0x44, 0x24}},
    {0x4FE7D0, (const void *)&FEQuatAdd_Hook, "FEQuaternion add", {0x83, 0xEC, 0x10, 0x8B, 0x44, 0x24}},
    {0x4FE830, (const void *)&FEQuatSub_Hook, "FEQuaternion sub", {0x83, 0xEC, 0x10, 0x8B, 0x44, 0x24}},
    {0x511C80, (const void *)&FEQuatNormalize, "FEQuaternion normalize", {0x8B, 0x4C, 0x24, 0x04, 0xD9, 0x41}},
    {0x519B00, (const void *)&FELerpQuaternion, "FELerpQuaternion", {0x83, 0xEC, 0x44, 0x8B, 0x4C, 0x24}},
    {0x541DB0, (const void *)&FEPackage_UpdateObjectTracks, "FEPackage::UpdateObjectTracks", {0x8A, 0x41, 0x14, 0x84, 0xC0, 0x8B}},
    {0x54CE60, (const void *)&FEPackage_UpdateObject, "FEPackage::UpdateObject", {0x51, 0x55, 0x8B, 0x6C, 0x24, 0x0C}},
    {0x504910, (const void *)&FEPackage_UpdateGroup, "FEPackage::UpdateGroup", {0x53, 0x8B, 0x5C, 0x24, 0x08, 0x56}},
    {0x54D190, (const void *)&FEPackage_Update, "FEPackage::Update", {0x8B, 0x44, 0x24, 0x04, 0x53, 0x8B}},
    {0x534C70, (const void *)&FEPackage_IssueScriptMessages, "FEPackage::IssueScriptMessages", {0x51, 0x8B, 0x44, 0x24, 0x10, 0x8B}},
    {0x504880, (const void *)&FEPackage_ForAllObjects, "FEPackage::ForAllObjects", {0x53, 0x56, 0x8B, 0xD9, 0x8B, 0x73}},
    {0x504830, (const void *)&FEPackage_ForAllChildren, "FEPackage::ForAllChildren", {0x8B, 0x44, 0x24, 0x04, 0x53, 0x56}},
    {0x517FE0, (const void *)&FEPackage_FindObjectByGUID, "FEPackage::FindObjectByGUID", {0x6A, 0xFF, 0x68, 0x88, 0x6F, 0x77}},
    {0x517F80, (const void *)&FEPackage_FindObjectByHash, "FEPackage::FindObjectByHash", {0x6A, 0xFF, 0x68, 0x88, 0x6F, 0x77}},
    {0x5047E0, (const void *)&FEPackage_SetControlMask, "FEPackage::SetControlMask", {0x8B, 0x44, 0x24, 0x04, 0x89, 0x41}},
    {0x504960, (const void *)&FEPackage_FindLibraryReference, "FEPackage::FindLibraryReference", {0x8B, 0x91, 0x84, 0x00, 0x00, 0x00}},
    {0x52B3A0, (const void *)&FEPackage_SetCurrentButton, "FEPackage::SetCurrentButton", {0x8A, 0x44, 0x24, 0x0C, 0x84, 0xC0}},
    {0x51AA20, (const void *)&FEngine_QueueMessage, "FEngine::QueueMessage", {0x56, 0x57, 0x6A, 0x20, 0x8B, 0xF9}},
    {0x51CCD0, (const void *)&cFEng_FindInLevelSortingList, "cFEng::FindInLevelSortingList", {0x53, 0x56, 0x8B, 0x71, 0x10, 0x57}},
    {0x517230, (const void *)&FEListBox_Update, "FEListBox::Update", {0xD9, 0x44, 0x24, 0x04, 0x83, 0xEC}},
    {0x5043B0, (const void *)&FEListBox_CompleteScroll, "FEListBox::CompleteScroll", {0x8B, 0x81, 0x94, 0x00, 0x00, 0x00}},
    {0x516E90, (const void *)&FEListBox_ScrollSelection, "FEListBox::ScrollSelection", {0x51, 0x8B, 0x51, 0x5C, 0x8B, 0xC2}},
    {0x5559C0, (const void *)&FEngine_ProcessPackageCommands, "FEngine::ProcessPackageCommands", {0x83, 0xEC, 0x10, 0x53, 0x55, 0x33}},
    {0x51AE80, (const void *)&FEngine_PackagePopQueuedPushPackage, "FEngine::PackagePopQueuedPushPackage", {0x8B, 0x44, 0x24, 0x04, 0x53, 0x55}},
    {0x51B070, (const void *)&FEngine_RecordPackageMarker, "FEngine::RecordPackageMarker", {0x8B, 0x81, 0x48, 0x41, 0x00, 0x00}},
    {0x50B390, (const void *)&FEngine_ClearPackageMarkers, "FEngine::ClearPackageMarkers", {0x8D, 0x81, 0x4C, 0x41, 0x00, 0x00}},
    {0x50B300, (const void *)&FEngine_RecordLastPackageButton, "FEngine::RecordLastPackageButton", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x8D}},
    {0x50B360, (const void *)&FEngine_RecallLastPackageButton, "FEngine::RecallLastPackageButton", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x33}},
    {0x519DE0, (const void *)&FENode_SetName, "FENode::SetName", {0x53, 0x56, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x51AB20, (const void *)&FEngine_QueuePackageSwitch, "FEngine::QueuePackageSwitch", {0x53, 0x55, 0x56, 0x57, 0x6A, 0x20}},
    {0x51AC10, (const void *)&FEngine_QueuePackagePush, "FEngine::QueuePackagePush", {0x53, 0x55, 0x56, 0x57, 0x6A, 0x20}},
    {0x51ACE0, (const void *)&FEngine_QueuePackagePushForModal, "FEngine::QueuePackagePushForModal", {0x55, 0x56, 0x57, 0x6A, 0x20, 0x8B}},
    {0x51ADB0, (const void *)&FEngine_QueuePackagePop, "FEngine::QueuePackagePop", {0x53, 0x56, 0x57, 0x6A, 0x20, 0x8B}},
    {0x51AF50, (const void *)&FEngine_QueuePackageUserTransfer, "FEngine::QueuePackageUserTransfer", {0x56, 0x6A, 0x20, 0x8B, 0xF1, 0xE8}},
    {0x51BB70, (const void *)&FEPkgMgr_FindByName, "FEPackageManager::FindFEPackageData(name)", {0x53, 0x56, 0x8B, 0x74, 0x24, 0x0C}},
    {0x51BB30, (const void *)&FEPkgMgr_FindByChunk, "FEPackageManager::FindFEPackageData(chunk)", {0x8B, 0x41, 0x04, 0x85, 0xC0, 0x8D}},
    {0x51BC40, (const void *)&FEPkgMgr_GetPackageData, "FEPackageManager::GetPackageData", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x50}},
    {0x51BAD0, (const void *)&FEPkgMgr_Checksum, "FEPackageManager::GetActiveScreensChecksum", {0x8B, 0x41, 0x04, 0x53, 0x56, 0x57}},
    {0x51BB20, (const void *)&FEPkgMgr_GetActiveScreensChecksum, "FEPkgMgr_GetActiveScreensChecksum", {0xB9, 0xB0, 0x37, 0x86, 0x00, 0xE9}},
    {0x50B920, (const void *)&FEPackageData_GetNameHash, "FEPackageData::GetNameHash", {0x8B, 0x51, 0x10, 0x8B, 0x02, 0x3D}},
    {0x55DC20, (const void *)&FEPkgMgr_SendMessageToPackage, "FEPackageManager::SendMessageToPackage", {0x8B, 0x44, 0x24, 0x04, 0x50, 0xE8}},
    {0x55DC50, (const void *)&FEPkgMgr_BroadcastMessage, "FEPackageManager::BroadcastMessage", {0x8B, 0x41, 0x04, 0x81, 0xEC, 0x80}},
    {0x55DCE0, (const void *)&FEPkgMgr_NotificationMessage, "FEPackageManager::NotificationMessage", {0x8B, 0x41, 0x04, 0x85, 0xC0, 0x53}},
    {0x51B8E0, (const void *)&FEPackageData_Ctor, "FEPackageData::FEPackageData", {0x33, 0xC0, 0x56, 0x8B, 0xF1, 0x8B}},
    {0x51B990, (const void *)&FEPackageData_Dtor, "FEPackageData::~FEPackageData", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x10}},
    {0x52CC20, (const void *)&FEPackageData_ScalarDtor, "FEPackageData scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x68, 0xED}},
    {0x51B9D0, (const void *)&FEPackageData_Activate, "FEPackageData::Activate", {0x56, 0x8B, 0xF1, 0x57, 0x8B, 0x7C}},
    {0x51BA60, (const void *)&FEPackageData_UnActivate, "FEPackageData::UnActivate", {0x56, 0x8B, 0xF1, 0x8B, 0x4E, 0x14}},
    {0x542340, (const void *)&FEPackageData_Close, "FEPackageData::Close", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x18}},
    {0x542390, (const void *)&FEPkgMgr_CloseAllPackages, "FEPackageManager::CloseAllPackages", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8}},
    {0x51BCA0, (const void *)&FEPkgMgr_Loader, "FEPackageManager::Loader", {0x6A, 0xFF, 0x68, 0x34, 0x71, 0x77}},
    {0x542530, (const void *)&FEPkgMgr_UnLoader, "FEPackageManager::UnLoader", {0x56, 0x68, 0x04, 0xC0, 0x79, 0x00}},
    {0x51BD30, (const void *)&LoaderFEngPackage, "LoaderFEngPackage", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x8B}},
    {0x5425A0, (const void *)&UnloaderFEngPackage, "UnloaderFEngPackage", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x8B}},
    {0x558EE0, (const void *)&JustUpdateFeng, "JustUpdateFeng", {0xA1, 0xC4, 0x84, 0x83, 0x00, 0x85}},
    {0x52B100, (const void *)&FEPackage_Ctor, "FEPackage::FEPackage", {0x8B, 0xC1, 0xC7, 0x00, 0xF8, 0xB8}},
    {0x52B1E0, (const void *)&FEPackage_Dtor, "FEPackage::~FEPackage", {0x6A, 0xFF, 0x68, 0xA5, 0x74, 0x77}},
    {0x534C50, (const void *)&FEPackage_ScalarDtor, "FEPackage scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x88, 0x65}},
    {0x517F30, (const void *)&FEPackage_SetFilename, "FEPackage::SetFilename", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x38}},
    {0x52B370, (const void *)&FEPackage_Startup, "FEPackage::Startup", {0x53, 0x56, 0x8B, 0xF1, 0x8B, 0x56}},
    {0x518140, (const void *)&FEPackage_ConnectObjectResources, "FEPackage::ConnectObjectResources", {0x6A, 0xFF, 0x68, 0x88, 0x6F, 0x77}},
    {0x505350, (const void *)&FENode_Dtor, "FENode::~FENode", {0x6A, 0xFF, 0x68, 0x68, 0x6F, 0x77}},
    {0x555720, (const void *)&FEPackageReader_Load, "FEPackageReader::Load", {0x8A, 0x44, 0x24, 0x18, 0x8A, 0x54}},
    {0x52B4D0, (const void *)&FEPackageReader_ReadHeaderChunk, "FEPackageReader::ReadHeaderChunk", {0x56, 0x8B, 0xF1, 0x57, 0x8B, 0x7E}},
    {0x5049A0, (const void *)&FEPackageReader_ReadTypeSizes, "FEPackageReader::ReadTypeSizes", {0x8B, 0x41, 0x04, 0x8B, 0x50, 0x04}, 1, 0, 0},
    {0x5182E0, (const void *)&FEPackageReader_ReadLibraryRefsChunk, "FEPackageReader::ReadLibraryRefsChunk", {0x53, 0x8B, 0xD9, 0x8B, 0x43, 0x04}},
    {0x518380, (const void *)&FEPackageReader_ReadResourceChunk, "FEPackageReader::ReadResourceChunk", {0x51, 0x53, 0x8B, 0xD9, 0x8B, 0x43}},
    {0x551580, (const void *)&FEPackageReader_ReadPackageResponseChunk, "FEPackageReader::ReadPackageResponseChunk", {0x8B, 0x41, 0x04, 0x8B, 0x50, 0x04}},
    {0x54D1D0, (const void *)&FEPackageReader_ReadMessageResponseTags, "FEPackageReader::ReadMessageResponseTags", {0x51, 0x8B, 0x44, 0x24, 0x0C, 0x53}},
    {0x518190, (const void *)&FEPackage_SetNumLibraryRefs, "FEPackage::SetNumLibraryRefs", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x52C570, (const void *)&FEngine_FindObject, "FEngine::FindObject", {0x8B, 0x44, 0x24, 0x08, 0x85, 0xC0}},
    {0x50B2A0, (const void *)&FEngine::IsInputMessage, "FEngine::IsInputMessage", {0x8B, 0x4C, 0x24, 0x04, 0x85, 0xC9}},
    {0x558C00, (const void *)&FEngine_UpdateHook, "FEngine::Update", {0x83, 0xEC, 0x0C, 0x55, 0x8B, 0xE9}},
    {0x54D4B0, (const void *)&FEngine_ProcessPadsForPackage, "FEngine::ProcessPadsForPackage", {0x8B, 0x44, 0x24, 0x04, 0x81, 0xEC}},
    {0x54E020, (const void *)&FEngine_ProcessMouseForPackage, "FEngine::ProcessMouseForPackage", {0x83, 0xEC, 0x18, 0x56, 0x57, 0x8B}},
    {0x504A50, (const void *)&FEQuaternion_GetMatrix, "FEQuaternion::GetMatrix", {0x83, 0xEC, 0x20, 0xD9, 0x01, 0x8B}, 1, 4, 1},   // 52C639 (original RenderGroup) reuses EDX
    {0x5187A0, (const void *)static_cast<void (*)(FEMatrix4 *, const FEMatrix4 *, const FEMatrix4 *)>(&FEMultMatrix), "FEMultMatrix(FEMatrix4)", {0x83, 0xEC, 0x40, 0x8B, 0x44, 0x24}},
    {0x50B410, (const void *)&InitFEngObjectSorting, "InitFEngObjectSorting", {0x8B, 0x4C, 0x24, 0x0C, 0x8B, 0x44}},
    {0x50B430, (const void *)&CloseFEngObjectSorting, "CloseFEngObjectSorting", {0xA1, 0x90, 0x84, 0x83, 0x00, 0x8B}},
    {0x5114F0, (const void *)&FEObjectSorter_SortObjects, "FEObjectSorter<1024>::SortObjects", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8}},
    {0x51A970, (const void *)&FEngine_RenderObject, "FEngine::RenderObject", {0x83, 0xEC, 0x18, 0x56, 0x8B, 0x74}},
    {0x52C5D0, (const void *)&FEngine_RenderGroup, "FEngine::RenderGroup", {0x81, 0xEC, 0x98, 0x00, 0x00, 0x00}},
    {0x536520, (const void *)&FEngine_Render, "FEngine::Render", {0xB8, 0x04, 0x11, 0x00, 0x00, 0xE8}},
    {0x519EB0, (const void *)&FETypeNode_AddField, "FETypeNode::AddField", {0x56, 0x57, 0x6A, 0x24, 0x8B, 0xF9}},
    {0x514780, (const void *)&FETypeNode_AddFieldNode, "FETypeNode::AddFieldNode", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x519F10, (const void *)&FETypeNode_GetField, "FETypeNode::GetField", {0x56, 0x8B, 0x71, 0x1C, 0x85, 0xF6}},
    {0x505480, (const void *)&FETypeLib_FindType, "FETypeLib::FindType", {0x8B, 0x41, 0x08, 0x85, 0xC0, 0x74}},
    {0x535610, (const void *)&FETypeLib_CreateBaseObjectType, "FETypeLib::CreateBaseObjectType", {0x83, 0xEC, 0x38, 0x53, 0x55, 0x56}},
    {0x5358D0, (const void *)&FETypeLib_CreateImageObjectType, "FETypeLib::CreateImageObjectType", {0x83, 0xEC, 0x08, 0x8B, 0x44, 0x24}},
    {0x5359D0, (const void *)&FETypeLib_Startup, "FETypeLib::Startup", {0x83, 0xEC, 0x14, 0x53, 0x57, 0x68}},
    {0x536170, (const void *)&FEngine_Ctor, "FEngine::FEngine", {0x6A, 0xFF, 0x68, 0x54, 0x76, 0x77}},
    {0x51A640, (const void *)&FEngine_ReleaseAll, "FEngine::ReleaseAll", {0x53, 0x56, 0x57, 0x8B, 0xF1, 0x33}},
    {0x51A740, (const void *)&FEngine_SetNumJoyPads, "FEngine::SetNumJoyPads", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x52CDD0, (const void *)&cFEng_RegisterJoy, "cFEng::RegisterJoy", {0xA1, 0xB8, 0x84, 0x83, 0x00, 0x85}},
    {0x50C680, (const void *)&cFEng_UnRegisterJoy, "cFEng::UnRegisterJoy", {0xA1, 0xB8, 0x84, 0x83, 0x00, 0x85}},
    {0x50C6D0, (const void *)&cFEng_GetJoyPadMask, "cFEng::GetJoyPadMask", {0x53, 0x8A, 0x5C, 0x24, 0x08, 0x56}},
    {0x50C630, (const void *)&cFEng::HandleJoy, "cFEng::HandleJoy", {0x53, 0x8A, 0x5C, 0x24, 0x10, 0x56}},
    {0x50C980, (const void *)&FEngGetTextureHash, "FEngGetTextureHash", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x50C990, (const void *)static_cast<void (*)(FEObject *, uint32_t)>(&FEngSetTextureHash), "FEngSetTextureHash", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x50C9B0, (const void *)&FEngSetMovieName, "FEngSetMovieName", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x50C9E0, (const void *)&FEngIsVisible, "FEngIsVisible", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x50CA00, (const void *)static_cast<void (*)(FEObject *)>(&FEngSetInvisible), "FEngSetInvisible", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}, 1, 4, 0},   // widget Show/Hide (50D530..) keep `this` in EDX
    {0x50CA50, (const void *)static_cast<void (*)(FEObject *)>(&FEngSetVisible), "FEngSetVisible", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}, 1, 4, 0},   // widget Show/Hide (50D530..) keep `this` in EDX
    {0x50CAB0, (const void *)&FEngIsScriptFinished, "FEngIsScriptFinished", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x50CB00, (const void *)&FEngSetSize, "FEngSetSize", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x51CF70, (const void *)static_cast<void (*)(FEObject *, const char *, bool)>(&FEngSetScript), "FEngSetScript(name)", {0x57, 0x8B, 0x7C, 0x24, 0x08, 0x85}},
    {0x51CFE0, (const void *)static_cast<void (*)(FEObject *, uint32_t, bool)>(&FEngSetScript), "FEngSetScript(id)", {0x57, 0x8B, 0x7C, 0x24, 0x08, 0x85}},
    {0x51D040, (const void *)static_cast<bool (*)(const FEObject *, const char *)>(&FEngIsScriptRunning), "FEngIsScriptRunning(name)", {0x8B, 0x44, 0x24, 0x08, 0x50, 0xE8}},
    {0x51CED0, (const void *)&FEngSetRotationZ, "FEngSetRotationZ", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x51D070, (const void *)&FEngGetTopLeft, "FEngGetTopLeft", {0x51, 0x53, 0x56, 0x8B, 0x74, 0x24}},
    {0x51D170, (const void *)&FEngGetBottomRight, "FEngGetBottomRight", {0x81, 0xEC, 0x08, 0x08, 0x00, 0x00}},
    {0x51D2D0, (const void *)&FEngSetBottomRight, "FEngSetBottomRight", {0x83, 0xEC, 0x18, 0x56, 0x8B, 0x74}},
    {0x51D380, (const void *)&FEngSetTopLeft, "FEngSetTopLeft", {0x83, 0xEC, 0x1C, 0x56, 0x8B, 0x74}},
    {0x51D4D0, (const void *)&FEngSetCenter, "FEngSetCenter", {0x83, 0xEC, 0x10, 0x56, 0x8B, 0x74}},
    {0x51D620, (const void *)&FEngGetCenter, "FEngGetCenter", {0x51, 0x56, 0x8B, 0x74, 0x24, 0x0C}},
    {0x51D730, (const void *)&FEngGetSize, "FEngGetSize", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x51CD10, (const void *)&FEngSetUseIdleList, "FEngSetUseIdleList", {0xA1, 0xC4, 0x84, 0x83, 0x00, 0x85}},
    {0x51CD40, (const void *)&FEngSetPermanent, "FEngSetPermanent", {0x8B, 0x44, 0x24, 0x04, 0x50, 0xB9}},
    {0x51CD60, (const void *)&FEngGetCurrentButton, "FEngGetCurrentButton", {0xA1, 0xC4, 0x84, 0x83, 0x00, 0x85}},
    {0x51CDA0, (const void *)&FEngSetButtonState, "FEngSetButtonState", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x51CDC0, (const void *)&FEngPostMessageToFEng, "FEngPostMessageToFEng", {0xA1, 0xC4, 0x84, 0x83, 0x00, 0x85}},
    {0x51CDF0, (const void *)&FEngQueueControllerMessageToPackage, "FEngQueueControllerMessageToPackage", {0x8B, 0x4C, 0x24, 0x08, 0x57, 0x8B}},
    {0x51CE60, (const void *)&FEngQueueMessageToPackage, "FEngQueueMessageToPackage", {0x8B, 0x44, 0x24, 0x08, 0x8B, 0x4C}},
    {0x51CE80, (const void *)&FEngSetPackageVisibility, "FEngSetPackageVisibility", {0x8B, 0x44, 0x24, 0x04, 0x50, 0xB9}},
    {0x5379C0, (const void *)&FEngFindObject, "FEngFindObject", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x537A60, (const void *)static_cast<FEObject *(*)(const char *, uint32_t)>(&FEngFindImage), "FEngFindImage", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x542630, (const void *)&FEngSetPosition, "FEngSetPosition", {0x8B, 0x44, 0x24, 0x08, 0x56, 0x57}},
    {0x5426C0, (const void *)static_cast<void (*)(const char *, const char *, const char *, bool)>(&FEngSetScript), "FEngSetScript(pkg, obj, name)", {0x8B, 0x44, 0x24, 0x08, 0x56, 0x57}},
    {0x537C00, (const void *)static_cast<void (*)(const char *, uint32_t, const char *, bool)>(&FEngSetScript), "FEngSetScript(pkg, hash, name)", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x537C60, (const void *)static_cast<void (*)(const char *, uint32_t, uint32_t, bool)>(&FEngSetScript), "FEngSetScript(pkg, hash, id)", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x52CE40, (const void *)&cFEng_SetCurrentButton, "cFEng::SetCurrentButton", {0x8B, 0x44, 0x24, 0x04, 0x55, 0x8B}},
    {0x52CEA0, (const void *)static_cast<void (*)(const char *, uint32_t)>(&FEngSetCurrentButton), "FEngSetCurrentButton(hash)", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x52CEC0, (const void *)static_cast<void (*)(const char *, const FEObject *)>(&FEngSetCurrentButton), "FEngSetCurrentButton(obj)", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x542AC0, (const void *)&FEScrollBar_Ctor, "FEScrollBar::FEScrollBar", {0x8A, 0x44, 0x24, 0x0C, 0x8A, 0x54}},
    {0x538080, (const void *)&FEScrollBar_Update, "FEScrollBar::Update", {0x53, 0x8B, 0x5C, 0x24, 0x08, 0x56}},
    {0x52E200, (const void *)&FEScrollBar_SetPosResized, "FEScrollBar::SetPosResized", {0x51, 0xDB, 0x44, 0x24, 0x08, 0x56}},
    {0x52E320, (const void *)&FEScrollBar_SetPosNonResized, "FEScrollBar::SetPosNonResized", {0x51, 0x56, 0x8B, 0xF1, 0x8A, 0x0E}},
    {0x50D820, (const void *)&FEScrollBar_SetArrowVisibility, "FEScrollBar::SetArrowVisibility", {0x8A, 0x44, 0x24, 0x08, 0x84, 0xC0}},
    {0x50D880, (const void *)&FEScrollBar_SetBackingVisibility, "FEScrollBar::SetBackingVisibility", {0x8A, 0x44, 0x24, 0x04, 0x8B, 0xD1}},
    {0x4953E0, (const void *)&FEngFindImageByName, "FEngFindImage(name)", {0x8B, 0x44, 0x24, 0x08, 0x50, 0xE8}},
    {0x495590, (const void *)&FEngSetWidth, "FEngSetWidth", {0x83, 0xEC, 0x08, 0x56, 0x8B, 0x74}},
    {0x525DB0, (const void *)&FEngSetHeight, "FEngSetHeight", {0x51, 0x56, 0x8B, 0x74, 0x24, 0x0C}},
    {0x51E840, (const void *)&FESliderWidget_Ctor, "FESliderWidget::FESliderWidget", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x54}},
    {0x50D6F0, (const void *)&FESliderWidget_DetermineSpecialSound, "FESliderWidget::DetermineSpecialSound", {0x8B, 0x44, 0x24, 0x08, 0x8B, 0xD0}},
    {0x52DEF0, (const void *)&FESliderWidget_Position, "FESliderWidget::Position", {0x83, 0xEC, 0x10, 0x56, 0x8B, 0xF1}},
    {0x50D740, (const void *)&FESliderWidget_Show, "FESliderWidget::Show", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x50D780, (const void *)&FESliderWidget_Hide, "FESliderWidget::Hide", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x52DFF0, (const void *)&FESliderWidget_SetFocus, "FESliderWidget::SetFocus", {0x53, 0x55, 0x8B, 0xE9, 0x8B, 0x0D}},
    {0x51E8D0, (const void *)&FESliderWidget_UnsetFocus, "FESliderWidget::UnsetFocus", {0x53, 0x55, 0x56, 0x8B, 0xE9, 0x57}},
    {0x51E230, (const void *)&FEToggleWidget_Ctor, "FEToggleWidget::FEToggleWidget", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x54}},
    {0x52D9F0, (const void *)&FEToggleWidget_CheckMouse, "FEToggleWidget::CheckMouse", {0x53, 0x8B, 0x5C, 0x24, 0x0C, 0x56}},
    {0x52DAC0, (const void *)&FEToggleWidget_Position, "FEToggleWidget::Position", {0x83, 0xEC, 0x0C, 0x56, 0x8B, 0xF1}},
    {0x51E270, (const void *)&FEToggleWidget_Enable, "FEToggleWidget::Enable", {0x53, 0x55, 0x56, 0x8B, 0xE9, 0x57}},
    {0x51E3E0, (const void *)&FEToggleWidget_Disable, "FEToggleWidget::Disable", {0x53, 0x55, 0x56, 0x8B, 0xE9, 0x57}},
    {0x50D5D0, (const void *)&FEToggleWidget_Show, "FEToggleWidget::Show", {0x8B, 0xD1, 0x8B, 0x42, 0x2C, 0x50}},
    {0x50D610, (const void *)&FEToggleWidget_Hide, "FEToggleWidget::Hide", {0x8B, 0xD1, 0x8B, 0x42, 0x2C, 0x50}},
    {0x52DB40, (const void *)&FEToggleWidget_SetFocus, "FEToggleWidget::SetFocus", {0x53, 0x55, 0x8B, 0xE9, 0x8B, 0x0D}},
    {0x51E550, (const void *)&FEToggleWidget_UnsetFocus, "FEToggleWidget::UnsetFocus", {0x53, 0x55, 0x56, 0x8B, 0xE9, 0x57}},
    {0x51E6B0, (const void *)&FEToggleWidget_BlinkArrows, "FEToggleWidget::BlinkArrows", {0x8B, 0x44, 0x24, 0x04, 0x3D, 0x9E}},
    {0x52DCC0, (const void *)&FEToggleImageWidget_Position, "FEToggleImageWidget::Position", {0x83, 0xEC, 0x14, 0x53, 0x56, 0x57}},
    {0x51E6F0, (const void *)&FEToggleImageWidget_Enable, "FEToggleImageWidget::Enable", {0x56, 0x57, 0x8B, 0xF1, 0xE8, 0x77}},
    {0x51E760, (const void *)&FEToggleImageWidget_Disable, "FEToggleImageWidget::Disable", {0x56, 0x57, 0x8B, 0xF1, 0xE8, 0x77}},
    {0x50D650, (const void *)&FEToggleImageWidget_Show, "FEToggleImageWidget::Show", {0x8B, 0xD1, 0x8B, 0x42, 0x2C, 0x50}},
    {0x50D6A0, (const void *)&FEToggleImageWidget_Hide, "FEToggleImageWidget::Hide", {0x8B, 0xD1, 0x8B, 0x42, 0x2C, 0x50}},
    {0x52DE80, (const void *)&FEToggleImageWidget_SetFocus, "FEToggleImageWidget::SetFocus", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x57}},
    {0x51E7D0, (const void *)&FEToggleImageWidget_UnsetFocus, "FEToggleImageWidget::UnsetFocus", {0x56, 0x57, 0x8B, 0xF1, 0xE8, 0x77}},
    {0x522AF0, (const void *)&TuningSlider_Ctor, "TuningSlider::TuningSlider", {0x8B, 0x44, 0x24, 0x20, 0x8B, 0x54}},
    {0x530D20, (const void *)&TuningSlider_SetSliderObjects, "TuningSlider::SetSliderObjects", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x54}},
    {0x530AA0, (const void *)&TuningSlider_Position, "TuningSlider::Position", {0x83, 0xEC, 0x1C, 0x53, 0x56, 0x8B}},
    {0x522BC0, (const void *)&TuningSlider_Hide, "TuningSlider::Hide", {0x8B, 0xD1, 0x8B, 0x42, 0x2C, 0x50}},
    {0x522C30, (const void *)&TuningSlider_Show, "TuningSlider::Show", {0x8B, 0xD1, 0x8B, 0x42, 0x2C, 0x50}},
    {0x530F30, (const void *)&TuningSlider_SetFocus, "TuningSlider::SetFocus", {0x8B, 0x44, 0x24, 0x04, 0x53, 0x55}},
    {0x522CA0, (const void *)&TuningSlider_UnsetFocus, "TuningSlider::UnsetFocus", {0x53, 0x55, 0x56, 0x57, 0x8B, 0xD9}},
    {0x530D90, (const void *)&TuningSlider_InitValues, "TuningSlider::InitValues", {0x83, 0xEC, 0x08, 0x56, 0x8B, 0xF1}},
    {0x53CD30, (const void *)&TuningSlider_Draw, "TuningSlider::Draw", {0x53, 0x56, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x564F10, (const void *)&TuningSlider_Act, "TuningSlider::Act", {0x55, 0x8B, 0xEC, 0x51, 0x53, 0x57}},
    {0x565050, (const void *)&TuningSlider_CheckMouse, "TuningSlider::CheckMouse", {0x53, 0x8B, 0x5C, 0x24, 0x0C, 0x55}},
    {0x552780, (const void *)&UIWidgetMenu_Ctor, "UIWidgetMenu::UIWidgetMenu", {0x6A, 0xFF, 0x68, 0xD3, 0x7C, 0x77}},
    {0x4CBC30, (const void *)&UIWidgetMenu_DtorBody, "UIWidgetMenu::~UIWidgetMenu body", {0x6A, 0xFF, 0x68, 0x18, 0x64, 0x77}},
    {0x4CBC90, (const void *)&UIWidgetMenu_ScalarDtor, "UIWidgetMenu scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x98, 0xFF}},
    {0x51FF40, (const void *)&UIWidgetMenu_RefreshWidgets, "UIWidgetMenu::RefreshWidgets", {0x8B, 0x41, 0x4C, 0x85, 0xC0, 0x56}},
    {0x51FF80, (const void *)&UIWidgetMenu_SetInitialPositions, "UIWidgetMenu::SetInitialPositions", {0x53, 0x55, 0x56, 0x8B, 0xF1, 0x8B}},
    {0x520020, (const void *)&UIWidgetMenu_GetWidgetIndex, "UIWidgetMenu::GetWidgetIndex", {0x56, 0x8D, 0x71, 0x4C, 0x8B, 0x0E}},
    {0x52FD40, (const void *)&UIWidgetMenu_ClearWidgets, "UIWidgetMenu::ClearWidgets", {0x53, 0x56, 0x8B, 0xD9, 0x8B, 0x43}},
    {0x52FDB0, (const void *)&UIWidgetMenu_Reposition, "UIWidgetMenu::Reposition", {0x83, 0xEC, 0x08, 0x53, 0x55, 0x56}},
    {0x52FE60, (const void *)&UIWidgetMenu_Reset, "UIWidgetMenu::Reset", {0x56, 0x8B, 0xF1, 0x8B, 0x4E, 0x4C}},
    {0x52FEA0, (const void *)&UIWidgetMenu_SyncViewToSelection, "UIWidgetMenu::SyncViewToSelection", {0x57, 0x8B, 0xF9, 0x8B, 0x4F, 0x4C}},
    {0x5390E0, (const void *)&UIWidgetMenu_Scroll, "UIWidgetMenu::Scroll", {0x56, 0x8B, 0xF1, 0x8B, 0x4E, 0x4C}},
    {0x539370, (const void *)&UIWidgetMenu_ScrollWrapped, "UIWidgetMenu::ScrollWrapped", {0x83, 0xEC, 0x08, 0x55, 0x56, 0x8B}},
    {0x4358E0, (const void *)&DeleteAllNodeOwners_Hook, "bTList::DeleteAllElements", {0x56, 0x8B, 0xF1, 0x8B, 0x06, 0x3B}},
    {0x43BE60, (const void *)&ListGetNode_Hook, "bList::GetNode", {0x8B, 0x01, 0x33, 0xD2, 0x3B, 0xC1}},
    {0x5445F0, (const void *)&UIWidgetMenu_GetCurrentFEObject, "UIWidgetMenu::GetCurrentFEObject", {0x8B, 0x44, 0x24, 0x04, 0x83, 0xEC}},
    {0x54EE40, (const void *)&UIWidgetMenu_AddButtonOption, "UIWidgetMenu::AddButtonOption", {0x83, 0xEC, 0x40, 0x56, 0x57, 0x8B}},
    {0x54F820, (const void *)&UIWidgetMenu_AddStatOption, "UIWidgetMenu::AddStatOption", {0x83, 0xEC, 0x48, 0x56, 0x57, 0x8B}},
    {0x54EF30, (const void *)&UIWidgetMenu_AddToggleOption, "UIWidgetMenu::AddToggleOption", {0x83, 0xEC, 0x44, 0x56, 0x57, 0x8B}},
    {0x54F170, (const void *)&UIWidgetMenu_AddToggleOptionB, "UIWidgetMenu::AddToggleOption(bool)", {0x8A, 0x54, 0x24, 0x08, 0x33, 0xC0}},
    {0x54F190, (const void *)&UIWidgetMenu_AddToggleImageOption, "UIWidgetMenu::AddToggleImageOption", {0x83, 0xEC, 0x44, 0x56, 0x57, 0x8B}},
    {0x54F410, (const void *)&UIWidgetMenu_AddSliderOption, "UIWidgetMenu::AddSliderOption", {0x81, 0xEC, 0x8C, 0x01, 0x00, 0x00}},
    {0x54F800, (const void *)&UIWidgetMenu_AddSliderOptionB, "UIWidgetMenu::AddSliderOption(bool)", {0x8A, 0x54, 0x24, 0x08, 0x33, 0xC0}},
    {0x5275B0, (const void *)&FESliderWidget_SetSliderObjects, "FESliderWidget::SetSliderObjects", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x54}},
    {0x50DAE0, (const void *)&UIWidgetMenu_StorePrevNotification, "UIWidgetMenu::StorePrevNotification", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x54}},
    {0x539650, (const void *)&UIWidgetMenu_SetInitialOption, "UIWidgetMenu::SetInitialOption", {0x56, 0x8B, 0xF1, 0x8A, 0x86, 0x45}},
    {0x556020, (const void *)&UIWidgetMenu_NotificationMessage, "UIWidgetMenu::NotificationMessage", {0x53, 0x55, 0x56, 0x57, 0x8B, 0x7C}},
    {0x51FEA0, (const void *)&UIWidgetMenu_WidgetAtPoint, "UIWidgetMenu::WidgetAtPoint", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8}},
    {0x50DB10, (const void *)&UIWidgetMenu_SelectWidget, "UIWidgetMenu::SelectWidget", {0x56, 0x8B, 0xF1, 0x8B, 0x4E, 0x54}},
    {0x52FC40, (const void *)&UIWidgetMenu_HandleMouse, "UIWidgetMenu::HandleMouse", {0x83, 0xEC, 0x08, 0xDB, 0x05, 0xD4}},
    {0x544490, (const void *)&UIWidgetMenu_HandleScrollBarMouse, "UIWidgetMenu::HandleScrollBarMouse", {0x56, 0x8B, 0xF1, 0x57, 0x8D, 0x7E}},
    {0x544550, (const void *)&UIWidgetMenu_GetCurrentFEString, "UIWidgetMenu::GetCurrentFEString", {0x8B, 0x44, 0x24, 0x04, 0x83, 0xEC}},
    {0x5445A0, (const void *)&UIWidgetMenu_GetCurrentFEImage, "UIWidgetMenu::GetCurrentFEImage", {0x8B, 0x44, 0x24, 0x04, 0x83, 0xEC}},
    {0x50D460, (const void *)&FEWidget_DetermineSound_Hook, "FEWidget::DetermineSound", {0x8B, 0x44, 0x24, 0x04, 0x3D, 0x9E}},
    {0x50CD00, (const void *)&FEngTestForIntersectionRect_Hook, "FEngTestForIntersection(pos,size)", {0x83, 0xEC, 0x10, 0x8B, 0x44, 0x24}},
    {0x55B5C0, (const void *)&MenuScreen_BaseNotify, "MenuScreen::BaseNotify", {0xA0, 0x21, 0x85, 0x83, 0x00, 0x83}},
    {0x54E5B0, (const void *)&MenuScreen_CheckKeyboard, "MenuScreen::CheckKeyboard", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x0C}},
    {0x51DC80, (const void *)&MenuScreen_EndTextInput, "MenuScreen::EndTextInput", {0x53, 0x56, 0x8B, 0xF1, 0x8B, 0x56}},
    {0x555F80, (const void *)&MenuScreen_BeginTextInput, "MenuScreen::FEngBeginTextInput", {0x6A, 0xFF, 0x68, 0xD4, 0x80, 0x77}},
    {0x51DCD0, (const void *)&MenuScreen_BaseNotifySound, "MenuScreen::BaseNotifySound", {0x56, 0x8B, 0xF1, 0x8A, 0x46, 0x2C}},
    {0x537E60, (const void *)&MenuScreen_SetAlternatingPlayerCash, "MenuScreen::SetAlternatingPlayerCash", {0x53, 0x56, 0x57, 0x8B, 0xF9, 0x8B}},
    {0x51DD90, (const void *)&MenuScreen_SetAlternatingText, "MenuScreen::SetAlternatingText", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x3C}},
    {0x537F50, (const void *)&MenuScreen_AlternateTheText, "MenuScreen::AlternateTheText", {0x51, 0x56, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x542720, (const void *)&MenuScreen_UpdateEAMIcons, "MenuScreen::UpdateEAMIcons", {0x51, 0x55, 0x56, 0x57, 0x8B, 0xF9}},
    {0x495430, (const void *)&FEngSetTextureHashPkg_Hook, "FEngSetTextureHash(pkg,hash)", {0x8B, 0x44, 0x24, 0x0C, 0x8B, 0x4C}},
    {0x495460, (const void *)&FEngSetLanguageHashPkg_Hook, "FEngSetLanguageHash(pkg,hash)", {0x8B, 0x44, 0x24, 0x0C, 0x8B, 0x4C}},
    {0x495490, (const void *)&FEngSetLanguageHashName_Hook, "FEngSetLanguageHash(pkg,name)", {0x8B, 0x44, 0x24, 0x0C, 0x8B, 0x4C}},
    {0x4954C0, (const void *)&FEngSetInvisibleName_Hook, "FEngSetInvisible(pkg,name)", {0x8B, 0x44, 0x24, 0x08, 0x50, 0xE8}},
    {0x4954E0, (const void *)&FEngSetInvisiblePkg_Hook, "FEngSetInvisible(pkg,hash)", {0x8B, 0x44, 0x24, 0x08, 0x8B, 0x4C}},
    {0x495500, (const void *)&FEngSetVisibleName_Hook, "FEngSetVisible(pkg,name)", {0x8B, 0x44, 0x24, 0x08, 0x50, 0xE8}},
    {0x495520, (const void *)&FEngSetVisiblePkg_Hook, "FEngSetVisible(pkg,hash)", {0x8B, 0x44, 0x24, 0x08, 0x8B, 0x4C}},
    {0x538130, (const void *)&FEScrollBar_HandleMouse, "FEScrollBar::HandleMouse", {0x83, 0xEC, 0x08, 0x56, 0x8B, 0xF1}},
    {0x52E470, (const void *)&FEScrollBar_ArrowClick, "FEScrollBar::ArrowClick", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x48}},
    {0x52E4E0, (const void *)&FEScrollBar_Drag, "FEScrollBar::Drag", {0x83, 0xEC, 0x10, 0xDB, 0x05, 0xD4}},
    {0x52E670, (const void *)&FEScrollBar_PageClick, "FEScrollBar::PageClick", {0x83, 0xEC, 0x10, 0xDB, 0x05, 0xD4}},
    {0x495810, (const void *)&FEScrollBar_BlinkArrow1, "FEScrollBar::BlinkArrow1", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x48}},
    {0x495850, (const void *)&FEScrollBar_BlinkArrow2, "FEScrollBar::BlinkArrow2", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x4C}},
    {0x52D100, (const void *)&FEngIsClickedOn_Hook, "FEngIsClickedOn", {0x83, 0xEC, 0x08, 0xDB, 0x05, 0xD4}},
    {0x551DE0, (const void *)&MenuScreen_Ctor, "MenuScreen::MenuScreen", {0x8B, 0x44, 0x24, 0x04, 0x53, 0x33}},
    {0x51DC60, (const void *)&MenuScreen_ScalarDtor, "MenuScreen::~MenuScreen", {0xF6, 0x44, 0x24, 0x04, 0x01, 0x56}},
    {0x49FFA0, (const void *)&IsOverlayPackage_Hook, "IsOverlayPackage", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x68}},
    {0x512170, (const void *)&IsGenericDialogPackage_Hook, "IsGenericDialogPackage", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x50}},
    {0x50C900, (const void *)&FEngSetLanguageHash_Hook, "FEngSetLanguageHash", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x50FE90, (const void *)&GetNameHashForTuningType_Hook, "GetNameHashForTuningType", {0x8B, 0x44, 0x24, 0x04, 0x83, 0xF8}, 1, 4, 0},   // TuningSlider::Draw reuses ECX
    {0x50FFB0, (const void *)&GetUnitsHashForTuningType_Hook, "GetUnitsHashForTuningType", {0x8B, 0x44, 0x24, 0x04, 0x83, 0xC0}},
    {0x4FF430, (const void *)&cSlider_Ctor, "cSlider::cSlider", {0x8B, 0xC1, 0x33, 0xC9, 0xC7, 0x00}},
    {0x4FF480, (const void *)&cSlider_Update, "cSlider::Update", {0x8B, 0x44, 0x24, 0x04, 0x53, 0x32}},
    {0x540C10, (const void *)&cSlider_Init, "cSlider::Init", {0x51, 0x56, 0x57, 0x8B, 0x7C, 0x24}},
    {0x4FF530, (const void *)&cSlider_Draw, "cSlider::Draw", {0x8A, 0x41, 0x04, 0x84, 0xC0, 0x74}},
    {0x540D20, (const void *)&cSlider_DrawHorizontal, "cSlider::DrawHorizontal", {0x83, 0xEC, 0x1C, 0x56, 0x57, 0x8B}},
    {0x540E90, (const void *)&cSlider_DrawVertical, "cSlider::DrawVertical", {0x83, 0xEC, 0x14, 0x56, 0x57, 0x8B}},
    {0x512240, (const void *)&cSlider_Toggle, "cSlider::Toggle", {0x8A, 0x44, 0x24, 0x04, 0x84, 0xC0}},
    {0x4FF550, (const void *)&cSlider_SetCurValue, "cSlider::SetCurValue", {0xD9, 0x44, 0x24, 0x04, 0x8B, 0x41}},
    {0x526D90, (const void *)&cSlider_Highlight, "cSlider::Highlight", {0x53, 0x55, 0x56, 0x8B, 0xE9, 0x57}},
    {0x526F80, (const void *)&cSlider_UnHighlight, "cSlider::UnHighlight", {0x53, 0x55, 0x56, 0x8B, 0xE9, 0x57}},
    {0x537A10, (const void *)&FEngFindStringHook, "FEngFindString", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x5122B0, (const void *)&PreviewSlider_Toggle, "PreviewSlider::Toggle", {0x8A, 0x44, 0x24, 0x04, 0x8B, 0xD1}},
    {0x527170, (const void *)&PreviewSlider_Draw, "PreviewSlider::Draw", {0x83, 0xEC, 0x14, 0x56, 0x8B, 0xF1}},
    {0x4FF590, (const void *)&ThreeStageSlider_Update, "ThreeStageSlider::Update", {0x8B, 0x44, 0x24, 0x04, 0x53, 0x32}},
    {0x5273A0, (const void *)&ThreeStageSlider_Draw, "ThreeStageSlider::Draw", {0x83, 0xEC, 0x18, 0x53, 0x56, 0x8B}},
    {0x50C300, (const void *)&cFEng_LoadResources, "cFEng::LoadResources", {0x8B, 0x44, 0x24, 0x08, 0x81, 0xEC}},
    {0x50C3A0, (const void *)&cFEng_UnloadResources, "cFEng::UnloadResources", {0x57, 0x8B, 0x7C, 0x24, 0x0C, 0x85}},
    {0x537480, (const void *)&cFEng_EndPackageRendering, "cFEng::EndPackageRendering", {0x8B, 0x49, 0x0C, 0xE9, 0x68, 0xFD}},
    {0x51C660, (const void *)&cFEng_GenerateRenderContext, "cFEng::GenerateRenderContext", {0x8B, 0x49, 0x0C, 0xE9, 0x88, 0xF1}},
    {0x50C2C0, (const void *)&cFEng_RenderObjectList, "cFEng::RenderObjectList", {0x56, 0x8B, 0x74, 0x24, 0x0C, 0x85}},
    {0x51C670, (const void *)&cFEng_RenderObject, "cFEng::RenderObject", {0x8B, 0x44, 0x24, 0x04, 0xF6, 0x40}},
    {0x50C490, (const void *)&cFEng_GetViewTransformation, "cFEng::GetViewTransformation", {0x8B, 0x44, 0x24, 0x04, 0x50, 0xE8}},
    {0x537690, (const void *)&cFEng_DoesPointTouchObject, "cFEng::DoesPointTouchObject", {0x8B, 0x44, 0x24, 0x0C, 0x8B, 0x4C}},
    {0x50C7C0, (const void *)&cFEng_OutputWarning, "cFEng::OutputWarning", {0x8B, 0x44, 0x24, 0x04, 0x50, 0x68}},
    {0x52CDC0, (const void *)&cFEng_NotifySoundMessage, "cFEng::NotifySoundMessage", {0xB9, 0xB0, 0x37, 0x86, 0x00, 0xE9}},
    {0x50C250, (const void *)&GetBaseName, "GetBaseName", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x51C990, (const void *)&cFEng_GetPackageData, "cFEng::GetPackageData", {0x8B, 0x4C, 0x24, 0x04, 0x8B, 0x44}},
    {0x51C690, (const void *)&cFEng_BeginPackageRendering, "cFEng::BeginPackageRendering", {0x53, 0x56, 0x8B, 0xF1, 0xC6, 0x46}},
    {0x50C3D0, (const void *)&cFEng_GetContextTransform, "cFEng::GetContextTransform", {0x8B, 0x44, 0x24, 0x08, 0x33, 0xD2}},
    {0x50C860, (const void *)&cFEng_GetMouseInfo, "cFEng::GetMouseInfo", {0xDB, 0x05, 0xD4, 0x63, 0x87, 0x00}},
    {0x52C330, (const void *)&FERenderObject_Ctor, "FERenderObject::FERenderObject", {0x6A, 0xFF, 0x68, 0xFB, 0x74, 0x77}},
    {0x52C390, (const void *)&FERenderObject_FreePolys, "FERenderObject::~FERenderObject (polys)", {0x83, 0xC1, 0x18, 0xE9, 0x28, 0x97}},
    {0x52C3A0, (const void *)&FERenderObject_Clear, "FERenderObject::Clear", {0x53, 0x8B, 0xD9, 0x8B, 0x43, 0x18}},
    {0x519FC0, (const void *)&FERenderObject_AddPoly, "FERenderObject::AddPoly", {0x6A, 0xFF, 0x68, 0xCB, 0x70, 0x77}},
    {0x51A1C0, (const void *)&FERenderObject_AddPolyClipped, "FERenderObject::AddPoly(clip)", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x51A5D0, (const void *)&FERenderObject_Render, "FERenderObject::Render", {0x53, 0x55, 0x56, 0x57, 0x6A, 0x00}},
    {0x536050, (const void *)&cFEngRender_Ctor, "cFEngRender::cFEngRender", {0x6A, 0xFF, 0x68, 0xEB, 0x75, 0x77}},
    {0x536100, (const void *)&cFEngRender_Dtor, "cFEngRender::~cFEngRender", {0x6A, 0xFF, 0x68, 0xEB, 0x75, 0x77}},
    {0x52CBA0, (const void *)&cFEngRender_CreateCachedRender, "cFEngRender::CreateCachedRender", {0x6A, 0xFF, 0x68, 0x1B, 0x75, 0x77}},
    {0x5371A0, (const void *)&cFEngRender_RemoveCachedRender, "cFEngRender::RemoveCachedRender", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x8B}},
    {0x51B7F0, (const void *)&cFEngRender_GenerateRenderContext, "cFEngRender::GenerateRenderContext", {0x0F, 0xB7, 0x44, 0x24, 0x04, 0x56}},
    {0x51B850, (const void *)&cFEngRender_AddToRenderList, "cFEngRender::AddToRenderList", {0x57, 0x8B, 0x7C, 0x24, 0x08, 0x8B}},
    {0x5371F0, (const void *)&cFEngRender_PackageFinished, "cFEngRender::PackageFinished", {0x51, 0x53, 0x55, 0x56, 0x57, 0x8B}},
    {0x5370A0, (const void *)&cFEngRender_RenderObject, "cFEngRender::RenderObject", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x57}},
    {0x51B0B0, (const void *)&cFEngRender_MakeRenderMatrix, "cFEngRender::MakeRenderMatrix", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x51B3A0, (const void *)&cFEngRender_GenerateClipInfo, "cFEngRender::GenerateClipInfo", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x536A50, (const void *)&cFEngRender_RenderImage, "cFEngRender::RenderImage", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x536C80, (const void *)&cFEngRender_RenderCBVImage, "cFEngRender::RenderCBVImage", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x536850, (const void *)&cFEngRender_RenderMovie, "cFEngRender::RenderMovie", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x536F60, (const void *)&cFEngRender_RenderString, "cFEngRender::RenderString", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x506CB0, (const void *)&FEClipEdgeXGe, "FEClipEdge x>=", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x507D80, (const void *)&FEClipEdgeYGe, "FEClipEdge y>=", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x508E60, (const void *)&FEClipEdgeXLe, "FEClipEdge x<=", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x509F30, (const void *)&FEClipEdgeYLe, "FEClipEdge y<=", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x50B010, (const void *)&FEClipAligned_Hook, "FERenderObject::ClipAligned", {0x8B, 0x4C, 0x24, 0x1C, 0x8B, 0x54}},
    {0x505660, (const void *)&FEClipGeneral_Hook, "FERenderObject::ClipGeneral", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x4FD980, (const void *)&FEQuatToMatrix_Hook, "quaternion -> matrix (4FD980)", {0x83, 0xEC, 0x20, 0xD9, 0x01, 0x8B}},
    {0x50B3C0, (const void *)&FESortClipCorners, "FESortClipCorners", {0x8B, 0x4C, 0x24, 0x04, 0xD9, 0x01}},
    {0x441000, (const void *)&eMulVector, "eMulVector", {0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x0C}},
    {0x5BB1C0, (const void *)&ePoly_Init, "ePoly::ePoly", {0x8B, 0xC1, 0x33, 0xC9, 0xBA, 0x00}},
    {0x5372E0, (const void *)&cFEng_Ctor, "cFEng::cFEng", {0x6A, 0xFF, 0x68, 0x93, 0x76, 0x77}},
    {0x5373F0, (const void *)&cFEng_Dtor, "cFEng::~cFEng", {0x6A, 0xFF, 0x68, 0xAB, 0x76, 0x77}},
    {0x5425E0, (const void *)&cFEng_ScalarDtor, "cFEng scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x08, 0x4E}},
    {0x54E4A0, (const void *)&cFEng_InitPackage, "cFEng::InitPackage", {0x6A, 0xFF, 0x68, 0x68, 0x7B, 0x77}},
    {0x51C8C0, (const void *)&cFEng_PackageWillUnload, "cFEng::PackageWillUnload", {0x6A, 0xFF, 0x68, 0x50, 0x71, 0x77}},
    {0x537540, (const void *)&cFEng_DrawBackground, "cFEng::DrawBackground", {0x56, 0x8B, 0xF1, 0x68, 0xAC, 0x9F}},
    {0x51CA50, (const void *)&cFEng_RenderPackageSpecial, "cFEng::RenderPackageSpecial", {0xB8, 0xA0, 0x10, 0x00, 0x00, 0xE8}},
    {0x51C9B0, (const void *)&cFEng_RenderObjectSpecial, "cFEng::RenderObjectSpecial", {0x8B, 0x54, 0x24, 0x04, 0x8B, 0x42}},
    {0x5662C0, (const void *)&cFEng_Service, "cFEng::Service", {0x83, 0x3D, 0xA4, 0x54, 0x86, 0x00}},
    {0x50C7E0, (const void *)&cFEng_MakeLoadedPackagesDirty, "cFEng::MakeLoadedPackagesDirty", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x52CF60, (const void *)&FEngIsPackagePushed_Hook, "FEngIsPackagePushed", {0xA1, 0xD0, 0x84, 0x83, 0x00, 0x85}},
    {0x517EE0, (const void *)&FEObjectInitializer_Hook, "FEObjectInitializer::Function", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x8B}},
    {0x566240, (const void *)&FEngMovieStarter_Hook, "FEngMovieStarter::Callback", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x48}},
    {0x51C8A0, (const void *)&FEngMovieStopper_Hook, "FEngMovieStopper::Function", {0x8B, 0x44, 0x24, 0x04, 0x83, 0x78}},
    {0x5374F0, (const void *)&RenderObjectDisconnect_Hook, "RenderObjectDisconnect::Function", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x51}},
    {0x537630, (const void *)&FEDirtyMaker_Hook, "FEDirtyMaker::Function", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x81}},
    {0x51BDC0, (const void *)&FEngFont_Ctor, "FEngFont::FEngFont", {0x53, 0x55, 0x56, 0x8B, 0xF1, 0x57}},
    {0x51BF60, (const void *)&FEngFont_Dtor, "FEngFont::~FEngFont", {0x8B, 0x41, 0x0C, 0x50, 0xE8, 0x57}},
    {0x51BF70, (const void *)&FEngFont_NotifyTextureLoading, "FEngFont::NotifyTextureLoading", {0x55, 0x56, 0x8B, 0x35, 0x68, 0x37}},
    {0x51C110, (const void *)&FEngFont_RenderString, "FEngFont::RenderString", {0x83, 0xEC, 0x38, 0x53, 0x55, 0x56}},
    {0x51BFC0, (const void *)&FEngFont_RenderCharacter, "FEngFont::RenderCharacter", {0x83, 0xEC, 0x1C, 0x8B, 0x54, 0x24}},
    {0x51BD70, (const void *)&FEngFindFont_Hook, "FindFont", {0x56, 0x57, 0xB9, 0x68, 0x37, 0x86}},
    {0x52CD00, (const void *)&LoaderFEngFont, "LoaderFEngFont", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x51DFC0, (const void *)&FEButtonWidget_Ctor, "FEButtonWidget::FEButtonWidget", {0x8B, 0xC1, 0x8B, 0x4C, 0x24, 0x04}},
    {0x50D530, (const void *)&FEButtonWidget_Show, "FEButtonWidget::Show", {0x8B, 0xD1, 0x8B, 0x42, 0x2C, 0x50}},
    {0x50D550, (const void *)&FEButtonWidget_Hide, "FEButtonWidget::Hide", {0x8B, 0xD1, 0x8B, 0x42, 0x2C, 0x50}},
    {0x52D770, (const void *)&FEButtonWidget_SetFocus, "FEButtonWidget::SetFocus", {0x53, 0x8B, 0xD9, 0x8B, 0x0D, 0xC4}},
    {0x51E020, (const void *)&FEButtonWidget_UnsetFocus, "FEButtonWidget::UnsetFocus", {0x53, 0x8B, 0xD9, 0x57, 0x8B, 0x7B}},
    {0x52D6B0, (const void *)&FEButtonWidget_Position, "FEButtonWidget::Position", {0x83, 0xEC, 0x10, 0x56, 0x8B, 0xF1}},
    {0x52D650, (const void *)&FEButtonWidget_CheckMouse, "FEButtonWidget::CheckMouse", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4AE9C0, (const void *)&FEWidget_SetPosVirtual, "FEButtonWidget::SetPos", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x0C}},
    {0x4CF650, (const void *)&FEWidget_Destruct, "FEWidget::~FEWidget", {0xC7, 0x01, 0x98, 0x0B, 0x79, 0x00}},
    {0x4AEA10, (const void *)&FEWidget_ScalarDtor, "FEWidget scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x38, 0x0C}},
    {0x4CF640, (const void *)&FESliderWidget_Destruct, "FESliderWidget::~FESliderWidget", {0xC7, 0x41, 0x54, 0x44, 0xF5, 0x78}},
    {0x4CF620, (const void *)&FESliderWidget_ScalarDtor, "FESliderWidget scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x50D800, (const void *)&FEButtonWidget_DetermineSpecialSound, "FEButtonWidget::DetermineSpecialSound", {0x8B, 0x4C, 0x24, 0x04, 0x33, 0xC0}},
    {0x51E9E0, (const void *)&FEInputWidget_Ctor, "FEInputWidget::FEInputWidget", {0x6A, 0xFF, 0x68, 0x68, 0x71, 0x77}},
    {0x55E120, (const void *)&FEInputWidget_Act, "FEInputWidget::Act", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x51EC10, (const void *)&FEInputWidget_CheckMouse, "FEInputWidget::CheckMouse", {0x83, 0xEC, 0x10, 0x56, 0x8B, 0xF1}},
    {0x51EC90, (const void *)&FEInputWidget_Enable, "FEInputWidget::Enable", {0x53, 0x55, 0x56, 0x8B, 0xD9, 0x57}},
    {0x51EA90, (const void *)&FEInputWidget_Disable, "FEInputWidget::Disable", {0x53, 0x55, 0x56, 0x8B, 0xE9, 0x57}},
    {0x52E120, (const void *)&FEInputWidget_SetFocus, "FEInputWidget::SetFocus", {0x53, 0x55, 0x8B, 0xE9, 0x8B, 0x0D}},
    {0x51ED60, (const void *)&FEInputWidget_UnsetFocus, "FEInputWidget::UnsetFocus", {0x53, 0x55, 0x56, 0x8B, 0xE9, 0x57}},
    {0x51EB60, (const void *)&FEInputWidget_SetInputFocus, "FEInputWidget::SetInputFocus", {0x53, 0x8B, 0xD9, 0x57, 0x8B, 0x7B}},
    {0x51EBD0, (const void *)&FEDateWidget_Ctor, "FEDateWidget::FEDateWidget", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x54}},
    {0x538060, (const void *)&FEDateWidget_SetDate, "FEDateWidget::SetDate", {0x8B, 0x41, 0x54, 0x8B, 0x51, 0x4C}},
    {0x55AC10, (const void *)&TSM_Ctor, "TuningSlidersMenu::TuningSlidersMenu", {0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68}},
    {0x55AD00, (const void *)&TSM_ScalarDtor, "TuningSlidersMenu scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x55AD20, (const void *)&TSM_Dtor, "TuningSlidersMenu::~TuningSlidersMenu", {0x6A, 0xFF, 0x68, 0xE8, 0x6F, 0x77}},
    {0x5650C0, (const void *)&TSM_Notify, "TuningSlidersMenu::NotificationMessage", {0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x14}},
    {0x510120, (const void *)&TSM_GetTipHash, "TuningSlidersMenu::GetTipHash", {0x8B, 0x44, 0x24, 0x04, 0x83, 0xF8}},
    {0x549850, (const void *)&TSM_AddTuningSlider, "TuningSlidersMenu::AddTuningSlider", {0x55, 0x8B, 0xEC, 0x64, 0xA1, 0x00}},
    {0x557780, (const void *)&TSM_Setup, "TuningSlidersMenu::Setup", {0xA1, 0x48, 0xA6, 0x83, 0x00, 0x83}},
    {0x549B40, (const void *)&TSM_SetupSuspension, "TuningSlidersMenu::SetupSuspension", {0x56, 0x6A, 0x00, 0x6A, 0x00, 0x68}},
    {0x549C20, (const void *)&TSM_SetupDrivetrain, "TuningSlidersMenu::SetupDrivetrain", {0x53, 0x56, 0x6A, 0x00, 0x6A, 0x00}},
    {0x549DC0, (const void *)&TSM_SetupAero, "TuningSlidersMenu::SetupAero", {0x56, 0x6A, 0x00, 0x6A, 0x00, 0x68}},
    {0x549E00, (const void *)&TSM_SetupWheelBrakes, "TuningSlidersMenu::SetupWheelBrakes", {0x56, 0x6A, 0x00, 0x6A, 0x00, 0x68}},
    {0x50D2D0, (const void *)&FEngMapPortToControlMask_Hook, "FEngMapPortToControlMask", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x4C3C70, (const void *)&OC_PPOTransmission_Act, "PPOTransmission::Act", {0x51, 0x8B, 0x44, 0x24, 0x0C, 0x3D}},
    {0x4C3D80, (const void *)&OC_PPOTransmission_Draw, "PPOTransmission::Draw", {0x56, 0x57, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x4C31D0, (const void *)&OC_PPOHandling_Act, "PPOHandling::Act", {0x51, 0x8B, 0x44, 0x24, 0x0C, 0x3D}},
    {0x4C32E0, (const void *)&OC_PPOHandling_Draw, "PPOHandling::Draw", {0x56, 0x57, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x4C3E10, (const void *)&OC_PPODriveCam_Act, "PPODriveCam::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x4C3E60, (const void *)&OC_PPODriveCam_Draw, "PPODriveCam::Draw", {0x56, 0x57, 0x8B, 0xF9, 0x8B, 0x47}},
    {0x4C4450, (const void *)&OC_PPOSplitTime_Act, "PPOSplitTime::Act", {0x8B, 0x15, 0xAC, 0x00, 0x89, 0x00}},
    {0x4C4510, (const void *)&OC_PPOSplitTime_Draw, "PPOSplitTime::Draw", {0x56, 0x57, 0x8B, 0xF9, 0x8B, 0x47}},
    {0x4C3F50, (const void *)&OptToggle_Act<10>, "PPOGauges::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0x9E}},
    {0x4C4010, (const void *)&OptToggle_Draw<10>, "PPOGauges::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4C4090, (const void *)&OptToggle_Act<11>, "PPOPosition::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0x9E}},
    {0x4C4150, (const void *)&OptToggle_Draw<11>, "PPOPosition::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4C41D0, (const void *)&OptToggle_Act<12>, "PPOLapInfo::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0x9E}},
    {0x4C4290, (const void *)&OptToggle_Draw<12>, "PPOLapInfo::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4C4310, (const void *)&OptToggle_Act<13>, "PPOScore::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0x9E}},
    {0x4C43D0, (const void *)&OptToggle_Draw<13>, "PPOScore::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4C4600, (const void *)&OptToggle_Act<14>, "PPOLeaderBoard::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0x9E}},
    {0x4C46C0, (const void *)&OptToggle_Draw<14>, "PPOLeaderBoard::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x51F670, (const void *)&IO_Ctor, "IconOption::IconOption", {0x8B, 0x54, 0x24, 0x08, 0x8B, 0xC1}},
    {0x521FF0, (const void *)&CPP_Ctor, "CPPaintOption::CPPaintOption", {0x8B, 0x54, 0x24, 0x0C, 0x8B, 0xC1}},
    {0x527770, (const void *)&IO_Dtor, "IconOption::~IconOption", {0xC7, 0x01, 0xDC, 0xAF, 0x79, 0x00}},
    {0x4AEB70, (const void *)&IO_ScalarDtor, "IconOption scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0xF8, 0x8B}},
    {0x543840, (const void *)&IP_Ctor, "IconPanel::IconPanel", {0x6A, 0xFF, 0x68, 0xBB, 0x79, 0x77}},
    {0x4CBC20, (const void *)&IP_Dtor, "IconPanel::~IconPanel", {0xC7, 0x01, 0xBC, 0x5C, 0x79, 0x00}},
    {0x543BF0, (const void *)&IP_ScalarDtor, "IconPanel scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x28, 0x80}},
    {0x538910, (const void *)&IP_Update, "IconPanel::Update", {0x83, 0xEC, 0x08, 0x8D, 0x44, 0x24}},
    {0x5439B0, (const void *)&IP_AddOption, "IconPanel::AddOption", {0x83, 0xEC, 0x40, 0x53, 0x57, 0x8B}},
    {0x52F390, (const void *)&IP_ClearList, "IconPanel::ClearList", {0x55, 0x8B, 0xE9, 0x8B, 0x45, 0x04}},
    {0x50D980, (const void *)&IP_Act, "IconPanel::Act", {0x8B, 0x41, 0x0C, 0x85, 0xC0, 0x74}},
    {0x495910, (const void *)&IP_GetIndex, "IconPanel::GetOptionIndex", {0x8B, 0x41, 0x0C, 0x85, 0xC0, 0x74}},
    {0x51F820, (const void *)&IP_GetIndexOf, "IconPanel::GetOptionIndex(opt)", {0x57, 0x8B, 0x7C, 0x24, 0x08, 0x85}},
    {0x52F460, (const void *)&IP_SetSelIdx, "IconPanel::SetSelection(int)", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x8B}},
    {0x52F510, (const void *)&IP_SetSelOpt, "IconPanel::SetSelection(opt)", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x52F5A0, (const void *)&IP_HandleMouse, "IconPanel::HandleMouse", {0x83, 0xEC, 0x18, 0x53, 0x56, 0x8B}},
    {0x51F870, (const void *)&IP_SetInitialPos, "IconPanel::SetInitialPos", {0x83, 0xEC, 0x1C, 0x53, 0x56, 0x57}},
    {0x51F9F0, (const void *)&IP_Scroll, "IconPanel::Scroll", {0x53, 0x8B, 0x5C, 0x24, 0x08, 0x83}},
    {0x51FAC0, (const void *)&IP_ScrollWrapped, "IconPanel::ScrollWrapped", {0x53, 0x8B, 0x5C, 0x24, 0x08, 0x83}},
    {0x527730, (const void *)&IP_ScrollNext, "IconPanel::ScrollNext", {0x8A, 0x41, 0x2C, 0x84, 0xC0, 0x6A}},
    {0x527750, (const void *)&IP_ScrollPrev, "IconPanel::ScrollPrev", {0x8A, 0x41, 0x2C, 0x84, 0xC0, 0x6A}},
    {0x51FB70, (const void *)&IP_AnimateList, "IconPanel::AnimateList", {0x53, 0x8B, 0x5C, 0x24, 0x0C, 0x55}},
    {0x5276D0, (const void *)&IP_IsHead, "IconPanel::IsHead", {0x8B, 0x41, 0x04, 0x85, 0xC0, 0x74}},
    {0x527700, (const void *)&IP_IsTail, "IconPanel::IsTail", {0x8B, 0x41, 0x08, 0x85, 0xC0, 0x74}},
    {0x543A90, (const void *)&IS_Ctor, "IconScroller::IconScroller", {0x6A, 0xFF, 0x68, 0xD8, 0x79, 0x77}},
    {0x538940, (const void *)&IS_Update, "IconScroller::Update", {0x53, 0x56, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x543C10, (const void *)&IS_AddOption, "IconScroller::AddOption", {0x83, 0xEC, 0x28, 0x57, 0x8B, 0xF9}},
    {0x538A30, (const void *)&IS_SetInitialPos, "IconScroller::SetInitialPos", {0x53, 0x55, 0x56, 0x8B, 0xF1, 0x8B}},
    {0x52F7C0, (const void *)&IS_SetSelIdx, "IconScroller::SetSelection(int)", {0x53, 0x55, 0x56, 0x8B, 0xE9, 0x8B}},
    {0x52F680, (const void *)&IS_SetSelOpt, "IconScroller::SetSelection(opt)", {0x53, 0x55, 0x56, 0x8B, 0xE9, 0x8B}},
    {0x51FD10, (const void *)&IS_StartDim, "IconScroller::StartDim", {0x8B, 0x41, 0x04, 0x85, 0xC0, 0x56}},
    {0x538B30, (const void *)&IS_StopDim, "IconScroller::StopDim", {0x53, 0x56, 0x57, 0x8B, 0xF9, 0x8B}},
    {0x50D9C0, (const void *)&IS_ClipEdges, "IconScroller::ClipEdges", {0xD9, 0x81, 0xCC, 0x00, 0x00, 0x00}},
    {0x52F8C0, (const void *)&IS_PositionOption, "IconScroller::PositionOption", {0x83, 0xEC, 0x08, 0x56, 0x8B, 0x74}},
    {0x51FD60, (const void *)&IS_UpdateFade, "IconScroller::UpdateFade", {0x53, 0x8B, 0x5C, 0x24, 0x08, 0x85}},
    {0x52FA70, (const void *)&IS_UpdateArrows, "IconScroller::UpdateArrows", {0x8B, 0x44, 0x24, 0x04, 0x83, 0xF8}},
    {0x538B80, (const void *)&IS_HandleMouse, "IconScroller::HandleMouse", {0x56, 0x8B, 0xF1, 0x8D, 0x4E, 0x38}},
    {0x552330, (const void *)&ISM_Ctor, "IconScrollerMenu::IconScrollerMenu", {0x6A, 0xFF, 0x68, 0x73, 0x7C, 0x77}},
    {0x4DEFB0, (const void *)&ISM_Dtor, "IconScrollerMenu::~IconScrollerMenu", {0x6A, 0xFF, 0x68, 0x18, 0x64, 0x77}},
    {0x4DF010, (const void *)&ISM_ScalarDtor, "IconScrollerMenu scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x98, 0xFF}},
    {0x543D40, (const void *)&ISM_Notify, "IconScrollerMenu::NotificationMessage", {0x83, 0xEC, 0x08, 0x56, 0x8B, 0xF1}},
    {0x538C00, (const void *)&ISM_StartDim, "IconScrollerMenu::StartDim", {0x56, 0x57, 0x68, 0xA0, 0xBD, 0x79}},
    {0x538CA0, (const void *)&ISM_StopDim, "IconScrollerMenu::StopDim", {0x56, 0x57, 0x68, 0xC0, 0xBD, 0x79}},
    {0x52FBA0, (const void *)&ISM_StartInput, "IconScrollerMenu::StartInput", {0x56, 0x8B, 0xF1, 0x8A, 0x86, 0x4C}},
    {0x52FBF0, (const void *)&ISM_StopInput, "IconScrollerMenu::StopInput", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x04}},
    {0x50DA20, (const void *)&ISM_StorePrev, "IconScrollerMenu::StorePrevNotification", {0x8B, 0x44, 0x24, 0x04, 0x8B, 0x54}},
    {0x50DA50, (const void *)&ISM_RefreshHeader, "IconScrollerMenu::RefreshHeader", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x58}},
    {0x50DAB0, (const void *)&ISM_AddOption, "IconScrollerMenu::AddOption", {0x8B, 0x41, 0x4C, 0x56, 0x8B, 0x74}},
    {0x50D8D0, (const void *)&thumbnail_scale, "thumbnail_scale", {0xD9, 0x44, 0x24, 0x0C, 0xD8, 0x0D}},
    {0x51DF00, (const void *)&UpdateIconOverlay, "UpdateIconOverlay", {0x8B, 0x44, 0x24, 0x04, 0x8A, 0x48}},
    {0x5435D0, (const void *)&TS_Ctor, "ThumbnailScroller::ThumbnailScroller", {0x6A, 0xFF, 0x68, 0xBB, 0x79, 0x77}},
    {0x52EA90, (const void *)&TS_Dtor, "ThumbnailScroller::~ThumbnailScroller", {0x6A, 0xFF, 0x68, 0xBB, 0x79, 0x77}},
    {0x5385A0, (const void *)&TS_ScalarDtor, "ThumbnailScroller scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0xE8, 0x64}},
    {0x5385C0, (const void *)&TS_ClearAll, "ThumbnailScroller::ClearAll", {0x56, 0x57, 0x8B, 0xF9, 0x8B, 0x47}},
    {0x51F6C0, (const void *)&TS_ResetNodes, "ThumbnailScroller::ResetNodes", {0x8B, 0x41, 0x04, 0x85, 0xC0, 0x56}},
    {0x51F750, (const void *)&TS_MoveMeOffscreen, "ThumbnailScroller::MoveMeOffscreen", {0x8B, 0x41, 0x04, 0x85, 0xC0, 0x56}},
    {0x51F7A0, (const void *)&TS_FindNode, "ThumbnailScroller::FindNode", {0x8B, 0x41, 0x04, 0x85, 0xC0, 0x8D}},
    {0x51F7E0, (const void *)&TS_GetCurrenNodeIndex, "ThumbnailScroller::GetCurrenNodeIndex", {0x8B, 0x51, 0x04, 0x56, 0x57, 0x8D}},
    {0x52EB90, (const void *)&TS_SetVisibility, "ThumbnailScroller::SetVisibility", {0x53, 0x8A, 0x5C, 0x24, 0x08, 0x55}},
    {0x52EC20, (const void *)&TS_GetNextNode, "ThumbnailScroller::GetNextNode", {0x53, 0x56, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x52EC90, (const void *)&TS_AddNodeId, "ThumbnailScroller::AddNode(id)", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x28}},
    {0x52ED80, (const void *)&TS_AddNodeObj, "ThumbnailScroller::AddNode(node)", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x28}},
    {0x52EE40, (const void *)&TS_SetDesiredItem, "ThumbnailScroller::SetDesiredItem", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x50}},
    {0x52EEA0, (const void *)&TS_MoveObjectsTo, "ThumbnailScroller::MoveObjectsTo", {0x83, 0xEC, 0x3C, 0x53, 0x55, 0x56}},
    {0x5386A0, (const void *)&TS_SnapToNode, "ThumbnailScroller::SnapToNode", {0x8B, 0x44, 0x24, 0x04, 0x3B, 0x41}},
    {0x538700, (const void *)&TS_SnapToItem, "ThumbnailScroller::SnapToItem", {0x8B, 0x44, 0x24, 0x04, 0x56, 0x50}},
    {0x538740, (const void *)&TS_SnapToHead, "ThumbnailScroller::SnapToHead", {0x8B, 0x41, 0x04, 0x85, 0xC0, 0x74}},
    {0x538760, (const void *)&TS_Update, "ThumbnailScroller::Update", {0x83, 0xEC, 0x0C, 0x56, 0x8B, 0xF1}},
    {0x5388F0, (const void *)&TS_StartDim, "ThumbnailScroller::StartDim", {0x6A, 0x00, 0xC6, 0x41, 0x2F, 0x00}},
    {0x538900, (const void *)&TS_StopDim, "ThumbnailScroller::StopDim", {0x6A, 0x01, 0xC6, 0x41, 0x2F, 0x01}},
    {0x5437C0, (const void *)&TS_Scroll, "ThumbnailScroller::Scroll", {0x8B, 0x41, 0x04, 0x56, 0x8D, 0x71}},
    {0x4984B0, (const void *)&QTS_CalcIconHash, "SelectableTrack::CalcIconHash", {0x8A, 0x41, 0x08, 0x84, 0xC0, 0x74}},
    {0x4985A0, (const void *)&TMS_Dtor, "TrackMapScroller::~TrackMapScroller", {0x56, 0x8B, 0xF1, 0xC7, 0x06, 0xC8}},
    {0x4B4B60, (const void *)&TMS_ScalarDtor, "TrackMapScroller scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x38, 0x3A}},
    {0x498600, (const void *)&TMS_SetTrackMap, "TrackMapScroller::SetTrackMap", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x4CDD40, (const void *)&TMS_Update, "TrackMapScroller::Update", {0x83, 0xEC, 0x20, 0x53, 0x56, 0x8B}},
    {0x4B4B80, (const void *)&TMS_InitZoom, "TrackMapScroller::InitZoom", {0x56, 0x8B, 0xF1, 0x8B, 0x4E, 0x08}},
    {0x498630, (const void *)&TMS_ZoomIn, "TrackMapScroller::ZoomIn", {0x83, 0xEC, 0x10, 0x56, 0x57, 0x8B}},
    {0x4AEBF0, (const void *)&TrackMapScroller_TexturesLoadedCallback, "TrackMapScroller::TexturesLoadedCallback", {0x8B, 0x44, 0x24, 0x04, 0xC6, 0x40}},
    {0x4987A0, (const void *)&QRTrack_SortCompare, "UIQRTrackSelect track sort compare", {0x8B, 0x44, 0x24, 0x04, 0x8A, 0x48}},
    {0x4E2F30, (const void *)&QTS_Ctor, "UIQRTrackSelect::UIQRTrackSelect", {0x6A, 0xFF, 0x68, 0x0C, 0x5C, 0x77}},
    {0x4DF030, (const void *)&QTS_Dtor, "UIQRTrackSelect::~UIQRTrackSelect", {0x6A, 0xFF, 0x68, 0x9E, 0x58, 0x77}},
    {0x4DF120, (const void *)&QTS_ScalarDtor, "UIQRTrackSelect scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x08, 0xFF}},
    {0x4EFA10, (const void *)&QTS_Notify, "UIQRTrackSelect::NotificationMessage", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x0C}},
    {0x4DF0C0, (const void *)&QTS_NotifySound, "UIQRTrackSelect::NotifySoundMessage", {0x8B, 0x44, 0x24, 0x04, 0x3D, 0x10}},
    {0x498740, (const void *)&QTS_SetSelectedTrack, "UIQRTrackSelect::SetSelectedTrack", {0x68, 0xD0, 0xF8, 0x78, 0x00, 0xE8}},
    {0x4CDED0, (const void *)&QTS_Build, "UIQRTrackSelect::BuildPresetTrackList", {0x51, 0xA1, 0xA4, 0x3D, 0x88, 0x00}},
    {0x4B4C10, (const void *)&QTS_AddTrack, "UIQRTrackSelect::AddTrack", {0x56, 0x57, 0x6A, 0x30, 0x8B, 0xF9}},
    {0x4987C0, (const void *)&QTS_IsAvailable, "UIQRTrackSelect::IsAvailable", {0x8B, 0x4C, 0x24, 0x04, 0x85, 0xC9}},
    {0x4E3270, (const void *)&QTS_Scroll, "UIQRTrackSelect::Scroll", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x4CE170, (const void *)&QTS_RefreshHeader, "UIQRTrackSelect::RefreshHeader", {0x81, 0xEC, 0x24, 0x02, 0x00, 0x00}},
    {0x4988C0, (const void *)&QTS_DisplayRaceType, "UIQRTrackSelect::DisplayRaceType", {0xA1, 0x78, 0x00, 0x85, 0x00, 0x83}},
    {0x5026F0, (const void *)&QRS_GetNumPlayers, "QuickRaceSettings::GetNumPlayers", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x502710, (const void *)&QRS_GetNumOpponents, "QuickRaceSettings::GetNumOpponents", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x502730, (const void *)&QRS_GetAISkill, "QuickRaceSettings::GetAISkill", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x502750, (const void *)&QRS_GetTrafficLevel, "QuickRaceSettings::GetTrafficLevel", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x502770, (const void *)&QRS_GetNumPointsToWinOutrun, "QuickRaceSettings::GetNumPointsToWinOutrun", {0x8B, 0x81, 0x08, 0x01, 0x00, 0x00}},
    {0x502780, (const void *)&QRS_GetOutrunTimeLimit, "QuickRaceSettings::GetOutrunTimeLimit", {0x8B, 0x81, 0x0C, 0x01, 0x00, 0x00}},
    {0x502790, (const void *)&QRS_GetNumPointsToWinOutrunValue, "QuickRaceSettings::GetNumPointsToWinOutrunValue", {0x8B, 0x81, 0x08, 0x01, 0x00, 0x00}},
    {0x5027A0, (const void *)&QRS_GetOutrunTimeLimitValue, "QuickRaceSettings::GetOutrunTimeLimitValue", {0x8B, 0x81, 0x0C, 0x01, 0x00, 0x00}},
    {0x5027B0, (const void *)&QRS_HasAPlayerWonTheOutrunTourney, "QuickRaceSettings::HasAPlayerWonTheOutrunTourney", {0x57, 0x8B, 0xB9, 0x08, 0x01, 0x00}},
    {0x502840, (const void *)&QRS_IsOutrunOvertime, "QuickRaceSettings::IsOutrunOvertime", {0x57, 0x8B, 0xB9, 0x08, 0x01, 0x00}},
    {0x5028D0, (const void *)&QRS_ResetOutrunScores, "QuickRaceSettings::ResetOutrunScores", {0xC7, 0x81, 0x14, 0x01, 0x00, 0x00}},
    {0x5028F0, (const void *)&QRS_IsCatchUpOn, "QuickRaceSettings::IsCatchUpOn", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x502910, (const void *)&QRS_GetCurrentTrackSettings, "QuickRaceSettings::GetCurrentTrackSettings", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x502930, (const void *)&QRS_SetNumPlayers, "QuickRaceSettings::SetNumPlayers", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x502960, (const void *)&QRS_SetNumOpponents, "QuickRaceSettings::SetNumOpponents", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x502990, (const void *)&QRS_SetAISkill, "QuickRaceSettings::SetAISkill", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x5029C0, (const void *)&QRS_SetTrafficLevel, "QuickRaceSettings::SetTrafficLevel", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x5029F0, (const void *)&QRS_SetCatchUp, "QuickRaceSettings::SetCatchUp", {0x8B, 0x01, 0x83, 0xF8, 0x0C, 0x75}},
    {0x502A20, (const void *)&QRS_SetNumPointsToWinOutrun, "QuickRaceSettings::SetNumPointsToWinOutrun", {0x8B, 0x44, 0x24, 0x04, 0x89, 0x81}},
    {0x502A30, (const void *)&QRS_SetOutrunTimeLimit, "QuickRaceSettings::SetOutrunTimeLimit", {0x8B, 0x44, 0x24, 0x04, 0x89, 0x81}},
    {0x502A40, (const void *)&QRS_SetSelectedCar, "QuickRaceSettings::SetSelectedCar", {0x8B, 0x44, 0x24, 0x08, 0x8B, 0x54}},
    {0x4B3750, (const void *)&QRT_Ctor<QRToggle::NumOpponents>, "QR toggle NumOpponents ctor", {0x6A, 0xFF, 0x68, 0x18, 0x38, 0x77}},
    {0x4B37F0, (const void *)&QRT_Act<QRToggle::NumOpponents>, "QR toggle NumOpponents Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0xF1}},
    {0x4B38C0, (const void *)&QRT_Draw<QRToggle::NumOpponents>, "QR toggle NumOpponents Draw", {0x56, 0x57, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x4B3930, (const void *)&QRT_Ctor<QRToggle::AISkill>, "QR toggle AISkill ctor", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x54}},
    {0x4B3960, (const void *)&QRT_Act<QRToggle::AISkill>, "QR toggle AISkill Act", {0x56, 0x8B, 0xF1, 0x57, 0xB9, 0xB4}},
    {0x4B39C0, (const void *)&QRT_Draw<QRToggle::AISkill>, "QR toggle AISkill Draw", {0x56, 0x57, 0x8B, 0xF9, 0xB9, 0xB4}},
    {0x4B3A10, (const void *)&QRT_Act<QRToggle::CatchUp>, "QR toggle CatchUp Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x4B3A60, (const void *)&QRT_Draw<QRToggle::CatchUp>, "QR toggle CatchUp Draw", {0x56, 0x8B, 0xF1, 0xB9, 0xB4, 0xAA}},
    {0x4B3AA0, (const void *)&QRT_Ctor<QRToggle::TrafficLevel>, "QR toggle TrafficLevel ctor", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x54}},
    {0x4B3AD0, (const void *)&QRT_Act<QRToggle::TrafficLevel>, "QR toggle TrafficLevel Act", {0x56, 0x8B, 0xF1, 0x57, 0xB9, 0xB4}},
    {0x4B3B30, (const void *)&QRT_Draw<QRToggle::TrafficLevel>, "QR toggle TrafficLevel Draw", {0x56, 0x57, 0x8B, 0xF9, 0xB9, 0xB4}},
    {0x4B3BA0, (const void *)&QRT_Act<QRToggle::PointsToWin>, "QR toggle PointsToWin Act", {0x56, 0x8B, 0xF1, 0x57, 0xB9, 0xB4}},
    {0x4B3C00, (const void *)&QRT_Draw<QRToggle::PointsToWin>, "QR toggle PointsToWin Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B3C60, (const void *)&QRT_Act<QRToggle::TimeLimit>, "QR toggle TimeLimit Act", {0x56, 0x8B, 0xF1, 0x57, 0xB9, 0xB4}},
    {0x4B3CC0, (const void *)&QRT_Draw<QRToggle::TimeLimit>, "QR toggle TimeLimit Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B3D30, (const void *)&QRT_Ctor<QRToggle::NumLaps>, "QR toggle NumLaps ctor", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x54}},
    {0x4B3D60, (const void *)&QRT_Act<QRToggle::NumLaps>, "QR toggle NumLaps Act", {0x53, 0x56, 0x8B, 0xF1, 0x57, 0xB9}},
    {0x4B3DD0, (const void *)&QRT_Draw<QRToggle::NumLaps>, "QR toggle NumLaps Draw", {0x56, 0x8B, 0xF1, 0x57, 0x8B, 0x7E}},
    {0x4B3E40, (const void *)&QRT_Ctor<QRToggle::OnlineNumLaps>, "QR toggle OnlineNumLaps ctor", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x54}},
    {0x4B3F10, (const void *)&QRT_Act<QRToggle::OnlineNumLaps>, "QR toggle OnlineNumLaps Act", {0x0F, 0xB7, 0x05, 0xBA, 0x00, 0x85}},
    {0x4B3F70, (const void *)&QRT_Draw<QRToggle::OnlineNumLaps>, "QR toggle OnlineNumLaps Draw", {0x0F, 0xB7, 0x05, 0xBA, 0x00, 0x85}},
    {0x4B3FA0, (const void *)&QRT_Ctor<QRToggle::LapKO>, "QR toggle LapKO ctor", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x54}},
    {0x4B4020, (const void *)&QRT_Act<QRToggle::LapKO>, "QR toggle LapKO Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x4B3FD0, (const void *)&QRT_Draw<QRToggle::LapKO>, "QR toggle LapKO Draw", {0x56, 0x8B, 0xF1, 0xB9, 0xB4, 0xAA}},
    {0x4B40B0, (const void *)&QRT_Ctor<QRToggle::TrackDirection>, "QR toggle TrackDirection ctor", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x54}},
    {0x4B40E0, (const void *)&QRT_Act<QRToggle::TrackDirection>, "QR toggle TrackDirection Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x4B4150, (const void *)&QRT_Draw<QRToggle::TrackDirection>, "QR toggle TrackDirection Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4EF3F0, (const void *)&QMO_Ctor, "UIQRModeOptions::UIQRModeOptions", {0x6A, 0xFF, 0x68, 0x28, 0x6E, 0x77}},
    {0x4EF610, (const void *)&QMO_Dtor, "UIQRModeOptions::~UIQRModeOptions", {0xC7, 0x01, 0x14, 0x80, 0x79, 0x00}},
    {0x4EF5F0, (const void *)&QMO_ScalarDtor, "UIQRModeOptions::scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4EF620, (const void *)&QMO_Notify, "UIQRModeOptions::NotificationMessage", {0x8B, 0x44, 0x24, 0x10, 0x8B, 0x54}},
    {0x4E2C70, (const void *)&QMO_Setup, "UIQRModeOptions::Setup", {0xA1, 0x78, 0x00, 0x85, 0x00, 0x83}},
    {0x4CCF60, (const void *)&QMO_SetupCircuit, "UIQRModeOptions::SetupCircuit", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x4CD2D0, (const void *)&QMO_SetupSprint, "UIQRModeOptions::SetupSprint", {0x6A, 0xFF, 0x68, 0x54, 0x43, 0x77}},
    {0x4CD530, (const void *)&QMO_SetupDrag, "UIQRModeOptions::SetupDrag", {0x6A, 0xFF, 0x68, 0x54, 0x43, 0x77}},
    {0x4CD7A0, (const void *)&QMO_SetupDrift, "UIQRModeOptions::SetupDrift", {0x6A, 0xFF, 0x68, 0xC4, 0x43, 0x77}},
    {0x4CDA20, (const void *)&QMO_SetupOutrun, "UIQRModeOptions::SetupOutrun", {0x6A, 0xFF, 0x68, 0x68, 0x44, 0x77}},
    {0x4CDB40, (const void *)&QMO_SetupGT, "UIQRModeOptions::SetupGT", {0x6A, 0xFF, 0x68, 0xD0, 0x44, 0x77}},
    {0x4B47B0, (const void *)&QMO_SetupShortTrack, "UIQRModeOptions::SetupShortTrack", {0x6A, 0xFF, 0x68, 0x34, 0x44, 0x77}},
    {0x4B4A10, (const void *)&QMO_SetupFreeRun, "UIQRModeOptions::SetupFreeRun", {0x6A, 0xFF, 0x68, 0xB8, 0x30, 0x77}},
    {0x4E94C0, (const void *)&PRM_Ctor, "PostRaceMenu::PostRaceMenu", {0x6A, 0xFF, 0x68, 0x78, 0x61, 0x77}},
    {0x4E9540, (const void *)&PRM_Dtor, "PostRaceMenu::~PostRaceMenu", {0x6A, 0xFF, 0x68, 0x78, 0x61, 0x77}},
    {0x4E9520, (const void *)&PRM_ScalarDtor, "PostRaceMenu scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4F39A0, (const void *)&PRM_Notify, "PostRaceMenu::NotificationMessage", {0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x14}},
    {0x4D7160, (const void *)&PRM_Setup, "PostRaceMenu::Setup", {0x53, 0x56, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x4BEE10, (const void *)&PRM_SetupRace, "PostRaceMenu::SetupRace", {0x6A, 0xFF, 0x68, 0xFC, 0x39, 0x77}},
    {0x4A3DC0, (const void *)&PRM_QuitToWorldMap, "PostRaceMenu::QuitToWorldMap", {0xB9, 0x40, 0xAD, 0x85, 0x00, 0xE8}},
    {0x4A3DF0, (const void *)&PRM_QuitRaceToFreeRoam, "PostRaceMenu::QuitRaceToFreeRoam", {0xB9, 0x40, 0xAD, 0x85, 0x00, 0xE8}},
    {0x4A3E30, (const void *)&PRM_RaceDone, "PostRaceMenu::QuitRaceToFreeRoamRaceDone", {0x83, 0xEC, 0x40, 0x56, 0x8D, 0x44}},
    {0x4A3E80, (const void *)&PRM_QuitGame, "PostRaceMenu::QuitGame", {0x83, 0x3D, 0x78, 0x00, 0x85, 0x00}},
    {0x4A3D50, (const void *)&PostRace_SetControlMask, "PostRace_SetControlMask", {0x8B, 0x15, 0x78, 0x00, 0x85, 0x00}},
    {0x55A460, (const void *)&CTP_Ctor, "CarTunePauseScreen::CarTunePauseScreen", {0x55, 0x8B, 0xEC, 0x6A, 0xFF, 0x68}},
    {0x55A570, (const void *)&CTP_Dtor, "CarTunePauseScreen::~CarTunePauseScreen", {0xC7, 0x01, 0x0C, 0xD3, 0x79, 0x00}},
    {0x55A550, (const void *)&CTP_ScalarDtor, "CarTunePauseScreen scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x5647E0, (const void *)&CTP_Notify, "CarTunePauseScreen::NotificationMessage", {0x55, 0x8B, 0xEC, 0xA0, 0xA4, 0xA7}},
    {0x510EB0, (const void *)&CTP_NotifySound, "CarTunePauseScreen::NotifySoundMessage", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0xC8}},
    {0x5100E0, (const void *)&CTP_IsOkToResume, "CarTunePauseScreen::IsOkToResume", {0x8B, 0x81, 0x50, 0x01, 0x00, 0x00}},
    {0x557350, (const void *)&CTP_Setup, "CarTunePauseScreen::Setup", {0xA1, 0x44, 0xA6, 0x83, 0x00, 0x83}},
    {0x53C9E0, (const void *)&CTP_SetupDyno, "CarTunePauseScreen::SetupDynoPause", {0x53, 0x56, 0x57, 0x6A, 0x48, 0x8B}},
    {0x5227E0, (const void *)&CTP_SetupTuning, "CarTunePauseScreen::SetupTuningPause", {0x6A, 0xFF, 0x68, 0xE4, 0x72, 0x77}},
    {0x549760, (const void *)&CTP_RefreshHeader, "CarTunePauseScreen::RefreshHeader", {0x56, 0x8B, 0xF1, 0xE8, 0xE8, 0x42}},
    {0x53CC70, (const void *)&CTP_DoQuitTuning, "CarTunePauseScreen::DoQuitTuning", {0x53, 0x56, 0x68, 0x00, 0x00, 0x79}},
    {0x5107D0, (const void *)&MDB_Car, "SCMagazineDatabase::GetCarSetupByHash", {0x56, 0x8B, 0x31, 0x33, 0xC0, 0x85}},
    {0x510800, (const void *)&MDB_Render, "SCMagazineDatabase::GetRenderSetupByHash", {0x56, 0x8B, 0x71, 0x04, 0x33, 0xC0}},
    {0x510840, (const void *)&MDB_Mag, "SCMagazineDatabase::GetMagazineSetupByHash", {0x56, 0x8B, 0x71, 0x08, 0x33, 0xC0}},
    {0x510880, (const void *)&MDB_MagIndex, "SCMagazineDatabase::GetMagazineSetupByIndex", {0x8B, 0x44, 0x24, 0x04, 0x6B, 0xC0}},
    {0x510B80, (const void *)&MagazineViewer::GetImageNameFromShowcaseDataIndex, "MagazineViewer::GetImageNameFromShowcaseDataIndex", {0x53, 0x55, 0x56, 0x8B, 0x74, 0x24}},
    {0x523CB0, (const void *)&MagazineViewer::GetImageNameFromFileName, "MagazineViewer::GetImageNameFromFileName", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x56}},
    {0x510AF0, (const void *)&MagazineViewer::MakeProfileImageName, "MagazineViewer::MakeProfileImageName", {0x56, 0x57, 0x68, 0xE0, 0xA9, 0x83}},
    {0x523C70, (const void *)&MagazineViewer::GetCurrentImageName, "MagazineViewer::SetMagSetupHash", {0x8B, 0x15, 0x08, 0x84, 0x83, 0x00}},
    {0x510C30, (const void *)&MagazineViewer::GetCurrentMode, "MagazineViewer::GetCurrentMode", {0xA1, 0x78, 0x85, 0x83, 0x00, 0xC3}},
    {0x510C40, (const void *)&MagazineViewer::GetMagSetupHash, "MagazineViewer::GetMagSetupHash", {0xA1, 0xD8, 0x7F, 0x83, 0x00, 0xC3}},
    {0x510C50, (const void *)&MagazineViewer::EnterIngameMagazineReward, "MagazineViewer::EnterIngameMagazineReward", {0xC7, 0x05, 0x78, 0x85, 0x83, 0x00}},
    {0x510C60, (const void *)&MagazineViewer::EnterIngameDvdReward, "MagazineViewer::EnterIngameDvdReward", {0xC7, 0x05, 0x78, 0x85, 0x83, 0x00}},
    {0x510C70, (const void *)&MagazineViewer::ReturnFromIngameReward, "MagazineViewer::ReturnFromIngameReward", {0xC7, 0x05, 0x78, 0x85, 0x83, 0x00}},
    {0x53DD30, (const void *)&MagazineViewer::EnterFrontendMagazineReward, "MagazineViewer::EnterFrontendMagazineReward", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x53DD70, (const void *)&MagazineViewer::EnterFrontendDvdReward, "MagazineViewer::EnterFrontendDvdReward", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x53DDB0, (const void *)&MagazineViewer::EnterFrontendMagazineSelect, "MagazineViewer::EnterFrontendMagazineSelect", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x68}},
    {0x53DDF0, (const void *)&MagazineViewer::EnterFrontendDvdSelect, "MagazineViewer::EnterFrontendDvdSelect", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x68}},
    {0x53DE30, (const void *)&MagazineViewer::ReturnToFrontend, "MagazineViewer::ReturnToFrontend", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x53DE60, (const void *)&MAG_ThingReact, "MagazineThing::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x557F20, (const void *)&MAG_Ctor, "UIMagazineSelect::UIMagazineSelect", {0x6A, 0xFF, 0x68, 0x78, 0x7D, 0x77}},
    {0x558010, (const void *)&MAG_Dtor, "UIMagazineSelect::~UIMagazineSelect", {0xC7, 0x01, 0x04, 0xCE, 0x79, 0x00}},
    {0x557FF0, (const void *)&MAG_ScalarDtor, "UIMagazineSelect scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x54B210, (const void *)&MAG_Notify, "UIMagazineSelect::NotificationMessage", {0x8B, 0x44, 0x24, 0x10, 0x8B, 0x54}},
    {0x550DD0, (const void *)&MAG_Setup, "UIMagazineSelect::Setup", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x04}},
    {0x523F60, (const void *)&MAG_Build, "UIMagazineSelect::BuildShowcaseList", {0x6A, 0xFF, 0x68, 0x24, 0x73, 0x77}},
    {0x54B150, (const void *)&MAG_Populate, "UIMagazineSelect::PopulateDataInfoStuff", {0x53, 0x56, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x510E10, (const void *)&MAG_Dealloc, "UIMagazineSelect::DeallocateScreenShot", {0x56, 0x8B, 0xF1, 0x8A, 0x86, 0x54}},
    {0x53C7D0, (const void *)&CT_React<CTKind::Resume>, "ct Resume React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x564C80, (const void *)&CT_React<CTKind::Restart>, "ct Restart React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x53C830, (const void *)&CT_React<CTKind::DynoChart>, "ct DynoChart React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x564CF0, (const void *)&CT_React<CTKind::QuitToTuning>, "ct QuitToTuning React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x53C860, (const void *)&CT_React<CTKind::Options>, "ct Options React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x53C890, (const void *)&CT_React<CTKind::TuningMaster>, "ct TuningMaster React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x53C8C0, (const void *)&CT_React<CTKind::BackToDyno>, "ct BackToDyno React", {0x55, 0x8B, 0xEC, 0x81, 0x7D, 0x0C}},
    {0x564D60, (const void *)&CT_React<CTKind::DiscardChanges>, "ct DiscardChanges React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x564DD0, (const void *)&CT_React<CTKind::QuitTuning>, "ct QuitTuning React", {0x55, 0x8B, 0xEC, 0x81, 0x7D, 0x0C}},
    {0x55A580, (const void *)&CT_React<CTKind::GoToSetting>, "ct GoToSetting React", {0x55, 0x8B, 0xEC, 0x81, 0x7D, 0x0C}},
    {0x4BEC90, (const void *)&PR_React<PRKind::NextEvent>, "pr NextEvent React", {0x55, 0x8B, 0xEC, 0x81, 0x7D, 0x0C}},
    {0x4BEC20, (const void *)&PR_React<PRKind::QuitToFreeRoam>, "pr QuitToFreeRoam React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4BEDC0, (const void *)&PR_React<PRKind::WorldMap>, "pr WorldMap React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4BECC0, (const void *)&PR_React<PRKind::RestartRace>, "pr RestartRace React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4BED10, (const void *)&PR_React<PRKind::Stats>, "pr Stats React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4BED90, (const void *)&PR_React<PRKind::Replay>, "pr Replay React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4BED40, (const void *)&PR_React<PRKind::QuitToFrontEnd>, "pr QuitToFrontEnd React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x527660, (const void *)&ISX_StaticArrow2, "FEScrollBar::StaticArrow2", {0x56, 0x57, 0x8B, 0xF9, 0x8B, 0x77}},
    {0x51D7F0, (const void *)&FEngSetColor, "FEngSetColor", {0x83, 0xEC, 0x10, 0x56, 0x8B, 0x74}},
    {0x50CCC0, (const void *)&ISX_GetObjectColor, "FEngGetObjectColor", {0x8B, 0x44, 0x24, 0x08, 0x33, 0xC9}},
    {0x5054D0, (const void *)&ISX_ColorPack, "FEColor pack", {0x8B, 0x41, 0x08, 0x85, 0xC0, 0x56}},
    {0x495570, (const void *)&FEngGetObjectColorPacked, "FEngGetObjectColor packed", {0x8B, 0x44, 0x24, 0x04, 0x83, 0xEC}},
    {0x4E3560, (const void *)&UOM_Ctor, "UIOptionsMain::UIOptionsMain", {0x6A, 0xFF, 0x68, 0x78, 0x61, 0x77}},
    {0x4E36A0, (const void *)&UOM_Dtor, "UIOptionsMain::~UIOptionsMain", {0xC7, 0x01, 0x94, 0x77, 0x79, 0x00}},
    {0x4E3680, (const void *)&UOM_ScalarDtor, "UIOptionsMain scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4B6080, (const void *)&UOM_Notify, "UIOptionsMain::NotificationMessage", {0x8B, 0x44, 0x24, 0x10, 0x8B, 0x54}},
    {0x49CB40, (const void *)&UOM_NotifySound, "UIOptionsMain::NotifySoundMessage", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x4C}},
    {0x4B6230, (const void *)&UOM_Setup, "UIOptionsMain::Setup", {0x6A, 0xFF, 0x68, 0x84, 0x31, 0x77}},
    {0x4EB580, (const void *)&UPOM_Ctor, "UIPauseOptionsMain::UIPauseOptionsMain", {0x6A, 0xFF, 0x68, 0x78, 0x61, 0x77}},
    {0x4EB640, (const void *)&UPOM_Dtor, "UIPauseOptionsMain::~UIPauseOptionsMain", {0xC7, 0x01, 0xCC, 0x7C, 0x79, 0x00}},
    {0x4EB620, (const void *)&UPOM_ScalarDtor, "UIPauseOptionsMain scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4A7D60, (const void *)&UPOM_Notify, "UIPauseOptionsMain::NotificationMessage", {0x8B, 0x44, 0x24, 0x08, 0x53, 0x8B}},
    {0x4C3000, (const void *)&UPOM_Setup, "UIPauseOptionsMain::Setup", {0x6A, 0xFF, 0x68, 0x30, 0x3D, 0x77}},
    {0x4B6010, (const void *)&ExitFEOptionsSystem, "ExitFEOptionsSystem", {0x53, 0x8A, 0x1D, 0xD8, 0x4A, 0x84}},
    {0x50D1A0, (const void *)&OM_FilterSoundH, "FilterSoundForHorizontalMenu", {0x8B, 0x44, 0x24, 0x08, 0x83, 0xF8}},
    {0x49C5F0, (const void *)&OM_SetPlayerToEdit, "SetPlayerToEditForOptions", {0x8B, 0x44, 0x24, 0x04, 0xA3, 0x64}},
    {0x537CC0, (const void *)&OM_IsScriptRunningName, "FEngIsScriptRunning(pkg, hash, name)", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x49A620, (const void *)&OMAudio_React, "OMAudio::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x49A640, (const void *)&OMVideo_React, "OMVideo::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x49A660, (const void *)&OMGameplay_React, "OMGameplay::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x49A680, (const void *)&OMPlayer_React, "OMPlayer::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x49A6A0, (const void *)&OMController_React, "OMController::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x49A6C0, (const void *)&OMEATrax_React, "OMEATrax::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x49A6E0, (const void *)&OMInline_React, "OMInline::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x49A700, (const void *)&OMCredits_React, "OMCredits::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4EB410, (const void *)&PM_Ctor, "PauseMenu::PauseMenu", {0x6A, 0xFF, 0x68, 0x78, 0x61, 0x77}},
    {0x4EB510, (const void *)&PM_Dtor, "PauseMenu::~PauseMenu", {0x6A, 0xFF, 0x68, 0x78, 0x61, 0x77}},
    {0x4EB4F0, (const void *)&PM_ScalarDtor, "PauseMenu scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4C2170, (const void *)&PM_Notify, "PauseMenu::NotificationMessage", {0x53, 0x8B, 0x5C, 0x24, 0x14, 0x55}},
    {0x4D9300, (const void *)&PM_Setup, "PauseMenu::Setup", {0x53, 0x56, 0x57, 0x8B, 0xF1, 0x8B}},
    {0x4C2900, (const void *)&PM_SetupExplore, "PauseMenu::SetupExplore", {0x6A, 0xFF, 0x68, 0xD0, 0x3B, 0x77}},
    {0x4C2BD0, (const void *)&PM_SetupRace, "PauseMenu::SetupRace", {0x6A, 0xFF, 0x68, 0xD0, 0x3C, 0x77}},
    {0x4A7BC0, (const void *)&PM_OkToHandle, "PauseMenu::OkToHandleInput", {0x8B, 0x81, 0x50, 0x01, 0x00, 0x00}},
    {0x4A7BF0, (const void *)&PM_NeedToSeeOptions, "PauseMenu::NeedToSeeOptions", {0x56, 0x8B, 0x35, 0xD8, 0x00, 0x89}},
    {0x4A7C40, (const void *)&GetPauseMenuReplayIndex, "GetPauseMenuReplayIndex", {0x8B, 0x0D, 0xB0, 0xE7, 0x89, 0x00}},
    {0x4A7C70, (const void *)&PM_QuitToCrib, "PauseMenu::QuitToCrib", {0xB9, 0x40, 0xAD, 0x85, 0x00, 0xE8}},
    {0x4A7CC0, (const void *)&PM_QuitToWorldMap, "PauseMenu::QuitToWorldMap", {0xB9, 0x40, 0xAD, 0x85, 0x00, 0xE8}},
    {0x4A7CF0, (const void *)&PM_QuitGame, "PauseMenu::QuitGame", {0x56, 0x8B, 0xF1, 0x6A, 0x00, 0xB9}},
    {0x4C2640, (const void *)&PM_ReplayCtor, "pm_Replay::pm_Replay", {0x8B, 0x44, 0x24, 0x0C, 0x8B, 0x54}},
    {0x4C27F0, (const void *)&PM_QuitFECtor, "pm_QuitToFrontEnd::pm_QuitToFrontEnd", {0x8B, 0x44, 0x24, 0x0C, 0x8B, 0x54}},
    {0x4C24C0, (const void *)&pm_ResumeRace_React, "pm_ResumeRace::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4C2510, (const void *)&pm_ResumeFreeRoam_React, "pm_ResumeFreeRoam::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4C2560, (const void *)&pm_ReturnToWorldMap_React, "pm_ReturnToWorldMap::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4C25D0, (const void *)&pm_RestartRace_React, "pm_RestartRace::React", {0xA1, 0xD8, 0x00, 0x89, 0x00, 0x83}},
    {0x4C26B0, (const void *)&pm_Replay_React, "pm_Replay::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4C2700, (const void *)&pm_SwitchToOptions_React, "pm_SwitchToOptions::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4C2730, (const void *)&pm_QuitToCrib_React, "pm_QuitToCrib::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4C2780, (const void *)&pm_QuitRaceToFreeRoam_React, "pm_QuitRaceToFreeRoam::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4C2820, (const void *)&pm_QuitToFrontEnd_React, "pm_QuitToFrontEnd::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4DF140, (const void *)&UIM_Ctor, "UIMain::UIMain", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x4DF360, (const void *)&UIM_Dtor, "UIMain::~UIMain", {0xC7, 0x01, 0x10, 0x75, 0x79, 0x00}},
    {0x4DF340, (const void *)&UIM_ScalarDtor, "UIMain scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4F98B0, (const void *)&UIM_Notify, "UIMain::NotificationMessage", {0x8B, 0x44, 0x24, 0x10, 0x8B, 0x54}},
    {0x4AEE30, (const void *)&UIM_Setup, "UIMain::Setup", {0x6A, 0xFF, 0x68, 0x7C, 0x2C, 0x77}},
    {0x4FC050, (const void *)&MMQuickRace_React, "MMQuickRace::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AECB0, (const void *)&MMOnline_React, "MMOnline::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AECE0, (const void *)&MMOptions_React, "MMOptions::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AED30, (const void *)&MMCarSelect_React, "MMCarSelect::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AED80, (const void *)&MMCareer_React, "MMCareer::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AEDB0, (const void *)&MMSaveLoad_React, "MMSaveLoad::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AEDE0, (const void *)&MMSplitScreen_React, "MMSplitScreen::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x50D2A0, (const void *)&MM_MapJoy, "FEngMapJoyParamToJoyport", {0x8B, 0x44, 0x24, 0x04, 0xA8, 0x01}},
    {0x50CEE0, (const void *)&MM_JoyActive, "FEngSetJoyPortActive", {0x8B, 0x4C, 0x24, 0x04, 0xB8, 0x01}},
    {0x4E2A50, (const void *)&QRMS_Ctor, "UIQRModeSelect::UIQRModeSelect", {0x6A, 0xFF, 0x68, 0x78, 0x61, 0x77}},
    {0x4E2C60, (const void *)&QRMS_Dtor, "UIQRModeSelect::~UIQRModeSelect", {0xC7, 0x01, 0x34, 0x77, 0x79, 0x00}},
    {0x4E2C40, (const void *)&QRMS_ScalarDtor, "UIQRModeSelect scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4B2B30, (const void *)&QRMS_Notify, "UIQRModeSelect::NotificationMessage", {0x8B, 0x44, 0x24, 0x10, 0x8B, 0x54}},
    {0x4B2D60, (const void *)&QRMS_Setup, "UIQRModeSelect::Setup", {0x6A, 0xFF, 0x68, 0x70, 0x2F, 0x77}},
    {0x4B2BE0, (const void *)&MSCircuit_React, "MSCircuit::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4B2C70, (const void *)&MSSprint_React, "MSSprint::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4B2C90, (const void *)&MSDrag_React, "MSDrag::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4B2CB0, (const void *)&MSDrift_React, "MSDrift::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4B2CD0, (const void *)&MSLan_React, "MSLan::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4B2CF0, (const void *)&MSFreeRun_React, "MSFreeRun::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4B2D20, (const void *)&MSShortTrack_React, "MSShortTrack::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4B2D40, (const void *)&MSURL_React, "MSURL::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4B2C00, (const void *)&QR_SetGameModeHook, "_SetGameMode", {0xA1, 0x78, 0x00, 0x85, 0x00, 0x83}},
    {0x4DF390, (const void *)&USC_Dtor, "UIStartCareer::~UIStartCareer", {0xC7, 0x01, 0x2C, 0x75, 0x79, 0x00}},
    {0x4DF370, (const void *)&USC_ScalarDtor, "UIStartCareer scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4AF2C0, (const void *)&USC_Notify, "UIStartCareer::NotificationMessage", {0x8B, 0x44, 0x24, 0x10, 0x8B, 0x54}},
    {0x4AF110, (const void *)&USC_Setup, "UIStartCareer::Setup", {0x6A, 0xFF, 0x68, 0xE0, 0x2C, 0x77}},
    {0x4ED710, (const void *)&USC_Create, "UIStartCareer factory", {0x6A, 0xFF, 0x68, 0xB4, 0x62, 0x77}},
    {0x4AF090, (const void *)&SCNewCareerMemcard_React, "SCNewCareerMemcard::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AF0C0, (const void *)&SCStart_React, "SCStart::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AF0E0, (const void *)&SCLoad_React, "SCLoad::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4EB700, (const void *)&UPM_Ctor, "UIProfileManager::UIProfileManager", {0x6A, 0xFF, 0x68, 0x78, 0x61, 0x77}},
    {0x4EB790, (const void *)&UPM_Dtor, "UIProfileManager::~UIProfileManager", {0xC7, 0x01, 0x14, 0x7D, 0x79, 0x00}},
    {0x4EB770, (const void *)&UPM_ScalarDtor, "UIProfileManager scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4D9800, (const void *)&UPM_Notify, "UIProfileManager::NotificationMessage", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x4C58B0, (const void *)&UPM_Setup, "UIProfileManager::Setup", {0x6A, 0xFF, 0x68, 0x70, 0x40, 0x77}},
    {0x4EB7A0, (const void *)&UDP_Ctor, "UIDeleteProfile::UIDeleteProfile", {0x6A, 0xFF, 0x68, 0x78, 0x61, 0x77}},
    {0x4EB830, (const void *)&UDP_Dtor, "UIDeleteProfile::~UIDeleteProfile", {0xC7, 0x01, 0x30, 0x7D, 0x79, 0x00}},
    {0x4EB810, (const void *)&UDP_ScalarDtor, "UIDeleteProfile scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4A93E0, (const void *)&UDP_Notify, "UIDeleteProfile::NotificationMessage", {0x8B, 0x44, 0x24, 0x10, 0x8B, 0x54}},
    {0x4C5A40, (const void *)&UDP_Setup, "UIDeleteProfile::Setup", {0x6A, 0xFF, 0x68, 0xA8, 0x40, 0x77}},
    {0x4A9360, (const void *)&UPM_Refresh, "UIProfileManager::Refresh", {0x56, 0x8B, 0xF1, 0x57, 0xB9, 0xE0}},
    {0x4C57F0, (const void *)&PMSave_React, "PMSave::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4C5820, (const void *)&PMLoad_React, "PMLoad::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4C5850, (const void *)&PMDelete_React, "PMDelete::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4C5880, (const void *)&PMCreateNew_React, "PMCreateNew::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4DF3A0, (const void *)&CCR_Ctor, "uiCareerCrib::uiCareerCrib", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF0}},
    {0x4DF560, (const void *)&CCR_Dtor, "uiCareerCrib::~uiCareerCrib", {0xC7, 0x01, 0x48, 0x75, 0x79, 0x00}},
    {0x4DF540, (const void *)&CCR_ScalarDtor, "uiCareerCrib scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4AF3A0, (const void *)&CCR_Notify, "uiCareerCrib::NotificationMessage", {0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x14}},
    {0x4AF820, (const void *)&CCR_Setup, "uiCareerCrib::Setup", {0x6A, 0xFF, 0x68, 0x90, 0x2D, 0x77}},
    {0x4AF360, (const void *)&CCR_CheckMemory, "uiCareerCrib::CheckMemory", {0x56, 0x8B, 0xF1, 0xB9, 0xD0, 0xA9}},
    {0x495AF0, (const void *)&CCR_DoStartTuning, "uiCareerCrib::DoStartTuning", {0x8B, 0x41, 0x04, 0x6A, 0x00, 0x50}},
    {0x4AF630, (const void *)&CCEnterGameplay_React, "CCEnterGameplay::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AF680, (const void *)&CCChangeParts_React, "CCChangeParts::React", {0x55, 0x8B, 0xEC, 0x81, 0x7D, 0x0C}},
    {0x4AF6B0, (const void *)&CCCarSel_React, "CCCarSel::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AF6E0, (const void *)&CCViewSponsors_React, "CCViewSponsors::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AF710, (const void *)&CCViewWorldMap_React, "CCViewWorldMap::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AF740, (const void *)&CCSaveMemcard_React, "CCSaveMemcard::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AF780, (const void *)&CCTestTrack_React, "CCTestTrack::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4AF7D0, (const void *)&CCBackToMain_React, "CCBackToMain::React", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4D93C0, (const void *)&UPO_Ctor, "UIPauseOptions::UIPauseOptions", {0x6A, 0xFF, 0x68, 0x28, 0x6E, 0x77}},
    {0x4D9470, (const void *)&UPO_Dtor, "UIPauseOptions::~UIPauseOptions", {0xC7, 0x01, 0x04, 0x65, 0x79, 0x00}},
    {0x4EB6B0, (const void *)&UPO_ScalarDtor, "UIPauseOptions scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0xB8, 0xDD}},
    {0x4A7F70, (const void *)&UPO_ReadyToExit, "UIPauseOptions::ReadyToExit", {0x8A, 0x15, 0xD8, 0x4A, 0x84, 0x00}},
    {0x4A8010, (const void *)&UPO_Notify, "UIPauseOptions::NotificationMessage", {0x56, 0x8B, 0xF1, 0x8A, 0x86, 0x5B}},
    {0x4C4BD0, (const void *)&UPO_Setup, "UIPauseOptions::Setup", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x04}},
    {0x4C3370, (const void *)&UPO_SetupAudio, "UIPauseOptions::SetupAudio", {0x6A, 0xFF, 0x68, 0xCC, 0x3D, 0x77}},
    {0x4C36C0, (const void *)&UPO_SetupVideo, "UIPauseOptions::SetupVideo", {0x6A, 0xFF, 0x68, 0x30, 0x3E, 0x77}},
    {0x4C38F0, (const void *)&UPO_SetupGameplay, "UIPauseOptions::SetupGameplay", {0x6A, 0xFF, 0x68, 0xCC, 0x3E, 0x77}},
    {0x4C4740, (const void *)&UPO_SetupPlayer, "UIPauseOptions::SetupPlayer", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x50B8A0, (const void *)&FEngGetLastButton, "FEngGetLastButton", {0x8B, 0x44, 0x24, 0x04, 0x50, 0xE8}},
    {0x50B8C0, (const void *)&FEngSetLastButton, "FEngSetLastButton", {0x8B, 0x44, 0x24, 0x04, 0x50, 0xE8}},
    {0x4CF750, (const void *)&UOS_Ctor, "UIOptionsScreen::UIOptionsScreen", {0x6A, 0xFF, 0x68, 0x28, 0x6E, 0x77}},
    {0x4CFAF0, (const void *)&UOS_ScalarDtor, "UIOptionsScreen scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4CFB10, (const void *)&UOS_Dtor, "UIOptionsScreen::~UIOptionsScreen", {0xC7, 0x01, 0x28, 0x5E, 0x79, 0x00}},
    {0x49C690, (const void *)&UOS_Notify, "UIOptionsScreen::NotificationMessage", {0x53, 0x56, 0x8B, 0xF1, 0x8A, 0x86}},
    {0x4CFAC0, (const void *)&UOS_NotifySound, "UIOptionsScreen::NotifySoundMessage", {0x8A, 0x81, 0x4E, 0x01, 0x00, 0x00}},
    {0x49C600, (const void *)&UOS_CheckForWheels, "UIOptionsScreen::CheckForWheels", {0x83, 0x3D, 0x04, 0xAA, 0x83, 0x00}},
    {0x4B8470, (const void *)&UOS_Setup, "UIOptionsScreen::Setup", {0x56, 0x57, 0x8B, 0xF1, 0x8B, 0x46}},
    {0x4B6E60, (const void *)&UOS_SetupAudio, "UIOptionsScreen::SetupAudio", {0x6A, 0xFF, 0x68, 0x10, 0x34, 0x77}},
    {0x4B7180, (const void *)&UOS_SetupVideo, "UIOptionsScreen::SetupVideo", {0x6A, 0xFF, 0x68, 0x30, 0x3E, 0x77}},
    {0x4B7310, (const void *)&UOS_SetupGameplay, "UIOptionsScreen::SetupGameplay", {0x6A, 0xFF, 0x68, 0xAC, 0x34, 0x77}},
    {0x4B7CD0, (const void *)&UOS_SetupPlayer, "UIOptionsScreen::SetupPlayer", {0x6A, 0xFF, 0x68, 0xB0, 0x35, 0x77}},
    {0x4B81A0, (const void *)&UOS_SetupOnline, "UIOptionsScreen::SetupOnline", {0x6A, 0xFF, 0x68, 0xE8, 0x35, 0x77}},
    {0x4B8350, (const void *)&UOS_SetupTrailers, "UIOptionsScreen::SetupTrailers", {0x6A, 0xFF, 0x68, 0x3C, 0x36, 0x77}},
    {0x51FE70, (const void *)&UIWidgetMenu_GetWidget, "UIWidgetMenu::GetWidget", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x49B2F0, (const void *)&OB_WheelAct, "COWheelOptions::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0xE3}},
    {0x49B330, (const void *)&OB_WheelDraw, "COWheelOptions::Draw", {0x8B, 0x41, 0x2C, 0x68, 0x31, 0xB9}},
    {0x4B5FD0, (const void *)&OB_WheelEnable, "COWheelOptions::Enable", {0x8B, 0x41, 0x2C, 0x6A, 0x01, 0x68}},
    {0x4B5FF0, (const void *)&OB_WheelDisable, "COWheelOptions::Disable", {0x8B, 0x41, 0x2C, 0x6A, 0x01, 0x68}},
    {0x49B930, (const void *)&OB_ForwardAct, "PCButton792060::Act", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x49B960, (const void *)&OB_792060Draw, "PCButton792060::Draw", {0x8B, 0x41, 0x2C, 0x68, 0xE9, 0xE1}},
    {0x49B980, (const void *)&OB_DefaultsAct, "PCDefaults7920A0::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0x10}},
    {0x49B9E0, (const void *)&OB_DefaultsDraw, "PCDefaults7920A0::Draw", {0x8B, 0x41, 0x2C, 0x68, 0xEB, 0xED}},
    {0x4B8300, (const void *)&OB_TrailerAct, "TOSSX/TONFLStreet/NASCAR::Act", {0x81, 0x7C, 0x24, 0x08, 0x10, 0x72}},
    {0x4B82C0, (const void *)&OB_TrailerDrawSSX, "TOSSX::Draw", {0x8B, 0x41, 0x2C, 0x68, 0xC0, 0x44}},
    {0x4B82E0, (const void *)&OB_TrailerDrawNFL, "TONFLStreet::Draw", {0x8B, 0x41, 0x2C, 0x68, 0x08, 0x45}},
    {0x4B8330, (const void *)&OB_TrailerDrawNascar, "PCTONascar::Draw", {0x8B, 0x41, 0x2C, 0x68, 0x58, 0x45}},
    {0x49B600, (const void *)&OC_Resolution_Act, "PCResolution791F30::Act", {0x8B, 0x54, 0x24, 0x08, 0x81, 0xFA}},
    {0x49B690, (const void *)&OC_Resolution_Draw, "PCResolution791F30::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49B740, (const void *)&Detail_Act, "PCDetailSlider791F78::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49B7A0, (const void *)&Detail_Draw, "PCDetailSlider791F78::Draw", {0x8B, 0x41, 0x2C, 0x68, 0x65, 0x76}},
    {0x49B7C0, (const void *)&Detail_Init, "PCDetailSlider791F78::SetInitialValues", {0x51, 0xDB, 0x05, 0x9C, 0x09, 0x87}},
    {0x49AD40, (const void *)&OC_JumpCams_Act, "GOJumpCams::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0xF1}},
    {0x49ADC0, (const void *)&OC_JumpCams_Draw, "GOJumpCams::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49B170, (const void *)&OC_AutoSave_Act, "GOAutoSave::Act", {0x53, 0x56, 0x57, 0x8B, 0x7C, 0x24}},
    {0x49B210, (const void *)&OC_AutoSave_Draw, "GOAutoSave::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B75D0, (const void *)&OC_Transmission_Act, "POTransmission::Act", {0x8B, 0x54, 0x24, 0x08, 0x81, 0xFA}},
    {0x4B7630, (const void *)&OC_Transmission_Draw, "POTransmission::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B7690, (const void *)&OC_Handling_Act, "POHandling::Act", {0x8B, 0x54, 0x24, 0x08, 0x81, 0xFA}},
    {0x4B76F0, (const void *)&OC_Handling_Draw, "POHandling::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B7750, (const void *)&OC_DriveCam_Act, "PODriveCam::Act", {0xA1, 0x64, 0x63, 0x83, 0x00, 0x8B}},
    {0x4B77B0, (const void *)&OC_DriveCam_Draw, "PODriveCam::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B7AA0, (const void *)&OC_SplitTime_Act, "POSplitTime::Act", {0xA1, 0x64, 0x63, 0x83, 0x00, 0x8B}},
    {0x4B7B00, (const void *)&OC_SplitTime_Draw, "POSplitTime::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49AB80, (const void *)&OC_AudioMode_Act, "AOAudioMode::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0x9E}},
    {0x49ABD0, (const void *)&OC_AudioMode_Draw, "AOAudioMode::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49AE80, (const void *)&OC_SpeedoUnits_Act, "GOSpeedoUnits::Act", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x49AEE0, (const void *)&OC_SpeedoUnits_Draw, "GOSpeedoUnits::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49AFE0, (const void *)&OC_RacingMiniMap_Act, "GORacingMiniMap::Act", {0x8B, 0x15, 0x78, 0x00, 0x85, 0x00}},
    {0x49B040, (const void *)&OC_RacingMiniMap_Draw, "GORacingMiniMap::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49AF20, (const void *)&OC_Difficulty_Act, "GOCareerDifficulty::Act", {0xA1, 0x30, 0xAC, 0x83, 0x00, 0x56}},
    {0x49AF70, (const void *)&OC_Difficulty_Draw, "GOCareerDifficulty::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49B0B0, (const void *)&OC_ExploringMiniMap_Act, "GOExploringMiniMap::Act", {0x0F, 0xB6, 0x05, 0x1C, 0xAA, 0x83}},
    {0x49B100, (const void *)&OC_ExploringMiniMap_Draw, "GOExploringMiniMap::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49AC20, (const void *)&OptToggle_Act<0>, "OOShowPlayers::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0x9E}},
    {0x49AC60, (const void *)&OptToggle_Draw<0>, "OOShowPlayers::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49ACB0, (const void *)&OptToggle_Act<1>, "OOBadConnectionWarning::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0x9E}},
    {0x49ACF0, (const void *)&OptToggle_Draw<1>, "OOBadConnectionWarning::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49AE00, (const void *)&OptToggle_Act<2>, "GORearview::Act", {0x8B, 0x44, 0x24, 0x08, 0x3D, 0x9E}},
    {0x49AE40, (const void *)&OptToggle_Draw<2>, "GORearview::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B7860, (const void *)&OptToggle_Act<3>, "POGauges::Act", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x4B78B0, (const void *)&OptToggle_Draw<3>, "POGauges::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B78F0, (const void *)&OptToggle_Act<4>, "POPosition::Act", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x4B7940, (const void *)&OptToggle_Draw<4>, "POPosition::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B7980, (const void *)&OptToggle_Act<5>, "POLapInfo::Act", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x4B79D0, (const void *)&OptToggle_Draw<5>, "POLapInfo::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B7A10, (const void *)&OptToggle_Act<6>, "POScore::Act", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x4B7A60, (const void *)&OptToggle_Draw<6>, "POScore::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B7BB0, (const void *)&OptToggle_Act<7>, "POLeaderBoard::Act", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x4B7C00, (const void *)&OptToggle_Draw<7>, "POLeaderBoard::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B7C40, (const void *)&OptToggle_Act<8>, "POTrannyPrompt::Act", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x4B7C90, (const void *)&OptToggle_Draw<8>, "POTrannyPrompt::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49B260, (const void *)&OptToggle_Act<9>, "PCPlayerToggle791DB0::Act", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x49B2B0, (const void *)&OptToggle_Draw<9>, "PCPlayerToggle791DB0::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B5F70, (const void *)&OptToggle_Enable, "PCPlayerToggle791DB0::Enable", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x4B5FA0, (const void *)&OptToggle_Disable, "PCPlayerToggle791DB0::Disable", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x2C}},
    {0x49A720, (const void *)&AOVol_Act<0>, "AOMasterVol::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49A770, (const void *)&AOVol_Draw<0>, "AOMasterVol::Draw", {0x8B, 0x41, 0x2C, 0x68, 0x3E, 0x2C}},
    {0x49A790, (const void *)&AOVol_Init<0>, "AOMasterVol::SetInitialValues", {0x51, 0xA1, 0x28, 0xAA, 0x83, 0x00}},
    {0x49A7C0, (const void *)&AOVol_Act<1>, "AOSFXMasterVol::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49A810, (const void *)&AOVol_Draw<1>, "AOSFXMasterVol::Draw", {0x8B, 0x41, 0x2C, 0x68, 0x43, 0x75}},
    {0x49A830, (const void *)&AOVol_Init<1>, "AOSFXMasterVol::SetInitialValues", {0x51, 0xA1, 0x38, 0xAA, 0x83, 0x00}},
    {0x49A860, (const void *)&AOVol_Act<2>, "AOTireVol::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49A8B0, (const void *)&AOVol_Draw<2>, "AOTireVol::Draw", {0x8B, 0x41, 0x2C, 0x68, 0xC6, 0x45}},
    {0x49A8D0, (const void *)&AOVol_Init<2>, "AOTireVol::SetInitialValues", {0x51, 0xA1, 0x40, 0xAA, 0x83, 0x00}},
    {0x49A900, (const void *)&AOVol_Act<3>, "AOEngineVol::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49A950, (const void *)&AOVol_Draw<3>, "AOEngineVol::Draw", {0x8B, 0x41, 0x2C, 0x68, 0x88, 0xF8}},
    {0x49A970, (const void *)&AOVol_Init<3>, "AOEngineVol::SetInitialValues", {0x51, 0xA1, 0x3C, 0xAA, 0x83, 0x00}},
    {0x49A9A0, (const void *)&AOVol_Act<4>, "AOSpeechVol::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49A9F0, (const void *)&AOVol_Draw<4>, "AOSpeechVol::Draw", {0x8B, 0x41, 0x2C, 0x68, 0x2A, 0xB8}},
    {0x49AA10, (const void *)&AOVol_Init<4>, "AOSpeechVol::SetInitialValues", {0x51, 0xA1, 0x2C, 0xAA, 0x83, 0x00}},
    {0x49AA40, (const void *)&AOVol_Act<5>, "AOFEMusicVol::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49AA90, (const void *)&AOVol_Draw<5>, "AOFEMusicVol::Draw", {0x8B, 0x41, 0x2C, 0x68, 0x1D, 0x68}},
    {0x49AAB0, (const void *)&AOVol_Init<5>, "AOFEMusicVol::SetInitialValues", {0x51, 0xA1, 0x30, 0xAA, 0x83, 0x00}},
    {0x49AAE0, (const void *)&AOVol_Act<6>, "AOIGMusicVol::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49AB30, (const void *)&AOVol_Draw<6>, "AOIGMusicVol::Draw", {0x8B, 0x41, 0x2C, 0x68, 0xC2, 0xEA}},
    {0x49AB50, (const void *)&AOVol_Init<6>, "AOIGMusicVol::SetInitialValues", {0x51, 0xA1, 0x34, 0xAA, 0x83, 0x00}},
    {0x5532B0, (const void *)&CBS_Ctor, "ChooseBrandsScreen::ChooseBrandsScreen", {0x6A, 0xFF, 0x68, 0x98, 0x7D, 0x77}},
    {0x55C930, (const void *)&CBS_Notify, "ChooseBrandsScreen::NotificationMessage", {0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x10}},
    {0x550330, (const void *)&CBS_Setup, "ChooseBrandsScreen::Setup", {0xA0, 0x5D, 0x85, 0x83, 0x00, 0x83}},
    {0x521EF0, (const void *)&CBS_SetChosenBrands, "ChooseBrandsScreen::SetChosenBrands", {0x8B, 0x41, 0x4C, 0x85, 0xC0, 0x56}},
    {0x521E30, (const void *)&BST_Ctor, "BrandScrollerThing::BrandScrollerThing", {0x8B, 0x44, 0x24, 0x18, 0x8B, 0x54}},
    {0x521E90, (const void *)&BST_Act, "BrandScrollerThing::Act", {0x8B, 0x54, 0x24, 0x08, 0x81, 0xFA}},
    {0x53B960, (const void *)&BST_Draw, "BrandScrollerThing::Draw", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x58}},
    {0x4CFBA0, (const void *)&UWOM_Ctor, "UIWheelOptionsMenu::UIWheelOptionsMenu", {0x6A, 0xFF, 0x68, 0x28, 0x6E, 0x77}},
    {0x4CFC80, (const void *)&UWOM_ScalarDtor, "UIWheelOptionsMenu scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x4CFCA0, (const void *)&UWOM_Dtor, "UIWheelOptionsMenu::~UIWheelOptionsMenu", {0xC7, 0x01, 0x4C, 0x5E, 0x79, 0x00}},
    {0x49CBB0, (const void *)&UWOM_Notify, "UIWheelOptionsMenu::NotificationMessage", {0x8B, 0x44, 0x24, 0x10, 0x8B, 0x54}},
    {0x49DFC0, (const void *)&UWOM_NotifySound, "UIWheelOptionsMenu::NotifySoundMessage", {0x8B, 0x44, 0x24, 0x14, 0x8B, 0x4C}},
    {0x4B8A10, (const void *)&UWOM_Setup, "UIWheelOptionsMenu::Setup", {0x6A, 0xFF, 0x68, 0xCC, 0x36, 0x77}},
    {0x50D1D0, (const void *)&FilterSoundForMenu_Hook, "FilterSoundForMenu", {0x8B, 0x44, 0x24, 0x08, 0x8B, 0xC8}},
    {0x537D40, (const void *)&FEngIsScriptRunningId_Hook, "FEngIsScriptRunning(pkg,hash,id)", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x49B350, (const void *)&WCOWheelRange_Act, "WCOWheelRange::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49B400, (const void *)&WCOStrengthOfForce_Act, "WCOStrengthOfForce::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49B550, (const void *)&WCOOverall_Act, "WCOOverallForceStrength::Act", {0x56, 0x57, 0x8B, 0x7C, 0x24, 0x10}},
    {0x49B3A0, (const void *)&WCOWheelRange_Draw, "WCOWheelRange::Draw", {0x8B, 0x41, 0x2C, 0x68, 0xD2, 0xB0}},
    {0x49B4B0, (const void *)&WCOStrengthOfForce_Draw, "WCOStrengthOfForce::Draw", {0x8B, 0x41, 0x2C, 0x68, 0x96, 0xB3}},
    {0x49B5A0, (const void *)&WCOOverall_Draw, "WCOOverallForceStrength::Draw", {0x8B, 0x41, 0x2C, 0x68, 0xE8, 0x2A}},
    {0x49B3C0, (const void *)&WCOWheelRange_SetInit, "WCOWheelRange::SetInitialValues", {0x51, 0x8B, 0x81, 0x98, 0x00, 0x00}},
    {0x49B4D0, (const void *)&WCOStrengthOfForce_SetInit, "WCOStrengthOfForce::SetInitialValues", {0x51, 0x8B, 0x81, 0x98, 0x00, 0x00}},
    {0x49B5C0, (const void *)&WCOOverall_SetInit, "WCOOverallForceStrength::SetInitialValues", {0x51, 0x8B, 0x81, 0x98, 0x00, 0x00}},
    {0x49B450, (const void *)&WCOStrengthOfForce_GetValue, "WCOStrengthOfForce::GetValue", {0x51, 0x8B, 0x81, 0x84, 0x00, 0x00}},
    {0x51D8A0, (const void *)&FEngGetObjBBox, "FEngGetObjBBox", {0x81, 0xEC, 0x10, 0x08, 0x00, 0x00}},
    {0x51D9F0, (const void *)&FEngGet2DExtents, "FEngGet2DExtents", {0x83, 0xEC, 0x18, 0x53, 0x55, 0x8B}},
    {0x52D020, (const void *)static_cast<bool (*)(float, float, const FEObject *)>(&FEngTestForIntersection), "FEngTestForIntersection", {0x8B, 0x44, 0x24, 0x0C, 0x83, 0xEC}},
    {0x51E090, (const void *)&FELabelWidget_Ctor, "FELabelWidget::FELabelWidget", {0x8B, 0xC1, 0x8B, 0x4C, 0x24, 0x04}},
    {0x51E0F0, (const void *)&FELabelWidget_Position, "FELabelWidget::Position", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x10}},
    {0x51E130, (const void *)&FELabelWidget_SetFocus, "FELabelWidget::SetFocus", {0x53, 0x8B, 0xD9, 0x57, 0x8B, 0x7B}},
    {0x51E1B0, (const void *)&FEStatWidget_Ctor, "FEStatWidget::FEStatWidget", {0x8B, 0xC1, 0x8B, 0x4C, 0x24, 0x04}},
    {0x50D570, (const void *)&FEStatWidget_Show, "FEStatWidget::Show", {0x8B, 0xD1, 0x8B, 0x42, 0x2C, 0x50}},
    {0x50D5A0, (const void *)&FEStatWidget_Hide, "FEStatWidget::Hide", {0x8B, 0xD1, 0x8B, 0x42, 0x2C, 0x50}},
    {0x52D930, (const void *)&FEStatWidget_SetPosX, "FEStatWidget::SetPosX", {0xD9, 0x41, 0x0C, 0x8B, 0x44, 0x24}},
    {0x52D990, (const void *)&FEStatWidget_SetPosY, "FEStatWidget::SetPosY", {0xD9, 0x41, 0x10, 0x8B, 0x44, 0x24}},
    {0x52D800, (const void *)&FEStatWidget_Position, "FEStatWidget::Position", {0x83, 0xEC, 0x10, 0x56, 0x8B, 0xF1}},
    {0x50BEC0, (const void *)&FEngFont_GetTextWidth, "FEngFont::GetTextWidth", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x57}},
    {0x50BF40, (const void *)&FEngFont_GetTextHeight, "FEngFont::GetTextHeight", {0x51, 0x53, 0x8B, 0x5C, 0x24, 0x0C}},
    {0x50C020, (const void *)&FEngFont_CalculateXOffset, "FEngFont::CalculateXOffset", {0x8A, 0x44, 0x24, 0x04, 0xA8, 0x01}, 1, 8, 1},   // original SetTopLeft/SetCenter (51D441, 51D575) reuse EDX (justify) for the Y offset
    {0x50C050, (const void *)&FEngFont_CalculateYOffset, "FEngFont::CalculateYOffset", {0x8A, 0x44, 0x24, 0x04, 0xA8, 0x04}},
    {0x6F45B0, (const void *)&FEngFindGlyph, "FEngFindGlyph (bsearch)", {0x55, 0x8B, 0xEC, 0x8B, 0x55, 0x10}},
    {0x50BCE0, (const void *)&FEngFont_GetLineWidth, "FEngFont::GetLineWidth", {0x83, 0xEC, 0x14, 0x57, 0x8B, 0x7C}},
    {0x4FDA70, (const void *)&FEngFontData_FindCharacter, "FEngFontData::FindCharacter", {0x8B, 0x51, 0x0C, 0x56, 0x8B, 0x71}},
    {0x6F45F0, (const void *)&FEngFontData_GetKerning, "FEngFontData::GetKerning", {0x55, 0x8B, 0xEC, 0x56, 0x57, 0x8B}},
    {0x50BA70, (const void *)&FEngIsJoyEventTexture, "FEngIsJoyEventTexture", {0x8B, 0x4C, 0x24, 0x04, 0x32, 0xC0}},
    {0x50BAC0, (const void *)&FEngSkipJoyEventTexture, "FEngSkipJoyEventTexture", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x50BC10, (const void *)&FEngFont_GetCharacterWidth, "FEngFont::GetCharacterWidth", {0x51, 0x8B, 0x44, 0x24, 0x08, 0x53}},
    {0x50C080, (const void *)&FEngFont_GetNumWordWrappedLines, "FEngFont::GetNumWordWrappedLines", {0x83, 0xEC, 0x10, 0x56, 0x8B, 0x74}},
    {0x50BB10, (const void *)&FEngFont_GetJoyEventTextureInfo, "FEngFont::GetJoyEventTextureInfo", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8}},
    {0x4FF980, (const void *)&FEngWideToCharString, "WideToCharString", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x85}},
    {0x519AE0, (const void *)&FEJoyPad_Ctor, "FEJoyPad::FEJoyPad", {0x8B, 0xD1, 0x57, 0x33, 0xC0, 0x8D}},
    {0x52C430, (const void *)&FEngine_Dtor, "FEngine::~FEngine", {0x6A, 0xFF, 0x68, 0x54, 0x76, 0x77}},
    {0x535870, (const void *)&FETypeNode_Dtor, "FETypeNode::~FETypeNode", {0x6A, 0xFF, 0x68, 0x28, 0x70, 0x77}},
    {0x535850, (const void *)&FETypeNode_ScalarDtor, "FETypeNode scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x514720, (const void *)&FEFieldNode_Dtor, "FEFieldNode::~FEFieldNode", {0x6A, 0xFF, 0x68, 0x28, 0x70, 0x77}},
    {0x514700, (const void *)&FEFieldNode_ScalarDtor, "FEFieldNode scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0x18, 0x00}},
    {0x504B50, (const void *)static_cast<void (*)(FEVector3 *, const float *, const FEVector3 *)>(&FEMultMatrix), "FEMultMatrix", {0x8B, 0x44, 0x24, 0x08, 0x8B, 0x4C}, 1, 12, 0},
    {0x504BB0, (const void *)&FEButtonMap_ComputeButtonLocation, "FEButtonMap::ComputeButtonLocation", {0x8B, 0x4C, 0x24, 0x08, 0x83, 0xEC}},
    {0x5189E0, (const void *)&FEButtonMap_GetButtonFrom, "FEButtonMap::GetButtonFrom", {0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8}},
    {0x504FF0, (const void *)&FEJoyPad_HeldFor, "FEJoyPad::HeldFor", {0x53, 0x56, 0x57, 0x8B, 0x7C, 0x24}},
    {0x504FA0, (const void *)&FEJoyPad_Update, "FEJoyPad::Update", {0x8B, 0xD1, 0x8B, 0x4C, 0x24, 0x04}},
    {0x537DA0, (const void *)&FEngPopChildPackages_Hook, "FEngPopChildPackages", {0x51, 0xA1, 0xC4, 0x84, 0x83, 0x00}},
    {0x50C7B0, (const void *)&cFEng_SetCellData_Hook, "cFEng::SetCellData", {0x32, 0xC0, 0xC2, 0x0C, 0x00, 0x90}},
    {0x5378C0, (const void *)&FEngRenderForeground_Hook, "FEngRenderForeground", {0xA1, 0xC4, 0x91, 0x7F, 0x00, 0x85}},
    {0x537930, (const void *)&FEngRenderBackground_Hook, "FEngRenderBackground", {0xA1, 0xC4, 0x91, 0x7F, 0x00, 0x85}},
    {0x525E50, (const void *)&FERefList_ScalarDtor, "FERefList scalar dtor", {0x56, 0x8B, 0xF1, 0x8A, 0x46, 0x04}},
    {0x50B150, (const void *)&FEMouse_HeldFor, "FEMouse::HeldFor", {0x53, 0x56, 0x57, 0x0F, 0xB7, 0x7C}},
    {0x50B0C0, (const void *)&FEMouse_Update, "FEMouse::Update", {0x8B, 0xC1, 0x8B, 0x4C, 0x24, 0x04}},
    {0x555860, (const void *)&FEngine_PushPackageHook, "FEngine::PushPackage", {0x6A, 0xFF, 0x64, 0xA1, 0x00, 0x00}},
    {0x5362E0, (const void *)&FEngine_UnloadPackageHook, "FEngine::UnloadPackage", {0x51, 0x53, 0x55, 0x57, 0x8B, 0xF9}},
    {0x51A8B0, (const void *)&FEngine_AddToIdleList, "FEngine::AddToIdleList", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x51A910, (const void *)&FEngine_RemoveFromLibraryList, "FEngine::RemoveFromLibraryList", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x52C500, (const void *)&FEngine_UnloadLibraryPackage, "FEngine::UnloadLibraryPackage", {0x56, 0x8B, 0x74, 0x24, 0x08, 0x57}},
    {0x518280, (const void *)&FEngine_AddPackageByPriority, "FEngine active list AddByPriority", {0x8B, 0x41, 0x0C, 0x85, 0xC0, 0x8B}},
    {0x555800, (const void *)&FEngine_LoadPackageHook, "FEngine::LoadPackage", {0x83, 0xEC, 0x50, 0x33, 0xC0, 0x89}},
    {0x504C40, (const void *)&FECodeListBox_FillAllCells, "FECodeListBox::FillAllCells", {0x83, 0xEC, 0x0C, 0x56, 0x8B, 0xF1}},
    {0x504DC0, (const void *)&FECodeListBox_Update, "FECodeListBox::Update", {0x56, 0x8B, 0xF1, 0x8B, 0x86, 0xBC}},
    {0x518E70, (const void *)&FECodeListBox_CheckMovement, "FECodeListBox::CheckMovement", {0x8B, 0x54, 0x24, 0x10, 0x53, 0x56}},
    {0x504E40, (const void *)&FECodeListBox_MakeMove, "FECodeListBox::MakeMove", {0x8B, 0x41, 0x68, 0xA8, 0x08, 0x53}},
    {0x518F20, (const void *)&FECodeListBox_ScrollAxis, "FECodeListBox::ScrollSelection(axis)", {0x53, 0x56, 0x8B, 0x74, 0x24, 0x0C}},
    {0x52BD70, (const void *)&FECodeListBox_ScrollSelection, "FECodeListBox::ScrollSelection", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x60}},
    {0x518E20, (const void *)&FECodeListBox::DefaultSelectCallback, "FECodeListBox::DefaultSelectCallback", {0x83, 0xEC, 0x10, 0x8B, 0x44, 0x24}},
    {0x5352D0, (const void *)&FECodeListBox_CopyProperties, "FECodeListBox::CopyProperties", {0x51, 0x55, 0x56, 0x8B, 0xF1, 0x8B}},
    {0x554710, (const void *)&FECodeListBox_CopyCtor, "FECodeListBox::FECodeListBox(copy)", {0x6A, 0xFF, 0x68, 0xD8, 0x74, 0x77}},
    {0x554830, (const void *)&FECodeListBox_Clone, "FECodeListBox::Clone", {0x6A, 0xFF, 0x68, 0x6B, 0x80, 0x77}},
    {0x52B9E0, (const void *)&FECodeListBox_Ctor, "FECodeListBox::FECodeListBox", {0x56, 0x8B, 0xF1, 0xE8, 0x38, 0xF5}},
    {0x52BAA0, (const void *)&FECodeListBox_Dtor, "FECodeListBox::~FECodeListBox", {0x6A, 0xFF, 0x68, 0xD8, 0x74, 0x77}},
    {0x5352B0, (const void *)&FECodeListBox_ScalarDtor, "FECodeListBox scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0xE8, 0x67}},
    {0x52BB30, (const void *)&FECodeListBox_Initialize, "FECodeListBox::Initialize", {0x83, 0xEC, 0x18, 0x53, 0x55, 0x8B}},
    {0x518BB0, (const void *)&FECodeListBox_SetTotalNumColumns, "FECodeListBox::SetTotalNumColumns", {0x8B, 0x41, 0x7C, 0x56, 0x8B, 0x74}},
    {0x518C00, (const void *)&FECodeListBox_SetTotalNumRows, "FECodeListBox::SetTotalNumRows", {0x8B, 0x81, 0x80, 0x00, 0x00, 0x00}},
    {0x518C60, (const void *)&FECodeListBox_AllocateStrings, "FECodeListBox::AllocateStrings", {0x83, 0xEC, 0x08, 0x53, 0x8B, 0x5C}},
    {0x5196B0, (const void *)&FECodeListBox_SetCellColor, "FECodeListBox::SetCellColor", {0x51, 0x8B, 0x44, 0x24, 0x0C, 0x8B}},
    {0x5197D0, (const void *)&FECodeListBox_SetCellScale, "FECodeListBox::SetCellScale", {0x51, 0x8B, 0x44, 0x24, 0x0C, 0x8B}},
    {0x5198F0, (const void *)&FECodeListBox_SetCellJustification, "FECodeListBox::SetCellJustification", {0x51, 0x8B, 0x44, 0x24, 0x0C, 0x8B}},
    {0x504790, (const void *)&FEPositiveMod, "FEPositiveMod", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x535060, (const void *)&FEPackageReader_ProcessCodeListBoxTag, "FEPackageReader::ProcessCodeListBoxTag", {0x8B, 0x44, 0x24, 0x04, 0x0F, 0xB7}},
    {0x5349C0, (const void *)&FEListBox_Ctor, "FEListBox::FEListBox", {0x56, 0x8B, 0xF1, 0xE8, 0x58, 0x65}},
    {0x534A50, (const void *)&FEListBox_Dtor, "FEListBox::~FEListBox", {0x6A, 0xFF, 0x68, 0xD8, 0x74, 0x77}},
    {0x541D90, (const void *)&FEListBox_ScalarDtor, "FEListBox scalar dtor", {0x56, 0x8B, 0xF1, 0xE8, 0xB8, 0x2C}},
    {0x516DC0, (const void *)&FEListBox_Terminate, "FEListBox::Terminate", {0x56, 0x8B, 0xF1, 0x8B, 0x46, 0x5C}},
    {0x52A910, (const void *)&FEListBox_SetNumColumns, "FEListBox::SetNumColumns", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x52AB30, (const void *)&FEListBox_SetNumRows, "FEListBox::SetNumRows", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x5042E0, (const void *)&FEListBox_CleanupCells, "FEListBox::CleanupCells", {0x57, 0x8B, 0xF9, 0x8B, 0x47, 0x64}},
    {0x504290, (const void *)&FEListBox::InitializeCells, "FEListBox::InitializeCells", {0x56, 0x8B, 0x74, 0x24, 0x0C, 0x33}},
    {0x504180, (const void *)&FEListBox_SetSelColumn, "FEListBox::SetSelColumn", {0x8B, 0x41, 0x60, 0x85, 0xC0, 0x76}},
    {0x5041B0, (const void *)&FEListBox_SetSelRow, "FEListBox::SetSelRow", {0x8B, 0x41, 0x64, 0x85, 0xC0, 0x76}},
    {0x5041E0, (const void *)&FEListBox_SetCellType, "FEListBox::SetCellType", {0x8B, 0x81, 0x8C, 0x00, 0x00, 0x00}},
    {0x504230, (const void *)&FEListBox_IncrementCellByColumn, "FEListBox::IncrementCellByColumn", {0x8B, 0x81, 0x88, 0x00, 0x00, 0x00}},
    {0x504270, (const void *)&FEListBox_SetAutoWrap, "FEListBox::SetAutoWrap", {0x8A, 0x44, 0x24, 0x04, 0x84, 0xC0}},
    {0x504360, (const void *)&FEListBox_RecalculateCummulative, "FEListBox::RecalculateCummulative", {0x8B, 0x41, 0x78, 0xD9, 0x05, 0x3C}},
    {0x516CD0, (const void *)&FEListBox_SetCellColor, "FEListBox::SetCellColor", {0x56, 0x8B, 0xF1, 0x8B, 0x4C, 0x24}},
    {0x516D00, (const void *)&FEListBox_SetCellScale, "FEListBox::SetCellScale", {0x8B, 0x81, 0x8C, 0x00, 0x00, 0x00}},
    {0x516D40, (const void *)&FEListBox_SetCellJustification, "FEListBox::SetCellJustification", {0x8B, 0x81, 0x8C, 0x00, 0x00, 0x00}},
    {0x516D80, (const void *)&FEListBox_SetCellData, "FEListBox::SetCellData", {0x8B, 0x81, 0x8C, 0x00, 0x00, 0x00}},
    {0x516E10, (const void *)&FEListBox_SetCellString, "FEListBox::SetCellString", {0x8B, 0x81, 0x8C, 0x00, 0x00, 0x00}},
    {0x52B740, (const void *)&FEPackageReader_ProcessListBoxTag, "FEPackageReader::ProcessListBoxTag", {0x83, 0xEC, 0x10, 0x53, 0x56, 0x8B}},
    {0x5515E0, (const void *)&FEPackageReader_ReadScriptTags, "FEPackageReader::ReadScriptTags", {0x6A, 0xFF, 0x68, 0xAB, 0x7B, 0x77}},
    {0x534FC0, (const void *)&FEPackageReader_ProcessStringTag, "FEPackageReader::ProcessStringTag", {0x8B, 0x54, 0x24, 0x04, 0x0F, 0xB7}},
    {0x4FE940, (const void *)&FEString_SetLabelHash, "FEString::SetLabelHash", {0x8B, 0xC1, 0x8B, 0x4C, 0x24, 0x04}},
    {0x534DF0, (const void *)&FEPackageReader_CreateObject, "FEPackageReader::CreateObject", {0x53, 0x8B, 0x5C, 0x24, 0x08, 0x83}},
    {0x555140, (const void *)&FEPackageReader_ReadObjectTags, "FEPackageReader::ReadObjectTags", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x5554A0, (const void *)&FEPackageReader_ReadObjectChunk, "FEPackageReader::ReadObjectChunk", {0x83, 0xEC, 0x0C, 0x55, 0x56, 0x57}},
    {0x52B550, (const void *)&FEPackageReader_ReadReferencedPackagesChunk, "FEPackageReader::ReadReferencedPackagesChunk", {0x83, 0xEC, 0x10, 0x53, 0x8B, 0xD9}},
    {0x5184E0, (const void *)&FEPackageReader_ReadMessageTargetListChunk, "FEPackageReader::ReadMessageTargetListChunk", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x5186C0, (const void *)&FEPackageReader_FindReferencedObject, "FEPackageReader::FindReferencedObject", {0x8B, 0x44, 0x24, 0x04, 0x53, 0x8B}},
    {0x50B240, (const void *)&FEngine_FindLibraryPackageHook, "FEngine::FindLibraryPackage", {0x8B, 0x81, 0x04, 0x01, 0x00, 0x00}},
    {0x517570, (const void *)&FEMessageTargets_SetCapacity, "FEMessageTargets::SetCapacity", {0x53, 0x55, 0x8B, 0x6C, 0x24, 0x0C}},
    {0x55DD50, (const void *)&cFEng_NotificationMessage, "cFEng::NotificationMessage", {0x8B, 0x44, 0x24, 0x04, 0x3D, 0x15}},
    {0x51CB20, (const void *)&cFEng_QueuePushPackage, "cFEng::QueuePushPackage", {0x8B, 0x44, 0x24, 0x08, 0x85, 0xC0}},
    {0x51CB60, (const void *)&cFEng_QueuePushModalPackage, "cFEng::QueuePushModalPackage", {0x8B, 0x44, 0x24, 0x08, 0x85, 0xC0}},
    {0x51CBA0, (const void *)&cFEng_SwitchFEngPackages, "cFEng::SwitchFEngPackages", {0x53, 0x56, 0x8B, 0x74, 0x24, 0x0C}},
    {0x51CC00, (const void *)&cFEng_QueuePopPackage, "cFEng::QueuePopPackage", {0x53, 0x56, 0x8B, 0x74, 0x24, 0x0C}},
    {0x51CC60, (const void *)&cFEng_SetButtonState, "cFEng::SetButtonState", {0x8B, 0x44, 0x24, 0x04, 0x50, 0xB9}},
    {0x505310, (const void *)&FEStricmp, "FEStricmp", {0x8B, 0x54, 0x24, 0x08, 0x56, 0x8B}},
    {0x519E50, (const void *)&FEList_FindNode, "FEList::FindNode", {0x53, 0x55, 0x56, 0x57, 0x8B, 0x7C}},
    {0x52CEF0, (const void *)&FEngFindPackage, "FEngFindPackage", {0xA1, 0xD0, 0x84, 0x83, 0x00, 0x85}},
    {0x5376B0, (const void *)&cFEng_ResyncSortingLevels, "cFEng::ResyncSortingLevels", {0x53, 0x56, 0x8B, 0x71, 0x10, 0x8D}},
    {0x537580, (const void *)&cFEng_PopPackage, "cFEng::PopPackage", {0x64, 0xA1, 0x00, 0x00, 0x00, 0x00}},
    {0x537750, (const void *)&cFEng_ForceUnloadPackage, "cFEng::ForceUnloadPackage", {0x53, 0x56, 0x57, 0x8B, 0x7C, 0x24}},
    {0x5376F0, (const void *)&cFEng_Popper, "cFEng::Popper", {0x53, 0x8B, 0x5C, 0x24, 0x08, 0x56}},
    {0x5377A0, (const void *)&cFEng_Switcher, "cFEng::Switcher", {0x53, 0x55, 0x8B, 0x6C, 0x24, 0x10}},
    {0x555D00, (const void *)&cFEng_Pusher, "cFEng::Pusher", {0xA1, 0xB0, 0x84, 0x83, 0x00, 0x53}},
    {0x555E80, (const void *)&FEngStartInteraction, "FEngStartInteraction", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x555EB0, (const void *)&FEngShowPackageModal, "FEngShowPackageModal", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x555EE0, (const void *)&FEngShowPackageTrueModal, "FEngShowPackageTrueModal", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x555F10, (const void *)&FEngShowPackage, "FEngShowPackage", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x555F50, (const void *)&FEngPushPackageSpecial, "FEngPushPackageSpecial", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x537950, (const void *)&FEngSetControlMask, "FEngSetControlMask", {0x8B, 0x44, 0x24, 0x04, 0x50, 0xE8}},
    {0x537980, (const void *)&FEngSwitchPackages, "FEngSwitchPackages", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x5379A0, (const void *)&FEngPopPackage, "FEngPopPackage", {0x8B, 0x0D, 0xC4, 0x84, 0x83, 0x00}},
    {0x541FC0, (const void *)&FEngine_ProcessMessageQueue, "FEngine::ProcessMessageQueue", {0x83, 0xEC, 0x0C, 0x53, 0x8B, 0xD9}},
    {0x5367E0, (const void *)&FEngine_ProcessObjectMessage, "FEngine::ProcessObjectMessage", {0x53, 0x8B, 0x5C, 0x24, 0x10, 0x55}},
    {0x51B000, (const void *)&FEngine_ProcessListBoxResponses, "FEngine::ProcessListBoxResponses", {0x8B, 0x44, 0x24, 0x0C, 0x3D, 0xF9}},
    {0x52C750, (const void *)&FEngine_ProcessCodeListBoxResponses, "FEngine::ProcessCodeListBoxResponses", {0x8B, 0x44, 0x24, 0x0C, 0x3D, 0xF9}},
    {0x52C7C0, (const void *)&FEngine_ProcessResponses, "FEngine::ProcessResponses", {0x83, 0xEC, 0x08, 0x55, 0x8B, 0x6C}},
    {0x504490, (const void *)&FEMessageResponse_FindConditionBranchTarget, "FEMessageResponse::FindConditionBranchTarget", {0x8B, 0x54, 0x24, 0x04, 0x57, 0x8B}},
    {0x5178B0, (const void *)&FEObject_SetCurrentScript, "FEObject::SetCurrentScript", {0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0}},
    {0x52AF20, (const void *)&FEObject_Ctor, "FEObject::FEObject()", {0x8B, 0xC1, 0xC7, 0x00, 0xE0, 0xB8}},
    {0x52AF90, (const void *)&FEObject_Dtor, "FEObject::~FEObject", {0x6A, 0xFF, 0x68, 0x4E, 0x74, 0x77}},
    {0x534BB0, (const void *)&FEObject_ScalarDtor, "FEObject scalar deleting destructor", {0x56, 0x8B, 0xF1, 0xE8, 0xD8, 0x63}},
};
const size_t kReplacementCount = sizeof(kReplacements) / sizeof(kReplacements[0]);

}  // namespace hook
