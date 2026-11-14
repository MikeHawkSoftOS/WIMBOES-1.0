/* Calculator (x87 doubles; formatting avoids 64-bit division so no libgcc needed). */
#include "os.h"
typedef struct { double acc; char entry[20]; int op; bool fresh, err; char hist[40]; } Calc;

static double val(const char *s) {
    double v = 0, f = 0.1; bool neg = false, frac = false;
    if (*s == '-') { neg = true; s++; }
    for (; *s; s++) {
        if (*s == '.') frac = true;
        else if (!frac) v = v * 10 + (*s - '0');
        else { v += (*s - '0') * f; f /= 10; }
    }
    return neg ? -v : v;
}
static void fmt(double v, char *out, int cap) {
    bool neg = v < 0; if (neg) v = -v;
    if (v >= 4000000000.0) { kstrcpy(out, "Overflow", cap); return; }
    u32 ip = (u32)v; double fr = v - (double)ip;
    u32 f = (u32)(fr * 100000000.0 + 0.5);
    if (f >= 100000000u) { ip++; f -= 100000000u; }
    char fs[12] = "";
    if (f) { ksnprintf(fs, sizeof fs, ".%08u", f); int l = (int)strlen(fs); while (l > 1 && fs[l - 1] == '0') fs[--l] = 0; }
    ksnprintf(out, cap, "%s%u%s", neg && (ip || f) ? "-" : "", ip, fs);
}
static double sqrt_(double x) { if (x <= 0) return 0; double g = x > 1 ? x / 2 : 1; for (int i = 0; i < 40; i++) g = (g + x / g) / 2; return g; }
static void set(Calc *c, double v) { fmt(v, c->entry, sizeof c->entry); }
static double apply(Calc *c, double a, double b) {
    switch (c->op) {
    case '+': return a + b; case '-': return a - b; case '*': return a * b;
    case '/': if (b == 0) { c->err = true; return 0; } return a / b;
    }
    return b;
}
static void press(Calc *c, int k) {
    if (c->err) { c->err = false; c->acc = 0; c->op = 0; kstrcpy(c->entry, "0", sizeof c->entry); c->hist[0] = 0; c->fresh = true; if (k == 'C') return; }
    if (k >= '0' && k <= '9') {
        if (c->fresh) { c->entry[0] = 0; c->fresh = false; }
        if (!strcmp(c->entry, "0")) c->entry[0] = 0;
        int l = (int)strlen(c->entry); if (l < 15) { c->entry[l] = (char)k; c->entry[l + 1] = 0; }
    } else if (k == '.') {
        if (c->fresh) { kstrcpy(c->entry, "0", sizeof c->entry); c->fresh = false; }
        bool has = false; for (char *p = c->entry; *p; p++) if (*p == '.') has = true;
        int l = (int)strlen(c->entry); if (!has && l < 15) { c->entry[l] = '.'; c->entry[l + 1] = 0; }
    } else if (k == '+' || k == '-' || k == '*' || k == '/') {
        if (c->op && !c->fresh) { c->acc = apply(c, c->acc, val(c->entry)); if (c->err) { kstrcpy(c->entry, "Cannot divide by zero", sizeof c->entry); return; } set(c, c->acc); }
        else c->acc = val(c->entry);
        c->op = k; c->fresh = true;
        char b[20]; fmt(c->acc, b, sizeof b); ksnprintf(c->hist, sizeof c->hist, "%s %c", b, k == '*' ? 'x' : k);
    } else if (k == '=') {
        if (c->op) {
            char a[20]; fmt(c->acc, a, sizeof a);
            double r = apply(c, c->acc, val(c->entry));
            if (c->err) { kstrcpy(c->entry, "Cannot divide by zero", sizeof c->entry); return; }
            ksnprintf(c->hist, sizeof c->hist, "%s %c %s =", a, c->op == '*' ? 'x' : c->op, c->entry);
            c->acc = r; set(c, r); c->op = 0;
        }
        c->fresh = true;
    } else if (k == 'C') { c->acc = 0; c->op = 0; kstrcpy(c->entry, "0", sizeof c->entry); c->hist[0] = 0; c->fresh = true; }
    else if (k == 'E') { kstrcpy(c->entry, "0", sizeof c->entry); c->fresh = true; }
    else if (k == 'B') { if (!c->fresh) { int l = (int)strlen(c->entry); if (l > 1) c->entry[l - 1] = 0; else kstrcpy(c->entry, "0", sizeof c->entry); } }
    else if (k == 'N') { double v = -val(c->entry); set(c, v); }
    else if (k == 'S') { set(c, sqrt_(val(c->entry))); c->fresh = true; }
    else if (k == 'Q') { double v = val(c->entry); set(c, v * v); c->fresh = true; }
    else if (k == 'R') { double v = val(c->entry); if (v == 0) { c->err = true; kstrcpy(c->entry, "Cannot divide by zero", sizeof c->entry); } else { set(c, 1 / v); c->fresh = true; } }
    else if (k == '%') { double v = val(c->entry); set(c, c->op ? c->acc * v / 100 : 0); c->fresh = true; }
}
static const struct { const char *l; int k; int kind; } btn[24] = {   /* kind 0 digit, 1 op, 2 equals */
    { "%", '%', 1 }, { "CE", 'E', 1 }, { "C", 'C', 1 }, { "<-", 'B', 1 },
    { "1/x", 'R', 1 }, { "x2", 'Q', 1 }, { "sqrt", 'S', 1 }, { "/", '/', 1 },
    { "7", '7', 0 }, { "8", '8', 0 }, { "9", '9', 0 }, { "x", '*', 1 },
    { "4", '4', 0 }, { "5", '5', 0 }, { "6", '6', 0 }, { "-", '-', 1 },
    { "1", '1', 0 }, { "2", '2', 0 }, { "3", '3', 0 }, { "+", '+', 1 },
    { "+/-", 'N', 0 }, { "0", '0', 0 }, { ".", '.', 0 }, { "=", '=', 2 } };
#define DISP_H 110
static void cell(int cw, int ch, int i, int *x, int *y, int *w, int *h) {
    int top = DISP_H, bw = cw / 4, bh = (ch - top) / 6;
    *x = (i % 4) * bw + 1; *y = top + (i / 4) * bh + 1; *w = bw - 2; *h = bh - 2;
}
static void calc_draw(Window *w, int cx, int cy, int cw, int ch) {
    Calc *c = w->priv;
    gfx_rect(cx, cy, cw, ch, 0xE6E6E6);
    gfx_rect(cx, cy, cw, DISP_H, 0xF3F3F3);
    text_draw_r(&font_small, cx + cw - 14, cy + 22, c->hist, 0x666666);
    const Font *f = text_width(&font_big, c->entry) < cw - 28 ? &font_big : &font_title;
    text_draw_r(f, cx + cw - 14, cy + 52, c->entry, 0x000000);
    for (int i = 0; i < 24; i++) {
        int x, y, bw, bh; cell(cw, ch, i, &x, &y, &bw, &bh);
        bool hov = ui_hover(w, cx + x, cy + y, bw, bh), down = hov && (g_mb & 1);
        u32 bg = btn[i].kind == 2 ? (down ? col_darken(g_accent, 40) : hov ? col_lighten(g_accent, 20) : g_accent)
               : btn[i].kind == 0 ? (down ? 0xCCCCCC : hov ? 0xF2F2F2 : 0xFAFAFA) : (down ? 0xBEBEBE : hov ? 0xDCDCDC : 0xEEEEEE);
        gfx_rect(cx + x, cy + y, bw, bh, bg);
        text_draw_c(&font_title, cx + x, bw, cy + y + (bh - font_title.h) / 2, btn[i].l, btn[i].kind == 2 ? 0xFFFFFF : 0x000000);
    }
}
static void calc_mouse(Window *w, int ev, int x, int y, int btnn) {
    (void)btnn; Calc *c = w->priv;
    if (ev != M_DOWN) return;
    int cw = w->w - 2, ch = w->h - TITLE_H - 1;
    for (int i = 0; i < 24; i++) { int bx, by, bw, bh; cell(cw, ch, i, &bx, &by, &bw, &bh); if (in_rect(x, y, bx, by, bw, bh)) press(c, btn[i].k); }
}
static void calc_key(Window *w, int key, int mod) {
    (void)mod; Calc *c = w->priv;
    if ((key >= '0' && key <= '9') || key == '.' || key == '+' || key == '-' || key == '*' || key == '/' || key == '%') press(c, key);
    else if (key == K_ENTER || key == '=') press(c, '=');
    else if (key == K_BACKSPACE) press(c, 'B');
    else if (key == K_ESC || key == 'c' || key == 'C') press(c, 'C');
}
static void calc_close(Window *w) { kfree(w->priv); }
Window *calc_new(void) {
    int x, y; win_cascade(&x, &y);
    Window *w = wm_create(APP_CALC, "Calculator", ICON_CALC, x, y, 322, 500);
    if (!w) return 0;
    Calc *c = kmalloc(sizeof(Calc)); kstrcpy(c->entry, "0", sizeof c->entry); c->fresh = true;
    w->priv = c; w->draw = calc_draw; w->mouse = calc_mouse; w->key = calc_key; w->close = calc_close; w->minw = 280; w->minh = 420;
    return w;
}
