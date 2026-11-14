/* Icons (drawn procedurally, no bitmaps), avatars, buttons, text inputs. */
#include "os.h"

u32 g_accent = 0x0078D7;
bool g_clock24 = false;
int g_mx, g_my, g_mb;
Window *g_hover_win;

bool ui_blink(void) { return (hal_ms() / 500) & 1; }
bool in_rect(int px, int py, int x, int y, int w, int h) { return px >= x && py >= y && px < x + w && py < y + h; }
bool ui_hover(Window *w, int x, int y, int bw, int bh) {
    if (w && g_hover_win != w) return false;
    return in_rect(g_mx, g_my, x, y, bw, bh);
}

static u32 isq(u32 n) {
    u32 r = 0, bit = 1u << 30;
    while (bit > n) bit >>= 2;
    while (bit) { if (n >= r + bit) { n -= r + bit; r = (r >> 1) + bit; } else r >>= 1; bit >>= 2; }
    return r;
}
static int cv(int r16, int d16) { return CLAMP(r16 - d16 + 8, 0, 16); }

/* ---- icons on a 32-unit grid scaled to `s` pixels ---- */
static void rr(int ox, int oy, int s, int x, int y, int w, int h, u32 c) {
    int ax = x * s / 32, ay = y * s / 32, bx = (x + w) * s / 32, by = (y + h) * s / 32;
    gfx_rect(ox + ax, oy + ay, MAX(1, bx - ax), MAX(1, by - ay), c);
}
static void ln(int ox, int oy, int s, int x0, int y0, int x1, int y1, u32 c) {
    int th = MAX(1, s / 22);
    for (int t = 0; t < th; t++) gfx_line(ox + x0 * s / 32 + t, oy + y0 * s / 32, ox + x1 * s / 32 + t, oy + y1 * s / 32, c);
}
static void arcgap(int cx, int cy, int r, int thick, int gap, u32 c) {   /* power symbol ring */
    for (int y = cy - r - 1; y <= cy + r + 1; y++)
        for (int x = cx - r - 1; x <= cx + r + 1; x++) {
            int dx = x - cx, dy = y - cy;
            if (dy < 0 && dx >= -gap && dx <= gap) continue;
            int d = (int)isq((u32)(dx * dx + dy * dy) * 256);
            int a = cv(r * 16, d) - cv((r - thick) * 16, d);
            if (a > 0) gfx_pixel(x, y, c, a * 255 / 16);
        }
}
void logo_draw(int x, int y, int size, u32 c) {
    int g = MAX(1, size / 14), p = (size - g) / 2;
    gfx_rect(x, y, p, p, c); gfx_rect(x + p + g, y, p, p, c);
    gfx_rect(x, y + p + g, p, p, c); gfx_rect(x + p + g, y + p + g, p, p, c);
}
void avatar_draw(int cx, int cy, int r, u32 color) {
    int hr = r * 28 / 100, hy = cy - r * 18 / 100, br = r * 62 / 100, by = cy + r * 78 / 100;
    for (int y = cy - r - 1; y <= cy + r + 1; y++)
        for (int x = cx - r - 1; x <= cx + r + 1; x++) {
            int d0 = (int)isq((u32)((x - cx) * (x - cx) + (y - cy) * (y - cy)) * 256);
            int ca = cv(r * 16, d0);
            if (!ca) continue;
            gfx_pixel(x, y, color, ca * 255 / 16);
            int d1 = (int)isq((u32)((x - cx) * (x - cx) + (y - hy) * (y - hy)) * 256);
            int d2 = (int)isq((u32)((x - cx) * (x - cx) + (y - by) * (y - by)) * 256);
            int a = MAX(cv(hr * 16, d1), cv(br * 16, d2));
            a = MIN(a, ca);
            if (a) gfx_pixel(x, y, 0xFFFFFF, a * 255 / 16);
        }
}

void icon_draw(int k, int x, int y, int s) {
#define R(a, b, c, d, col) rr(x, y, s, a, b, c, d, col)
#define L(a, b, c, d, col) ln(x, y, s, a, b, c, d, col)
    int h = s / 2;
    switch (k) {
    case ICON_FOLDER: R(2, 6, 28, 21, 0xE2A619); R(2, 4, 12, 5, 0xE2A619); R(2, 10, 28, 17, 0xFFCB3D); R(2, 10, 28, 2, 0xFFE08A); break;
    case ICON_PC: R(3, 5, 26, 18, 0xC9CED6); R(5, 7, 22, 14, 0x1A8CFF); R(5, 7, 22, 5, 0x57B0FF); R(13, 23, 6, 3, 0x9AA0AA); R(9, 26, 14, 2, 0xC9CED6); break;
    case ICON_BIN: R(6, 6, 20, 3, 0xAEB8C4); R(12, 4, 8, 2, 0xAEB8C4); R(8, 9, 16, 19, 0xD5DCE6); R(8, 9, 16, 1, 0xF2F5F9);
        R(12, 12, 1, 13, 0x9AA6B4); R(16, 12, 1, 13, 0x9AA6B4); R(20, 12, 1, 13, 0x9AA6B4); break;
    case ICON_NOTEPAD: R(6, 3, 20, 26, 0x39A0E6); R(6, 3, 20, 5, 0x1B6FBF); R(9, 12, 14, 1, 0xFFFFFF); R(9, 16, 14, 1, 0xFFFFFF);
        R(9, 20, 14, 1, 0xFFFFFF); R(9, 24, 8, 1, 0xFFFFFF); break;
    case ICON_CALC: R(6, 2, 20, 28, 0x3C3C3C); R(8, 4, 16, 7, 0xBFE6C9);
        for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++) R(8 + i * 6, 14 + j * 5, 4, 3, (i == 2 && j == 2) ? 0xFF9800 : 0xE0E0E0);
        break;
    case ICON_PAINT: gfx_circle(x + h, y + h, 14 * s / 32, 0xEBD6AE);
        gfx_circle(x + 11 * s / 32, y + 12 * s / 32, 3 * s / 32 + 1, 0xE53935); gfx_circle(x + 18 * s / 32, y + 8 * s / 32, 3 * s / 32 + 1, 0x43A047);
        gfx_circle(x + 24 * s / 32, y + 14 * s / 32, 3 * s / 32 + 1, 0x1E88E5); gfx_circle(x + 21 * s / 32, y + 22 * s / 32, 3 * s / 32 + 1, 0xFDD835); break;
    case ICON_TERM: R(3, 5, 26, 22, 0x0C0C0C); R(3, 5, 26, 4, 0xE6E6E6); L(8, 13, 14, 17, 0xFFFFFF); L(14, 17, 8, 21, 0xFFFFFF); R(16, 21, 8, 2, 0xFFFFFF); break;
    case ICON_GEAR: gfx_ring(x + h, y + h, 9 * s / 32 + 1, 4 * s / 32 + 1, 0x9AA0A6);
        R(14, 3, 4, 6, 0x9AA0A6); R(14, 23, 4, 6, 0x9AA0A6); R(3, 14, 6, 4, 0x9AA0A6); R(23, 14, 6, 4, 0x9AA0A6);
        R(6, 6, 4, 4, 0x9AA0A6); R(22, 6, 4, 4, 0x9AA0A6); R(6, 22, 4, 4, 0x9AA0A6); R(22, 22, 4, 4, 0x9AA0A6); break;
    case ICON_TASKMGR: R(3, 5, 26, 22, 0xFFFFFF); R(3, 5, 26, 4, 0x2E7D32); R(3, 5, 1, 22, 0x7A7A7A); R(28, 5, 1, 22, 0x7A7A7A); R(3, 26, 26, 1, 0x7A7A7A);
        L(6, 22, 11, 16, 0x2E7D32); L(11, 16, 15, 20, 0x2E7D32); L(15, 20, 20, 12, 0x2E7D32); L(20, 12, 26, 15, 0x2E7D32); break;
    case ICON_FILE: R(7, 3, 18, 26, 0xFFFFFF); R(7, 3, 18, 1, 0x8A8A8A); R(7, 28, 18, 1, 0x8A8A8A); R(7, 3, 1, 26, 0x8A8A8A); R(24, 3, 1, 26, 0x8A8A8A);
        R(19, 3, 6, 6, 0xDDE3EA); R(10, 14, 12, 1, 0x9AA3AD); R(10, 18, 12, 1, 0x9AA3AD); R(10, 22, 8, 1, 0x9AA3AD); break;
    case ICON_DRIVE: R(3, 12, 26, 11, 0xCDD2D9); R(3, 12, 26, 2, 0x8E96A3); R(23, 18, 3, 2, 0x4CAF50); break;
    case ICON_USER: { Clip o = gfx_clip_push(x, y, s, s); gfx_circle(x + h, y + 10 * s / 32, 5 * s / 32 + 1, 0xFFFFFF);
        gfx_circle(x + h, y + 29 * s / 32, 10 * s / 32, 0xFFFFFF); gfx_clip_pop(o); break; }
    case ICON_POWER: arcgap(x + h, y + 17 * s / 32, 9 * s / 32 + 1, MAX(1, s / 14), MAX(1, s / 12), 0xFFFFFF); R(15, 5, 2, 12, 0xFFFFFF); break;
    case ICON_LOGO: logo_draw(x, y, s, 0xFFFFFF); break;
    case ICON_SEARCH: gfx_ring(x + 13 * s / 32, y + 13 * s / 32, 8 * s / 32, MAX(1, s / 14), 0xFFFFFF); L(19, 19, 27, 27, 0xFFFFFF); break;
    case ICON_DOCS: R(8, 4, 16, 1, 0xFFFFFF); R(8, 27, 16, 1, 0xFFFFFF); R(8, 4, 1, 24, 0xFFFFFF); R(23, 4, 1, 24, 0xFFFFFF);
        R(11, 12, 10, 1, 0xFFFFFF); R(11, 16, 10, 1, 0xFFFFFF); R(11, 20, 7, 1, 0xFFFFFF); break;
    case ICON_PLUS: R(14, 6, 4, 20, 0xFFFFFF); R(6, 14, 20, 4, 0xFFFFFF); break;
    case ICON_BACK: R(8, 15, 17, 2, 0x444444); L(8, 16, 15, 9, 0x444444); L(8, 16, 15, 23, 0x444444); break;
    case ICON_UP: R(15, 8, 2, 17, 0x444444); L(16, 7, 9, 14, 0x444444); L(16, 7, 23, 14, 0x444444); break;
    case ICON_NEWFOLDER: R(2, 7, 24, 18, 0xE2A619); R(2, 5, 10, 4, 0xE2A619); R(2, 10, 24, 15, 0xFFCB3D); R(21, 15, 3, 12, 0x2E9E4F); R(17, 19, 11, 3, 0x2E9E4F); break;
    case ICON_TRASH: R(8, 8, 16, 3, 0x555555); R(13, 6, 6, 2, 0x555555); R(10, 11, 12, 16, 0x555555); R(13, 14, 1, 10, 0xFFFFFF); R(16, 14, 1, 10, 0xFFFFFF); R(19, 14, 1, 10, 0xFFFFFF); break;
    case ICON_CALENDAR: R(5, 5, 22, 23, 0xFFFFFF); R(5, 5, 22, 7, 0xE53935); R(9, 15, 4, 3, 0x777777); R(14, 15, 4, 3, 0x777777); R(19, 15, 4, 3, 0x777777); R(9, 20, 4, 3, 0x777777); R(14, 20, 4, 3, 0x777777); break;
    }
#undef R
#undef L
}

/* ---- widgets ---- */
bool ui_button(Window *w, int x, int y, int bw, int bh, const char *label, bool primary) {
    bool hov = ui_hover(w, x, y, bw, bh), down = hov && (g_mb & 1);
    u32 bg, bd, fg = 0x000000;
    if (primary) { bg = down ? col_darken(g_accent, 40) : hov ? col_lighten(g_accent, 20) : g_accent; bd = bg; fg = 0xFFFFFF; }
    else { bg = down ? 0xCCE4F7 : hov ? 0xE5F1FB : 0xE1E1E1; bd = down ? 0x005499 : hov ? g_accent : 0xADADAD; }
    gfx_rect(x, y, bw, bh, bg); gfx_frame(x, y, bw, bh, bd);
    text_draw_c(&font_ui, x, bw, y + (bh - font_ui.h) / 2, label, fg);
    return hov;
}
bool ui_toggle(Window *w, int x, int y, bool on) {
    bool hov = ui_hover(w, x, y, 40, 20);
    if (on) { gfx_rect(x, y, 40, 20, g_accent); gfx_rect(x + 24, y + 4, 12, 12, 0xFFFFFF); }
    else { gfx_rect(x, y, 40, 20, 0xFFFFFF); gfx_frame(x, y, 40, 20, hov ? 0x000000 : 0x555555); gfx_rect(x + 4, y + 4, 12, 12, hov ? 0x000000 : 0x555555); }
    return hov;
}
void ti_clear(TextInput *t) { t->buf[0] = 0; t->len = 0; }
bool ti_key(TextInput *t, int key, int mod) {
    if (mod & MOD_CTRL) return false;
    if (key == K_BACKSPACE) { if (t->len) t->buf[--t->len] = 0; return true; }
    if (key >= 32 && key < 127 && t->len < 22) { t->buf[t->len++] = (char)key; t->buf[t->len] = 0; return true; }
    return false;
}
void ti_draw(TextInput *t, int x, int y, int w, int h, const char *ph, bool blink) {
    gfx_rect(x, y, w, h, 0xFFFFFF);
    gfx_frame(x, y, w, h, t->focus ? g_accent : 0x7A7A7A);
    if (t->focus) gfx_frame(x + 1, y + 1, w - 2, h - 2, g_accent);
    int tx = x + 8;
    if (!t->len && ph && !t->focus) text_draw_vc(&font_ui, tx, y, h, ph, 0x6B6B6B);
    if (t->pass) { for (int i = 0; i < t->len; i++) gfx_circle(tx + 4 + i * 11, y + h / 2, 4, 0x000000); tx += t->len * 11; }
    else { text_draw_vc(&font_ui, tx, y, h, t->buf, 0x000000); tx += text_width(&font_ui, t->buf); }
    if (t->focus && blink) gfx_rect(tx + 1, y + 7, 1, h - 14, 0x000000);
}
