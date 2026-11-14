/* First-fit allocator with coalescing; used for file data and app buffers. */
#include "os.h"
typedef struct Blk { size_t size; struct Blk *next; int free; } __attribute__((aligned(16))) Blk;
static Blk *head; static size_t total, used;

void mem_init(void *base, size_t size) {
    head = base; head->size = size - sizeof(Blk); head->next = 0; head->free = 1;
    total = size; used = 0;
}
void *kmalloc(size_t n) {
    n = (n + 15) & ~(size_t)15;
    if (!n) n = 16;
    for (Blk *b = head; b; b = b->next) {
        if (!b->free || b->size < n) continue;
        if (b->size >= n + sizeof(Blk) + 16) {
            Blk *r = (Blk *)((char *)(b + 1) + n);
            r->size = b->size - n - sizeof(Blk); r->next = b->next; r->free = 1;
            b->next = r; b->size = n;
        }
        b->free = 0; used += b->size;
        memset(b + 1, 0, b->size);
        return b + 1;
    }
    return 0;
}
void kfree(void *p) {
    if (!p) return;
    Blk *b = (Blk *)p - 1;
    if (b->free) return;
    b->free = 1; used -= b->size;
    for (Blk *c = head; c; c = c->next)
        while (c->free && c->next && c->next->free) { c->size += sizeof(Blk) + c->next->size; c->next = c->next->next; }
}
size_t mem_used(void) { return used; }
size_t mem_total(void) { return total; }
