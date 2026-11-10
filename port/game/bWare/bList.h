// bWare doubly linked list (GC bList / bNode, zBWare.cpp): a sentinel {next, prev}; nodes start with the same links.
// Comparators are __cdecl int(bNode *a, bNode *b): nonzero = a goes first (AddSorted inserts before the first node
// with cmp(new, node) == 0; MergeSort takes a when cmp(a, b) > 0).
#pragma once
#include <cstdint>

struct bNode {
    bNode *next, *prev;
};
using bListCompare = int32_t (*)(bNode *, bNode *);

struct bList {
    bNode mHead;                     // sentinel (an empty list points at itself)

    bNode *Sentinel() { return &mHead; }
    // GC 800589AC / PC 43BE80 (ret 4): 1-based position of `node`; not found: count if node is null, else 0.
    int32_t TraversebList(const bNode *node);
    // GC 800589F0 / PC 43BEB0 (ret 4): move every node of `other` to the tail; `other` becomes empty.
    void AddTail(bList *other);
    // GC 80058A20 / PC 43BEE0 (ret 8): insert before the first node with cmp(node, it) == 0, else at the tail.
    bNode *AddSorted(bListCompare cmp, bNode *node);
    // GC 80058AB4 / PC 43E650 (ret 4): one pass moving each node that should precede its predecessor (cmp(prev,
    // node) == 0) in front of it; MergeSort if anything moved.
    void Sort(bListCompare cmp);
    void MergeSort(bListCompare cmp);   // GC 80058B64 / PC 43BF40 (ret 4): bottom-up merge sort of the ring
};
