#include "FinishedRaceStats.h"
#include <cstring>
#include "bridge/Bridge.h"
#include "bWare/bList.h"
#include "bWare/bSlotPool.h"

namespace {
struct PNode { bNode n; uint8_t *data; };   // bPNode (12 bytes on PC)
template <typename T> T Rd(const uint8_t *p, size_t off) { T v; std::memcpy(&v, p + off, sizeof v); return v; }
uint8_t *Data(bNode *node) { return reinterpret_cast<PNode *>(node)->data; }
int32_t NumDrivers()   // RaceParameters: 4 counts summed (PC 0x89E7C0..0x89E7CC)
{
    return *bridge::GameInt(0x89E7CC) + *bridge::GameInt(0x89E7C8) + *bridge::GameInt(0x89E7C4) + *bridge::GameInt(0x89E7C0);
}
bool DriftRace() { return *bridge::GameByte(0x89E7D9) != 0 || *bridge::GameInt(0x890104) != 0; }
int32_t &CurLap() { return *bridge::GameInt(0x83846C); }
uint8_t *Entry(int32_t i) { return bridge::FinishedRaceStatsGlobal() + i * 0x224; }
using L = long double;
int32_t Result(int32_t ra, int32_t rb) { return static_cast<uint32_t>(rb) < static_cast<uint32_t>(ra) ? 0 : 1; }
}  // namespace

int32_t SortRacingCarByTime(bNode *pa, bNode *pb)
{
    const uint8_t *a = Data(pa), *b = Data(pb);
    const int32_t lap = CurLap();
    int32_t ra = static_cast<L>(Rd<int32_t>(a, 0x1C + lap * 4)) * static_cast<L>(0.00025f) == 0.0L ? 2 : 0;
    int32_t rb = static_cast<L>(Rd<int32_t>(b, 0x1C + lap * 4)) * static_cast<L>(0.00025f) == 0.0L ? 2 : 0;
    if (Rd<int32_t>(a, 0x70 + lap * 4) > Rd<int32_t>(b, 0x70 + lap * 4)) ++ra;
    else ++rb;
    return Result(ra, rb);
}

int32_t SortRacingCarByDrift(bNode *pa, bNode *pb)
{
    const uint8_t *a = Data(pa), *b = Data(pb);
    const int32_t lap = CurLap();
    int32_t ra = Rd<int32_t>(a, 0x144 + lap * 4) == 0 ? 2 : 0;
    int32_t rb = static_cast<L>(Rd<int32_t>(b, 0x144 + lap * 4)) == 0.0L ? 2 : 0;
    if (Rd<int32_t>(a, 0x198 + lap * 4) < Rd<int32_t>(b, 0x198 + lap * 4)) ++ra;
    else ++rb;
    return Result(ra, rb);
}

int32_t SortResultsByDriftPoints(bNode *pa, bNode *pb)
{
    const int32_t laps = *bridge::GameInt(0x89E7BC);
    return Rd<int32_t>(Data(pb), 0x198 + laps * 4) <= Rd<int32_t>(Data(pa), 0x198 + laps * 4) ? 1 : 0;
}

uint8_t *cFinishedRaceStats::GetStatsForDriver(int32_t driver)
{
    for (int32_t i = 0; i < mCount; ++i)
        if (Rd<int32_t>(mEntries[i], 4) == driver) return mEntries[i];
    return nullptr;
}

uint8_t *cFinishedRaceStats::GetFinisher(int32_t place) const { return place - 1 < mCount ? mFinishers[place - 1] : nullptr; }

void cFinishedRaceStats::CalculatePositionsForEachLap()
{
    bList list;
    list.mHead.next = list.mHead.prev = &list.mHead;
    for (int32_t i = 0; i < NumDrivers(); ++i) {
        uint8_t *e = Entry(i);
        std::memset(e + 0xF4, 0, 4);
        auto *n = static_cast<PNode *>(bPNode_Malloc());
        n->data = e;
        bNode *t = list.mHead.prev;
        t->next = &n->n; n->n.prev = t; n->n.next = &list.mHead; list.mHead.prev = &n->n;
    }
    auto pass = [&](bListCompare cmp, size_t posOff) {
        for (CurLap() = 1;; ++CurLap()) {
            const int32_t laps = *bridge::GameInt(0x89E7BC);
            if (CurLap() > (laps < 0x15 ? laps : 0x15)) break;
            list.Sort(cmp);
            int32_t pos = 1;
            for (bNode *n = list.mHead.next; n != &list.mHead; n = n->next, ++pos) {
                uint8_t *e = Data(n);
                std::memcpy(e + posOff + CurLap() * 4, &pos, 4);
                if (pos == 1) { int32_t led = Rd<int32_t>(e, 0xF4) + 1; std::memcpy(e + 0xF4, &led, 4); }
            }
        }
    };
    pass(&SortRacingCarByTime, 0xC8);
    if (DriftRace()) {
        for (int32_t i = 0; i < NumDrivers(); ++i) std::memset(Entry(i) + 0xF4, 0, 4);
        pass(&SortRacingCarByDrift, 0x1E8);
    }
    while (list.mHead.next != &list.mHead) {
        bNode *n = list.mHead.next;
        n->prev->next = n->next;
        n->next->prev = n->prev;
        bPNode_Free(n);
    }
}

void cFinishedRaceStats::Construct(void *race)
{
    mCount = 0;
    const int32_t n = bridge::RaceNumRacers(race);
    for (int32_t i = 0; i < n; ++i) {
        bridge::FinishedRaceStatsEntry_Construct(mEntries[i], bridge::RaceRacer(race, i), false);
        ++mCount;
    }
    CalculatePositionsForEachLap();
    bList list;
    list.mHead.next = list.mHead.prev = &list.mHead;
    for (int32_t i = 0; i < bridge::RaceNumRacers(bridge::CurrentRace()); ++i) {
        auto *node = static_cast<PNode *>(bridge::Alloc(sizeof(PNode)));   // operator new(0xC)
        node->data = Entry(i);
        bNode *t = list.mHead.prev;
        t->next = &node->n; node->n.prev = t; node->n.next = &list.mHead; list.mHead.prev = &node->n;
        if (DriftRace()) list.Sort(&SortResultsByDriftPoints);
    }
    for (int32_t i = 0; i < NumDrivers(); ++i) {
        bNode *nd = list.mHead.next;
        for (int32_t k = 0; nd != &list.mHead && k != i; ++k) nd = nd->next;   // bList::GetNode (PC 43BE60)
        mFinishers[i] = Data(nd);
    }
    while (list.mHead.next != &list.mHead) {
        bNode *nd = list.mHead.next;
        nd->prev->next = nd->next;
        nd->next->prev = nd->prev;
        bridge::FreeArray(nd);
    }
}
