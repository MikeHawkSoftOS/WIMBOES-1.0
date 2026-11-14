/* Hardware independent helpers shared by kernel and host test build. */
#include "os.h"

void kstrcpy(char *dst, const char *src, int cap) {
    int i = 0;
    if (cap <= 0) return;
    for (; i < cap - 1 && src[i]; i++) dst[i] = src[i];
    dst[i] = 0;
}
static int lc(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
int kstricmp(const char *a, const char *b) {
    while (*a && lc((u8)*a) == lc((u8)*b)) { a++; b++; }
    return lc((u8)*a) - lc((u8)*b);
}
bool kcontains_ci(const char *hay, const char *needle) {
    int n = (int)strlen(needle), h = (int)strlen(hay);
    if (!n) return true;
    for (int i = 0; i + n <= h; i++) {
        int j = 0;
        while (j < n && lc((u8)hay[i + j]) == lc((u8)needle[j])) j++;
        if (j == n) return true;
    }
    return false;
}
int katoi(const char *s) {
    int v = 0, neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return neg ? -v : v;
}

typedef struct { char *p; int n, cap; } Out;
static void put(Out *o, char c) { if (o->n < o->cap - 1) o->p[o->n] = c; o->n++; }
static void putnum(Out *o, u32 v, int base, bool neg, int width, bool zero, bool left) {
    char tmp[12]; int n = 0;
    if (!v) tmp[n++] = '0';
    while (v) { tmp[n++] = "0123456789abcdef"[v % base]; v /= base; }
    int len = n + (neg ? 1 : 0);
    int pad = width > len ? width - len : 0;
    if (!left && !zero) while (pad--) put(o, ' ');
    if (neg) put(o, '-');
    if (!left && zero) while (pad--) put(o, '0');
    while (n) put(o, tmp[--n]);
    if (left) while (pad--) put(o, ' ');
}
int ksnprintf(char *buf, int cap, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    Out o = { buf, 0, cap };
    for (; *fmt; fmt++) {
        if (*fmt != '%') { put(&o, *fmt); continue; }
        fmt++;
        bool zero = false, left = false; int width = 0;
        if (*fmt == '-') { left = true; fmt++; }
        if (*fmt == '0') { zero = true; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        switch (*fmt) {
        case 'd': { int v = va_arg(ap, int); putnum(&o, v < 0 ? (u32)(-(v + 1)) + 1u : (u32)v, 10, v < 0, width, zero, left); break; }
        case 'u': putnum(&o, va_arg(ap, u32), 10, false, width, zero, left); break;
        case 'x': putnum(&o, va_arg(ap, u32), 16, false, width, zero, left); break;
        case 'c': put(&o, (char)va_arg(ap, int)); break;
        case 's': { const char *s = va_arg(ap, const char *); if (!s) s = "(null)";
            int len = (int)strlen(s), pad = width > len ? width - len : 0;
            if (!left) while (pad--) put(&o, ' ');
            while (*s) put(&o, *s++);
            if (left) while (pad--) put(&o, ' ');
            break; }
        case '%': put(&o, '%'); break;
        case 0: fmt--; break;
        default: put(&o, *fmt);
        }
    }
    va_end(ap);
    if (cap > 0) buf[o.n < cap ? o.n : cap - 1] = 0;
    return o.n;
}
