#include "bList.h"

int32_t bList::TraversebList(const bNode *node)
{
    int32_t i = 0;
    for (bNode *n = mHead.next; n != &mHead; n = n->next) {
        ++i;
        if (n == node) return i;
    }
    return node ? 0 : i;
}

void bList::AddTail(bList *other)
{
    bNode *first = other->mHead.next, *last = other->mHead.prev;
    if (first == &other->mHead) return;
    bNode *tail = mHead.prev;
    last->next = &mHead;
    first->prev = tail;
    tail->next = first;
    mHead.prev = last;
    other->mHead.next = other->mHead.prev = &other->mHead;
}

bNode *bList::AddSorted(bListCompare cmp, bNode *node)
{
    bNode *at = mHead.next;
    while (at != &mHead && cmp(node, at) != 0) at = at->next;
    bNode *prev = at->prev;   // at the end: at == sentinel -> tail insert
    prev->next = node;
    at->prev = node;
    node->prev = prev;
    node->next = at;
    return node;
}

void bList::Sort(bListCompare cmp)
{
    int32_t moved = 0;
    bNode *a = mHead.next;
    if (a == &mHead) return;
    bNode *b = a->next;
    while (b != &mHead) {
        if (cmp(a, b) == 0) {   // b goes in front of a
            b->prev->next = b->next;
            b->next->prev = b->prev;
            bNode *p = a->prev;
            p->next = b;
            a->prev = b;
            b->prev = p;
            b->next = a;
            b = a->next;
            ++moved;
        } else {
            a = b;
            b = b->next;
        }
        if (a == &mHead) break;
    }
    if (moved) MergeSort(cmp);
}

void bList::MergeSort(bListCompare cmp)
{
    bNode *list = mHead.next;
    if (list == &mHead) return;
    list->prev = mHead.prev;   // ring without the sentinel
    mHead.prev->next = list;
    int32_t insize = 1;
    bNode *tail;
    for (;;) {
        bNode *const start = list;
        bNode *p = start;
        list = nullptr;
        tail = nullptr;
        int32_t nmerges = 0;
        while (p) {
            ++nmerges;
            bNode *q = p;
            int32_t psize = 0;
            for (int32_t i = 0; i < insize;) {
                q = q->next;
                ++psize;
                if (q == start) { q = nullptr; break; }
                if (!q) break;
                ++i;
            }
            int32_t qsize = insize;
            for (;;) {
                bNode *e;
                if (psize > 0) {
                    if (qsize != 0 && q && !(cmp(p, q) > 0)) {
                        e = q; q = q->next; --qsize;
                        if (q == start) q = nullptr;
                    } else {
                        e = p; p = p->next; --psize;
                        if (p == start) p = nullptr;
                    }
                } else {
                    if (qsize <= 0 || !q) break;
                    e = q; q = q->next; --qsize;
                    if (q == start) q = nullptr;
                }
                if (tail) {
                    tail->next = e;
                    e->prev = tail;
                } else {
                    e->prev = tail;
                    list = e;
                }
                tail = e;
            }
            p = q;
        }
        tail->next = list;
        list->prev = tail;
        if (nmerges <= 1) break;
        insize *= 2;
    }
    list->prev = &mHead;
    mHead.next = list;
    mHead.prev = tail;
    tail->next = &mHead;
}
