/* Real hardware layer: VBE framebuffer, PIT+IDT, PS/2 keyboard & mouse, CMOS RTC, ACPI poweroff. */
#include "os.h"
static inline void outb(u16 p, u8 v) { __asm__ volatile("outb %0,%1" : : "a"(v), "Nd"(p)); }
static inline void outw(u16 p, u16 v) { __asm__ volatile("outw %0,%1" : : "a"(v), "Nd"(p)); }
static inline u8 inb(u16 p) { u8 v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(p)); return v; }

static volatile u32 g_ms;
static u8 *fb; static int pitch, W, H; static u32 *bbuf;
void timer_tick(void) { g_ms += 4; }
void exc_panic(void) {
    for (int i = 0; fb && i < H; i++) { u32 *r = (u32 *)(fb + i * pitch); for (int x = 0; x < W; x++) r[x] = 0xAA0000; }
    for (;;) __asm__ volatile("hlt");
}
extern void isr_timer(void), isr_exc(void);
struct __attribute__((packed)) IdtE { u16 lo, sel; u8 zero, flags; u16 hi; };
static struct IdtE idt[256];
static struct __attribute__((packed)) { u16 lim; u32 base; } idtr;
static void idt_set(int n, void (*h)(void)) { u32 a = (u32)(uintptr_t)h; idt[n] = (struct IdtE){ a & 0xFFFF, 0x08, 0, 0x8E, a >> 16 }; }

/* ---- keyboard ---- */
static const char kn[] = "\0\033" "1234567890-=" "\b\t" "qwertyuiop[]" "\n\0" "asdfghjkl;'`" "\0\\" "zxcvbnm,./" "\0*\0 ";
static const char ks[] = "\0\033" "!@#$%^&*()_+" "\b\t" "QWERTYUIOP{}" "\n\0" "ASDFGHJKL:\"~" "\0|" "ZXCVBNM<>?" "\0*\0 ";
static int mods, caps, ext;
static Event q[64]; static int qh, qt;
static void push(Event e) { int n = (qt + 1) & 63; if (n != qh) { q[qt] = e; qt = n; } }
static void pushkey(int k) { push((Event){ EV_KEY, 0, 0, 0, k, mods }); }
static void kbd_byte(u8 sc) {
    if (sc == 0xE0) { ext = 1; return; }
    bool rel = sc & 0x80; sc &= 0x7F;
    if (sc == 0x2A || sc == 0x36) { mods = rel ? mods & ~MOD_SHIFT : mods | MOD_SHIFT; ext = 0; return; }
    if (sc == 0x1D) { mods = rel ? mods & ~MOD_CTRL : mods | MOD_CTRL; ext = 0; return; }
    if (sc == 0x38) { mods = rel ? mods & ~MOD_ALT : mods | MOD_ALT; ext = 0; return; }
    if (rel) { ext = 0; return; }
    if (sc == 0x3A) { caps ^= 1; return; }
    int k = 0;
    if (ext) {
        switch (sc) { case 0x48: k = K_UP; break; case 0x50: k = K_DOWN; break; case 0x4B: k = K_LEFT; break; case 0x4D: k = K_RIGHT; break;
        case 0x47: k = K_HOME; break; case 0x4F: k = K_END; break; case 0x49: k = K_PGUP; break; case 0x51: k = K_PGDN; break;
        case 0x53: k = K_DEL; break; case 0x5B: case 0x5C: k = K_WIN; break; case 0x1C: k = K_ENTER; break; case 0x35: k = '/'; break; }
        ext = 0;
    } else if (sc == 0x3E) k = K_F4;
    else if (sc == 0x3F) k = K_F5;
    else if (sc < sizeof kn - 1) {
        bool sh = mods & MOD_SHIFT; char c = sh ? ks[sc] : kn[sc];
        if (caps && kn[sc] >= 'a' && kn[sc] <= 'z') c = sh ? kn[sc] : ks[sc] ;
        if ((mods & MOD_CTRL) && c >= 'A' && c <= 'Z') c += 32;
        k = (u8)c;
    }
    if (k) pushkey(k);
}
/* ---- mouse ---- */
static int mpk[4], mi, mouse_len = 3, mx, my, mbtn;
static void wait_in(void) { for (int i = 0; i < 100000 && (inb(0x64) & 2); i++); }
static int wait_out(void) { for (int i = 0; i < 100000; i++) if (inb(0x64) & 1) return 1; return 0; }
static void mouse_cmd(u8 v) { wait_in(); outb(0x64, 0xD4); wait_in(); outb(0x60, v); if (wait_out()) inb(0x60); }
static void mouse_init(void) {
    wait_in(); outb(0x64, 0xA8);
    wait_in(); outb(0x64, 0x20); wait_out(); u8 st = inb(0x60);
    st |= 2; st &= ~0x20;
    wait_in(); outb(0x64, 0x60); wait_in(); outb(0x60, st);
    mouse_cmd(0xF6);
    mouse_cmd(0xF3); mouse_cmd(200); mouse_cmd(0xF3); mouse_cmd(100); mouse_cmd(0xF3); mouse_cmd(80);   /* IntelliMouse wheel */
    wait_in(); outb(0x64, 0xD4); wait_in(); outb(0x60, 0xF2); wait_out(); inb(0x60);
    if (wait_out() && inb(0x60) == 3) mouse_len = 4;
    mouse_cmd(0xF4);
    mx = W / 2; my = H / 2;
}
static void mouse_byte(u8 b) {
    if (mi == 0 && !(b & 8)) return;
    mpk[mi++] = b;
    if (mi < mouse_len) return;
    mi = 0;
    int dx = mpk[1] - ((mpk[0] << 4) & 0x100), dy = mpk[2] - ((mpk[0] << 3) & 0x100);
    mx = CLAMP(mx + dx, 0, W - 1); my = CLAMP(my - dy, 0, H - 1);
    push((Event){ EV_MOVE, mx, my, 0, 0, mods });
    int nb = mpk[0] & 7;
    for (int i = 0; i < 3; i++) {
        int m = 1 << i;
        if ((nb & m) != (mbtn & m)) push((Event){ (nb & m) ? EV_DOWN : EV_UP, mx, my, m, 0, mods });
    }
    mbtn = nb;
    if (mouse_len == 4) { int z = mpk[3] & 0xF; if (z) { if (z & 8) z -= 16; push((Event){ EV_WHEEL, mx, my, 0, -z, mods }); } }
}
bool hal_next_event(Event *e) {
    for (int n = 0; n < 64 && (inb(0x64) & 1); n++) { u8 st = inb(0x64), b = inb(0x60); if (st & 0x20) mouse_byte(b); else kbd_byte(b); }
    if (qh == qt) return false;
    *e = q[qh]; qh = (qh + 1) & 63;
    return true;
}
void hal_init(int *w, int *h, u32 **bb) {
    u16 *mi16 = (u16 *)0x9000; (void)mi16;
    pitch = *(volatile u16 *)0x9010; W = *(volatile u16 *)0x9012; H = *(volatile u16 *)0x9014;
    fb = (u8 *)(uintptr_t) * (volatile u32 *)0x9028;
    bbuf = (u32 *)0x400000;
    *w = W; *h = H; *bb = bbuf;
    for (int i = 0; i < 32; i++) idt_set(i, isr_exc);
    idt_set(0x20, isr_timer);
    idtr.lim = sizeof idt - 1; idtr.base = (u32)(uintptr_t)idt;
    __asm__ volatile("lidt %0" : : "m"(idtr));
    outb(0x20, 0x11); outb(0xA0, 0x11); outb(0x21, 0x20); outb(0xA1, 0x28); outb(0x21, 4); outb(0xA1, 2); outb(0x21, 1); outb(0xA1, 1);
    outb(0x21, 0xFE); outb(0xA1, 0xFF);
    outb(0x43, 0x34); outb(0x40, 4773 & 0xFF); outb(0x40, 4773 >> 8);       /* 250 Hz */
    while (inb(0x64) & 1) inb(0x60);
    mouse_init();
    __asm__ volatile("sti");
}
u32 hal_ms(void) { return g_ms; }
void hal_idle(void) { __asm__ volatile("hlt"); }
void hal_present(const u32 *b) { for (int y = 0; y < H; y++) memcpy(fb + y * pitch, b + y * W, (size_t)W * 4); }
static u8 cmos(u8 r) { outb(0x70, r); return inb(0x71); }
void hal_rtc(RtcTime *t) {
    for (int i = 0; i < 100000; i++) { outb(0x70, 0x0A); if (!(inb(0x71) & 0x80)) break; }
    u8 s = cmos(0), mi = cmos(2), h = cmos(4), d = cmos(7), mo = cmos(8), y = cmos(9), b = cmos(0x0B);
    if (!(b & 4)) {
#define BCD(v) ((v) = (u8)(((v) & 15) + ((v) >> 4) * 10))
        BCD(s); BCD(mi); BCD(d); BCD(mo); BCD(y); h = (u8)(((h & 0x7F) & 15) + (((h & 0x7F) >> 4) * 10) + (h & 0x80));
    }
    if (!(b & 2) && (h & 0x80)) h = (u8)(((h & 0x7F) % 12) + 12);
    t->sec = s; t->min = mi; t->hour = h & 0x7F; t->day = d; t->month = mo; t->year = 2000 + y; t->wday = 0;
}
void hal_poweroff(void) { outw(0x604, 0x2000); outw(0xB004, 0x2000); outw(0x4004, 0x3400); }
void hal_reboot(void) {
    wait_in(); outb(0x64, 0xFE);
    idtr.lim = 0; __asm__ volatile("lidt %0; int3" : : "m"(idtr));
}
