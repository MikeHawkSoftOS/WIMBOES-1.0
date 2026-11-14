/* Minimal freestanding libc - kernel build only (host uses the real one). */
#include "os.h"
void *memcpy(void *d, const void *s, size_t n) {
    void *r = d; size_t q = n >> 2;
    __asm__ volatile("rep movsl" : "+D"(d), "+S"(s), "+c"(q) : : "memory");
    n &= 3; __asm__ volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(n) : : "memory");
    return r;
}
void *memmove(void *d, const void *s, size_t n) {
    u8 *dd = d; const u8 *ss = s;
    if (dd == ss || !n) return d;
    if (dd < ss) { while (n--) *dd++ = *ss++; }
    else { dd += n; ss += n; while (n--) *--dd = *--ss; }
    return d;
}
void *memset(void *d, int c, size_t n) {
    void *r = d; u32 v = (u8)c; v |= v << 8; v |= v << 16; size_t q = n >> 2;
    __asm__ volatile("rep stosl" : "+D"(d), "+c"(q) : "a"(v) : "memory");
    n &= 3; __asm__ volatile("rep stosb" : "+D"(d), "+c"(n) : "a"(v) : "memory");
    return r;
}
int memcmp(const void *a, const void *b, size_t n) {
    const u8 *x = a, *y = b;
    for (; n; n--, x++, y++) if (*x != *y) return *x - *y;
    return 0;
}
size_t strlen(const char *s) { size_t n = 0; while (s[n]) n++; return n; }
int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (u8)*a - (u8)*b;
}
int strncmp(const char *a, const char *b, size_t n) {
    for (; n; n--, a++, b++) { if (*a != *b) return (u8)*a - (u8)*b; if (!*a) return 0; }
    return 0;
}
