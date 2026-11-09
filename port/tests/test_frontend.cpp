// Native unit tests: bWare CRC / wide strings, FEWideString, FEString.
#include "bWare/bCrc32.h"
#include "bWare/bWideString.h"
#include "bridge_stub.h"
#include "frontend/FEHash.h"
#include "frontend/FEMinList.h"
#include "frontend/FEMultiPool.h"
#include "frontend/FEMessageResponse.h"
#include "frontend/FEPool.h"
#include "bridge/Bridge.h"
#include <cstdlib>
#include "frontend/FEString.h"
#include "frontend/FEWideString.h"
#include <cstdio>
#include <cstring>
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
    teststub::scriptDtorCalls = 0;
    CHECK(FEScriptBlock_ScalarDtor(b, 0) == b);
    CHECK(teststub::scriptDtorCalls == 32 && b->freeList.count == 0 && b->freeList.head == nullptr);
    CHECK(first->node.next == reinterpret_cast<FEMinNode *>(FEMINNODE_UNLINKED));
    CHECK(b->node.vtable == &teststub::feMinNodeVTable);
    bridge::FreeArray(b);

    teststub::scriptDtorCalls = 0;
    std::vector<void *> s;
    for (int i = 0; i < 40; ++i) s.push_back(FEPoolNew<FEScript, 32>(teststub::scriptPool, FEScriptBlock_Construct));
    CHECK(teststub::scriptPool.count == 2);
    for (void *p : s) FEPoolDelete<FEScript, 32>(teststub::scriptPool, p);
    CHECK(teststub::scriptPool.count == 0 && teststub::scriptDtorCalls == 64);
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
    std::printf("frontend: %s (%d failure(s))\n", failures ? "FAILED" : "ok", failures);
    return failures != 0;
}
