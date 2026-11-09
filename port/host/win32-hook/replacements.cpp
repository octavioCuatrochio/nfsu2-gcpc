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
#include "frontend/FEngine.h"
#include "frontend/FENode.h"
#include "frontend/cFEng.h"
#include "frontend/FEListBox.h"
#include "bWare/bMath.h"
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
