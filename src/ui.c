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
    case ICON_FOLDER: R(2, 5, 28, 22, 0xFFB300); R(2, 3, 14, 6, 0xFFB300); R(2, 9, 28, 18, 0xFFD600); R(2, 9, 28, 3, 0xFFF59D); break;
    case ICON_PC: R(3, 4, 26, 20, 0xE0E0E0); R(5, 6, 22, 16, 0x1565C0); R(5, 6, 22, 6, 0x42A5F5); R(13, 24, 6, 3, 0xBDBDBD); R(9, 26, 14, 2, 0xE0E0E0); break;
    case ICON_BIN: R(6, 5, 20, 3, 0xBDBDBD); R(12, 3, 8, 2, 0xBDBDBD); R(8, 8, 16, 20, 0xECEFF1); R(8, 8, 16, 1, 0xFFFFFF);
        R(12, 11, 1, 14, 0x9E9E9E); R(16, 11, 1, 14, 0x9E9E9E); R(20, 11, 1, 14, 0x9E9E9E); break;
    case ICON_NOTEPAD: R(5, 3, 22, 26, 0x1E88E5); R(5, 3, 22, 6, 0x0D47A1); R(9, 11, 14, 1, 0xFFFFFF); R(9, 15, 14, 1, 0xFFFFFF);
        R(9, 19, 14, 1, 0xFFFFFF); R(9, 23, 10, 1, 0xFFFFFF); break;
    case ICON_CALC: R(5, 2, 22, 28, 0x37474F); R(7, 4, 18, 8, 0x80CBC4);
        for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++) R(7 + i * 6, 14 + j * 5, 4, 3, (i == 2 && j == 2) ? 0xFF9800 : 0xECEFF1);
        break;
    case ICON_PAINT: gfx_circle(x + h, y + h, 14 * s / 32, 0xFFF3E0);
        gfx_circle(x + 11 * s / 32, y + 12 * s / 32, 4 * s / 32, 0xE53935); gfx_circle(x + 18 * s / 32, y + 8 * s / 32, 4 * s / 32, 0x43A047);
        gfx_circle(x + 24 * s / 32, y + 14 * s / 32, 4 * s / 32, 0x1E88E5); gfx_circle(x + 21 * s / 32, y + 22 * s / 32, 4 * s / 32, 0xFDD835); break;
    case ICON_TERM: R(3, 4, 26, 24, 0x1A1A1A); R(3, 4, 26, 5, 0xECEFF1); L(8, 12, 14, 18, 0xFFFFFF); L(14, 18, 8, 22, 0xFFFFFF); R(16, 22, 8, 2, 0xFFFFFF); break;
    case ICON_GEAR: gfx_ring(x + h, y + h, 10 * s / 32, 4 * s / 32, 0x90A4AE);
        R(14, 2, 4, 7, 0x90A4AE); R(14, 24, 4, 7, 0x90A4AE); R(2, 14, 7, 4, 0x90A4AE); R(24, 14, 7, 4, 0x90A4AE);
        R(5, 5, 5, 5, 0x90A4AE); R(22, 5, 5, 5, 0x90A4AE); R(5, 22, 5, 5, 0x90A4AE); R(22, 22, 5, 5, 0x90A4AE); break;
    case ICON_TASKMGR: R(3, 4, 26, 24, 0xFFFFFF); R(3, 4, 26, 5, 0x2E7D32); R(3, 4, 1, 24, 0x9E9E9E); R(28, 4, 1, 24, 0x9E9E9E); R(3, 27, 26, 1, 0x9E9E9E);
        L(6, 21, 11, 17, 0x2E7D32); L(11, 17, 15, 20, 0x2E7D32); L(15, 20, 20, 13, 0x2E7D32); L(20, 13, 26, 16, 0x2E7D32); break;
    case ICON_FILE: R(6, 3, 20, 26, 0xFFFFFF); R(6, 3, 20, 1, 0xCFD8DC); R(6, 28, 20, 1, 0xCFD8DC); R(6, 3, 1, 26, 0xCFD8DC); R(25, 3, 1, 26, 0xCFD8DC);
        R(18, 3, 8, 8, 0xE3F2FD); R(10, 14, 12, 1, 0x90A4AE); R(10, 18, 12, 1, 0x90A4AE); R(10, 22, 8, 1, 0x90A4AE); break;
    case ICON_DRIVE: R(3, 10, 26, 14, 0xCFD8DC); R(3, 10, 26, 2, 0xB0BEC5); R(23, 16, 3, 3, 0x4CAF50); break;
    case ICON_USER: { Clip o = gfx_clip_push(x, y, s, s); gfx_circle(x + h, y + 10 * s / 32, 6 * s / 32, 0xFFFFFF);
        gfx_circle(x + h, y + 28 * s / 32, 11 * s / 32, 0xFFFFFF); gfx_clip_pop(o); break; }
    case ICON_POWER: arcgap(x + h, y + 16 * s / 32, 10 * s / 32, MAX(1, s / 12), MAX(1, s / 10), 0xFFFFFF); R(15, 4, 2, 12, 0xFFFFFF); break;
    case ICON_LOGO: logo_draw(x, y, s, 0xFFFFFF); break;
    case ICON_SEARCH: gfx_ring(x + 12 * s / 32, y + 12 * s / 32, 8 * s / 32, MAX(1, s / 12), 0xFFFFFF); L(18, 18, 26, 26, 0xFFFFFF); break;
    case ICON_DOCS: R(7, 3, 18, 1, 0xFFFFFF); R(7, 28, 18, 1, 0xFFFFFF); R(7, 3, 1, 26, 0xFFFFFF); R(24, 3, 1, 26, 0xFFFFFF);
        R(10, 11, 11, 1, 0xFFFFFF); R(10, 15, 11, 1, 0xFFFFFF); R(10, 19, 8, 1, 0xFFFFFF); break;
    case ICON_PLUS: R(14, 5, 4, 22, 0xFFFFFF); R(5, 14, 22, 4, 0xFFFFFF); break;
    case ICON_BACK: R(8, 14, 17, 2, 0x616161); L(8, 15, 15, 8, 0x616161); L(8, 15, 15, 24, 0x616161); break;
    case ICON_UP: R(15, 8, 2, 17, 0x616161); L(16, 6, 9, 15, 0x616161); L(16, 6, 23, 15, 0x616161); break;
    case ICON_NEWFOLDER: R(2, 6, 24, 20, 0xFFB300); R(2, 4, 10, 4, 0xFFB300); R(2, 9, 24, 16, 0xFFD600); R(21, 14, 3, 14, 0x2E7D32); R(17, 18, 11, 3, 0x2E7D32); break;
    case ICON_TRASH: R(8, 6, 16, 3, 0x616161); R(13, 4, 6, 2, 0x616161); R(10, 9, 12, 18, 0x616161); R(13, 12, 1, 12, 0xFFFFFF); R(16, 12, 1, 12, 0xFFFFFF); R(19, 12, 1, 12, 0xFFFFFF); break;
    case ICON_CALENDAR: R(4, 4, 24, 24, 0xFFFFFF); R(4, 4, 24, 8, 0xE53935); R(9, 14, 4, 3, 0x616161); R(14, 14, 4, 3, 0x616161); R(19, 14, 4, 3, 0x616161); R(9, 19, 4, 3, 0x616161); R(14, 19, 4, 3, 0x616161); R(19, 19, 4, 3, 0x616161); break;
    case ICON_GAME: gfx_ring(x + h, y + h, 14 * s / 32, 2 * s / 32, 0x2E7D32);
        gfx_rect(x + 6 * s / 32, y + 10 * s / 32, 4 * s / 32, 6 * s / 32, 0x2E7D32);
        gfx_rect(x + 22 * s / 32, y + 10 * s / 32, 4 * s / 32, 6 * s / 32, 0x2E7D32);
        gfx_circle(x + 20 * s / 32, y + 12 * s / 32, 2 * s / 32, 0xE53935);
        gfx_circle(x + 12 * s / 32, y + 12 * s / 32, 2 * s / 32, 0x1565C0); break;

    /* System */
    case ICON_DISPLAY: R(4, 3, 24, 20, 0xE0E0E0); R(6, 5, 20, 16, 0x1A1A1A);
        R(13, 25, 6, 2, 0xBDBDBD); R(10, 27, 12, 2, 0xE0E0E0);
        R(14, 29, 4, 2, 0x9E9E9E); R(14, 9, 4, 4, 0xE53935); break;
    case ICON_SOUND: R(7, 13, 10, 6, 0xFFFFFF); R(17, 9, 8, 14, 0xFFFFFF);
        gfx_line(x + 25 * s / 32, y + 6 * s / 32, x + 29 * s / 32, y + 3 * s / 32, 0xFFFFFF);
        gfx_line(x + 26 * s / 32, y + 9 * s / 32, x + 31 * s / 32, y + 3 * s / 32, 0xFFFFFF);
        gfx_line(x + 27 * s / 32, y + 11 * s / 32, x + 32 * s / 32, y + 5 * s / 32, 0xFFFFFF);
        gfx_line(x + 28 * s / 32, y + 14 * s / 32, x + 33 * s / 32, y + 8 * s / 32, 0xFFFFFF);
        gfx_line(x + 27 * s / 32, y + 19 * s / 32, x + 32 * s / 32, y + 26 * s / 32, 0xFFFFFF);
        gfx_line(x + 26 * s / 32, y + 21 * s / 32, x + 31 * s / 32, y + 28 * s / 32, 0xFFFFFF);
        gfx_line(x + 25 * s / 32, y + 25 * s / 32, x + 29 * s / 32, y + 30 * s / 32, 0xFFFFFF); break;
    case ICON_NOTIFICATIONS: R(13, 4, 6, 16, 0xFFFFFF); R(11, 20, 10, 4, 0xFFFFFF);
        R(10, 24, 12, 2, 0xFFFFFF); gfx_circle(x + 16 * s / 32, y + 28 * s / 32, 3 * s / 32, 0xFFFFFF); break;
    case ICON_POWER_BATTERY: R(9, 5, 18, 22, 0xFFFFFF); R(14, 3, 8, 3, 0xFFFFFF);
        R(11, 7, 14, 16, 0x4CAF50); R(11, 7, 5, 16, 0x81C784); break;
    case ICON_STORAGE: R(5, 7, 22, 7, 0xCFD8DC); R(5, 16, 22, 7, 0xCFD8DC); R(5, 25, 22, 7, 0xCFD8DC);
        R(4, 6, 24, 2, 0xB0BEC5); R(4, 15, 24, 2, 0xB0BEC5); R(4, 24, 24, 2, 0xB0BEC5); break;
    case ICON_ABOUT: gfx_ring(x + h, y + h, 13 * s / 32, MAX(1, s / 10), 0x1E88E5);
        R(15, 7, 2, 12, 0x1E88E5); R(15, 23, 2, 2, 0x1E88E5); break;
    case ICON_THEMES: gfx_circle(x + 8 * s / 32, y + 8 * s / 32, 6 * s / 32, 0xE53935);
        gfx_circle(x + 24 * s / 32, y + 8 * s / 32, 6 * s / 32, 0x43A047);
        gfx_circle(x + 8 * s / 32, y + 24 * s / 32, 6 * s / 32, 0x1E88E5);
        gfx_circle(x + 24 * s / 32, y + 24 * s / 32, 6 * s / 32, 0xFDD835);
        gfx_circle(x + h, y + h, 5 * s / 32, 0xFFFFFF); break;
    case ICON_LOCK_SCREEN: R(9, 5, 14, 12, 0xFFFFFF); R(9, 5, 14, 1, 0x1E88E5);
        R(9, 17, 14, 11, 0x1E88E5); gfx_circle(x + 16 * s / 32, y + 21 * s / 32, 3 * s / 32, 0xFFFFFF); break;
    case ICON_START: logo_draw(x, y, s, 0xFFFFFF); break;
    case ICON_TASKBAR_SETTINGS: R(3, 18, 26, 10, 0xCFD8DC); R(3, 18, 26, 1, 0xB0BEC5);
        R(13, 6, 6, 14, 0x90A4AE); R(11, 8, 10, 1, 0x90A4AE); R(11, 16, 10, 1, 0x90A4AE);
        R(11, 12, 3, 4, 0x90A4AE); R(18, 12, 3, 4, 0x90A4AE); break;
    case ICON_APPS: for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++)
        R(5 + i * 9, 5 + j * 9, 7, 7, (i + j) % 2 == 0 ? 0x1E88E5 : 0xFFFFFF); break;
    case ICON_DEFAULT_APPS: R(7, 11, 9, 9, 0xFFFFFF); R(16, 11, 9, 9, 0xFFFFFF);
        gfx_line(x + 16 * s / 32, y + 11 * s / 32, x + 25 * s / 32, y + 3 * s / 32, 0xFFFFFF);
        gfx_line(x + 16 * s / 32, y + 20 * s / 32, x + 25 * s / 32, y + 29 * s / 32, 0xFFFFFF); break;
    case ICON_STARTUP: R(14, 6, 4, 20, 0xFFFFFF); gfx_line(x + 16 * s / 32, y + 26 * s / 32, x + 9 * s / 32, y + 16 * s / 32, 0xFFFFFF);
        gfx_line(x + 16 * s / 32, y + 26 * s / 32, x + 23 * s / 32, y + 16 * s / 32, 0xFFFFFF); break;
    case ICON_OFFLINE_MAPS: R(13, 6, 6, 14, 0xE53935); gfx_circle(x + 16 * s / 32, y + 24 * s / 32, 5 * s / 32, 0xE53935);
        R(15, 11, 2, 5, 0xFFFFFF); break;

    /* Accounts */
    case ICON_YOUR_INFO: R(5, 3, 22, 26, 0xFFFFFF); R(5, 3, 22, 9, 0x1E88E5);
        gfx_circle(x + h, y + 11 * s / 32, 5 * s / 32, 0xFFFFFF);
        R(7, 19, 18, 9, 0xF5F5F5); break;
    case ICON_SIGNIN_OPTIONS: R(9, 7, 14, 18, 0xFFFFFF); R(12, 9, 8, 9, 0xF5F5F5);
        R(13, 11, 6, 5, 0x1E88E5); R(13, 18, 6, 2, 0x1E88E5); break;
    case ICON_FAMILY: gfx_circle(x + 8 * s / 32, y + 9 * s / 32, 5 * s / 32, 0xFFFFFF);
        gfx_circle(x + 24 * s / 32, y + 9 * s / 32, 5 * s / 32, 0xFFFFFF);
        gfx_circle(x + h, y + 23 * s / 32, 6 * s / 32, 0xFFFFFF);
        gfx_circle(x + 8 * s / 32, y + 19 * s / 32, 3 * s / 32, 0x81D4FA);
        gfx_circle(x + 24 * s / 32, y + 19 * s / 32, 3 * s / 32, 0xF8BBD0); break;

    /* Backup */
    case ICON_BACKUP: R(7, 10, 18, 14, 0xFFFFFF); R(7, 10, 18, 7, 0x81D4FA);
        gfx_circle(x + h, y + 9 * s / 32, 6 * s / 32, 0x81D4FA);
        R(13, 5, 6, 9, 0xFFFFFF); L(11, 9, 17, 6, 0xFFFFFF); L(17, 9, 21, 6, 0xFFFFFF); break;

    /* Time & Language */
    case ICON_DATE_TIME: R(5, 3, 22, 22, 0xFFFFFF); R(5, 3, 22, 7, 0xE53935);
        R(8, 11, 5, 9, 0xE53935); R(19, 11, 5, 9, 0x1E88E5);
        gfx_circle(x + h, y + 15 * s / 32, 9 * s / 32, 0xF5F5F5);
        L(16, 15, 16, 9, 0xE53935); L(16, 15, 22, 15, 0x1E88E5); break;
    case ICON_LANGUAGE: gfx_ring(x + h, y + h, 13 * s / 32, MAX(1, s / 10), 0x1E88E5);
        R(13, 9, 6, 14, 0x1E88E5); L(9, 22, 23, 10, 0x1E88E5); break;
    case ICON_TYPING: R(3, 15, 26, 14, 0xCFD8DC); R(5, 17, 22, 10, 0xFFFFFF);
        for (int i = 0; i < 11; i++) R(5 + i * 2, 17, 1, 10, i % 2 == 0 ? 0x1E88E5 : 0xFFFFFF); break;
    case ICON_SPEECH: R(9, 7, 14, 16, 0x1E88E5); R(13, 23, 6, 4, 0x1E88E5);
        L(15, 23, 19, 27, 0x1E88E5); L(15, 23, 19, 19, 0x1E88E5); break;

    /* Gaming */
    case ICON_GAMING: gfx_ring(x + h, y + h, 15 * s / 32, 2 * s / 32, 0x512DA8);
        gfx_rect(x + 9 * s / 32, y + 11 * s / 32, 5 * s / 32, 7 * s / 32, 0x512DA8);
        gfx_rect(x + 18 * s / 32, y + 11 * s / 32, 5 * s / 32, 7 * s / 32, 0x512DA8);
        gfx_circle(x + 12 * s / 32, y + 21 * s / 32, 3 * s / 32, 0x512DA8);
        gfx_circle(x + 20 * s / 32, y + 21 * s / 32, 3 * s / 32, 0x512DA8);
        gfx_circle(x + 16 * s / 32, y + 15 * s / 32, 3 * s / 32, 0xFFFFFF); break;
    case ICON_CAPTURES: R(7, 9, 18, 16, 0x424242); R(9, 11, 14, 12, 0x1A1A1A);
        gfx_circle(x + h, y + 17 * s / 32, 5 * s / 32, 0xE53935);
        R(12, 14, 8, 5, 0xFFFFFF); break;
    case ICON_GAME_BAR: gfx_ring(x + 9 * s / 32, y + 15 * s / 32, 7 * s / 32, MAX(1, s / 14), 0x512DA8);
        gfx_ring(x + 23 * s / 32, y + 15 * s / 32, 7 * s / 32, MAX(1, s / 14), 0x512DA8);
        R(9, 15, 14, 7, 0x512DA8); R(13, 7, 6, 9, 0x512DA8); R(19, 7, 6, 9, 0x512DA8); break;

    /* Accessibility */
    case ICON_ACCESSIBILITY: gfx_circle(x + h, y + 7 * s / 32, 5 * s / 32, 0x1E88E5);
        R(13, 13, 6, 14, 0x1E88E5);
        L(9, 21, 15, 14, 0x1E88E5); L(17, 13, 22, 21, 0x1E88E5); break;
    case ICON_VISION: gfx_ring(x + h, y + h, 13 * s / 32, MAX(1, s / 10), 0x1E88E5);
        gfx_circle(x + h, y + h, 6 * s / 32, 0x1E88E5);
        gfx_circle(x + h, y + h, 2 * s / 32, 0xFFFFFF); break;
    case ICON_HEARING: R(13, 5, 6, 22, 0x1E88E5);
        gfx_line(x + 9 * s / 32, y + 27 * s / 32, x + 19 * s / 32, y + 27 * s / 32, 0x1E88E5);
        gfx_line(x + 7 * s / 32, y + 7 * s / 32, x + 14 * s / 32, y + 5 * s / 32, 0x1E88E5);
        gfx_line(x + 5 * s / 32, y + 11 * s / 32, x + 12 * s / 32, y + 9 * s / 32, 0x1E88E5);
        gfx_line(x + 23 * s / 32, y + 7 * s / 32, x + 25 * s / 32, y + 5 * s / 32, 0x1E88E5);
        gfx_line(x + 21 * s / 32, y + 11 * s / 32, x + 27 * s / 32, y + 9 * s / 32, 0x1E88E5); break;
    case ICON_INTERACTION: R(9, 9, 14, 22, 0xFFFFFF); R(9, 9, 14, 5, 0x1E88E5);
        gfx_circle(x + 9 * s / 32, y + 13 * s / 32, 4 * s / 32, 0x1E88E5);
        gfx_circle(x + 23 * s / 32, y + 13 * s / 32, 4 * s / 32, 0x1E88E5);
        gfx_circle(x + 16 * s / 32, y + 25 * s / 32, 4 * s / 32, 0x1E88E5); break;

    /* Privacy & Security */
    case ICON_PRIVACY: R(6, 4, 20, 24, 0x1E88E5); R(8, 6, 16, 20, 0xFFFFFF);
        gfx_circle(x + h, y + 13 * s / 32, 5 * s / 32, 0x1E88E5);
        R(15, 10, 2, 10, 0xFFFFFF); R(15, 22, 2, 2, 0xFFFFFF); break;
    case ICON_APP_PERMISSIONS: R(6, 4, 20, 24, 0x1E88E5); R(8, 6, 16, 20, 0xFFFFFF);
        R(10, 9, 12, 12, 0xF5F5F5);
        R(11, 10, 3, 3, 0x1E88E5); R(16, 10, 3, 3, 0x1E88E5); R(21, 10, 3, 3, 0x1E88E5);
        R(11, 14, 3, 3, 0x1E88E5); R(16, 14, 3, 3, 0x1E88E5); R(21, 14, 3, 3, 0x1E88E5);
        R(11, 18, 3, 3, 0x1E88E5); R(16, 18, 3, 3, 0x1E88E5); R(21, 18, 3, 3, 0x1E88E5); break;
    case ICON_WINDOWS_SECURITY: R(6, 4, 20, 24, 0x2E7D32); R(8, 6, 16, 20, 0xFFFFFF);
        L(12, 17, 15, 21, 0x2E7D32); L(15, 21, 20, 12, 0x2E7D32); break;
    case ICON_FIND_DEVICE: gfx_ring(x + h, y + h, 13 * s / 32, MAX(1, s / 10), 0x1E88E5);
        L(16, 4, 16, 28, 0x1E88E5); L(4, 16, 28, 16, 0x1E88E5);
        gfx_circle(x + h, y + h, 5 * s / 32, 0x1E88E5); break;
    case ICON_BITLOCKER: R(9, 6, 14, 10, 0x1E88E5); R(9, 6, 14, 1, 0xFFFFFF);
        R(9, 16, 14, 10, 0x1E88E5);
        gfx_circle(x + 16 * s / 32, y + 20 * s / 32, 3 * s / 32, 0xFFFFFF); break;
    case ICON_WINDOWS_UPDATE: gfx_ring(x + h, y + h, 13 * s / 32, MAX(1, s / 10), 0x1E88E5);
        gfx_line(x + 16 * s / 32, y + 4 * s / 32, x + 20 * s / 32, y + 8 * s / 32, 0x1E88E5);
        gfx_line(x + 16 * s / 32, y + 4 * s / 32, x + 20 * s / 32, y + 4 * s / 32, 0x1E88E5);
        gfx_line(x + 16 * s / 32, y + 28 * s / 32, x + 12 * s / 32, y + 24 * s / 32, 0x1E88E5);
        gfx_line(x + 16 * s / 32, y + 28 * s / 32, x + 12 * s / 32, y + 28 * s / 32, 0x1E88E5); break;
    case ICON_UPDATE_HISTORY: gfx_ring(x + h, y + h, 13 * s / 32, MAX(1, s / 10), 0x1E88E5);
        R(15, 5, 2, 12, 0x1E88E5); L(16, 17, 22, 12, 0x1E88E5); R(21, 11, 4, 4, 0x1E88E5); break;
    case ICON_ADVANCED_OPTIONS: R(11, 7, 10, 18, 0x607D8B); R(11, 7, 10, 1, 0xFFFFFF);
        R(7, 11, 5, 5, 0x607D8B); R(20, 11, 5, 5, 0x607D8B);
        R(7, 19, 5, 5, 0x607D8B); R(20, 19, 5, 5, 0x607D8B); break;

    /* Common UI */
    case ICON_CHEVRON_RIGHT: L(10, 8, 18, 16, 0xFFFFFF); L(18, 16, 10, 24, 0xFFFFFF); break;
    case ICON_CHEVRON_DOWN: L(8, 10, 16, 18, 0xFFFFFF); L(16, 18, 24, 10, 0xFFFFFF); break;
    case ICON_MORE_VERT: gfx_circle(x + 16 * s / 32, y + 8 * s / 32, 2 * s / 32, 0xFFFFFF);
        gfx_circle(x + 16 * s / 32, y + 16 * s / 32, 2 * s / 32, 0xFFFFFF);
        gfx_circle(x + 16 * s / 32, y + 24 * s / 32, 2 * s / 32, 0xFFFFFF); break;
    case ICON_REFRESH: gfx_ring(x + h, y + h, 13 * s / 32, MAX(1, s / 10), 0xFFFFFF);
        gfx_line(x + 16 * s / 32, y + 4 * s / 32, x + 20 * s / 32, y + 8 * s / 32, 0xFFFFFF);
        gfx_line(x + 16 * s / 32, y + 4 * s / 32, x + 20 * s / 32, y + 4 * s / 32, 0xFFFFFF); break;
    case ICON_ADD: R(14, 6, 4, 20, 0xFFFFFF); R(6, 14, 20, 4, 0xFFFFFF); break;
    case ICON_REMOVE: R(6, 14, 20, 4, 0xFFFFFF); break;
    case ICON_EDIT: R(3, 13, 26, 6, 0xFFFFFF); L(3, 16, 12, 25, 0xFFFFFF); L(21, 16, 12, 25, 0xFFFFFF); break;
    case ICON_SETTINGS_GEAR: gfx_ring(x + h, y + h, 11 * s / 32, 3 * s / 32, 0x90A4AE);
        R(14, 2, 4, 7, 0x90A4AE); R(14, 23, 4, 7, 0x90A4AE); R(2, 14, 7, 4, 0x90A4AE); R(23, 14, 7, 4, 0x90A4AE);
        R(5, 5, 5, 5, 0x90A4AE); R(22, 5, 5, 5, 0x90A4AE); R(5, 22, 5, 5, 0x90A4AE); R(22, 22, 5, 5, 0x90A4AE); break;
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
