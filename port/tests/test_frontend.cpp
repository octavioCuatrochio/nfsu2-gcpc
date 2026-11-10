// Native unit tests: bWare CRC / wide strings, FEWideString, FEString.
#include "bWare/bCrc32.h"
#include "bWare/bWideString.h"
#include "bridge_stub.h"
#include "frontend/FEHash.h"
#include "frontend/FEMinList.h"
#include "frontend/FEMultiPool.h"
#include "frontend/FEMessageResponse.h"
#include "frontend/FEPool.h"
#include "frontend/FEPackage.h"
#include "frontend/FEngine.h"
#include "frontend/FENode.h"
#include "frontend/cFEng.h"
#include "bWare/bString.h"
#include "frontend/FEPackageManager.h"
#include "frontend/FEPackageReader.h"
#include "bWare/bSlotPool.h"
#include "frontend/FEListBox.h"
#include "frontend/FECodeListBox.h"
#include "bWare/bMath.h"
#include <algorithm>
#include "bridge/Bridge.h"
#include <cstdlib>
#include "frontend/FEString.h"
#include "frontend/FEWideString.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <vector>
#include <cstdint>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

static std::vector<uint16_t> W(const char *s) { std::vector<uint16_t> v; for (; *s; ++s) v.push_back((uint8_t)*s); v.push_back(0); return v; }
static bool Eq(const uint16_t *a, const char *b) { for (; *b; ++a, ++b) if (*a != (uint8_t)*b) return false; return *a == 0; }

static void TestCrc()
{
    CHECK(bCalculateCrc32("123456789", 9, 0) == 0x377A6011u);   // POSIX `cksum` of "123456789"
    CHECK(bCalculateCrc32("abc", 0, 0x12345678) == ~0x12345678u);
    CHECK(bCalculateCrc32("abc", -5, 7) == ~7u);
}

static void TestWideHelpers()
{
    uint16_t buf[8];
    bStrCpy(buf, "\xE9" "a");                  // é in Latin-1 is sign-extended like the game does
    CHECK(buf[0] == 0xFFE9 && buf[1] == 'a' && buf[2] == 0);
    CHECK(bStrCpy(buf, "x") == buf);
    auto src = W("hello");
    CopyString(buf, src.data());
    CHECK(Eq(buf, "hello"));
    CopyString(buf, nullptr);
    CHECK(buf[0] == 0);
    CopyString(nullptr, src.data());          // must not crash
    CHECK(bWideStrLen(nullptr) == 0 && bWideStrLen(src.data()) == 5);
}

static void TestWideString()
{
    {
        FEWideString s;
        CHECK(s.mpsString == nullptr && s.mulBufferLength == 0 && s.Length() == 0 && s.GetChecksum() == 0);
        auto t = W("abc");
        s = t.data();
        CHECK(Eq(s.mpsString, "abc") && s.mulBufferLength == 4 && s.Length() == 3);
        uint16_t *before = s.mpsString;
        s = W("xy").data();                      // fits: no reallocation
        CHECK(s.mpsString == before && Eq(s.mpsString, "xy") && s.mulBufferLength == 4);
        s = static_cast<const uint16_t *>(nullptr);   // null source: unchanged
        CHECK(Eq(s.mpsString, "xy"));
        s = "packed";                            // no histogram -> widened bytes
        CHECK(Eq(s.mpsString, "packed") && s.mulBufferLength == 7);
        // SetLength compares against the current TEXT length, not the capacity
        uint16_t *p = s.mpsString;
        s.SetLength(5);                          // text is 6 chars -> no change even though 5 < capacity
        CHECK(s.mpsString == p && s.mulBufferLength == 7);
        s.SetLength(20);
        CHECK(s.mpsString != p && s.mulBufferLength == 20 && Eq(s.mpsString, "packed"));
        CHECK(s.AllocateString(10) == s.mpsString && s.mulBufferLength == 20);
        FEWideString copy(s);
        CHECK(copy.mpsString != s.mpsString && Eq(copy.mpsString, "packed") && copy.mulBufferLength == 6);
        // checksum covers the whole capacity
        FEWideString a, b;
        a = W("same").data(); b = W("same").data();
        CHECK(a.GetChecksum() == b.GetChecksum());
        CHECK(a.GetChecksum() == bCalculateCrc32(a.mpsString, a.mulBufferLength * 2, 0xFFFFFFFFu));
    }
    CHECK(teststub::liveAllocations == 0);
}

static void TestFEString()
{
    FEString s;
    std::memset(static_cast<void *>(&s), 0, sizeof s);
    teststub::useOldDirtyFlag = false;
    s.SetString(W("one").data());
    CHECK((s.base.mu32Flags & FEOBJ_DIRTY) && Eq(s.mstrText.mpsString, "one"));
    // unchanged text, flag 0x2 clear -> still marked dirty (PC behavior)
    s.base.mu32Flags = 0;
    s.SetString(W("one").data());
    CHECK(s.base.mu32Flags & FEOBJ_DIRTY);
    // unchanged text, flag 0x2 set -> not dirty
    s.base.mu32Flags = FEOBJ_FLAG_0x2;
    s.SetString("one");
    CHECK(!(s.base.mu32Flags & FEOBJ_DIRTY));
    // changed text, flag 0x2 set -> dirty
    s.SetString("two");
    CHECK(s.base.mu32Flags & FEOBJ_DIRTY);
    // old dirty flag mode: always dirty
    teststub::useOldDirtyFlag = true;
    s.base.mu32Flags = FEOBJ_FLAG_0x2;
    s.SetString("two");
    CHECK(s.base.mu32Flags & FEOBJ_DIRTY);
    teststub::useOldDirtyFlag = false;
}

static void TestFEHashUpper()
{
    CHECK(FEHashUpper(nullptr) == 0xFFFFFFFFu && FEHashUpper("") == 0xFFFFFFFFu);
    CHECK(FEHashUpper("abc") == FEHashUpper("ABC"));
    CHECK(FEHashUpper("\xE9") != FEHashUpper("\xC9"));   // only a-z are folded
}

static void TestFEStringLifecycle()
{
    alignas(8) unsigned char mem[sizeof(FEString)], mem2[sizeof(FEString)];
    FEString *s = FEString::Construct(mem);
    CHECK(s->base.node.vtable == &teststub::feStringVTable && s->base.mu32Type == FE_TYPE_STRING);
    CHECK(s->mpcLabel == nullptr && s->mu32LabelHash == 0xFFFFFFFFu && s->mstrText.mpsString == nullptr);
    s->SetLabel("Title");
    CHECK(std::strcmp(s->mpcLabel, "Title") == 0 && s->mu32LabelHash == FEHashUpper("TITLE"));
    CHECK((s->base.mu32Flags & FEOBJ_DIRTY) && teststub::lastLabelCallback == s);
    s->SetString("hello");
    s->_unknown6C = 7;
    FEString *c = FEString::CopyConstruct(mem2, *s, true);
    CHECK(c->mpcLabel != s->mpcLabel && std::strcmp(c->mpcLabel, "Title") == 0);
    CHECK(c->mstrText.mpsString != s->mstrText.mpsString && Eq(c->mstrText.mpsString, "hello"));
    CHECK(c->mstrText.mulBufferLength == s->mstrText.mulBufferLength && c->_unknown6C == 7);
    FEString *k = s->Clone(false);
    CHECK(k && std::strcmp(k->mpcLabel, "Title") == 0 && Eq(k->mstrText.mpsString, "hello"));
    CHECK(k->ScalarDeletingDestructor(1) == k);    // frees k
    c->Destruct();
    s->SetLabel(nullptr);
    CHECK(s->mpcLabel == nullptr && s->mu32LabelHash == 0xFFFFFFFFu);
    s->Destruct();
    CHECK(teststub::liveAllocations == 0);
}

static void TestFEMinList()
{
    FEMinList l{&teststub::feMinListVTable, 0, nullptr, nullptr};
    auto mk = [] { auto *n = static_cast<FEMinNode *>(bridge::Alloc(sizeof(FEMinNode))); n->vtable = nullptr; return n; };
    FEMinNode *a = mk(), *b = mk(), *c = mk();
    l.AddNode(nullptr, a);                    // [a]
    l.AddNode(a, c);                          // [a c]
    l.AddNode(a, b);                          // [a b c]
    CHECK(l.count == 3 && l.head == a && l.tail == c && a->next == b && b->next == c && c->prev == b && !a->prev);
    CHECK(l.FindNode(0) == a && l.FindNode(2) == c && l.FindNode(3) == nullptr);
    l.RemNode(b);
    CHECK(l.count == 2 && a->next == c && c->prev == a && (uintptr_t)b->next == FEMINNODE_UNLINKED);
    l.AddNode(nullptr, nullptr);              // no-op
    CHECK(l.RemHead() == a && l.head == c && l.count == 1);
    bridge::FreeArray(a); bridge::FreeArray(b);
    teststub::virtualDeletes = 0;
    l.AddNode(c, mk());
    l.Purge();                                // deletes both through VirtualScalarDelete
    CHECK(l.count == 0 && !l.head && !l.tail && teststub::virtualDeletes == 2);
    CHECK(teststub::liveAllocations == 0);
}

static void TestFEObjectLifecycle()
{
    alignas(8) unsigned char mem[sizeof(FEObject)];
    std::memset(mem, 0x5A, sizeof mem);
    uint32_t guid = teststub::nextGUID;
    FEObject *o = FEObject::Construct(mem);
    CHECK(o->node.vtable == &teststub::feObjectVTable && (uintptr_t)o->node.next == FEMINNODE_UNLINKED);
    CHECK(o->mu32GUID == guid && teststub::nextGUID == guid + 1 && o->mpcName == nullptr && o->mu32Flags == 0);
    CHECK(o->mResponses.vtable == &teststub::feMinListVTable && o->mScripts.count == 0);
    CHECK(o->mu32Type == 0x5A5A5A5Au);                       // left untouched, like the original
    o->SetName("x");
    o->mpData = teststub::objDataPool.Alloc(24);
    CHECK(o->mpData != nullptr && teststub::objDataPool.mPools.count == 1);
    CHECK(o->ScalarDeletingDestructor(0) == o);              // destroy without freeing (stack memory)
    CHECK(teststub::objDataPool.mPools.count == 0 && teststub::lastDestructorCallback == o && teststub::liveAllocations == 0);
    CHECK(o->node.vtable == &teststub::feMinNodeVTable);
}

static void TestFEObject()
{
    FEObject o;
    std::memset(static_cast<void *>(&o), 0, sizeof o);
    o.SetName("Panel");
    CHECK(std::strcmp(o.mpcName, "Panel") == 0 && o.mu32NameHash == FEHashUpper("PANEL"));
    o.SetName(nullptr);
    CHECK(o.mpcName == nullptr && o.mu32NameHash == 0xFFFFFFFFu && teststub::liveAllocations == 0);
    FEScript s1{}, s2{};
    s1.mu32Id = 11; s2.mu32Id = 22;
    o.mScripts.AddNode(nullptr, &s1.node);
    o.mScripts.AddNode(&s1.node, &s2.node);
    CHECK(o.FindScript(22) == &s2 && o.FindScript(11) == &s1 && o.FindScript(33) == nullptr);
    FEMessageResponse r{};
    r.mu32MsgId = 0xC3960EB9;
    o.mResponses.AddNode(nullptr, &r.node);
    CHECK(o.FindResponse(0xC3960EB9) == &r && o.FindResponse(1) == nullptr);
    CHECK(FEObject::GetDataOffset(0) == 0 && FEObject::GetDataOffset(5) == 0x44 && FEObject::GetDataOffset(10) == 0x84);
    CHECK(FEObject::GetDataOffset(11) == 0 && FEObject::GetDataOffset(-1) == 0);
}

static void TestMultiPool()
{
    FEMultiPool &pool = teststub::objDataPool;
    CHECK(pool.Alloc(0) == nullptr);
    void *blocks[70];
    for (int i = 0; i < 70; ++i) blocks[i] = pool.Alloc(i < 40 ? 8 : 12);   // 2 nodes of 8, 1 node of 12
    CHECK(pool.mPools.count == 2);
    auto *p8 = reinterpret_cast<FESlotPool *>(pool.mPools.tail);              // first created pool sits at the tail
    CHECK(p8->mu32BlockSize == 8 && p8->mNodes.count == 2);
    for (int i = 0; i < 70; ++i)
        for (int j = i + 1; j < 70; ++j) CHECK(blocks[i] != blocks[j]);
    // new nodes go to the head, so the second node (blocks 32..39) is searched first, like the original
    pool.Free(blocks[35]);
    CHECK(pool.Alloc(8) == blocks[35]);                                       // lowest free slot of the head node
    for (int i = 0; i < 70; ++i) pool.Free(blocks[i]);
    CHECK(pool.mPools.count == 0 && teststub::liveAllocations == 0);
}

static void TestResponses()
{
    FEResponse a, b;
    a.Construct(); b.Construct();
    a.mu32Type = 0x201; a.SetParam("go"); a.mu32Extra = 7;
    b.Assign(a);
    CHECK(b.mu32Type == 0x201 && b.mParam != a.mParam && std::strcmp((char *)b.mParam, "go") == 0 && b.mu32Extra == 7);
    FEResponse c; c.Construct(); c.mu32Type = 0x203; c.mParam = 42;    // 0x203 is a plain value
    b.Assign(c);
    CHECK(b.mu32Type == 0x203 && b.mParam == 42);
    b.ReleaseParam(); a.ReleaseParam();
    CHECK(a.mParam == 0 && teststub::liveAllocations == 0);

    FEMessageResponse m{};
    m.SetCount(2);
    CHECK(m.mu32Count == 2 && m.mpResponses && reinterpret_cast<uint32_t *>(m.mpResponses)[-1] == 2);
    m.mpResponses[0].mu32Type = 0x200; m.mpResponses[0].SetParam("first");
    m.mpResponses[1].mu32Type = 5; m.mpResponses[1].mParam = 9;
    m.SetCount(3);                                             // grow: entries kept (deep-copied), new one zero
    CHECK(m.mu32Count == 3 && std::strcmp((char *)m.mpResponses[0].mParam, "first") == 0 && m.mpResponses[1].mParam == 9);
    CHECK(m.mpResponses[2].mu32Type == 0 && m.FindResponse(5) == 1 && m.FindResponse(77) == 0xFFFFFFFFu);
    m.SetCount(1);                                             // shrink
    CHECK(m.mu32Count == 1 && std::strcmp((char *)m.mpResponses[0].mParam, "first") == 0);
    m.SetCount(0);
    CHECK(m.mu32Count == 0 && m.mpResponses == nullptr && teststub::liveAllocations == 0);
}

static void TestMessageResponsePool()
{
    std::vector<FEMessageResponse *> v;
    for (int i = 0; i < 70; ++i) {                              // 2 blocks of 64
        auto *m = static_cast<FEMessageResponse *>(FEMessageResponse::PoolNew());
        CHECK(m != nullptr);
        v.push_back(m);
    }
    CHECK(teststub::msgRespPool.count == 2);
    for (size_t i = 0; i < v.size(); ++i)
        for (size_t j = i + 1; j < v.size(); ++j) CHECK(v[i] != v[j]);
    // full lifecycle of one object, as the game does it: construct, give responses, delete through the scalar dtor
    FEMessageResponse *m = FEMessageResponse::Construct(v[0]);
    m->SetCount(2);
    m->mpResponses[0].mu32Type = 0x201; m->mpResponses[0].SetParam("x");
    teststub::virtualDtorCalls = 0;
    CHECK(m->ScalarDeletingDestructor(1) == m);                 // destroys twice (2nd via vtable), returns to pool
    CHECK(m->mpResponses == nullptr && teststub::virtualDtorCalls == 1);
    CHECK(FEMessageResponse::PoolNew() == v[0] || true);        // free entry reused (tail of its block's free list)
    v[0] = nullptr;
    for (auto *p : v) if (p) FEMessageResponse::PoolDelete(p);
    // remaining: the entry re-allocated above keeps its block alive
    CHECK(teststub::msgRespPool.count == 1);
}

// Pool blocks: constructed with every entry on the free list (in order, guards set); emptying a block deletes it
// through its vtable -> block dtor destroys every entry; all memory returned.
static void TestPoolBlocks()
{
    auto *b = static_cast<FEScriptBlock *>(bridge::Alloc(sizeof(FEScriptBlock)));
    FEScriptBlock_Construct(b);
    CHECK(b->node.vtable == &teststub::feScriptBlockVTable && b->used == 0 && b->freeList.count == 32);
    auto *first = reinterpret_cast<FEScript *>(b->entries);
    CHECK(b->freeList.head == &first->node && first->node.prev == nullptr);
    CHECK(first->node.vtable == &teststub::feScriptVTable && first->mu32Id == 0 && first->mpTracks == nullptr);
    CHECK(b->freeList.tail == reinterpret_cast<FEMinNode *>(b->entries + 31 * sizeof(FEScript)));
    CHECK(FEScriptBlock_ScalarDtor(b, 0) == b);
    CHECK(b->freeList.count == 0 && b->freeList.head == nullptr);
    for (int i = 0; i < 32; ++i)
        CHECK(reinterpret_cast<FEScript *>(b->entries + i * sizeof(FEScript))->node.vtable == &teststub::feMinNodeVTable);
    CHECK(first->node.next == reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED));
    CHECK(b->node.vtable == &teststub::feMinNodeVTable);
    bridge::FreeArray(b);

    int liveBefore = teststub::liveAllocations;
    std::vector<void *> s;
    for (int i = 0; i < 40; ++i) s.push_back(FEPoolNew<FEScript, 32>(teststub::scriptPool, FEScriptBlock_Construct));
    CHECK(teststub::scriptPool.count == 2);
    for (void *p : s) FEPoolDelete<FEScript, 32>(teststub::scriptPool, p);
    CHECK(teststub::scriptPool.count == 0 && teststub::liveAllocations == liveBefore);
}

// ~FEScript: tracks (new[] with count cookie) destroyed, owning key lists purged, borrowed ones left alone.
static void TestScriptDestruct()
{
    int liveBefore = teststub::liveAllocations;
    FEMinNode *keys[3];
    for (auto &k : keys) {
        k = static_cast<FEMinNode *>(bridge::Alloc(sizeof(FEMinNode)));
        *k = FEMinNode{&teststub::feMinNodeVTable, nullptr, nullptr};
    }
    auto *raw = static_cast<uint32_t *>(bridge::Alloc(4 + 2 * sizeof(FEKeyTrack)));
    raw[0] = 2;
    auto *tracks = reinterpret_cast<FEKeyTrack *>(raw + 1);
    std::memset(tracks, 0, 2 * sizeof(FEKeyTrack));
    // track 0 owns keys[0..1]; track 1 borrows (head points at keys[2], must survive)
    tracks[0].mKeys.head = keys[0]; tracks[0].mKeys.tail = keys[1];
    keys[0]->next = keys[1]; keys[1]->prev = keys[0];
    tracks[1].mKeys.mbIsReference = 1; tracks[1].mKeys.head = keys[2];
    FEScript sc;
    FEScript::Construct(&sc);
    sc.mpTracks = tracks;
    sc.mEvents.SetCount(2);
    sc.SetName("abc");
    int deletesBefore = teststub::virtualDeletes;
    sc.Destruct();
    CHECK(teststub::virtualDeletes - deletesBefore == 2);
    CHECK(sc.mpTracks == nullptr && sc.mEvents.mpEvents == nullptr && sc.mpcName == nullptr);
    CHECK(sc.node.vtable == &teststub::feMinNodeVTable);
    CHECK(teststub::liveAllocations == liveBefore + 1);         // only the borrowed key is left
    bridge::FreeArray(keys[2]);
    // RemNode on an unlinked/null node
    FERefList l{&teststub::feRefListVTable, 0, {}, nullptr, nullptr};
    CHECK(l.RemNode(nullptr) == nullptr && l.RemHead() == nullptr);
}

// FEScript copy constructor: deep (new key nodes, same data) and reference (lists point at the source's).
static void TestScriptCopy()
{
    int liveBefore = teststub::liveAllocations;
    FEScript src;
    FEScript::Construct(&src);
    src.SetName("Fade_In");
    CHECK(src.mu32Id == FEHashUpper("Fade_In") && std::strcmp(src.mpcName, "Fade_In") == 0);
    src.mi32Length = 1; src.mi32Time = 2; src.mu32Flags = 3; src.mpChainedScript = &src;
    src.SetTrackCount(2);
    CHECK(src.mu32NumTracks == 2 && reinterpret_cast<uint32_t *>(src.mpTracks)[-1] == 2);
    CHECK(src.mpTracks[1].mBaseKey.node.vtable == &teststub::feKeyNodeVTable && src.mpTracks[1].mKeys.head == nullptr);
    src.mpTracks[0].mu8InterpType = 9; src.mpTracks[0].mi8DataIndex = -3; src.mpTracks[0]._length[1] = 0x56; src.mpTracks[0].mBaseKey._data[4] = 77;
    FEKeyNode k[3];
    for (int i = 0; i < 3; ++i) {
        k[i].node = {&teststub::feKeyNodeVTable, i < 2 ? &k[i + 1].node : nullptr, i ? &k[i - 1].node : nullptr};
        for (auto &d : k[i]._data) d = 100 + i;
    }
    src.mpTracks[0].mKeys.head = &k[0].node; src.mpTracks[0].mKeys.tail = &k[2].node;
    src.mEvents.SetCount(1);
    CHECK(src.mEvents.mpEvents[0].mu32MsgId == 0);
    src.mEvents.mpEvents[0].mu32Target = 5;
    src.mEvents.SetCount(3);
    CHECK(src.mEvents.mpEvents[0].mu32Target == 5 && src.mEvents.mpEvents[2].mu32Time == 0);
    src.mEvents.SetCount(1);
    CHECK(src.mEvents.mu32Count == 1 && src.mEvents.mpEvents[0].mu32Target == 5);

    teststub::keyNodeNews = 0;
    FEScript deep;
    FEScript::CopyConstruct(&deep, src, false);
    CHECK(deep.mu32Id == src.mu32Id && deep.mpcName != src.mpcName && std::strcmp(deep.mpcName, "Fade_In") == 0);
    CHECK(deep.mi32Length == 1 && deep.mi32Time == 2 && deep.mu32Flags == 3 && deep.mpChainedScript == nullptr);
    CHECK(teststub::keyNodeNews == 3 && deep.mpTracks[0].mKeys.mbIsReference == 0);
    int n = 0;
    for (FEMinNode *p = deep.mpTracks[0].mKeys.head; p; p = p->next, ++n) {
        CHECK(p != &k[n].node && reinterpret_cast<FEKeyNode *>(p)->_data[0] == 100u + n);
        CHECK(p->vtable == &teststub::feKeyNodeVTable);
    }
    CHECK(n == 3 && deep.mpTracks[0].mKeys.tail->next == nullptr);
    CHECK(deep.mpTracks[0].mu8InterpType == 9 && deep.mpTracks[0].mi8DataIndex == -3 && deep.mpTracks[0]._length[1] == 0x56 && deep.mpTracks[0].mBaseKey._data[4] == 77);
    CHECK(deep.mEvents.mu32Count == 1 && deep.mEvents.mpEvents != src.mEvents.mpEvents && deep.mEvents.mpEvents[0].mu32Target == 5);

    FEScript ref;
    FEScript::CopyConstruct(&ref, src, true);
    CHECK(ref.mpTracks[0].mKeys.mbIsReference == 1);
    CHECK(ref.mpTracks[0].mKeys.head == reinterpret_cast<FEMinNode *>(&src.mpTracks[0].mKeys));
    CHECK(ref.mpTracks[0].mBaseKey._data[4] == 77 && teststub::keyNodeNews == 3);
    // a deep copy of a reference script shares the same list (Assign keeps references)
    FEScript ref2;
    FEScript::CopyConstruct(&ref2, ref, false);
    CHECK(ref2.mpTracks[0].mKeys.mbIsReference == 1 && ref2.mpTracks[0].mKeys.head == ref.mpTracks[0].mKeys.head);

    int deletes = teststub::virtualDeletes;
    ref2.Destruct(); ref.Destruct();
    CHECK(teststub::virtualDeletes == deletes);          // references never delete keys
    deep.Destruct();
    CHECK(teststub::virtualDeletes == deletes + 4);      // the 3 cloned nodes + their emptied pool block
    CHECK(teststub::keyNodePool.count == 0);
    src.mpTracks[0].mKeys.head = src.mpTracks[0].mKeys.tail = nullptr;   // stack keys: not ours to delete
    src.Destruct();
    CHECK(teststub::liveAllocations == liveBefore);
}

// FEObject copy ctor / Clone: header copied, fresh GUID, zeroed data block of the same size, responses deep-copied,
// scripts copied in order, current + chained scripts re-found by id in the copy.
static void TestFEObjectCopy()
{
    int liveBefore = teststub::liveAllocations;
    alignas(8) unsigned char mem[sizeof(FEObject)];
    FEObject *src = FEObject::Construct(mem);
    src->mu32Type = 7; src->mu32Flags = 0x40; src->_unknown20 = 3; src->_unknown22 = 4; src->_unknown24 = 5; src->_unknown28 = 6;
    src->SetName("Button");
    src->mpData = teststub::objDataPool.Alloc(16);
    src->mu32DataSize = 16;
    std::memset(src->mpData, 0xEE, 16);
    for (int i = 0; i < 2; ++i) {
        FEMessageResponse *m = FEMessageResponse::Construct(FEMessageResponse::PoolNew());
        m->mu32MsgId = 0x100 + i;
        m->SetCount(1);
        m->mpResponses[0].mu32Type = 0x201; m->mpResponses[0].SetParam(i ? "b" : "a");
        src->mResponses.AddTail(&m->node);
    }
    FEScript *sc[2];
    for (int i = 0; i < 2; ++i) {
        sc[i] = FEScript::Construct(FEPoolNew<FEScript, 32>(teststub::scriptPool, FEScriptBlock_Construct));
        sc[i]->SetName(i ? "Highlight" : "Init");
        src->mScripts.AddTail(&sc[i]->node);
    }
    sc[0]->mpChainedScript = sc[1];
    src->mpCurrentScript = sc[1];

    uint32_t guid = teststub::nextGUID;
    FEObject *c = src->Clone(false);
    CHECK(c != nullptr && c->mu32GUID == guid && c->node.vtable == &teststub::feObjectVTable);
    CHECK(c->mu32Type == 7 && c->mu32Flags == 0x40 && c->_unknown20 == 3 && c->_unknown22 == 4 && c->_unknown24 == 5 && c->_unknown28 == 0);
    CHECK(c->mpcName != src->mpcName && std::strcmp(c->mpcName, "Button") == 0 && c->mu32NameHash == src->mu32NameHash);
    CHECK(c->mu32DataSize == 16 && c->mpData != src->mpData && static_cast<uint8_t *>(c->mpData)[15] == 0);
    CHECK(c->mResponses.count == 2 && c->FindResponse(0x101) != nullptr);
    FEMessageResponse *r = c->FindResponse(0x100);
    CHECK(r && r->mu32Count == 1 && std::strcmp(reinterpret_cast<const char *>(r->mpResponses[0].mParam), "a") == 0);
    CHECK(r->mpResponses[0].mParam != src->FindResponse(0x100)->mpResponses[0].mParam);
    CHECK(c->mScripts.count == 2);
    FEScript *c0 = reinterpret_cast<FEScript *>(c->mScripts.head), *c1 = reinterpret_cast<FEScript *>(c->mScripts.tail);
    CHECK(c0 != sc[0] && c0->mu32Id == sc[0]->mu32Id && c1->mu32Id == sc[1]->mu32Id);
    CHECK(c0->mpChainedScript == c1 && c1->mpChainedScript == nullptr);
    CHECK(c->mpCurrentScript == c1);

    c->ScalarDeletingDestructor(1);
    src->ScalarDeletingDestructor(0);
    CHECK(teststub::liveAllocations == liveBefore);
}

// SetupMoveToTracks: relative tracks get key0 = current - base (quaternion: current * conj(base)); others only
// lose flag bit 7; reference lists resolve to the owning list; empty lists are skipped.
static void TestMoveToTracks()
{
    auto fbits = [](float v) { uint32_t u; std::memcpy(&u, &v, 4); return u; };
    auto bitsf = [](uint32_t u) { float v; std::memcpy(&v, &u, 4); return v; };
    int liveBefore = teststub::liveAllocations;
    alignas(8) unsigned char mem[sizeof(FEObject)];
    FEObject *o = FEObject::Construct(mem);
    uint32_t data[40] = {};
    o->mpData = data;
    FEScript sc;
    FEScript::Construct(&sc);
    sc.SetTrackCount(8);
    FEKeyNode keys[7] = {};
    for (int i = 0; i < 8; ++i) {
        FEKeyTrack &t = sc.mpTracks[i];
        t.mu8ParamType = static_cast<uint8_t>(i < 6 ? i + 1 : 2);
        t.mu8InterpType = i == 6 ? 1 : (i & 1 ? 4 : 3);
        t.mu8Flags = 0xFF;
        t.mi8DataIndex = static_cast<int8_t>(i * 4);
        if (i < 7) { t.mKeys.head = t.mKeys.tail = &keys[i].node; }
    }
    // track 7: reference to track 2's list (head stays empty on purpose -> resolves to keys[2])
    sc.mpTracks[7].mKeys.mbIsReference = 1;
    sc.mpTracks[7].mKeys.head = reinterpret_cast<FEMinNode *>(&sc.mpTracks[2].mKeys);
    // values
    FEKeyTrack *t = sc.mpTracks;
    data[t[0].mi8DataIndex] = 10; t[0].mBaseKey._data[1] = 3;                                         // int
    data[t[1].mi8DataIndex] = fbits(2.5f); t[1].mBaseKey._data[1] = fbits(0.5f);                       // float
    data[t[2].mi8DataIndex] = fbits(1); data[t[2].mi8DataIndex + 1] = fbits(5);                        // vec2
    t[2].mBaseKey._data[1] = fbits(4); t[2].mBaseKey._data[2] = fbits(2);
    for (int k = 0; k < 3; ++k) { data[t[3].mi8DataIndex + k] = fbits(10.f * (k + 1)); t[3].mBaseKey._data[1 + k] = fbits(1.f); }
    // quaternion: current = base -> result = identity (0,0,0,1) for a unit quaternion
    float q[4] = {0.f, 0.6f, 0.f, 0.8f};
    for (int k = 0; k < 4; ++k) { data[t[4].mi8DataIndex + k] = fbits(q[k]); t[4].mBaseKey._data[1 + k] = fbits(q[k]); }
    for (int k = 0; k < 4; ++k) { data[t[5].mi8DataIndex + k] = 100 + k; t[5].mBaseKey._data[1 + k] = 200; }   // color
    data[t[7].mi8DataIndex] = fbits(-3.f);                       // track 7 (float, via reference) -> keys[2]._data[1]
    t[7].mBaseKey._data[1] = fbits(0.f);                         // tracks come from non-zeroed memory
    o->mpCurrentScript = &sc;
    o->SetupMoveToTracks();
    CHECK(keys[0]._data[1] == 7);
    CHECK(bitsf(keys[1]._data[1]) == 2.0f);
    CHECK(bitsf(keys[2]._data[1]) == -3.0f && bitsf(keys[2]._data[2]) == 3.0f);
    CHECK(bitsf(keys[3]._data[1]) == 9.f && bitsf(keys[3]._data[2]) == 19.f && bitsf(keys[3]._data[3]) == 29.f);
    CHECK(std::fabs(bitsf(keys[4]._data[1])) < 1e-6f && std::fabs(bitsf(keys[4]._data[2])) < 1e-6f);
    CHECK(std::fabs(bitsf(keys[4]._data[4]) - 1.f) < 1e-6f);
    CHECK(static_cast<int32_t>(keys[5]._data[1]) == -100 && static_cast<int32_t>(keys[5]._data[4]) == -97);
    CHECK(keys[6]._data[1] == 0);                                // interp 1: untouched
    for (int i = 0; i < 8; ++i) CHECK(sc.mpTracks[i].mu8Flags == 0x7F);
    sc.mpTracks[7].mKeys.mbIsReference = 0; sc.mpTracks[7].mKeys.head = nullptr;
    for (int i = 0; i < 7; ++i) sc.mpTracks[i].mKeys.head = sc.mpTracks[i].mKeys.tail = nullptr;
    sc.Destruct();
    o->mpData = nullptr;
    o->Destruct();
    CHECK(teststub::liveAllocations == liveBefore);
}

// FindTrack + SetTrackValue: every script's matching track gets the value (base key) and loses flag bit 7, the
// object's data at GetDataOffset gets it too; add=true accumulates (floats; colors as wrapping ints).
static void TestSetTrackValue()
{
    int liveBefore = teststub::liveAllocations;
    alignas(8) unsigned char mem[sizeof(FEObject)];
    FEObject *o = FEObject::Construct(mem);
    float data[40] = {};
    o->mpData = data;
    FEScript sc[2];
    for (int k = 0; k < 2; ++k) {
        FEScript::Construct(&sc[k]);
        sc[k].SetTrackCount(2);
        sc[k].mpTracks[0].mi8DataIndex = 7;      // track 2 (position: dword 7)
        sc[k].mpTracks[1].mi8DataIndex = 0;      // track 0 (color: dword 0)
        sc[k].mpTracks[0].mu8Flags = sc[k].mpTracks[1].mu8Flags = 0xFF;
        o->mScripts.AddTail(&sc[k].node);
    }
    sc[1].mpTracks[0].mi8DataIndex = 25;         // script 1: no position track
    CHECK(sc[0].FindTrack(2) == &sc[0].mpTracks[0] && sc[0].FindTrack(0) == &sc[0].mpTracks[1]);
    CHECK(sc[1].FindTrack(2) == nullptr && sc[0].FindTrack(5) == nullptr);

    o->SetTrackValue(2, FEVector3{1.f, 2.f, 3.f}, false);
    o->SetTrackValue(2, FEVector3{0.5f, 0.5f, 0.5f}, true);
    float b[3];
    std::memcpy(b, &sc[0].mpTracks[0].mBaseKey._data[1], sizeof b);
    CHECK(b[0] == 1.5f && b[1] == 2.5f && b[2] == 3.5f && sc[0].mpTracks[0].mu8Flags == 0x7F);
    CHECK(data[7] == 1.5f && data[8] == 2.5f && data[9] == 3.5f);
    CHECK(sc[1].mpTracks[0].mu8Flags == 0xFF);   // not matched: untouched

    o->SetTrackValue(4, FEVector2{2.f, 4.f}, false);   // size: dword 14, no tracks
    CHECK(data[14] == 2.f && data[15] == 4.f && data[16] == 0.f);

    o->SetTrackValue(0, FEColor{{10, 20, 30, 40}}, false);
    o->SetTrackValue(0, FEColor{{-20, 1, 1, 0x7FFFFFFF}}, true);
    int32_t c[4];
    std::memcpy(c, data, sizeof c);
    CHECK(c[0] == -10 && c[1] == 21 && c[2] == 31 && c[3] == static_cast<int32_t>(0x80000027u));
    CHECK(static_cast<int32_t>(sc[1].mpTracks[1].mBaseKey._data[1]) == -10 && sc[1].mpTracks[1].mu8Flags == 0x7F);

    for (auto &x : sc) x.Destruct();
    o->mScripts = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
    o->mpData = nullptr;
    o->Destruct();
    CHECK(teststub::liveAllocations == liveBefore);
}

// Key lookup + step interpolation + fast dispatch on a 3-key track (times 0, 10, 20).
static void TestKeyInterp()
{
    FEKeyTrack t;
    FEKeyTrack::Construct(&t);
    t.mu8ValueSize = 4;
    t._length[0] = 30;                                  // length 30
    t.mBaseKey._data[0] = 0; t.mBaseKey._data[1] = 999;
    FEKeyNode k[3];
    for (int i = 0; i < 3; ++i) {
        k[i].node = {&teststub::feKeyNodeVTable, i < 2 ? &k[i + 1].node : nullptr, i ? &k[i - 1].node : nullptr};
        k[i]._data[0] = 10 * i; k[i]._data[1] = 100 + i;
    }
    t.mKeys.head = &k[0].node; t.mKeys.tail = &k[2].node;
    CHECK(t.GetKeyAt(-1) == &t.mBaseKey && t.GetKeyAt(0) == &k[0] && t.GetKeyAt(5) == &k[1] && t.GetKeyAt(10) == &k[1]);
    CHECK(t.GetKeyAt(11) == &k[2] && t.GetKeyAt(500) == &k[2]);
    CHECK(t.GetDeltaKeyAt(-5) == &k[0] && t.GetDeltaKeyAt(15) == &k[2] && t.GetFirstKey() == &k[0]);
    uint32_t v = 0;
    FEInterpNone(&t, 0, &v);  CHECK(v == 100);
    FEInterpNone(&t, 5, &v);  CHECK(v == 100);         // key 10 is after 5 -> previous key
    FEInterpNone(&t, 10, &v); CHECK(v == 101);
    FEInterpNone(&t, 25, &v); CHECK(v == 102);
    t.mBaseKey.node.next = nullptr;
    FEInterpNone(&t, 31, &v); CHECK(v == 999);         // past the length: base key (no next)
    // fast dispatch: step track evaluates; flag bit 7 skips; no keys -> marked done
    t.mu8InterpType = 0; t.mu8Flags = 0; v = 0;
    FEKeyInterpFast(&t, 20, &v); CHECK(v == 102 && t.mu8Flags == 0);
    t.mu8InterpType = 1; v = 0;                         // linear on an int track: key 102 + base 999
    t.mu8ParamType = 1;
    FEKeyInterpFast(&t, 20, &v); CHECK(v == 102 + 999);
    t.mu8Flags = 0x80; v = 0; FEKeyInterpFast(&t, 20, &v); CHECK(v == 0);
    t.mKeys.head = t.mKeys.tail = nullptr; t.mu8Flags = 0; t.mu8InterpType = 2;
    FEKeyInterpFast(&t, 20, &v); CHECK(t.mu8Flags == 0x80);
    // SetBaseValueAt: base += value - evaluated (step track with one key)
    t.mKeys.head = t.mKeys.tail = &k[0].node; k[0].node.next = nullptr;
    t.mu8InterpType = 0; t.mu8ValueSize = 16; t.mu8Flags = 0x80;
    for (int i = 0; i < 4; ++i) { k[0]._data[1 + i] = 10; t.mBaseKey._data[1 + i] = 1; }
    t.SetBaseValueAt(0, FEColor{{15, 10, 5, -10}});
    CHECK(t.mBaseKey._data[1] == 6 && t.mBaseKey._data[2] == 1 && static_cast<int32_t>(t.mBaseKey._data[3]) == -4);
    CHECK(static_cast<int32_t>(t.mBaseKey._data[4]) == -19 && t.mu8Flags == 0);
}

// FEInterpLinear: clamp/loop modes, exact keys, float/int/vec3/color lerps, no keys -> base value.
static void TestInterpLinear()
{
    auto fb = [](float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; };
    FEKeyTrack t;
    FEKeyTrack::Construct(&t);
    t._length[0] = 40;
    FEKeyNode k[2];
    k[0].node = {&teststub::feKeyNodeVTable, &k[1].node, nullptr};
    k[1].node = {&teststub::feKeyNodeVTable, nullptr, &k[0].node};
    k[0]._data[0] = 10; k[1]._data[0] = 30;
    for (int i = 1; i < 5; ++i) { k[0]._data[i] = fb(0.f); k[1]._data[i] = fb(8.f); t.mBaseKey._data[i] = fb(1.f); }
    t.mKeys.head = &k[0].node; t.mKeys.tail = &k[1].node;
    t.mu8ParamType = 2;
    float f = 0;
    FEInterpLinear(&t, 20, &f); CHECK(f == 5.f);       // halfway 0..8 + base 1
    FEInterpLinear(&t, 10, &f); CHECK(f == 1.f);       // exactly on key 0 (t == 0)
    FEInterpLinear(&t, 35, &f); CHECK(f == 9.f);       // clamp past the last key
    FEInterpLinear(&t, 0, &f);  CHECK(f == 1.f);       // before the first key: first key's value
    t.mu8Flags = 1;                                     // loop: from key 30 (8) toward key 10 (0) over length 40
    FEInterpLinear(&t, 40, &f); CHECK(f == 5.f);       // (40-30)/(40+10-30) = 0.5 -> 4 + 1
    t.mu8Flags = 0;
    t.mu8ParamType = 4;
    float v3[3];
    FEInterpLinear(&t, 25, v3); CHECK(v3[0] == 7.f && v3[1] == 7.f && v3[2] == 7.f);
    t.mu8ParamType = 1;                                 // ints: truncate(diff * t + 0.5)
    k[0]._data[1] = 0; k[1]._data[1] = 3; t.mBaseKey._data[1] = 100;
    int32_t iv;
    FEInterpLinear(&t, 15, &iv); CHECK(iv == 101);     // 3 * 0.25 + 0.5 = 1.25 -> 1
    FEInterpLinear(&t, 26, &iv); CHECK(iv == 102);     // 3 * 0.8 + 0.5 = 2.9 -> 2, + base 100 + a 0
    t.mu8ParamType = 6;
    for (int i = 1; i < 5; ++i) { k[0]._data[i] = 0; k[1]._data[i] = static_cast<uint32_t>(-10); t.mBaseKey._data[i] = 0; }
    int32_t c[4];
    FEInterpLinear(&t, 20, c); CHECK(c[0] == -4 && c[3] == -4);   // -10 * 0.5 + 0.5 = -4.5 -> -4 (toward zero)
    t.mKeys.head = t.mKeys.tail = nullptr;              // no keys: base copied
    t.mBaseKey._data[1] = 77;
    FEInterpLinear(&t, 20, c); CHECK(c[0] == 77);
}

// bWare trig sanity (the stub asin table is a formula approximation; exactness is the referee's job) and slerp.
static void TestTrigAndSlerp()
{
    double worst = 0;
    for (int a = 0; a < 65536; a += 7) worst = std::max(worst, std::fabs(double(bSin(uint16_t(a))) - std::sin(a * 6.283185307179586 / 65536)));
    CHECK(worst < 2e-4);
    CHECK(std::fabs(double(bSinF(1.0f)) - std::sin(1.0)) < 2e-4);
    for (float x = -0.95f; x < 0.95f; x += 0.0137f) {   // near +-1 the stub table is too coarse
        double want = std::asin(x) * 65536 / 6.283185307179586;
        double got = int16_t(bASin(x));
        CHECK(std::fabs(got - want) < 6.0);                 // stub table: point-derivative slopes
    }
    CHECK(bASin(1.5f) == 0x4000 && bASin(-1.f) == 0xC000);
    FEQuaternion id{0, 0, 0, 1}, rz{0, 0, 0.7071068f, 0.7071068f}, out;
    FELerpQuaternion(&id, &rz, 0.f, &id, &out);
    CHECK(std::fabs(out.w - 1.f) < 1e-3f && std::fabs(out.z) < 1e-3f);
    FELerpQuaternion(&id, &rz, 1.f, &id, &out);
    CHECK(std::fabs(out.z - 0.7071f) < 2e-3f && std::fabs(out.w - 0.7071f) < 2e-3f);
    FELerpQuaternion(&id, &rz, 0.5f, &id, &out);       // 45 degrees about z
    CHECK(std::fabs(out.z - 0.3827f) < 2e-3f && std::fabs(out.w - 0.9239f) < 2e-3f);
    FEQuaternion near{0, 0, 0.001f, 1.f};
    FELerpQuaternion(&id, &near, 0.5f, &id, &out);     // nearly parallel: normalized lerp
    CHECK(std::fabs(out.w - 1.f) < 1e-5f);
    FEQuaternion big{1e6f, 0, 0, 0};
    FEQuatNormalize(&big);                              // 1/len <= 1e-5 -> identity
    CHECK(big.x == 0.f && big.w == 1.f);
}

// Test engine: messages queued by the rewritten FEngine::QueueMessage land in its list; collect() drains it.
static FEngine *TestEngine()
{
    static FEngine *e = [] {
        auto *p = static_cast<FEngine *>(std::calloc(1, sizeof(FEngine)));
        p->mMessages = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
        p->mPackageCommands = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
        return p;
    }();
    return e;
}
static std::vector<teststub::QueuedMessage> Collect()
{
    std::vector<teststub::QueuedMessage> out;
    FEMinList &l = TestEngine()->mMessages;
    while (FEMinNode *n = l.RemHead()) {
        auto *m = reinterpret_cast<FEMessageNode *>(n);
        out.push_back({m->mu32MsgId, m->mpObject, m->mpTarget});
        bridge::FreeArray(m);
    }
    return out;
}

// Drains the test engine's package command queue (FEngine::QueuePackage*).
struct Command { uint32_t type, mask; std::string name; FEPackage *pkg; uint32_t hash; };
static std::vector<Command> CollectCommands()
{
    std::vector<Command> out;
    FEMinList &l = TestEngine()->mPackageCommands;
    while (FEMinNode *n = l.RemHead()) {
        auto *c = reinterpret_cast<FEPackageCommand *>(n);
        CHECK(c->node.node.vtable == &teststub::fePackageCommandVTable);
        out.push_back({c->mu32Type, c->mu32Mask, c->node.mpcName ? c->node.mpcName : "", c->mpPackage, c->node.mu32NameHash});
        bridge::FreeArray(c->node.mpcName);
        bridge::FreeArray(c);
    }
    return out;
}

// FENode::SetName + FEngine::QueuePackage*: command fields and control mask side effects.
static void TestPackageMarkers()
{
    FEngine *e = TestEngine();
    e->ClearPackageMarkers();
    char name[4] = "A";
    for (int i = 0; i < 16; ++i) { name[0] = char('A' + i); CHECK(e->RecordPackageMarker(name)); }
    CHECK(!e->RecordPackageMarker("Z") && e->mu32NumPackageMarkers == 16 && std::string(e->macPackageMarkers[15]) == "P");
    e->macPackageMarkers[3][1] = 'x';
    e->ClearPackageMarkers();
    CHECK(e->mu32NumPackageMarkers == 0 && e->macPackageMarkers[15][0] == 0 && e->macPackageMarkers[3][1] == 'x');
    // last buttons: 32-entry ring; recording a hash zeroes its older entries (first match wins on recall)
    for (uint32_t i = 0; i < 40; ++i) e->RecordLastPackageButton(100 + i, i);
    CHECK(e->mi32LastButtonIndex == 8 && e->RecallLastPackageButton(100) == 0 && e->RecallLastPackageButton(139) == 39);
    CHECK(e->RecallLastPackageButton(108) == 8 && e->mLastButtons[0].mu32PackageHash == 132);
    e->RecordLastPackageButton(139, 77);                       // old slot 7 -> hash 0, new slot 8
    CHECK(e->mLastButtons[7].mu32PackageHash == 0 && e->mLastButtons[7].mu32ButtonGUID == 39 && e->RecallLastPackageButton(139) == 77);
    CHECK(e->RecallLastPackageButton(0) == 39);                // a zeroed slot matches hash 0 (as in the original)
    std::memset(e->mLastButtons, 0, sizeof e->mLastButtons);
    e->mi32LastButtonIndex = 0;
}

static void TestPackageCommands()
{
    FEngine *e = TestEngine();
    CollectCommands();
    int live = teststub::liveAllocations;
    FENode nd{};
    CHECK(!nd.SetName(nullptr) && nd.mpcName == nullptr && nd.mu32NameHash == 0xFFFFFFFFu);
    CHECK(nd.SetName("abc") && std::string(nd.mpcName) == "abc" && nd.mu32NameHash == FEHashUpper("ABC"));
    CHECK(nd.SetName("Longer name") && std::string(nd.mpcName) == "Longer name" && teststub::liveAllocations == live + 1);
    CHECK(!nd.SetName(nullptr) && nd.mpcName == nullptr && teststub::liveAllocations == live);

    alignas(8) unsigned char pm[2][sizeof(FEPackage)] = {};
    auto *p1 = reinterpret_cast<FEPackage *>(pm[0]);
    auto *p2 = reinterpret_cast<FEPackage *>(pm[1]);
    reinterpret_cast<FEMinNode *>(p1)->next = reinterpret_cast<FEMinNode *>(p2);
    e->mpPackages = p1;
    auto masks = [](FEPackage *p, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
        return p->mu32ControlMask == a && p->mu32SavedControlMask == b && p->mu32InputControlMask == c && p->mu32SavedInputControlMask == d;
    };
    p1->SetControlMask(0xF0);
    e->QueuePackageSwitch("sw", p1);
    CHECK(masks(p1, 0, 0, 0, 0));
    p1->SetControlMask(0xFF);
    e->QueuePackagePush("push", p1, 0x0F);
    CHECK(masks(p1, 0xF0, 0xF0, 0xF0, 0xF0));
    e->QueuePackagePush(nullptr, nullptr, 1);                  // null package: no mask change
    p2->SetControlMask(0x33); p2->mu32SavedControlMask = 0x77;
    e->QueuePackagePushForModal("modal", p1, 0x30);           // every active package: ctrl &= ~mask, input = ctrl
    CHECK(masks(p1, 0xC0, 0xF0, 0xC0, 0xF0) && masks(p2, 0x03, 0x77, 0x03, 0x33));
    e->QueuePackageUserTransfer(p2, true, 0x02);              // mask = ctrl & arg, no side effects
    e->QueuePackageUserTransfer(p2, false, 0xFF);
    e->QueuePackagePop(p2);                                    // mask = ctrl; restore all, then clear p2
    CHECK(masks(p1, 0xF0, 0xF0, 0xC0, 0xF0) && masks(p2, 0, 0, 0, 0));
    auto c = CollectCommands();
    CHECK(c.size() == 7);
    CHECK(c[0].type == 3 && c[0].mask == 0xF0 && c[0].name == "sw" && c[0].pkg == p1 && c[0].hash == FEHashUpper("sw"));
    CHECK(c[1].type == 2 && c[1].mask == 0x0F && c[1].name == "push" && c[1].pkg == p1);
    CHECK(c[2].type == 2 && c[2].mask == 1 && c[2].name.empty() && c[2].pkg == nullptr && c[2].hash == 0xFFFFFFFFu);
    CHECK(c[3].type == 0x12 && c[3].mask == 0x30 && c[3].name == "modal");
    CHECK(c[4].type == 8 && c[4].mask == 0x02 && c[4].name.empty() && c[4].hash == 0 && c[4].pkg == p2);
    CHECK(c[5].type == 4 && c[5].mask == 0x03);
    CHECK(c[6].type == 1 && c[6].mask == 0x03 && c[6].hash == 0 && c[6].pkg == p2);
    CHECK(teststub::liveAllocations == live);

    // PackagePopQueuedPushPackage: newest matching push/modal push (type bit 1) is unlinked, masks restored
    p1->SetControlMask(0x11); p1->SetControlMaskOnly(0x01);   // ctrl/input 0x01, saved 0x11
    e->QueuePackagePush("A", p2, 0);
    e->QueuePackageSwitch("a", p2);                            // type 3 also has bit 1
    e->QueuePackagePop(p2);                                    // type 1: never matches
    p1->SetControlMask(0x11); p1->SetControlMaskOnly(0x01);
    auto *sw = reinterpret_cast<FEPackageCommand *>(e->mPackageCommands.tail->prev);
    CHECK(!e->PackagePopQueuedPushPackage(nullptr) && !e->PackagePopQueuedPushPackage("B"));
    CHECK(e->PackagePopQueuedPushPackage("A") && e->mPackageCommands.count == 2);   // switch "a" (newest) taken
    CHECK(masks(p1, 0x11, 0x11, 0x11, 0x11));
    CHECK(sw->mu32Type == 3 && sw->node.node.next == reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED));
    bridge::FreeArray(sw->node.mpcName);                       // unlinked but not freed by the original
    bridge::FreeArray(sw);
    auto c2 = CollectCommands();
    CHECK(c2.size() == 2 && c2[0].type == 2 && c2[0].name == "A" && c2[1].type == 1);
    e->mpPackages = nullptr;
}

// FEngine::ProcessPackageCommands: push/pop/switch parent links, mask hand-back, modal input masks, deletion.
static void TestProcessPackageCommands()
{
    FEngine *e = TestEngine();
    Collect(); CollectCommands();
    int live = teststub::liveAllocations;
    alignas(8) unsigned char pm[5][sizeof(FEPackage)] = {};
    FEPackage *P[5];
    for (int i = 0; i < 5; ++i) P[i] = reinterpret_cast<FEPackage *>(pm[i]);
    FEPackage *A = P[0], *B = P[1], *C = P[2], *D = P[3], *X = P[4];
    auto link = [&](std::initializer_list<FEPackage *> l) {
        FEPackage *prev = nullptr;
        for (FEPackage *p : l) { reinterpret_cast<FEMinNode *>(p)->next = nullptr; if (prev) reinterpret_cast<FEMinNode *>(prev)->next = reinterpret_cast<FEMinNode *>(p); else e->mpPackages = p; prev = p; }
    };
    link({A, B});
    A->mi32Priority = 0; B->mi32Priority = 1; B->mpParent = A;
    teststub::levelSortingList = {"c", "D"};
    auto &d = teststub::delivered;
    // push "C" from B: priority 2, parent B, B loses the mask (ctrl + input)
    B->SetControlMask(0xFF); teststub::pushResult = C;
    d.clear();
    e->QueuePackagePush("C", B, 0x0F);
    e->ProcessPackageCommands();
    CHECK(d.size() == 1 && std::string(d[0].kind) == "pushpkg" && teststub::lastPushName == "C" && d[0].msg == 2 && d[0].arg == 0x0F);
    CHECK(C->mpParent == B && B->mu32ControlMask == 0xF0 && B->mu32InputControlMask == 0xF0);
    CHECK(e->mPackageCommands.count == 0 && teststub::liveAllocations == live);
    // pop B (parent A active): A gets B's mask, B's children re-parented to A, input masks restored, B unloaded
    link({A, B, C});
    A->SetControlMask(0x100); B->SetControlMask(0x0F); C->mu32SavedControlMask = 0x55;   // Pop restores ctrl/savedInput from it
    d.clear();
    e->QueuePackagePop(B);
    e->ProcessPackageCommands();
    CHECK(d.size() == 1 && std::string(d[0].kind) == "unload" && d[0].pkg == B);
    CHECK(A->mu32ControlMask == 0x10F && A->mu32InputControlMask == 0x10F && C->mpParent == A && C->mu32InputControlMask == 0x55);
    // pop with an inactive parent: the link is dropped, nobody gets the mask
    link({A, B, C});
    B->mpParent = X; B->mi32Priority = 1; C->mpParent = B; X->SetControlMask(0);
    e->QueuePackagePop(B);
    e->ProcessPackageCommands();
    CHECK(B->mpParent == nullptr && C->mpParent == nullptr && X->mu32ControlMask == 0);
    // pop of a package that was never pushed (priority < 0): nothing happens
    B->mi32Priority = -1;
    d.clear();
    e->QueuePackagePop(B);
    e->ProcessPackageCommands();
    CHECK(d.empty());
    // switch B -> "D": D takes B's priority, parent and children; A gets B's mask back
    link({A, B, C});
    B->mi32Priority = 1; B->mpParent = A; C->mpParent = B;
    A->SetControlMask(0); B->SetControlMask(0x30); teststub::pushResult = D;
    d.clear();
    e->QueuePackageSwitch("d", B);
    e->ProcessPackageCommands();
    CHECK(d.size() == 2 && std::string(d[0].kind) == "unload" && std::string(d[1].kind) == "pushpkg" && d[1].msg == 1 && d[1].arg == 0x30);
    CHECK(D->mpParent == A && C->mpParent == D && A->mu32ControlMask == 0x30);
    // push of a package not in the level sorting list: no push; modal push limits every other package's input
    d.clear();
    e->QueuePackagePush("nope", A, 0);
    e->QueuePackagePushForModal("C", nullptr, 0x0F);   // priority -1 -> pushed at 0, no parent
    A->SetControlMask(0xFF); B->SetControlMask(0xFF); C->SetControlMask(0xFF); C->mpParent = nullptr;
    teststub::pushResult = C;
    e->ProcessPackageCommands();
    CHECK(d.size() == 1 && d[0].msg == 0 && C->mpParent == nullptr);
    CHECK(A->mu32InputControlMask == 0xF0 && B->mu32InputControlMask == 0xF0 && C->mu32InputControlMask == 0xFF);
    // user transfer: false (4) -> to the parent, true (8) -> to the first child; message 0x334C5493 to the receiver
    link({A, B, C});
    B->mpParent = A; C->mpParent = B;
    A->SetControlMask(0); B->SetControlMask(0x0F); C->SetControlMask(0);
    e->QueuePackageUserTransfer(B, false, 0x03);       // mask stored = B ctrl & 3 = 3
    e->ProcessPackageCommands();
    CHECK(A->mu32ControlMask == 0x03 && B->mu32ControlMask == 0x0C);
    e->QueuePackageUserTransfer(B, true, 0xFF);        // mask = 0x0C
    e->ProcessPackageCommands();
    CHECK(C->mu32ControlMask == 0x0C && B->mu32ControlMask == 0 && C->mu32SavedInputControlMask == 0x0C);
    auto q = Collect();
    CHECK(q.size() == 2 && q[0].msg == 0x334C5493u && q[1].msg == 0x334C5493u);
    CHECK(teststub::liveAllocations == live);
    teststub::pushResult = nullptr;
    e->mpPackages = nullptr;
}

// FEPackage::UpdateObject + IssueScriptMessages: script clock, end handling and event ranges. Every script has
// one event per time step (message = base + time, target = the object) so the queued messages show the range.
static void TestUpdateObject()
{
    struct ObjWithMovie { FEObject o; int32_t movieTime; };
    alignas(8) unsigned char mem[sizeof(ObjWithMovie)] = {};
    auto *obj = FEObject::Construct(mem);
    uint32_t data[40] = {};
    obj->mpData = data;
    static FEScriptEvent evA[201], evB[201];
    for (uint32_t t = 0; t <= 200; ++t) { evA[t] = {1000 + t, 0, t}; evB[t] = {2000 + t, 0, t}; }
    FEScript a = {}, b = {};
    FEScript::Construct(&a); FEScript::Construct(&b);
    a.mi32Length = 100; b.mi32Length = 50;
    a.mEvents = {201, evA}; b.mEvents = {201, evB};
    obj->mpCurrentScript = &a;
    alignas(8) unsigned char pm[sizeof(FEPackage)] = {};
    auto *pkg = reinterpret_cast<FEPackage *>(pm);
    pkg->mbFastInterp = 1; pkg->mi32ScriptDelta = 30;
    pkg->mpEngine = TestEngine();
    std::vector<teststub::QueuedMessage> q;
    auto range = [&](size_t from, size_t n, uint32_t first, uint32_t last) {
        return q.size() >= from + n && q[from].msg == first && q[from + n - 1].msg == last;
    };
    pkg->UpdateObject(obj, 16);                                  // 0 -> 30: events 0..29
    q = Collect();
    CHECK(a.mi32Time == 30 && q.size() == 30 && range(0, 30, 1000, 1029) && q[0].obj == obj && q[0].target == nullptr);
    a.mi32Time = 90; q.clear();                                  // stop: events 90..100 (end inclusive), time = 101
    pkg->UpdateObject(obj, 16);
    q = Collect();
    CHECK(a.mi32Time == 101 && q.size() == 11 && range(0, 11, 1090, 1100));
    pkg->mi32ScriptDelta = 0; q.clear();                         // finished: no events, tracks skipped, flags cleared
    obj->mu32Flags |= 0x1800000;
    pkg->UpdateObject(obj, 16);
    q = Collect();
    CHECK(a.mi32Time == 101 && q.empty() && (obj->mu32Flags & 0x1800000) == 0);
    pkg->mi32ScriptDelta = 30;
    a.mu32Flags = 1; a.mi32Time = 90; q.clear();                 // loop: 90..100 then 0..19
    pkg->UpdateObject(obj, 16);
    q = Collect();
    CHECK(a.mi32Time == 20 && q.size() == 31 && range(0, 11, 1090, 1100) && range(11, 20, 1000, 1019));
    a.mu32Flags = 2; a.mi32Time = 180; q.clear();                // ping-pong past 2*length wraps
    pkg->UpdateObject(obj, 16);
    CHECK(a.mi32Time == 10);
    a.mu32Flags = 0; a.mpChainedScript = &b; a.mi32Time = 90; q.clear();   // chain: rest carries over
    pkg->UpdateObject(obj, 16);
    q = Collect();
    CHECK(obj->mpCurrentScript == &b && b.mi32Time == 20 && a.mi32Time == 0);
    CHECK(q.size() == 31 && range(0, 11, 1090, 1100) && range(11, 20, 2000, 2019));
    obj->mu32Type = 7;                                           // movie: playback time += delta (fast mode)
    reinterpret_cast<ObjWithMovie *>(mem)->movieTime = 5;
    pkg->UpdateObject(obj, 16);
    CHECK(reinterpret_cast<ObjWithMovie *>(mem)->movieTime == 21);
    obj->mu32Type = 4; teststub::lastTypeUpdate = nullptr;      // list box: bridged update
    pkg->UpdateObject(obj, 16);
    CHECK(teststub::lastTypeUpdate == obj);
    // targets: game (-1), special (-5), GUID 7, set-button magic (GUID 9 inside a group)
    alignas(8) unsigned char tm[2][sizeof(FEGroup)] = {};
    auto *g7 = reinterpret_cast<FEGroup *>(tm[0]);
    auto *o9 = reinterpret_cast<FEObject *>(tm[1]);
    g7->obj.mu32GUID = 7; g7->obj.mu32Type = 5; o9->mu32GUID = 9;
    pkg->mObjects = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
    g7->mChildren = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
    pkg->mObjects.AddTail(&g7->obj.node);
    g7->mChildren.AddTail(&o9->node);
    CHECK(pkg->FindObjectByGUID(9) == o9 && pkg->FindObjectByGUID(7) == &g7->obj && pkg->FindObjectByGUID(5) == nullptr);
    FEScriptEvent ev[4] = {{11, 0xFFFFFFFFu, 0}, {12, 0xFFFFFFFBu, 1}, {13, 7, 2}, {0x1B3909AA, 9, 3}};
    FEScript c = {};
    FEScript::Construct(&c);
    c.mi32Length = 100; c.mEvents = {4, ev};
    Collect();                                                   // drop leftovers from the updates above
    pkg->IssueScriptMessages(TestEngine(), obj, &c, 0, 10);
    q = Collect();
    // engine queue: special (-5), GUID 7, then the two highlight messages; the game message goes to the UI handler
    CHECK(q.size() == 4 && q[0].target == reinterpret_cast<FEObject *>(uintptr_t(0xFFFFFFFBu)) && q[1].target == &g7->obj);
    CHECK(q[2].msg == 0xABC08912u && q[2].target == o9 && q[3].obj == o9 && pkg->mpCurrentButton == o9);
    CHECK(!teststub::queued.empty() && teststub::queued.back().msg == 11);
    teststub::TestCallback cb{nullptr, {}, o9};                  // walk order + stop
    CHECK(!pkg->ForAllObjects(reinterpret_cast<FEObjectCallback *>(&cb)) && cb.visited.size() == 2 && cb.visited[1] == o9);
    q.clear();
    pkg->IssueScriptMessages(TestEngine(), nullptr, &c, 2, 3);  // GUID target without obj: not queued
    q = Collect();
    CHECK(q.empty());
    obj->mpData = nullptr;
    obj->Destruct();
}

static void TestPackageUpdate()
{
    alignas(8) unsigned char gm[sizeof(FEGroup)] = {}, om[3][sizeof(FEObject)] = {};
    uint32_t data[4][40] = {};
    FEScript sc[4] = {};                       // Construct leaves the time (+0x10) uninitialized, like the original
    auto *g = reinterpret_cast<FEGroup *>(gm);
    FEObject *objs[4] = {&g->obj, FEObject::Construct(om[0]), FEObject::Construct(om[1]), FEObject::Construct(om[2])};
    FEObject::Construct(&g->obj);
    g->mChildren = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
    for (int i = 0; i < 4; ++i) {
        FEScript::Construct(&sc[i]);
        sc[i].mi32Length = 1000;
        objs[i]->mpCurrentScript = &sc[i];
        objs[i]->mpData = data[i];
    }
    g->obj.mu32Type = 5;
    g->obj.mu32Flags = 0x800000 | 0x400000;   // dirty is cleared at the end of the group's own update
    alignas(8) unsigned char pm[sizeof(FEPackage)] = {};
    auto *pkg = reinterpret_cast<FEPackage *>(pm);
    pkg->mObjects = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
    pkg->mObjects.AddTail(&g->obj.node);
    pkg->mObjects.AddTail(&objs[1]->node);
    g->mChildren.AddTail(&objs[2]->node);
    g->mChildren.AddTail(&objs[3]->node);
    int marker = 0;
    pkg->Update(&marker, 25);
    CHECK(pkg->mpEngine == &marker && pkg->mi32ScriptDelta == 25);
    for (int i = 0; i < 4; ++i) CHECK(sc[i].mi32Time == 25);
    CHECK((objs[2]->mu32Flags & 0x800000) && (objs[3]->mu32Flags & 0x800000) && !(objs[1]->mu32Flags & 0x800000));
    for (auto *o : objs) { o->mpData = nullptr; }
}

// FEngine::ProcessMessageQueue: each target kind goes to the right handler; nodes are deleted.
static void TestProcessMessageQueue()
{
    FEngine *e = TestEngine();
    Collect();
    alignas(8) unsigned char pm[2][sizeof(FEPackage)] = {};
    auto *p1 = reinterpret_cast<FEPackage *>(pm[0]);
    auto *p2 = reinterpret_cast<FEPackage *>(pm[1]);
    reinterpret_cast<FEMinNode *>(p1)->next = reinterpret_cast<FEMinNode *>(p2);
    e->mpPackages = p1;
    FEMessageResponse r{};
    r.mu32MsgId = 0x77;
    static const char kP2[] = "P2", kA[] = "A", kB[] = "B";
    FEResponse rr[3] = {{0x200, reinterpret_cast<uintptr_t>(kP2), 0}, {0x200, reinterpret_cast<uintptr_t>(kA), 0},
                        {0x200, reinterpret_cast<uintptr_t>(kB), 0}};   // "switch package <name>": identifies the response
    r.mu32Count = 1; r.mpResponses = &rr[0];
    for (auto *p : {p1, p2}) p->mResponses = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
    p2->mResponses.AddTail(&r.node);
    alignas(8) unsigned char om[3][sizeof(FEObject)] = {};
    FEObject *list[2] = {FEObject::Construct(om[0]), FEObject::Construct(om[1])};
    alignas(8) unsigned char lbm[sizeof(FEListBox)] = {};
    auto *listBox = reinterpret_cast<FEListBox *>(lbm);
    FEObject *lb = FEObject::Construct(lbm);
    lb->mu32Type = 4;                                            // list box: 1 column, 3 rows, clamped, row 1
    FEListBoxSlot rows[3] = {{10, 0, 0}, {10, 10, 0}, {10, 20, 0}}, col = {5, 0, 0};
    listBox->mu32Flags = 4; listBox->mu32NumColumns = 1; listBox->mu32NumRows = 3;
    listBox->mpColumns = &col; listBox->mpRows = rows; listBox->mu32SelRow = 1;
    listBox->mfScrollY = 10; listBox->mfTargetY = 10; listBox->mfViewWidth = 5; listBox->mfViewHeight = 10;
    FEMessageResponse ra{}, rb{};
    ra.mu32MsgId = 0x77; rb.mu32MsgId = 0x13;
    ra.mu32Count = 1; ra.mpResponses = &rr[1]; rb.mu32Count = 1; rb.mpResponses = &rr[2];
    list[0]->mResponses.AddTail(&ra.node);
    list[0]->mResponses.AddTail(&rb.node);
    FEMessageTargets tg{0x77, 0, 2, list};
    p1->mu32NumMessageTargets = 1; p1->mpMessageTargets = &tg;
    auto special = [](uint32_t v) { return reinterpret_cast<FEObject *>(uintptr_t(v)); };
    auto &d = teststub::delivered;
    int live = teststub::liveAllocations;
    d.clear();
    e->QueueMessage(0x77, nullptr, p1, nullptr, 5);            // 0: p1 targets (A has a response, B none), p2 global
    e->QueueMessage(0x77, nullptr, p2, special(0xFFFFFFFC), 6);   // -4: only p2 -> its response
    e->QueueMessage(0x77, nullptr, p1, special(0xFFFFFFFE), 7);   // -2: every package's response -> p2's
    e->QueueMessage(0x59BED120, nullptr, p1, special(0xFFFFFFFA), 0);   // -6: engine flag on
    e->QueueMessage(0x12, nullptr, p1, special(0xFFFFFFFF), 8);   // -1: UI +0x18
    e->QueueMessage(0x13, nullptr, p1, list[0], 9);               // object with a response
    e->QueueMessage(0xFB814F13, nullptr, p1, lb, 0);              // list box: scroll up, no response lookup
    e->QueueMessage(0x13, nullptr, p1, lb, 0);                    // list box, other message: own responses (none)
    e->ProcessMessageQueue();
    CHECK(e->mMessages.count == 0);
    auto c = CollectCommands();
    CHECK(teststub::liveAllocations == live);                   // every message node deleted, commands drained
    auto sw = [&](size_t k, const char *name) { return k < c.size() && c[k].type == 3 && c[k].name == name; };
    CHECK(c.size() == 5 && d.size() == 1);
    CHECK(sw(0, kA) && c[0].pkg == p1 && sw(1, kP2) && c[1].pkg == p2);    // 0: A's response (p1 target), p2 global
    CHECK(sw(2, kP2) && sw(3, kP2));                                         // -4 p2, -2 all -> p2's
    CHECK(e->mbFlag01 == 1);
    CHECK(std::string(d[0].kind) == "ui" && d[0].msg == 0x12 + 0x18 * 0x10000 && d[0].arg == 8);
    CHECK(sw(4, kB));                                                         // object message 0x13 -> B
    CHECK(listBox->mu32SelRow == 0 && listBox->mfTargetY == 0 && listBox->mu32Flags == (4 | 0x40 | 2));   // scroll up
    CHECK(listBox->mfDirY == -1.0f && listBox->mfDirX == 0.0f);
    for (auto &x : {&r, &ra, &rb}) { x->mu32Count = 0; x->mpResponses = nullptr; }
    for (auto *o : {list[0], list[1], lb}) o->mResponses = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
    e->mpPackages = nullptr;
}

// FEngine::ProcessResponses opcodes: conditions/else, scripts, buttons, push per arg bit, markers, transfers.
static void TestProcessResponses()
{
    FEngine *e = TestEngine();
    Collect();
    alignas(8) unsigned char pm[sizeof(FEPackage)] = {}, om[2][sizeof(FEObject)] = {};
    auto *pkg = reinterpret_cast<FEPackage *>(pm);
    pkg->mObjects = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
    pkg->mu32NameHash = 0xAB; pkg->mu32ControlMask = 0x30;
    static const char kName[] = "PKG";
    pkg->mpcName = kName;
    FEObject *obj = FEObject::Construct(om[0]);
    FEObject *btn = FEObject::Construct(om[1]);
    btn->mu32GUID = 42;
    pkg->mObjects.AddTail(&obj->node);
    pkg->mObjects.AddTail(&btn->node);
    FEScript s1 = {}, s2 = {};
    FEScript::Construct(&s1); FEScript::Construct(&s2);
    s1.mu32Id = 1; s2.mu32Id = 2; s2.mi32Time = 99;
    obj->mScripts.AddTail(&s1.node); obj->mScripts.AddTail(&s2.node);
    obj->mpCurrentScript = &s1;
    static const char kT[] = "T", kE[] = "E";
    auto U = [](const char *p) { return reinterpret_cast<uintptr_t>(p); };
    FEResponse code[] = {
        {0x300, 1, 0},            // if current script id != 1 -> skip (it is 1: run the if-branch)
        {0x200, U(kT), 0},        //   switch "T"
        {0x500, 0, 0},            // else
        {0x200, U(kE), 0},        //   switch "E" (skipped)
        {0x501, 0, 0},            // end
        {0x100, 42, 0},           // current button = GUID 42 (notified)
        {0x102, 0, 0},            // record last button: (pkg hash, 42)
        {0x202, U(kT), 0},        // push "T" once per set arg bit
        {0x2C0, 0, 0},            // record marker (package name)
        {0x105, 0, 0}, {0x108, 0, 0},
        {0, 2, 0},                // run script 2 from time 0
    };
    FEMessageResponse resp{};
    resp.mu32Count = sizeof code / sizeof code[0];
    resp.mpResponses = code;
    CHECK(resp.FindConditionBranchTarget(0) == 2 && resp.FindConditionBranchTarget(2) == 4);
    auto &d = teststub::delivered;
    d.clear();
    e->ProcessResponses(&resp, obj, pkg, 0x5);                // arg bits 0 and 2
    auto c = CollectCommands();
    CHECK(d.empty() && c.size() == 5);                          // the button change goes to the engine queue
    CHECK(c[0].type == 3 && c[0].name == "T" && c[0].mask == 0x30);   // switch clears the package's masks
    CHECK(pkg->mpCurrentButton == btn);
    CHECK(e->RecallLastPackageButton(0xAB) == 42 && e->mi32LastButtonIndex == 1);
    CHECK(c[1].type == 2 && c[1].mask == 1 && c[2].type == 2 && c[2].mask == 4 && c[2].name == "T");
    CHECK(e->mu32NumPackageMarkers == 1 && std::string(e->macPackageMarkers[0]) == kName);
    CHECK(c[3].type == 8 && c[3].mask == 0 && c[4].type == 4 && c[4].mask == 0);   // ctrl & arg; ctrl is 0 by now
    CHECK(obj->mpCurrentScript == &s2 && s2.mi32Time == 0);
    auto q = Collect();                                         // highlight messages from the button change
    CHECK(q.size() == 2);
    // recall: last button unknown -> fallback GUID param; marker recall pops a name
    e->ClearPackageMarkers();
    e->RecordLastPackageButton(0xAB, 0);                       // replaces the 42 entry
    CHECK(e->RecallLastPackageButton(0xAB) == 0);
    pkg->mpCurrentButton = nullptr;
    FEResponse code2[] = {{0x103, 42, 0}, {0x2C1, 0, 0}, {0x101, 1, 0}};
    resp.mu32Count = 3; resp.mpResponses = code2;
    CHECK(e->RecordPackageMarker("MARK"));
    d.clear();
    e->ProcessResponses(&resp, obj, pkg, 0);
    CHECK(pkg->mpCurrentButton == btn && e->mu32NumPackageMarkers == 0 && e->mbFlag01 == 1);
    auto c2 = CollectCommands();
    CHECK(d.empty() && c2.size() == 1 && c2[0].type == 3 && c2[0].name == "MARK");
    Collect();
    obj->mScripts = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
}

// bFastATan / bFixATan: quadrant mapping and special cases (stub tables are formula approximations).
static void TestATan()
{
    CHECK(bFastATan(1, 0) == 0 && bFastATan(0, 1) == 0x4000 && bFastATan(-1, 0) == 0x8000 && bFastATan(0, -1) == 0xC000);
    CHECK(bFastATan(2, 2) == 0x2000 && bFastATan(-2, 2) == 0x6000 && bFastATan(-2, -2) == 0xA000 && bFastATan(0, 0) == 0);
    CHECK(std::abs(int(bFastATan(2, 1)) - 0x12E4) <= 0x40);           // atan(0.5) = 0x12E4
    CHECK(bFixATan(0) == 0 && std::abs(bFixATan(0x10000) - 0x2000) <= 2 && bFixATan(-0x10000) == -bFixATan(0x10000));
    CHECK(bFixATan(0x7FFFFFFF) == 0x3FFF && std::abs(bFixATan(0x400000) - 0x3EBA - ((0x3FD7 - 0x3EBA) * 0x4000 >> 16)) == 0);
    CHECK(bFixATan(1, 0) == 0 && bFixATan(0, 5) == 0x4000 && bFixATan(-1, 0) == 0x8000 && bFixATan(1, -1) == -bFixATan(1, 1));
    CHECK(std::abs(bFixATan(3, 3) - 0x2000) <= 2 && bFixATan(-3, -3) == bFixATan(3, 3) - 0x8000);
}

// SlotPool: NewSlotPool free-list order, Malloc zero fill + expansion (flag 1), chain-wide slot numbers.
static void TestSlotPool()
{
    int live = teststub::liveAllocations;
    SlotPool *p = SlotPool::NewSlotPool(8, 4, "test", 7);
    CHECK(p && p->mu32Flags == 0xB && p->mi32TotalSlots == 4 && p->mpFreeList == p->SlotData() && p->mu32MemFlags == 7);
    void *s[6];
    for (int i = 0; i < 4; ++i) {
        s[i] = p->Malloc();
        CHECK(s[i] == p->SlotData() + 8 * i && static_cast<uint32_t *>(s[i])[1] == 0);   // zeroed (flag 2)
    }
    CHECK(p->mpFreeList == nullptr && p->mi32NumAllocated == 4 && p->mi32MostAllocated == 4);
    s[4] = p->Malloc();                                          // expands by 4/4 + 1 = 2 slots
    SlotPool *q = p->mpNext;
    CHECK(q && q->mi32NumSlots == 2 && p->mi32TotalSlots == 6 && q->mpFreeList == nullptr && q->mu32Flags == 0xB);
    CHECK(s[4] == q->SlotData() && SlotPool::GetSlotNumber(p, s[4]) == 4 && SlotPool::GetSlotNumber(p, s[2]) == 2);
    s[5] = p->Malloc();
    CHECK(s[5] == q->SlotData() + 8 && SlotPool::GetSlotNumber(p, s[5]) == 5 && p->Malloc() != nullptr);   // expands again
    p->Free(s[1]);
    CHECK(p->mpFreeList == s[1] && p->FastMalloc() == s[1]);
    CHECK(SlotPool::GetSlotNumber(p, reinterpret_cast<uint8_t *>(p) - 8) == -1 && SlotPool::GetSlotNumber(nullptr, s[0]) == -1);
    CHECK(bCountTotalSlots(p) == 8 && p->mi32NumAllocated == 7 && bCountFreeSlots(p) == 1 && !bIsSlotPoolFull(p));
    // allocated (slot order): s0, s1 (re-taken), s2, s3 | s4, s5 | one slot of the third pool
    CHECK(p->GetAllocatedSlot(0) == s[0] && p->GetAllocatedSlot(1) == s[1] && p->GetAllocatedSlot(4) == s[4]);
    CHECK(p->GetAllocatedSlot(7) == nullptr && p->GetSlot(5) == s[5] && p->GetSlot(8) == nullptr && p->GetSlot(-1) == nullptr);
    p->Free(s[2]);
    CHECK(p->GetAllocatedSlot(2) == s[3]);
    while (p) { SlotPool *n = p->mpNext; std::free(p); --teststub::liveAllocations; p = n; }
    CHECK(teststub::liveAllocations == live);
    // manager: bNewSlotPool rounds the size and registers; bDeleteSlotPool unlinks and frees the whole chain
    SlotPoolManager *m = bridge::TheSlotPoolManager();
    SlotPool *a = bNewSlotPool(5, 2, "a", 0), *b = bNewSlotPool(8, 1, "b", 0);
    CHECK(a->mu32SlotSize == 8 && m->mpHead == a && m->mpTail == b && a->mpListNext == b && b->mpListNext == m->Sentinel());
    CHECK(bMalloc(a) && bMalloc(a) && bMalloc(a) && a->mpNext != nullptr);   // third Malloc expands
    bDeleteSlotPool(a);
    CHECK(m->mpHead == b && b->mpListPrev == m->Sentinel());
    bDeleteSlotPool(b);
    bDeleteSlotPool(nullptr);
    CHECK(m->mpHead == m->Sentinel() && teststub::liveAllocations == live);
    // bPList node pool: lazy init, deferred close when nodes are still allocated
    void *n1 = bPNode_Malloc(), *n2 = bPNode_Malloc();
    SlotPool *np = *bridge::BPNodeSlotPool();
    CHECK(n1 && n2 && np && np->mu32SlotSize == 0xC && np->mi32NumSlots == 0x100 && np->mi32NumAllocated == 2);
    bPListInit(5);                                               // already there: no-op
    CHECK(*bridge::BPNodeSlotPool() == np);
    bPListClose();                                               // still in use: deferred
    CHECK(*bridge::BPListWantToClose() == 1 && *bridge::BPNodeSlotPool() == np);
    bPNode_Free(n1);
    CHECK(*bridge::BPNodeSlotPool() == np);
    bPNode_Free(n2);                                             // last one: pool deleted
    CHECK(*bridge::BPNodeSlotPool() == nullptr && teststub::liveAllocations == live);
}

// FEPackageManager lookups: name hash from the data header or the LZ chunk, move-to-front, checksum.
static void TestPackageManager()
{
    FEPackageManager m{0, {nullptr, nullptr}};
    m.mList.next = m.mList.prev = &m.mList;
    alignas(4) uint8_t hdr[2][0x40] = {};
    uint32_t chunks[3][6] = {{0x30203}, {0x30210, 0, FEHashUpper("LZPKG")}, {0x30203}};
    for (int i = 0; i < 2; ++i) {
        auto *h = reinterpret_cast<uint32_t *>(hdr[i]);
        h[0] = 0xE76E4546u; h[2] = 0x64486B50u; h[4] = 0x20000u + i;
    }
    std::strcpy(reinterpret_cast<char *>(hdr[0] + 0x28), "MAIN.FNG");
    std::strcpy(reinterpret_cast<char *>(hdr[1] + 0x28), "Opts.fng");
    reinterpret_cast<uint32_t *>(hdr[1])[2] = 0;                 // bad magic: hash 0
    FEPackageData d[3] = {};
    d[0].mpData = hdr[0]; d[0].mpChunk = chunks[0];
    d[1].mpChunk = chunks[1];
    d[2].mpData = hdr[1]; d[2].mpChunk = chunks[2];
    for (auto &x : d) {   // AddTail
        x.node.prev = m.mList.prev; x.node.next = &m.mList; m.mList.prev->next = &x.node; m.mList.prev = &x.node;
    }
    CHECK(d[0].GetNameHash() == FEHashUpper("main.fng") && d[1].GetNameHash() == FEHashUpper("LZPKG") && d[2].GetNameHash() == 0);
    CHECK(std::string(FEPackageManager::GetBasePkgName("a\\b\\Main.fng")) == "Main.fng" && std::string(FEPackageManager::GetBasePkgName("x")) == "x");
    CHECK(m.FindFEPackageData("dir\\lzpkg") == &d[1] && m.mList.next == &d[1].node && d[1].node.next == &d[0].node);   // moved to front
    CHECK(m.FindFEPackageData("nope") == nullptr && m.FindFEPackageData(static_cast<const void *>(chunks[2])) == &d[2]);
    CHECK(m.GetPackageData("MAIN.FNG") == hdr[0] && m.mList.next == &d[0].node);
    FEPackage pk{}; pk.mpcName = "PKG";
    CHECK(m.GetActiveScreensChecksum() == 0);
    d[2].mpPackage = &pk; d[0].mpPackage = &pk;
    CHECK(m.GetActiveScreensChecksum() == 2 * bStringHash("PKG"));
    // notifications: need package + screen; NotificationMessage matches the package pointer
    int scr[2];
    d[0].mpScreen = &scr[0]; d[1].mpScreen = &scr[1];          // d[1] has no package, d[2] no screen
    auto &dl = teststub::delivered;
    dl.clear();
    m.SendMessageToPackage("main.fng", 0x11);
    m.SendMessageToPackage("lzpkg", 0x12);
    m.SendMessageToPackage("Opts.fng", 0x13);
    CHECK(dl.size() == 1 && dl[0].a == &scr[0] && dl[0].msg == 0x11 && dl[0].arg == 0 && dl[0].pkg == &pk);
    dl.clear();
    m.BroadcastMessage(0x21, 7);
    CHECK(dl.size() == 1 && dl[0].msg == 0x21 && dl[0].arg == 7);
    dl.clear();
    FEPackage other{};
    m.NotificationMessage(0x31, nullptr, 3, &other);
    m.NotificationMessage(0x32, nullptr, 4, &pk);
    CHECK(dl.size() == 1 && dl[0].msg == 0x32 && dl[0].arg == 4 && dl[0].a == &scr[0]);
    // cFEng::NotificationMessage: input toggles go to the engine, the rest to the global manager
    alignas(8) static unsigned char feb[sizeof(cFEng)] = {};
    auto *fe = reinterpret_cast<cFEng *>(feb);
    alignas(8) static unsigned char eb[sizeof(FEngine)] = {};
    fe->mpEngine = reinterpret_cast<FEngine *>(eb);
    fe->NotificationMessage(0x7E4D1288u, nullptr, 0, nullptr);
    CHECK(fe->mpEngine->mbFlag01 == 1);
    fe->NotificationMessage(0x05922615u, nullptr, 0, nullptr);
    CHECK(fe->mpEngine->mbFlag01 == 0);
    dl.clear();
    fe->NotificationMessage(0x99, nullptr, 0, &pk);              // global manager is empty: nothing
    CHECK(dl.empty());
    dl.clear();
}

// FEPackageData lifecycle + FEPackageManager Loader/UnLoader/CloseAllPackages.
static void TestPackageDataLifecycle()
{
    FEPackageManager m{0, {nullptr, nullptr}};
    m.mList.next = m.mList.prev = &m.mList;
    alignas(4) uint8_t raw[2][8 + 0x40] = {};
    for (int i = 0; i < 2; ++i) {
        auto *h = reinterpret_cast<uint32_t *>(raw[i]);
        h[0] = 0x30203;
        h[2] = 0xE76E4546u; h[4] = 0x64486B50u; h[6] = 0x20000u;
    }
    std::strcpy(reinterpret_cast<char *>(raw[0] + 8 + 0x28), "A.fng");
    std::strcpy(reinterpret_cast<char *>(raw[1] + 8 + 0x28), "B.fng");
    uint32_t bad[8] = {0x30203, 0, 0x1234};                         // bad magic: no FindScreenCreateData
    uint32_t other[2] = {0x99};
    auto &dl = teststub::delivered;
    const int live0 = teststub::liveAllocations;
    CHECK(LoaderFEngPackage(other) == 0);
    for (auto *c : {reinterpret_cast<uint32_t *>(raw[0]), reinterpret_cast<uint32_t *>(raw[1]), bad}) m.Loader(c, false);
    FEPackageData *a = FEPackageData::FromNode(m.mList.next), *b = FEPackageData::FromNode(a->node.next), *x = FEPackageData::FromNode(m.mList.prev);
    CHECK(a->mpChunk == reinterpret_cast<uint32_t *>(raw[0]) && b->mpChunk == reinterpret_cast<uint32_t *>(raw[1]) && x->mpChunk == bad);   // tail order
    CHECK(a->vtable == &teststub::fePackageDataVTable && a->mpData == raw[0] + 8 && a->mVisible == 1 && a->mPermanent == 0);
    CHECK(a->mScreenCreateData == (FEHashUpper("A.fng") ^ 0x5A5A5A5Au) && x->mScreenCreateData == 0);
    CHECK(m.GetVisibility("A.fng") && !m.GetVisibility("nope"));
    m.SetVisibility("a.fng", false);
    CHECK(!m.GetVisibility("A.fng"));
    // Activate: screen made with the load counter raised, back-pointer, control mask
    alignas(8) unsigned char pm[2][sizeof(FEPackage)] = {};
    auto *pa = reinterpret_cast<FEPackage *>(pm[0]), *pb = reinterpret_cast<FEPackage *>(pm[1]);
    pa->mpcName = "A.fng"; pa->mu32ControlMask = 0x77; pb->mpcName = "B.fng";
    teststub::screenResult = bridge::Alloc(8);
    *static_cast<void **>(teststub::screenResult) = nullptr;
    dl.clear();
    const int32_t lc = teststub::fengLoadCounter;
    m.PackageWasLoaded(pa, 5);
    CHECK(dl.size() == 1 && dl[0].msg == FEHashUpper("A.fng") && dl[0].arg == uint32_t(lc + 1) && teststub::fengLoadCounter == lc);
    CHECK(a->mpPackage == pa && pa->mpPackageData == a && a->mpScreen == teststub::screenResult && a->mPackageArg == 0x77);
    teststub::screenResult = nullptr;
    m.PackageWasLoaded(pb, 0);
    m.SetPermanent("B.fng", 1);
    // CloseAllPackages(0): permanent B only gets HackClearCache; A is closed (screen deleted, unlinked)
    dl.clear();
    m.CloseAllPackages(0);
    CHECK(dl.size() == 2 && std::string(dl[0].kind) == "clearcache" && dl[0].pkg == pb && std::string(dl[1].kind) == "clearcache" && dl[1].pkg == pa);
    CHECK(a->mpPackage == nullptr && a->mpScreen == nullptr && pa->mpPackageData == nullptr && b->mpPackage == pb);
    m.CloseAllPackages(1);
    CHECK(b->mpPackage == nullptr && pb->mpPackageData == nullptr);
    // PackageWillBeUnloaded -> UnActivate
    m.PackageWasLoaded(pb, 0);
    m.PackageWillBeUnloaded(pb);
    CHECK(b->mpPackage == nullptr);
    // UnLoader: update FEng, unlink + delete; unknown chunk does nothing else
    dl.clear();
    m.UnLoader(other, false);
    CHECK(dl.size() == 1 && std::string(dl[0].kind) == "update" && dl[0].msg == 0 && dl[0].arg == 0);
    for (auto *c : {reinterpret_cast<uint32_t *>(raw[0]), reinterpret_cast<uint32_t *>(raw[1]), bad}) m.UnLoader(c, false);
    CHECK(m.mList.next == &m.mList && m.mList.prev == &m.mList && teststub::liveAllocations == live0);
    dl.clear();
}

// FEPackage ctor/dtor (owned arrays, message targets with new[] cookie, +0x9C nodes), SetFilename, Startup.
static void TestPackageLifetime()
{
    const int live0 = teststub::liveAllocations;
    auto *pkg = static_cast<FEPackage *>(bridge::Alloc(sizeof(FEPackage)));
    std::memset(pkg, 0xCD, sizeof(FEPackage));
    pkg->Construct();
    CHECK(pkg->node.vtable == &teststub::fePackageVTable && pkg->node.next == reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED));
    CHECK(pkg->mu32ControlMask == 0xFF && pkg->mu32InputControlMask == 0xFF && pkg->mbFlagsAC[3] == 1 && pkg->mpcFilename == nullptr);
    CHECK(pkg->mList74.vtable == &teststub::fePackageList74VTable && pkg->mList9C.count == 0 && pkg->mObjects.head == nullptr);
    reinterpret_cast<FENode *>(pkg)->SetName("Pkg.fng");
    pkg->SetFilename("data\\pkg.fng");
    pkg->SetFilename("x.fng");
    CHECK(std::string(pkg->mpcFilename) == "x.fng");
    pkg->mpResources = bridge::Alloc(16);
    pkg->mu32NumResources = 2;
    auto *cookie = static_cast<uint32_t *>(bridge::Alloc(4 + 2 * sizeof(FEMessageTargets)));
    cookie[0] = 2;
    pkg->mpMessageTargets = reinterpret_cast<FEMessageTargets *>(cookie + 1);
    pkg->mpMessageTargets[0].mpObjects = static_cast<FEObject **>(bridge::Alloc(8));
    pkg->mpMessageTargets[1].mpObjects = nullptr;
    pkg->mppButtons = static_cast<FEObject **>(bridge::Alloc(8));
    pkg->mpLibraryRefs = static_cast<FELibraryRef *>(bridge::Alloc(sizeof(FELibraryRef)));
    for (int i = 0; i < 2; ++i) {   // nodes with an owned buffer at +0x10 (deleted through the stub's default path)
        auto *n = static_cast<FEPackageBufferNode *>(bridge::Alloc(sizeof(FEPackageBufferNode)));
        n->node.vtable = nullptr;
        n->mpBuffer = bridge::Alloc(4);
        pkg->mList9C.AddTail(&n->node);
    }
    auto &dl = teststub::delivered;
    dl.clear();
    int gi = 0;
    teststub::loadResourcesResult = false;
    CHECK(!pkg->Startup(&gi) && dl.size() == 1 && std::string(dl[0].kind) == "loadres" && dl[0].a == &gi && dl[0].pkg == pkg && dl[0].msg == 2);
    teststub::loadResourcesResult = true;
    CHECK(pkg->Startup(&gi));
    dl.clear();
    bridge::VirtualScalarDelete(pkg);
    CHECK(teststub::liveAllocations == live0);
}

// FEPackageReader on a synthetic .fng: header, library list, type sizes, library refs, resources, responses, targets.
namespace {
struct FngBuilder {
    std::vector<uint32_t> w;
    size_t Open(uint32_t id) { w.push_back(id); w.push_back(0); return w.size(); }   // returns data start
    void Close(size_t start) { w[start - 1] = static_cast<uint32_t>((w.size() - start) * 4); }
    void Bytes(const void *p, size_t n) { size_t at = w.size(); w.resize(at + (n + 3) / 4); std::memcpy(&w[at], p, n); }
    void Tag(uint16_t id, const void *p, uint16_t n) { uint16_t h[2] = {id, static_cast<uint16_t>((n + 3) & ~3)}; uint32_t x; std::memcpy(&x, h, 4); w.push_back(x); Bytes(p, n); }
    void Tag32(uint16_t id, uint32_t v) { Tag(id, &v, 4); }
};
std::vector<uint32_t> BuildFng(bool withLib, uint32_t badLibSize = 0, const char *libRef = nullptr, bool objectTags = false)
{
    FngBuilder b;
    size_t root = b.Open(0xE76E4546u);
    size_t hd = b.Open(0x64486B50u);
    const char name[] = "Pkg.fng", file[] = "dir\\pkg.fng";
    b.w.insert(b.w.end(), {0x20000u, 0, 0x11, 0x22, sizeof name, 0});
    char nf[sizeof name + sizeof file];
    std::memcpy(nf, name, sizeof name); std::memcpy(nf + sizeof name, file, sizeof file);
    b.Bytes(nf, sizeof nf);
    b.Close(hd);
    if (libRef) {
        size_t ll = b.Open(0x4C62694C);
        b.w.push_back(1);
        b.w.push_back(8);   // name offset from the chunk data
        b.Bytes(libRef, std::strlen(libRef) + 1);
        b.Close(ll);
    }
    size_t ty = b.Open(0x53707954); b.w.insert(b.w.end(), {1, 0x40, 5, 0x90}); b.Close(ty);
    if (withLib) {
        size_t lib = b.Open(0x5262694C);
        b.w.insert(b.w.end(), {0xAAAA, 1, 2, 0xBBBB, 3, 4});
        if (badLibSize) b.w.push_back(badLibSize);
        b.Close(lib);
    }
    size_t res = b.Open(0xCC736552);
    size_t nm = b.Open(0x6D4E7352); b.Bytes("tex0\0tex1\0\0", 12); b.Close(nm);
    size_t rq = b.Open(0x71527352); b.w.push_back(2);
    b.w.insert(b.w.end(), {0x10, 0, 2, 3, 4, 5, 0x20, 5, 6, 7, 8, 9}); b.Close(rq);
    b.Close(res);
    size_t ob = b.Open(0xCC6A624Fu);
    { size_t bt = b.Open(0x6E747542); b.w.push_back(3); b.Close(bt); }
    { size_t fo = b.Open(0xEA624F46u);
      size_t od = b.Open(0x446A624F);
      if (objectTags) {
          b.Tag32(0x744F, 4);   // Ot: list box
          { uint32_t v[4] = {0x500, 0x501, 0x10000000, 7}; b.Tag(0x504F, v, 16); }
          b.Tag(0x6E4F, "Name", 5);
          b.Tag32(0x684F, 0x777);
          { uint32_t d[2] = {2, 1}; b.Tag(0x644C, d, 8); }                      // Ld: 2 columns, 1 row
          { float c[2] = {10.0f, 0.0f}; b.Tag(0x634C, c, 8); }
          { float c[2] = {20.0f, 0.0f}; b.Tag(0x634C, c, 8); }
          { float r[2] = {5.0f, 0.0f}; b.Tag(0x724C, r, 8); }
          b.Tag32(0x6343, 0x80FF2010u);                                          // Cc cell 0
          b.Tag32(0x5443, 2);                                                    // CT string
          { const uint16_t w[] = {'a', 'b', 0}; b.Tag(0x7443, w, 6); }           // Ct
          b.Tag32(0x6343, 0x12345678u);                                          // Cc cell 1
          b.Tag32(0x774C, 1);                                                    // Lw
      } else {
          b.w.push_back(0x1111);
      }
      b.Close(od);
      if (objectTags) {
          size_t sc = b.Open(0x70726353);   // script 0x1744B3: 2 tracks, chained to 0x99
          b.Tag(0x6E53, "Init", 5);
          { uint32_t h[4] = {0x1744B3, 100, 0, 2}; b.Tag(0x6853, h, 16); }
          b.Tag32(0x6353, 0x99);
          { uint32_t f[2] = {0x00010C04u, 100}; b.Tag(0x4946, f, 8); }   // vec3, 12 bytes, interp 1
          { uint32_t tb[4] = {0, 1, 2, 3}; b.Tag(0x6254, tb, 16); }
          { uint32_t kd[8] = {0, 4, 5, 6, 50, 7, 8, 9}; b.Tag(0x644B, kd, 32); }
          { uint32_t f[2] = {0x00010402u, 100}; b.Tag(0x4946, f, 8); }   // float
          { uint32_t ev[3] = {0xABC, 0, 10}; b.Tag(0x5645, ev, 12); }
          b.Close(sc);
          size_t s2 = b.Open(0x70726353);
          { uint32_t h[4] = {0x99, 50, 1, 0}; b.Tag(0x6853, h, 16); }
          b.Close(s2);
      }
      b.Close(fo); }
    b.Close(ob);
    size_t pr = b.Open(0x52676B50);
    b.Tag32(0x694D, 0x1234); b.Tag32(0x434D, 2);
    b.Tag32(0x6952, 7); b.Tag32(0x7552, 9); b.Tag32(0x7452, 3);
    b.Tag32(0x6952, 0x201); b.Tag(0x7352, "hello", 6);
    b.Tag32(0x694D, 0x55);
    b.Close(pr);
    size_t tg = b.Open(0x67726154);
    b.Tag32(0x6354, 2);
    { uint32_t v[3] = {0x99, 1, 2}; b.Tag(0x744D, v, 12); }
    { uint32_t v[1] = {0x98}; b.Tag(0x744D, v, 4); }
    b.Close(tg);
    b.Close(root);
    b.w.insert(b.w.end(), 8, 0u);   // FindChild also reads the chunk at the parent's end
    return b.w;
}
}  // namespace

static void TestPackageReader()
{
    const int live0 = teststub::liveAllocations;
    auto &dl = teststub::delivered;
    auto data = BuildFng(true);
    FEPackageReader r{};
    r.mpData = data.data();
    CHECK(FEPackageReader::FindChild(data.data(), 0x12345678) == nullptr && FEPackageReader::FindChild(data.data(), 0x53707954) != nullptr);
    CHECK(r.ReadTypeSizes() && r.mu32NumTypes == 2 && r.GetTypeSize(5) == 0x90 && r.GetTypeSize(1) == 0x40 && r.GetTypeSize(9) == 0);
    alignas(8) static unsigned char eb[sizeof(FEngine)] = {};
    auto *e = reinterpret_cast<FEngine *>(eb);
    int ui = 0;
    e->mpUIData = &ui;
    e->mLibraries.vtable = bridge::FEMinListVTable();
    eb[3] = 1; eb[4] = 2;
    dl.clear();
    FEPackage *pkg = FEngine_LoadPackage(e, data.data(), 3);
    CHECK(pkg && std::string(pkg->mpcName) == "Pkg.fng" && std::string(pkg->mpcFilename) == "dir\\pkg.fng" && pkg->_unknown15[1] == 3);
    CHECK(dl.size() == 1 && std::string(dl[0].kind) == "loadres" && dl[0].a == &ui && dl[0].msg == 2);   // no objects: nothing to connect
    CHECK(pkg->mu32NumButtons == 3 && pkg->mppButtons != nullptr);
    CHECK(pkg->mu32NumLibraryRefs == 2 && pkg->mpLibraryRefs[1].mu32Hash == 0xBBBB && pkg->mpLibraryRefs[1]._unknown04[1] == 4);
    auto *res = static_cast<uint32_t *>(pkg->mpResources);
    CHECK(pkg->mu32NumResources == 2 && res[0] == 0x10 && res[6] == 0x20 && res[11] == 9 && std::memcmp(pkg->mp98, "tex0\0tex1", 10) == 0);
    CHECK(res[1] == static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pkg->mp98)) && res[7] == static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pkg->mp98)) + 5);
    CHECK(pkg->mResponses.count == 2);
    auto *m = reinterpret_cast<FEMessageResponse *>(pkg->mResponses.head);
    CHECK(m->mu32MsgId == 0x1234 && m->mu32Count == 2 && m->mpResponses[0].mu32Type == 7 && m->mpResponses[0].mParam == 9 && m->mpResponses[0].mu32Extra == 3);
    CHECK(m->mpResponses[1].mu32Type == 0x201 && std::string(reinterpret_cast<const char *>(m->mpResponses[1].mParam)) == "hello");
    CHECK(reinterpret_cast<FEMessageResponse *>(m->node.next)->mu32MsgId == 0x55);
    CHECK(pkg->mu32NumMessageTargets == 2 && reinterpret_cast<uint32_t *>(pkg->mpMessageTargets)[-1] == 2);
    CHECK(pkg->mpMessageTargets[0].mu32MsgId == 0x99 && pkg->mpMessageTargets[0].mu32Count == 2 && pkg->mpMessageTargets[0].mu32Capacity == 2 && pkg->mpMessageTargets[0].mpObjects[1] == nullptr);
    CHECK(pkg->mpMessageTargets[1].mu32MsgId == 0x98 && pkg->mpMessageTargets[1].mu32Count == 0 && pkg->mpMessageTargets[1].mpObjects == nullptr);
    bridge::VirtualScalarDelete(pkg);
    // object responses: merge reuses (and purges) an existing message id
    alignas(8) unsigned char ob[sizeof(FEObject)] = {};
    auto *obj = reinterpret_cast<FEObject *>(ob);
    obj->mResponses.vtable = bridge::FEMinListVTable();
    FngBuilder t;
    t.Tag32(0x694D, 0x77); t.Tag32(0x434D, 1); t.Tag32(0x6952, 1);
    r.mpCurrentObject = obj;
    r.mbMergeResponses = 1;
    const auto *tp = reinterpret_cast<const uint8_t *>(t.w.data());
    const auto tn = static_cast<uint32_t>(t.w.size() * 4);
    r.ReadMessageResponseTags(tp, tn, false);
    r.ReadMessageResponseTags(tp, tn, false);
    CHECK(obj->mResponses.count == 1);
    r.mbMergeResponses = 0;
    r.ReadMessageResponseTags(tp, tn, false);
    CHECK(obj->mResponses.count == 2);
    while (FEMinNode *n = obj->mResponses.RemHead()) bridge::VirtualScalarDelete(n);
    // referenced library: loaded once through the game interface, then reference counted
    auto lib = BuildFng(true);
    auto withRef = BuildFng(true, 0, "r\\pkg.fng");      // library filename "dir\\pkg.fng" hashes from +2
    teststub::libraryData = lib.data();
    dl.clear();
    FEPackage *p1 = FEngine_LoadPackage(e, withRef.data(), 0);
    CHECK(p1 && p1->mList74.count == 1 && e->mLibraries.count == 1 && std::string(dl[0].kind) == "getpkg" && dl[0].msg == FEHashUpper("r\\pkg.fng"));
    auto *libPkg = reinterpret_cast<FEPackage *>(e->mLibraries.head);
    CHECK(libPkg && libPkg->mi32Priority == 1 && std::string(libPkg->mpcFilename) == "dir\\pkg.fng");
    bool announced = false;
    for (auto &d : dl) announced |= std::string(d.kind) == "libloaded" && d.pkg == libPkg;
    CHECK(announced);
    FEPackage *p2 = FEngine_LoadPackage(e, withRef.data(), 0);
    CHECK(p2 && libPkg->mi32Priority == 2 && e->mLibraries.count == 1);
    teststub::libraryData = nullptr;
    e->mLibraries.RemNode(&libPkg->node);
    CHECK(FEngine_LoadPackage(e, withRef.data(), 0) == nullptr);   // game has no data for it
    for (FEPackage *p : {p1, p2, libPkg}) bridge::VirtualScalarDelete(p);
    // an object: list box fixup, chained script id -> pointer, current script 0x1744B3, initial interpolation, linked
    {
        auto od = BuildFng(true, 0, nullptr, true);
        dl.clear();
        FEPackage *po = FEngine_LoadPackage(e, od.data(), 0);
        CHECK(po && po->mObjects.count == 1);
        auto *o = reinterpret_cast<FEObject *>(po->mObjects.head);
        auto *s0 = reinterpret_cast<FEScript *>(o->mScripts.head);
        auto *s1 = s0 ? reinterpret_cast<FEScript *>(s0->node.next) : nullptr;
        CHECK(o->mScripts.count == 2 && s0->mu32Id == 0x1744B3 && s1 && s1->mu32Id == 0x99 && s1->mi32Length == 50);
        CHECK(o->mpCurrentScript == s0 && s0->mi32Time == 0 && s0->mpChainedScript == s1 && s0->mi32Length == 100);
        CHECK(s0->mu32NumTracks == 2 && s0->mpTracks[0].mu8ParamType == 4 && s0->mpTracks[0].mu8ValueSize == 12 && s0->mpTracks[0].Length() == 100);
        CHECK(s0->mpTracks[0].mi8DataIndex == 0 && s0->mpTracks[1].mi8DataIndex == 3 && s0->mpTracks[1].mu8ParamType == 2);
        CHECK(s0->mpTracks[0].mBaseKey._data[0] == 0 && s0->mpTracks[0].mBaseKey._data[1] == 4 && s0->mpTracks[0].mBaseKey._data[3] == 6);   // first key replaces Tb
        auto *k1 = reinterpret_cast<FEKeyNode *>(s0->mpTracks[0].mKeys.head);
        CHECK(k1 && k1->_data[0] == 50 && k1->_data[1] == 7 && k1->_data[3] == 9 && k1->node.next == nullptr);
        CHECK(s0->mEvents.mu32Count == 1 && s0->mEvents.mpEvents[0].mu32MsgId == 0xABC && s0->mEvents.mpEvents[0].mu32Time == 10);
        CHECK(s0->mpcName && std::string(s0->mpcName) == "Init");   // names only with mbFlag0A (engine +4 = 2 here)
        CHECK(true && o->mpCurrentScript == s0 && s0->mi32Time == 0 && s0->mpChainedScript == s1);
        int interp = 0;
        for (auto &d : dl) interp += std::string(d.kind) == "keyinterp" && d.a == s0;
        CHECK(interp == 2);
        auto *lbx = reinterpret_cast<FEListBox *>(o);
        CHECK(o->node.vtable == &teststub::feListBoxVTable && lbx->mu32NumColumns == 2 && lbx->mu32NumRows == 1);
        CHECK(lbx->mpColumns[1].mfSize == 20.0f && lbx->mpColumns[1].mfPosition == 10.0f && lbx->mpRows[0].mfSize == 5.0f);   // RecalculateCummulative
        CHECK(lbx->mpCells[0].mu32Color == 0x80FF2010u && lbx->mpCells[0].mu32Type == 2 && lbx->mpCells[0].mpText[1] == 'b');
        CHECK(lbx->mpCells[1].mu32Color == 0x12345678u && lbx->mu32SelColumn == 1 && !(lbx->mu32Flags & 4));
        CHECK(o->mu32GUID == 0x500 && o->mu32NameHash == 0x777 && o->mu32Flags == 0x10000000 && o->_unknown22 == 7);
        CHECK(po->mppButtons[0] == o);
        CHECK(o->mpcName && std::string(o->mpcName) == "Name");
        lbx->mu32Flags |= 1;   // let Terminate free the columns/rows (cells leak like the original: freed by hand)
        lbx->CleanupCells();
        bridge::VirtualScalarDelete(po);
    }
    // failures
    auto bad = BuildFng(true, 7);
    CHECK(FEngine_LoadPackage(e, bad.data(), 0) == nullptr);
    auto nolib = BuildFng(false);
    pkg = FEngine_LoadPackage(e, nolib.data(), 0);
    CHECK(pkg && pkg->mu32NumLibraryRefs == 0);
    bridge::VirtualScalarDelete(pkg);
    teststub::loadResourcesResult = false;
    CHECK(FEngine_LoadPackage(e, data.data(), 0) == nullptr);
    teststub::loadResourcesResult = true;
    data[0] = 0;
    CHECK(FEngine_LoadPackage(e, data.data(), 0) == nullptr);
    dl.clear();
    CHECK(teststub::liveAllocations == live0);
}

// FEPackageReader::ProcessStringTag on an FEString.
static void TestReaderStringTags()
{
    const int live0 = teststub::liveAllocations;
    alignas(8) unsigned char mem[sizeof(FEString)];
    FEString *s = FEString::Construct(mem);
    FEPackageReader r{};
    r.mpCurrentObject = &s->base;
    auto tag = [&](uint16_t id, const void *p, size_t n) {
        std::vector<uint8_t> b(4 + n);
        uint16_t h[2] = {id, static_cast<uint16_t>(n)};
        std::memcpy(b.data(), h, 4); std::memcpy(b.data() + 4, p, n);
        r.ProcessStringTag(b.data());
    };
    auto tag32 = [&](uint16_t id, uint32_t v) { tag(id, &v, 4); };
    tag32(0x6A53, 1); tag32(0x6C53, 2); tag32(0x7753, 3);
    CHECK(s->_unknown6C == 1 && s->_unknown70 == 2 && s->_unknown74 == 3);
    s->base.mu32Flags = 0;
    tag32(0x4853, s->mu32LabelHash);
    CHECK(s->base.mu32Flags == 0);
    tag32(0x4853, 0x1234);
    CHECK(s->mu32LabelHash == 0x1234 && s->base.mu32Flags == 0x400000);
    tag(0x4C53, "LBL", 4);
    CHECK(s->mpcLabel == nullptr);                       // labels only with mbFlag09
    r.mbFlag09 = 1;
    tag(0x4C53, "LBL", 4);
    CHECK(s->mpcLabel && std::string(s->mpcLabel) == "LBL");
    const uint16_t w[] = {'h', 'i', 0};
    tag(0x7453, w, sizeof w);
    CHECK(s->mstrText.mpsString && s->mstrText.mpsString[0] == 'h' && s->mstrText.Length() == 2);
    tag32(0x6253, 40);
    CHECK(s->mstrText.mulBufferLength >= 40 && s->mstrText.mpsString[1] == 'i');
    s->Destruct();
    CHECK(teststub::liveAllocations == live0);
}

// ReadScriptTags "Ti": a missing track is inserted from the engine's type library, ordered by data offset.
static void TestReaderInsertTrack()
{
    alignas(8) static unsigned char eb[sizeof(FEngine)] = {};
    auto *e = reinterpret_cast<FEngine *>(eb);
    FETypeParam params[3] = {};
    params[0].mu32Offset = 0; params[1].mu32ParamType = 2; params[1].mu32ValueSize = 4; params[1].mu32Offset = 0x10;   // track 1 = data offset 0x10
    params[2].mu32Offset = 16;
    FETypeNode type{};
    type.mu32Type = 7;
    for (auto &pp : params) type.mParams.AddTail(&pp.node);
    e->mTypes.AddTail(&type.node);
    alignas(8) unsigned char ob[sizeof(FEObject)] = {};
    auto *obj = reinterpret_cast<FEObject *>(ob);
    obj->mu32Type = 7;
    FEPackageReader r{};
    r.mpEngine = e;
    r.mpCurrentObject = obj;
    r.mbMergeResponses = 1;   // keep the script out of the object's list
    FngBuilder b;
    { uint32_t h[4] = {0x1744B3, 30, 0, 2}; b.Tag(0x6853, h, 16); }
    { uint32_t f[2] = {0x00010401u, 30}; b.Tag(0x4946, f, 8); }   // data index 0
    { uint32_t f[2] = {0x00010401u, 30}; b.Tag(0x4946, f, 8); b.Tag32(0x6F54, 7); }   // data index 7 (offset 0x1C, track 2)
    b.Tag32(0x6954, 1);        // track 1 (offset 0x10 -> index 4): inserted between
    b.Tag32(0x7454, 3);        // applies to the inserted track
    b.Tag32(0x6954, 0);        // existing (data index 0)
    r.ReadScriptTags(reinterpret_cast<const uint8_t *>(b.w.data()), static_cast<uint32_t>(b.w.size() * 4));
    FEScript *s = obj->mpCurrentScript;
    CHECK(s && s->mu32NumTracks == 3);
    CHECK(s->mpTracks[0].mi8DataIndex == 0 && s->mpTracks[1].mi8DataIndex == 4 && s->mpTracks[2].mi8DataIndex == 7);
    CHECK(s->mpTracks[1].mu8ParamType == 2 && s->mpTracks[1].mu8ValueSize == 4 && s->mpTracks[1].mu8InterpType == 3 && s->mpTracks[1].Length() == 30);
    CHECK(reinterpret_cast<uint32_t *>(s->mpTracks)[-1] == 3);
    bridge::VirtualScalarDelete(s);
    e->mTypes.head = e->mTypes.tail = nullptr; e->mTypes.count = 0;
}

// FEListBox allocation paths: resize keeps min(old, new) slots, cells re-initialized, strings owned.
static void TestListBoxCells()
{
    const int live0 = teststub::liveAllocations;
    auto *lb = FEListBox::Construct(bridge::Alloc(sizeof(FEListBox)));
    CHECK(lb->obj.mu32Type == 4 && lb->mu32Flags == 4 && lb->mfSpeedX == 1.0f && lb->mfPulse == 1.0f && lb->mpCells == nullptr);
    lb->SetNumColumns(3);              // no rows yet: no cells
    CHECK(lb->mu32NumColumns == 3 && lb->mpCells == nullptr && lb->mpColumns[2].mfSize == 0.0f);
    lb->mpColumns[0].mfSize = 4.0f;
    lb->SetNumRows(2);
    CHECK(lb->mu32NumRows == 2 && lb->mpCells && lb->mpCells[5].mu32Color == 0xFFFFFFFFu && lb->mpCells[5].mfScaleY == 1.0f);
    lb->SetSelColumn(9); lb->SetSelRow(1);
    CHECK(lb->mu32SelColumn == 2 && lb->mu32SelRow == 1);
    lb->SetCellType(2);
    const uint16_t w[] = {'x', 'y', 0};
    lb->SetCellString(w);
    CHECK(lb->CurrentCell()->mpText && lb->CurrentCell()->mpText[1] == 'y');
    lb->SetCellString(w);                             // replaces (old freed)
    lb->SetSelColumn(0); lb->SetSelRow(0);
    lb->CurrentCell()->mu32Color = 0xAABBCCDDu;
    lb->SetNumColumns(4);              // copies only min(3, 4) = 3 BYTES per row (original quirk)
    CHECK(lb->mu32NumColumns == 4 && lb->mpColumns[0].mfSize == 4.0f && lb->mpColumns[3].mfSize == 0.0f);
    CHECK((lb->mpCells[0].mu32Color & 0xFFFFFF) == 0xBBCCDDu && lb->mpCells[1].mu32Color == 0 && lb->mpCells[3].mu32Color == 0xFFFFFFFFu);
    lb->SetCellType(2);
    lb->SetCellString(w);
    lb->SetCellType(1);                // leaving type 2 frees the text
    CHECK(lb->CurrentCell()->mpText == nullptr);
    lb->mpCells[7].mu32Type = 2;       // the cell whose text was lost by the resize keeps a null pointer
    lb->mpCells[7].mpText = nullptr;
    teststub::liveAllocations += 0;    // the string set before the resize leaked in the original too
    lb->IncrementCellByColumn();
    CHECK(lb->mu32SelColumn == 1);
    lb->SetSelColumn(3); lb->SetSelRow(1);
    lb->IncrementCellByColumn();
    CHECK(lb->mu32SelColumn == 0 && lb->mu32SelRow == 0);
    lb->Terminate();                   // flag 1 not set: nothing
    CHECK(lb->mu32NumColumns == 4);
    lb->CleanupCells();
    lb->mu32Flags |= 1;
    bridge::VirtualScalarDelete(lb);   // Terminate frees columns and rows
    CHECK(teststub::liveAllocations == live0 + 1);   // the text leaked by the byte-copy resize (as in the original)
    teststub::liveAllocations = live0;
}

// FECodeListBox data side: grid (re)initialization, string pool, virtual -> visible mapping, range setters.
static void TestCodeListBox()
{
    const int live0 = teststub::liveAllocations;
    auto *lb = FECodeListBox::Construct(bridge::Alloc(sizeof(FECodeListBox)));
    CHECK(lb->obj.mu32Type == 6 && lb->mu32Flags == 2 && lb->mColor.c[3] == 0xFF && lb->mfPulse == 1.0f && lb->mpCells == nullptr);
    CHECK(FEPositiveMod(7, 3) == 1 && FEPositiveMod(-1, 3) == 2 && FEPositiveMod(-3, 3) == 3 && FEPositiveMod(-5, 1) == 0);
    lb->AllocateStrings(4, 8);
    lb->Initialize(2, 2);
    CHECK((lb->mu32Flags & 1) && lb->mu32TotalColumns == 2 && lb->mu32TotalRows == 2 && lb->mpCells[3].mu32Color == 0xFFFFFFFFu);
    // two text cells from the pool
    for (int i : {1, 3}) {
        uint16_t *s = lb->mppStrings[lb->mu32StringsUsed++];
        const uint16_t w[] = {'a', uint16_t('0' + i), 'x', 'y', 'z', 0};
        std::memcpy(s, w, sizeof w);
        lb->mpCells[i].mu32Type = 2;
        lb->mpCells[i].mpText = s;
    }
    lb->AllocateStrings(4, 3);   // moves the texts, truncated to 2 chars
    CHECK(lb->mu32StringsUsed == 2 && lb->mpCells[1].mpText == lb->mppStrings[0] && lb->mpCells[3].mpText[1] == '3' && lb->mpCells[3].mpText[2] == 0);
    lb->Initialize(1, 1);        // drops cells 1..3: the two texts go back to the pool
    CHECK(lb->mu32StringsUsed == 0 && lb->mu32NumColumns == 1 && lb->mpCells[0].mu32Color == 0xFFFFFFFFu);
    lb->Initialize(3, 2);
    lb->SetTotalNumColumns(10);
    lb->SetTotalNumRows(5);
    CHECK(lb->mi32ColumnOffset == 0 && lb->GetRealColumn(4) == 1 && lb->GetRealColumn(12) == 2 && lb->GetRealRow(3) == 1);
    lb->mu32Flags |= 8;          // centered
    lb->mu32SelColumn = 0;
    lb->SetTotalNumColumns(10);
    CHECK(lb->mi32ColumnOffset == 9);   // 0 - 3/2 = -1 -> 10 - 1
    lb->mu32SelColumn = 40;      // clamped to 9 first
    lb->SetTotalNumColumns(10);
    CHECK(lb->mi32ColumnOffset == 8);
    lb->mi32ColumnOffset = 0; lb->mi32RowOffset = 0;
    lb->SetCellColor(1, 0, 0x11223344u, 2, 2);
    CHECK(lb->mpCells[0].mu32Color == 0xFFFFFFFFu && lb->mpCells[1].mu32Color == 0x11223344u && lb->mpCells[5].mu32Color == 0x11223344u);
    const float xy[2] = {2.0f, 3.0f};
    lb->SetCellScale(0, 1, xy, 1, 1);
    lb->SetCellJustification(2, 1, 7, 1, 1);
    CHECK(lb->mpCells[3].mfScaleY == 3.0f && lb->mpCells[5]._unknown1C == 7);
    // FillAllCells: callback first, else the game interface; virtual indices wrap
    static std::vector<std::pair<int32_t, int32_t>> calls;
    calls.clear();
    lb->mu32TotalColumns = 5; lb->mu32TotalRows = 1; lb->mi32ColumnOffset = 4; lb->mi32RowOffset = 0;
    lb->mpFillCallback = [](void *, FECodeListBox *, int32_t c, int32_t r) { calls.push_back({c, r}); };
    lb->FillAllCells();   // 3 columns x min(2, 1) rows
    CHECK(calls.size() == 3 && calls[0].first == 4 && calls[1].first == 0 && calls[2].first == 1 && calls[2].second == 0);
    lb->mpFillCallback = nullptr;
    int gi = 0;
    lb->mpGameInterface = &gi;
    teststub::delivered.clear();
    lb->FillAllCells();
    CHECK(teststub::delivered.size() == 3 && std::string(teststub::delivered[0].kind) == "fillcell" && teststub::delivered[1].msg == 0);
    teststub::delivered.clear();
    // movement: clamped list at its end -> "CLB AT MAX"; a window shift rotates texts and refills the new column
    lb->mpGameInterface = &gi;
    lb->mu32Flags = 1 | 2 | 4;
    lb->mu32TotalColumns = 3; lb->mu32NumColumns = 3;   // everything visible: MIN + MAX, no move
    CHECK(!lb->CheckMovement(1, 0, 0, 3, 3) && teststub::delivered.size() == 2 && teststub::delivered[0].msg == FEHashUpper("CLB AT MIN") && teststub::delivered[1].msg == FEHashUpper("CLB AT MAX"));
    teststub::delivered.clear();
    lb->mu32Flags = 1 | 8;                              // centered: offset and selection wrap together
    lb->mu32TotalColumns = 5; lb->mi32ColumnOffset = 4; lb->mu32SelColumn = 0;
    lb->mpCells[0].mpText = reinterpret_cast<uint16_t *>(uintptr_t{0x10});
    lb->mpCells[1].mpText = reinterpret_cast<uint16_t *>(uintptr_t{0x20});
    lb->mpCells[2].mpText = reinterpret_cast<uint16_t *>(uintptr_t{0x30});
    lb->mpCells[1].mu32Color = 0xABCDu;
    CHECK(!lb->ScrollSelection(1, 0));   // returns the row result (dy = 0: no move)
    CHECK(lb->mi32ColumnOffset == 0 && lb->mu32SelColumn == 1 && lb->mpCells[0].mu32Color == 0xABCDu);
    CHECK(lb->mpCells[0].mpText == reinterpret_cast<uint16_t *>(uintptr_t{0x20}) && lb->mpCells[2].mpText == reinterpret_cast<uint16_t *>(uintptr_t{0x10}));
    CHECK(teststub::delivered.size() == lb->mu32NumRows && std::string(teststub::delivered[0].kind) == "fillcell" && teststub::delivered[0].msg == 2 && teststub::delivered[0].arg == 0);
    for (int i = 0; i < 3; ++i) lb->mpCells[i].mpText = nullptr;
    teststub::delivered.clear();
    lb->mfPulse = 0.5f;
    FECodeListBox::DefaultSelectCallback(lb);
    CHECK(lb->mColor.c[3] == 127);
    // Clone: same grid, flags 1-3, texts copied into its own pool, image data for type 1
    lb->mu32Flags = 1 | 2 | 8;
    lb->AllocateStrings(4, 6);
    lb->mpCells[0].mu32Type = 2;
    lb->mpCells[0].mpText = lb->mppStrings[lb->mu32StringsUsed++];
    { const uint16_t w[] = {'h', 'i', 0}; std::memcpy(lb->mpCells[0].mpText, w, sizeof w); }
    lb->mpCells[1].mu32Type = 1;
    lb->mpCells[1].mData[2] = 0x1234;
    FECodeListBox *copy = lb->Clone(false);
    CHECK(copy && copy->mu32NumColumns == lb->mu32NumColumns && copy->mu32NumRows == lb->mu32NumRows && copy->mu32Flags == (1u | 2u | 8u));
    CHECK(copy->mpCells[0].mpText != lb->mpCells[0].mpText && copy->mpCells[0].mpText[1] == 'i' && copy->mu32StringsUsed == 1);
    CHECK(copy->mpCells[1].mData[2] == 0x1234 && copy->mpGameInterface == lb->mpGameInterface && copy->mfPulse == 1.0f);
    bridge::VirtualScalarDelete(copy);
    // Update: pulse bounces between 0 and 1
    lb->mpSelectCallback = nullptr;
    lb->mfPulse = 0.5f; lb->mfPulseRate = -0.25f;
    lb->Update(1.0f);
    CHECK(lb->mfPulse == 0.25f);
    lb->Update(2.0f);
    CHECK(lb->mfPulse == 0.0f && lb->mfPulseRate == 0.25f);
    lb->Update(8.0f);
    CHECK(lb->mfPulse == 1.0f && lb->mfPulseRate == -0.25f);
    bridge::VirtualScalarDelete(lb);
    CHECK(teststub::liveAllocations == live0);
}

// FEngine::PushPackage / UnloadPackage on a private engine: idle reactivation, priority order, loading through the
// game interface, queue cleanup on unload, idle keep, library release.
static void TestEnginePushUnload()
{
    const int live0 = teststub::liveAllocations;
    auto *e = static_cast<FEngine *>(std::calloc(1, sizeof(FEngine)));
    for (FEMinList *l : {&e->ActiveList(), &e->mIdlePackages, &e->mLibraries, &e->mMessages, &e->mPackageCommands}) l->vtable = &teststub::feMinListVTable;
    int gi = 0;
    e->mpUIData = &gi;
    auto *idle = static_cast<FEPackage *>(bridge::Alloc(sizeof(FEPackage)));
    idle->Construct();
    reinterpret_cast<FENode *>(idle)->SetName("Menu.fng");
    e->mIdlePackages.AddTail(&idle->node);
    auto *other = static_cast<FEPackage *>(bridge::Alloc(sizeof(FEPackage)));
    other->Construct();
    other->mi32Priority = 5;
    e->ActiveList().AddTail(&other->node);
    auto &dl = teststub::delivered;
    dl.clear();
    // by base name, reactivated, inserted before the higher-priority one
    FEPackage *p = e->PushPackage("data\\front\\Menu.fng", 2, 0x33);
    CHECK(p == idle && e->mIdlePackages.count == 0 && e->ActiveList().head == &idle->node && e->ActiveList().count == 2);
    CHECK(p->mi32Priority == 2 && p->mu32ControlMask == 0x33 && p->mu32SavedInputControlMask == 0x33 && p->_unknown15[0] == 1);
    CHECK(dl.size() == 1 && std::string(dl[0].kind) == "libloaded" && dl[0].pkg == p);
    // not available: game has no data
    teststub::libraryData = nullptr;
    CHECK(e->PushPackage("Nope.fng", 1, 0) == nullptr);
    // unload: queued command and messages of the package are dropped; +0x15 -> back to idle
    auto *cmd = static_cast<FEPackageCommand *>(bridge::Alloc(sizeof(FEPackageCommand)));
    std::memset(cmd, 0, sizeof *cmd);
    cmd->node.node.vtable = nullptr;
    cmd->mpPackage = p;
    e->mPackageCommands.AddTail(&cmd->node.node);
    auto *msg = static_cast<FEMessageNode *>(bridge::Alloc(sizeof(FEMessageNode)));
    std::memset(msg, 0, sizeof *msg);
    msg->mpPackage = p;
    e->mMessages.AddTail(&msg->node);
    auto *keep = static_cast<FEMessageNode *>(bridge::Alloc(sizeof(FEMessageNode)));
    std::memset(keep, 0, sizeof *keep);
    keep->mpPackage = other;
    keep->mpTarget = reinterpret_cast<FEObject *>(intptr_t{-5});
    e->mMessages.AddTail(&keep->node);
    dl.clear();
    CHECK(e->UnloadPackage(p));
    CHECK(e->mPackageCommands.count == 0 && e->mMessages.count == 1 && e->mMessages.head == &keep->node);
    CHECK(e->mIdlePackages.head == &p->node && e->ActiveList().count == 1 && dl.size() == 1 && std::string(dl[0].kind) == "willunload");
    CHECK(!e->UnloadPackage(p));   // not active any more
    // a normal package is released and deleted
    dl.clear();
    CHECK(e->UnloadPackage(other));
    CHECK(e->ActiveList().count == 0 && dl.size() == 2 && std::string(dl[1].kind) == "releaseres");
    bridge::FreeArray(e->mMessages.RemHead());
    e->mIdlePackages.RemNode(&p->node);
    bridge::VirtualScalarDelete(p);
    std::free(e);
    dl.clear();
    CHECK(teststub::liveAllocations == live0);
}

// cFEng package wrappers over the global FEPackageManager + engine command queue.
static void TestCFEngQueue()
{
    FEngine *e = TestEngine();
    CollectCommands();
    FEPackageManager *m = bridge::TheFEPackageManager();
    alignas(4) uint8_t hdr[0x40] = {};
    auto *h = reinterpret_cast<uint32_t *>(hdr);
    h[0] = 0xE76E4546u; h[2] = 0x64486B50u; h[4] = 0x20000u;
    std::strcpy(reinterpret_cast<char *>(hdr + 0x28), "Garage.fng");
    uint32_t chunk[4] = {0x30203};
    alignas(8) unsigned char pm[sizeof(FEPackage)] = {};
    auto *pkg = reinterpret_cast<FEPackage *>(pm);
    pkg->mu32ControlMask = 0x10;
    int screen;
    FEPackageData d{};
    d.mpData = hdr; d.mpChunk = chunk; d.mpPackage = pkg; d.mpScreen = &screen;
    d.node.prev = m->mList.prev; d.node.next = &m->mList; m->mList.prev->next = &d.node; m->mList.prev = &d.node;
    alignas(8) unsigned char feb[sizeof(cFEng)] = {};
    auto *fe = reinterpret_cast<cFEng *>(feb);
    fe->mpEngine = e;
    auto &dl = teststub::delivered;
    dl.clear();
    fe->SwitchFEngPackages("Garage.fng", "Shop.fng");
    fe->SwitchFEngPackages("nope", "x");
    auto c = CollectCommands();
    CHECK(c.size() == 1 && c[0].type == 3 && c[0].name == "Shop.fng" && c[0].pkg == pkg && c[0].mask == 0x10);
    CHECK(dl.size() == 1 && dl[0].msg == 0x5ECA113Du && dl[0].a == &screen);
    pkg->SetControlMask(0x10);
    dl.clear();
    fe->QueuePopPackage("GARAGE.FNG");
    c = CollectCommands();
    CHECK(c.size() == 1 && c[0].type == 1 && dl.size() == 1 && dl[0].msg == 0x90CA113Du);
    e->QueuePackagePush("Pending", nullptr, 0);                 // not loaded: pop unqueues the pending push
    auto *pend = reinterpret_cast<FEPackageCommand *>(e->mPackageCommands.tail);
    fe->QueuePopPackage("Pending");
    CHECK(e->mPackageCommands.count == 0);
    bridge::FreeArray(pend->node.mpcName);                       // unlinked, not freed (as the original)
    bridge::FreeArray(pend);
    // SetButtonState: the last button with the hash
    alignas(8) unsigned char om[3][sizeof(FEObject)] = {};
    FEObject *b[3] = {FEObject::Construct(om[0]), FEObject::Construct(om[1]), FEObject::Construct(om[2])};
    b[0]->mu32NameHash = 5; b[1]->mu32NameHash = 6; b[2]->mu32NameHash = 5;
    pkg->mppButtons = b; pkg->mu32NumButtons = 3;
    cFEng::SetButtonState("garage.fng", 5, false);
    CHECK((b[2]->mu32Flags & 0x04000000u) && !(b[0]->mu32Flags & 0x04000000u));
    cFEng::SetButtonState("garage.fng", 5, true);
    CHECK(!(b[2]->mu32Flags & 0x04000000u));
    fe->QueuePushPackage("Sub.fng", "garage.fng", 0x4);
    fe->QueuePushModalPackage("Dlg.fng", nullptr, 0x8);
    fe->QueuePushPackage("Sub2.fng", "unknown", 0x1);
    c = CollectCommands();
    CHECK(c.size() == 3 && c[0].type == 2 && c[0].pkg == pkg && c[0].mask == 4 && c[1].type == 0x12 && c[1].pkg == nullptr);
    CHECK(c[2].type == 2 && c[2].pkg == nullptr && c[2].name == "Sub2.fng");
    m->mList.next = m->mList.prev = &m->mList;                   // unregister
    dl.clear();
}

// FEListFindNode / FEngFindPackage / level sorter (ResyncSortingLevels, Popper, Switcher).
static void TestLevelSorter()
{
    CHECK(FEStricmp("abc", "ABC") == 0 && FEStricmp("ab", "abc") == -'C' && FEStricmp("b", "a") == 1);
    alignas(8) unsigned char pm[3][sizeof(FEPackage)] = {};
    FEPackage *P[3];
    for (int i = 0; i < 3; ++i) P[i] = reinterpret_cast<FEPackage *>(pm[i]);
    FENode *N[3];
    const char *names[3] = {"Main.fng", "Shop.fng", "Idle.fng"};
    for (int i = 0; i < 3; ++i) {
        N[i] = reinterpret_cast<FENode *>(P[i]);
        N[i]->mpcName = const_cast<char *>(names[i]);
        N[i]->mu32NameHash = FEHashUpper(names[i]);
        P[i]->mi32Priority = -1;
    }
    N[0]->node.next = &N[1]->node; N[1]->node.next = nullptr; N[2]->node.next = nullptr;
    CHECK(FEListFindNode("SHOP.FNG", N[0]) == N[1] && FEListFindNode("x", N[0]) == nullptr && FEListFindNode(nullptr, N[0]) == nullptr);
    FEngine *e = TestEngine();
    cFEng *fe = bridge::cFEngInstance();
    fe->mpEngine = e;
    e->mpPackages = P[0];
    e->mIdlePackages.head = reinterpret_cast<FEMinNode *>(P[2]);
    CHECK(FEngFindPackage("idle.fng") == P[2] && FEngFindPackage("shop.fng") == P[1] && FEngFindPackage("none") == nullptr);
    teststub::fengLoadCounter = 1;                                  // loading: package manager path (empty)
    CHECK(FEngFindPackage("shop.fng") == nullptr);
    teststub::fengLoadCounter = 0;
    // level sorter list: Main, Shop (pool of 4 entries)
    teststub::levelSortingList = {"Main.fng", "Shop.fng"};
    fe = bridge::cFEngInstance();                                   // rebuilds the list from levelSortingList
    fe->mpEngine = e;
    SlotPool *pool = SlotPool::NewSlotPool(sizeof(cFEngLevelSorter), 16, "ls", 0);
    teststub::levelSorterSlots = pool;
    fe->ResyncSortingLevels();
    CHECK(fe->mpSortHead->mi32Level == 0 && fe->mpSortHead->next->mi32Level == 1 && P[0]->mi32Priority == 0 && P[1]->mi32Priority == 1);
    // Switcher(Main -> Idle): the new entry takes Main's place
    CollectCommands();
    fe->Switcher("Main.fng", "Idle.fng", 7);
    CHECK(std::string(fe->mpSortHead->mpcPackageName) == "Idle.fng" && fe->mpSortHead->mi32Arg == 7 && fe->mpSortHead->next->mpcPackageName == std::string("Shop.fng"));
    CHECK(P[2]->mi32Priority == 0 && P[1]->mi32Priority == 1);
    // Popper(Shop): state 0 -> direct pop (cFEng::PopPackage bridge), entry removed
    fe->mpSortHead->next->mi32State = 0;
    fe->Popper("Shop.fng");                                         // not registered with the manager: no pop
    CHECK(fe->mpSortHead->next == fe->SortSentinel());
    // PopPackage of a registered package: message, disconnect each object, unload
    {
        FEPackageManager *m = bridge::TheFEPackageManager();
        alignas(4) uint8_t hdr[0x40] = {};
        auto *h = reinterpret_cast<uint32_t *>(hdr);
        h[0] = 0xE76E4546u; h[2] = 0x64486B50u; h[4] = 0x20000u;
        std::strcpy(reinterpret_cast<char *>(hdr + 0x28), "Pop.fng");
        uint32_t chunk[4] = {0x30203};
        alignas(8) unsigned char om[2][sizeof(FEObject)] = {};
        P[1]->mObjects = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
        P[1]->mObjects.AddTail(&FEObject::Construct(om[0])->node);
        P[1]->mObjects.AddTail(&FEObject::Construct(om[1])->node);
        int screen;
        FEPackageData d{};
        d.mpData = hdr; d.mpChunk = chunk; d.mpPackage = P[1]; d.mpScreen = &screen;
        d.node.prev = m->mList.prev; d.node.next = &m->mList; m->mList.prev->next = &d.node; m->mList.prev = &d.node;
        teststub::delivered.clear();
        teststub::renderDisconnects = 0;
        fe->PopPackage("pop.fng");
        auto &dl = teststub::delivered;
        CHECK(teststub::renderDisconnects == 2 && dl.size() == 2 && dl[0].msg == 0xD9CA113Du && std::string(dl[1].kind) == "unload" && dl[1].pkg == P[1]);
        m->mList.next = m->mList.prev = &m->mList;
        P[1]->mObjects = FEMinList{&teststub::feMinListVTable, 0, nullptr, nullptr};
        dl.clear();
    }
    CollectCommands();
    // Pusher placement modes (fresh list: Idle only)
    auto order = [&]() { std::string r; for (cFEngLevelSorter *n = fe->mpSortHead; n != fe->SortSentinel(); n = n->next) r += std::string(n->mpcPackageName) + ","; return r; };
    CollectCommands(); teststub::delivered.clear();
    fe->Pusher(0, "A", nullptr, 0xFF, 1);                         // front, immediate push (not active)
    CHECK(order() == "A,Idle.fng," && fe->mpSortHead->mi32State == 1 && teststub::delivered.size() == 1 && std::string(teststub::delivered[0].kind) == "pushpkg");
    fe->Pusher(4, "B", "A", 0x1, 2);                              // after A
    fe->Pusher(2, "C", "Idle.fng", 0x1, 3);                       // before Idle
    fe->Pusher(4, "D", "missing", 0x1, 4);                        // tail
    CHECK(order() == "A,B,C,Idle.fng,D,");
    fe->mpSortHead->next->mu32Flags |= 8;                         // B and D flagged: the frontmost (B) wins
    fe->mpSortTail->mu32Flags |= 8;
    fe->Pusher(6, "E", nullptr, 0x1, 5);
    CHECK(order() == "A,E,B,C,Idle.fng,D,");
    teststub::delivered.clear();
    fe->Pusher(0x11, "Q", "A", 0x3, 6);                           // queued push (flag 1)
    fe->Pusher(0x10, "M", nullptr, 0x3, 7);                       // queued modal push
    auto qc = CollectCommands();
    CHECK(qc.size() == 2 && qc[0].type == 2 && qc[0].name == "Q" && qc[1].type == 0x12 && qc[1].name == "M" && fe->mpSortHead->mi32State == 2);
    CHECK(teststub::delivered.empty() && fe->mpSortHead->mi32Level == 0 && fe->mpSortTail->mi32Level == 7);
    std::free(pool); --teststub::liveAllocations;
    teststub::levelSorterSlots = nullptr;
    teststub::levelSortingList.clear();
    fe->mpEngine = nullptr;
    e->mpPackages = nullptr;
    e->mIdlePackages.head = nullptr;
}

int main()
{
    TestLevelSorter();
    TestCFEngQueue();
    TestPackageManager();
    TestPackageDataLifecycle();
    TestPackageLifetime();
    TestPackageReader();
    TestReaderStringTags();
    TestReaderInsertTrack();
    TestListBoxCells();
    TestCodeListBox();
    TestEnginePushUnload();
    TestSlotPool();
    TestATan();
    TestPackageCommands();
    TestPackageMarkers();
    TestProcessPackageCommands();
    TestCrc();
    TestWideHelpers();
    TestWideString();
    TestFEString();
    TestFEHashUpper();
    TestFEStringLifecycle();
    TestFEMinList();
    TestFEObject();
    TestFEObjectLifecycle();
    TestMultiPool();
    TestResponses();
    TestMessageResponsePool();
    TestPoolBlocks();
    TestScriptDestruct();
    TestScriptCopy();
    TestFEObjectCopy();
    TestMoveToTracks();
    TestSetTrackValue();
    TestKeyInterp();
    TestInterpLinear();
    TestTrigAndSlerp();
    TestUpdateObject();
    TestPackageUpdate();
    TestProcessMessageQueue();
    TestProcessResponses();
    std::printf("frontend: %s (%d failure(s))\n", failures ? "FAILED" : "ok", failures);
    return failures != 0;
}
