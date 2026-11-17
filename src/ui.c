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
    case ICON_GAME: gfx_ring(x + h, y + h, 13 * s / 32, 2 * s / 32, 0x34C759);
        gfx_rect(x + 6 * s / 32, y + 10 * s / 32, 4 * s / 32, 6 * s / 32, 0x34C759);
        gfx_rect(x + 22 * s / 32, y + 10 * s / 32, 4 * s / 32, 6 * s / 32, 0x34C759);
        gfx_circle(x + 20 * s / 32, y + 12 * s / 32, 2 * s / 32, 0xFF3B30);
        gfx_circle(x + 12 * s / 32, y + 12 * s / 32, 2 * s / 32, 0x007AFF); break;

    /* System */
    case ICON_DISPLAY: R(4, 4, 24, 18, 0xC9CED6); R(6, 6, 20, 14, 0x0C0C0C);
        R(13, 24, 6, 2, 0x8E96A3); R(10, 26, 12, 2, 0xC9CED6);
        R(14, 28, 4, 2, 0x6A727C); R(14, 10, 4, 4, 0xE53935); break; /* monitor with ruler mark */
    case ICON_SOUND: R(8, 14, 8, 4, 0xFFFFFF); R(16, 10, 6, 12, 0xFFFFFF);
        gfx_line(x + 22 * s / 32, y + 7 * s / 32, x + 26 * s / 32, y + 4 * s / 32, 0xFFFFFF);
        gfx_line(x + 23 * s / 32, y + 9 * s / 32, x + 28 * s / 32, y + 4 * s / 32, 0xFFFFFF);
        gfx_line(x + 24 * s / 32, y + 11 * s / 32, x + 30 * s / 32, y + 6 * s / 32, 0xFFFFFF);
        gfx_line(x + 25 * s / 32, y + 13 * s / 32, x + 32 * s / 32, y + 10 * s / 32, 0xFFFFFF);
        gfx_line(x + 24 * s / 32, y + 19 * s / 32, x + 30 * s / 32, y + 26 * s / 32, 0xFFFFFF);
        gfx_line(x + 23 * s / 32, y + 21 * s / 32, x + 28 * s / 32, y + 28 * s / 32, 0xFFFFFF);
        gfx_line(x + 22 * s / 32, y + 25 * s / 32, x + 26 * s / 32, y + 30 * s / 32, 0xFFFFFF); break; /* speaker with waves */
    case ICON_NOTIFICATIONS: R(14, 5, 4, 14, 0xFFFFFF); R(12, 19, 8, 4, 0xFFFFFF);
        R(11, 23, 10, 2, 0xFFFFFF); gfx_circle(x + 16 * s / 32, y + 27 * s / 32, 2 * s / 32 + 1, 0xFFFFFF); break; /* bell */
    case ICON_POWER_BATTERY: R(10, 6, 16, 20, 0xFFFFFF); R(15, 4, 6, 3, 0xFFFFFF);
        R(12, 8, 12, 14, 0x4CAF50); R(12, 8, 4, 14, 0x81C784); break; /* battery with lightning (green fill) */
    case ICON_STORAGE: R(6, 8, 20, 6, 0xC9CED6); R(6, 16, 20, 6, 0xC9CED6); R(6, 24, 20, 6, 0xC9CED6);
        R(5, 7, 22, 2, 0x8E96A3); R(5, 15, 22, 2, 0x8E96A3); R(5, 23, 22, 2, 0x8E96A3); break; /* stack of disks */
    case ICON_ABOUT: gfx_ring(x + h, y + h, 12 * s / 32, MAX(1, s / 12), 0x0078D7);
        R(15, 8, 2, 10, 0x0078D7); R(15, 22, 2, 2, 0x0078D7); break; /* info circle */
    case ICON_THEMES: gfx_circle(x + 9 * s / 32, y + 9 * s / 32, 5 * s / 32 + 1, 0xE53935);
        gfx_circle(x + 23 * s / 32, y + 9 * s / 32, 5 * s / 32 + 1, 0x43A047);
        gfx_circle(x + 9 * s / 32, y + 23 * s / 32, 5 * s / 32 + 1, 0x1E88E5);
        gfx_circle(x + 23 * s / 32, y + 23 * s / 32, 5 * s / 32 + 1, 0xFDD835);
        gfx_circle(x + h, y + h, 4 * s / 32 + 1, 0xFFFFFF); break; /* paint palette */
    case ICON_LOCK_SCREEN: R(10, 6, 12, 10, 0xFFFFFF); R(10, 6, 12, 1, 0x0078D7);
        R(10, 16, 12, 10, 0x0078D7); gfx_circle(x + 16 * s / 32, y + 20 * s / 32, 2 * s / 32 + 1, 0xFFFFFF); break; /* lock on screen */
    case ICON_START: logo_draw(x, y, s, 0xFFFFFF); break; /* Windows logo / 4 squares */
    case ICON_TASKBAR_SETTINGS: R(3, 20, 26, 8, 0xC9CED6); R(3, 20, 26, 1, 0x8E96A3);
        R(13, 8, 6, 12, 0x9AA0A6); R(11, 10, 10, 1, 0x9AA0A6); R(11, 16, 10, 1, 0x9AA0A6);
        R(11, 12, 3, 4, 0x9AA0A6); R(18, 12, 3, 4, 0x9AA0A6); break; /* taskbar with gear */
    case ICON_APPS: for (int j = 0; j < 3; j++) for (int i = 0; i < 3; i++)
        R(6 + i * 8, 6 + j * 8, 6, 6, (i + j) % 2 == 0 ? 0x0078D7 : 0xFFFFFF); break; /* grid of squares */
    case ICON_DEFAULT_APPS: R(8, 12, 8, 8, 0xFFFFFF); R(16, 12, 8, 8, 0xFFFFFF);
        gfx_line(x + 16 * s / 32, y + 12 * s / 32, x + 24 * s / 32, y + 4 * s / 32, 0xFFFFFF);
        gfx_line(x + 16 * s / 32, y + 20 * s / 32, x + 24 * s / 32, y + 28 * s / 32, 0xFFFFFF); break; /* link/chain */
    case ICON_STARTUP: R(14, 8, 4, 16, 0xFFFFFF); gfx_line(x + 16 * s / 32, y + 24 * s / 32, x + 10 * s / 32, y + 16 * s / 32, 0xFFFFFF);
        gfx_line(x + 16 * s / 32, y + 24 * s / 32, x + 22 * s / 32, y + 16 * s / 32, 0xFFFFFF); break; /* rocket/play triangle */
    case ICON_OFFLINE_MAPS: R(14, 8, 4, 12, 0xE53935); gfx_circle(x + 16 * s / 32, y + 22 * s / 32, 4 * s / 32 + 1, 0xE53935);
        R(15, 12, 2, 4, 0xFFFFFF); break; /* map pin */

    /* Accounts */
    case ICON_YOUR_INFO: R(6, 4, 20, 24, 0xFFFFFF); R(6, 4, 20, 8, 0x0078D7);
        gfx_circle(x + h, y + 12 * s / 32, 4 * s / 32 + 1, 0xFFFFFF);
        R(8, 20, 16, 8, 0xE8EBF0); break; /* user card */
    case ICON_SIGNIN_OPTIONS: R(10, 8, 12, 16, 0xFFFFFF); R(13, 10, 6, 8, 0xE8EBF0);
        R(14, 12, 4, 4, 0x0078D7); R(14, 18, 4, 2, 0x0078D7); break; /* key/fingerprint card */
    case ICON_FAMILY: gfx_circle(x + 9 * s / 32, y + 10 * s / 32, 4 * s / 32 + 1, 0xFFFFFF);
        gfx_circle(x + 23 * s / 32, y + 10 * s / 32, 4 * s / 32 + 1, 0xFFFFFF);
        gfx_circle(x + h, y + 24 * s / 32, 5 * s / 32 + 1, 0xFFFFFF);
        gfx_circle(x + 9 * s / 32, y + 20 * s / 32, 3 * s / 32 + 1, 0x81D4FA);
        gfx_circle(x + 23 * s / 32, y + 20 * s / 32, 3 * s / 32 + 1, 0xF8BBD0); break; /* group of users */

    /* Backup */
    case ICON_BACKUP: R(8, 12, 16, 12, 0xFFFFFF); R(8, 12, 16, 6, 0x81D4FA);
        gfx_circle(x + h, y + 10 * s / 32, 5 * s / 32 + 1, 0x81D4FA);
        R(14, 6, 4, 8, 0xFFFFFF); L(12, 10, 16, 6, 0xFFFFFF); L(16, 10, 20, 6, 0xFFFFFF); break; /* cloud with arrow */

    /* Time & Language */
    case ICON_DATE_TIME: R(6, 4, 20, 20, 0xFFFFFF); R(6, 4, 20, 6, 0xE53935);
        R(9, 12, 4, 8, 0xE53935); R(19, 12, 4, 8, 0x0078D7);
        gfx_circle(x + h, y + 16 * s / 32, 8 * s / 32, 0xE8EBF0);
        L(16, 16, 16, 10, 0xE53935); L(16, 16, 22, 16, 0x0078D7); break; /* calendar + clock */
    case ICON_LANGUAGE: gfx_ring(x + h, y + h, 12 * s / 32, MAX(1, s / 10), 0x0078D7);
        R(14, 10, 4, 12, 0x0078D7); L(10, 22, 22, 10, 0x0078D7); break; /* globe */
    case ICON_TYPING: R(4, 16, 24, 12, 0xC9CED6); R(6, 18, 20, 8, 0xFFFFFF);
        for (int i = 0; i < 10; i++) R(6 + i * 2, 18, 1, 8, i % 2 == 0 ? 0x0078D7 : 0xFFFFFF); break; /* keyboard */
    case ICON_SPEECH: R(10, 8, 12, 14, 0x0078D7); R(14, 22, 4, 4, 0x0078D7);
        L(16, 22, 20, 26, 0x0078D7); L(16, 22, 20, 18, 0x0078D7); break; /* microphone */

    /* Gaming */
    case ICON_GAMING: gfx_ring(x + h, y + h, 14 * s / 32, 2 * s / 32, 0x512DA8);
        gfx_rect(x + 10 * s / 32, y + 12 * s / 32, 4 * s / 32, 6 * s / 32, 0x512DA8);
        gfx_rect(x + 18 * s / 32, y + 12 * s / 32, 4 * s / 32, 6 * s / 32, 0x512DA8);
        gfx_circle(x + 13 * s / 32, y + 22 * s / 32, 2 * s / 32 + 1, 0x512DA8);
        gfx_circle(x + 19 * s / 32, y + 22 * s / 32, 2 * s / 32 + 1, 0x512DA8);
        gfx_circle(x + 16 * s / 32, y + 16 * s / 32, 2 * s / 32 + 1, 0xFFFFFF); break; /* controller */
    case ICON_CAPTURES: R(8, 10, 16, 14, 0x333333); R(10, 12, 12, 10, 0x000000);
        gfx_circle(x + h, y + 17 * s / 32, 4 * s / 32 + 1, 0xFF3B30);
        R(13, 15, 6, 4, 0xFFFFFF); break; /* camera */
    case ICON_GAME_BAR: gfx_ring(x + 10 * s / 32, y + 16 * s / 32, 6 * s / 32, MAX(1, s / 16), 0x512DA8);
        gfx_ring(x + 22 * s / 32, y + 16 * s / 32, 6 * s / 32, MAX(1, s / 16), 0x512DA8);
        R(10, 16, 12, 6, 0x512DA8); R(14, 8, 4, 8, 0x512DA8); R(20, 8, 4, 8, 0x512DA8); break; /* gamepad + bar */

    /* Accessibility */
    case ICON_ACCESSIBILITY: gfx_circle(x + h, y + 8 * s / 32, 4 * s / 32 + 1, 0x0078D7);
        R(14, 14, 4, 12, 0x0078D7);
        L(10, 22, 16, 14, 0x0078D7); L(18, 14, 22, 22, 0x0078D7); break; /* human figure with arms */
    case ICON_VISION: gfx_ring(x + h, y + h, 12 * s / 32, MAX(1, s / 12), 0x0078D7);
        gfx_circle(x + h, y + h, 5 * s / 32 + 1, 0x0078D7);
        gfx_circle(x + h, y + h, 2 * s / 32 + 1, 0xFFFFFF); break; /* eye */
    case ICON_HEARING: R(14, 6, 4, 20, 0x0078D7);
        gfx_line(x + 10 * s / 32, y + 26 * s / 32, x + 18 * s / 32, y + 26 * s / 32, 0x0078D7);
        gfx_line(x + 8 * s / 32, y + 8 * s / 32, x + 14 * s / 32, y + 6 * s / 32, 0x0078D7);
        gfx_line(x + 6 * s / 32, y + 12 * s / 32, x + 12 * s / 32, y + 10 * s / 32, 0x0078D7);
        gfx_line(x + 22 * s / 32, y + 8 * s / 32, x + 24 * s / 32, y + 6 * s / 32, 0x0078D7);
        gfx_line(x + 20 * s / 32, y + 12 * s / 32, x + 26 * s / 32, y + 10 * s / 32, 0x0078D7); break; /* ear with waves */
    case ICON_INTERACTION: R(10, 10, 12, 20, 0xFFFFFF); R(10, 10, 12, 4, 0x0078D7);
        gfx_circle(x + 10 * s / 32, y + 14 * s / 32, 3 * s / 32 + 1, 0x0078D7);
        gfx_circle(x + 22 * s / 32, y + 14 * s / 32, 3 * s / 32 + 1, 0x0078D7);
        gfx_circle(x + 16 * s / 32, y + 26 * s / 32, 3 * s / 32 + 1, 0x0078D7); break; /* hand/touch */

    /* Privacy & Security */
    case ICON_PRIVACY: R(8, 6, 16, 18, 0x0078D7); R(10, 8, 12, 14, 0xFFFFFF);
        gfx_circle(x + h, y + 14 * s / 32, 5 * s / 32 + 1, 0x0078D7);
        R(15, 12, 2, 8, 0xFFFFFF); R(15, 22, 2, 2, 0xFFFFFF); break; /* shield with lock */
    case ICON_APP_PERMISSIONS: R(8, 6, 16, 18, 0x0078D7); R(10, 8, 12, 14, 0xFFFFFF);
        R(12, 10, 8, 8, 0xE8EBF0); R(13, 11, 2, 2, 0x0078D7); R(17, 11, 2, 2, 0x0078D7);
        R(13, 15, 2, 2, 0x0078D7); R(17, 15, 2, 2, 0x0078D7); break; /* shield with apps grid */
    case ICON_WINDOWS_SECURITY: R(8, 6, 16, 18, 0x4CAF50); R(10, 8, 12, 14, 0xFFFFFF);
        L(12, 16, 15, 20, 0x4CAF50); L(15, 20, 20, 12, 0x4CAF50); break; /* shield with check */
    case ICON_FIND_DEVICE: gfx_ring(x + h, y + h, 12 * s / 32, MAX(1, s / 12), 0x0078D7);
        L(16, 4, 16, 28, 0x0078D7); L(4, 16, 28, 16, 0x0078D7);
        gfx_circle(x + h, y + h, 4 * s / 32 + 1, 0x0078D7); break; /* radar/location */
    case ICON_BITLOCKER: R(10, 6, 12, 10, 0x0078D7); R(10, 6, 12, 1, 0xFFFFFF);
        R(10, 16, 12, 10, 0x0078D7); gfx_circle(x + 16 * s / 32, y + 20 * s / 32, 2 * s / 32 + 1, 0xFFFFFF); break; /* lock */
    case ICON_WINDOWS_UPDATE: gfx_ring(x + h, y + h, 12 * s / 32, MAX(1, s / 12), 0x0078D7);
        gfx_line(x + 16 * s / 32, y + 4 * s / 32, x + 20 * s / 32, y + 8 * s / 32, 0x0078D7);
        gfx_line(x + 16 * s / 32, y + 4 * s / 32, x + 20 * s / 32, y + 4 * s / 32, 0x0078D7);
        gfx_line(x + 16 * s / 32, y + 28 * s / 32, x + 12 * s / 32, y + 24 * s / 32, 0x0078D7);
        gfx_line(x + 16 * s / 32, y + 28 * s / 32, x + 12 * s / 32, y + 28 * s / 32, 0x0078D7); break; /* circular arrows */
    case ICON_UPDATE_HISTORY: gfx_ring(x + h, y + h, 12 * s / 32, MAX(1, s / 12), 0x0078D7);
        R(15, 6, 2, 10, 0x0078D7); L(16, 16, 22, 12, 0x0078D7); R(21, 11, 3, 3, 0x0078D7); break; /* clock with arrow */
    case ICON_ADVANCED_OPTIONS: R(12, 8, 8, 16, 0x9AA0A6); R(12, 8, 8, 1, 0xFFFFFF);
        R(8, 12, 4, 4, 0x9AA0A6); R(20, 12, 4, 4, 0x9AA0A6);
        R(8, 20, 4, 4, 0x9AA0A6); R(20, 20, 4, 4, 0x9AA0A6); break; /* gear + sliders */

    /* Common UI */
    case ICON_CHEVRON_RIGHT: L(10, 8, 18, 16, 0xFFFFFF); L(18, 16, 10, 24, 0xFFFFFF); break;
    case ICON_CHEVRON_DOWN: L(8, 10, 16, 18, 0xFFFFFF); L(16, 18, 24, 10, 0xFFFFFF); break;
    case ICON_MORE_VERT: gfx_circle(x + 16 * s / 32, y + 8 * s / 32, 2 * s / 32 + 1, 0xFFFFFF);
        gfx_circle(x + 16 * s / 32, y + 16 * s / 32, 2 * s / 32 + 1, 0xFFFFFF);
        gfx_circle(x + 16 * s / 32, y + 24 * s / 32, 2 * s / 32 + 1, 0xFFFFFF); break;
    case ICON_REFRESH: gfx_ring(x + h, y + h, 12 * s / 32, MAX(1, s / 12), 0xFFFFFF);
        gfx_line(x + 16 * s / 32, y + 4 * s / 32, x + 20 * s / 32, y + 8 * s / 32, 0xFFFFFF);
        gfx_line(x + 16 * s / 32, y + 4 * s / 32, x + 20 * s / 32, y + 4 * s / 32, 0xFFFFFF); break;
    case ICON_ADD: R(14, 6, 4, 20, 0xFFFFFF); R(6, 14, 20, 4, 0xFFFFFF); break; /* same as ICON_PLUS */
    case ICON_REMOVE: R(6, 14, 20, 4, 0xFFFFFF); R(15, 18, 2, 2, 0xFFFFFF); R(15, 22, 2, 2, 0xFFFFFF); R(15, 26, 2, 2, 0xFFFFFF); break; /* trash/minus */
    case ICON_EDIT: R(4, 14, 24, 4, 0xFFFFFF); L(4, 16, 12, 24, 0xFFFFFF); L(20, 16, 12, 24, 0xFFFFFF); break; /* pencil */
    case ICON_SETTINGS_GEAR: gfx_ring(x + h, y + h, 9 * s / 32 + 1, 4 * s / 32 + 1, 0x9AA0A6);
        R(14, 3, 4, 6, 0x9AA0A6); R(14, 23, 4, 6, 0x9AA0A6); R(3, 14, 6, 4, 0x9AA0A6); R(23, 14, 6, 4, 0x9AA0A6);
        R(6, 6, 4, 4, 0x9AA0A6); R(22, 6, 4, 4, 0x9AA0A6); R(6, 22, 4, 4, 0x9AA0A6); R(22, 22, 4, 4, 0x9AA0A6); break; /* same as ICON_GEAR */
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
