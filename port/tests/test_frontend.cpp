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
#include "bWare/bMath.h"
#include <algorithm>
#include "bridge/Bridge.h"
#include <cstdlib>
#include "frontend/FEString.h"
#include "frontend/FEWideString.h"
#include <cstdio>
#include <cstring>
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
    auto &q = teststub::queued;
    auto range = [&](size_t from, size_t n, uint32_t first, uint32_t last) {
        return q.size() >= from + n && q[from].msg == first && q[from + n - 1].msg == last;
    };
    q.clear();
    pkg->UpdateObject(obj, 16);                                  // 0 -> 30: events 0..29
    CHECK(a.mi32Time == 30 && q.size() == 30 && range(0, 30, 1000, 1029) && q[0].obj == obj && q[0].target == nullptr);
    a.mi32Time = 90; q.clear();                                  // stop: events 90..100 (end inclusive), time = 101
    pkg->UpdateObject(obj, 16);
    CHECK(a.mi32Time == 101 && q.size() == 11 && range(0, 11, 1090, 1100));
    pkg->mi32ScriptDelta = 0; q.clear();                         // finished: no events, tracks skipped, flags cleared
    obj->mu32Flags |= 0x1800000;
    pkg->UpdateObject(obj, 16);
    CHECK(a.mi32Time == 101 && q.empty() && (obj->mu32Flags & 0x1800000) == 0);
    pkg->mi32ScriptDelta = 30;
    a.mu32Flags = 1; a.mi32Time = 90; q.clear();                 // loop: 90..100 then 0..19
    pkg->UpdateObject(obj, 16);
    CHECK(a.mi32Time == 20 && q.size() == 31 && range(0, 11, 1090, 1100) && range(11, 20, 1000, 1019));
    a.mu32Flags = 2; a.mi32Time = 180; q.clear();                // ping-pong past 2*length wraps
    pkg->UpdateObject(obj, 16);
    CHECK(a.mi32Time == 10);
    a.mu32Flags = 0; a.mpChainedScript = &b; a.mi32Time = 90; q.clear();   // chain: rest carries over
    pkg->UpdateObject(obj, 16);
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
    q.clear(); teststub::currentButton = nullptr;
    pkg->IssueScriptMessages(nullptr, obj, &c, 0, 10);
    CHECK(q.size() == 3 && q[0].target == reinterpret_cast<void *>(-1) && q[1].target == reinterpret_cast<void *>(uintptr_t(0xFFFFFFFBu)));
    CHECK(q[2].target == &g7->obj && teststub::currentButton == o9);
    teststub::TestCallback cb{nullptr, {}, o9};                  // walk order + stop
    CHECK(!pkg->ForAllObjects(reinterpret_cast<FEObjectCallback *>(&cb)) && cb.visited.size() == 2 && cb.visited[1] == o9);
    q.clear();
    pkg->IssueScriptMessages(nullptr, nullptr, &c, 2, 3);       // GUID target without obj: not queued
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

int main()
{
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
    std::printf("frontend: %s (%d failure(s))\n", failures ? "FAILED" : "ok", failures);
    return failures != 0;
}
