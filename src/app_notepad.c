/* Notepad: wrapped monospace editor, menu bar, save to the RAM filesystem. */
#include "os.h"
#define NP_CAP FS_FILE_MAX
#define NP_LINES 4096
#define LH 17
#define MENU_H 22
#define STAT_H 22

typedef struct { char *buf; int len, cur, top, node; bool modified; int menu; bool about; int starts[NP_LINES], nlines; int cols; } Np;

static void title(Window *w) {
    Np *n = w->priv; char t[64];
    ksnprintf(t, sizeof t, "%s%s - Notepad", n->modified ? "*" : "", n->node >= 0 ? g_fs[n->node].name : "Untitled");
    wm_set_title(w, t);
}
static void layout(Np *n, int cols) {
    n->cols = MAX(8, cols); n->nlines = 0; n->starts[n->nlines++] = 0;
    int col = 0;
    for (int i = 0; i < n->len && n->nlines < NP_LINES - 1; i++) {
        if (n->buf[i] == '\n') { n->starts[n->nlines++] = i + 1; col = 0; }
        else if (++col >= n->cols) { n->starts[n->nlines++] = i + 1; col = 0; }
    }
}
static int row_of(Np *n, int idx) { int r = 0; for (int i = 0; i < n->nlines; i++) if (n->starts[i] <= idx) r = i; return r; }
static int row_end(Np *n, int r) { int e = r + 1 < n->nlines ? n->starts[r + 1] : n->len; if (r + 1 < n->nlines && n->buf[e - 1] == '\n') e--; return e; }
static void visible(Np *n, int rows) {
    int r = row_of(n, n->cur);
    if (r < n->top) n->top = r;
    if (r >= n->top + rows) n->top = r - rows + 1;
    if (n->top < 0) n->top = 0;
}
static void geom(Window *w, int *cols, int *rows) { *cols = (w->w - 2 - 12) / 8; *rows = (w->h - TITLE_H - 1 - MENU_H - STAT_H) / LH; }
static void save(Window *w) {
    Np *n = w->priv;
    if (n->node < 0) {
        int d = g_user >= 0 ? user_docs(g_user) : 0;
        n->node = fs_unique(d >= 0 ? d : 0, "Untitled", ".txt", false);
        if (n->node < 0) return;
    }
    if (fs_write(n->node, n->buf, n->len)) n->modified = false;
    title(w);
}
static void reset(Window *w) { Np *n = w->priv; n->len = n->cur = n->top = 0; n->buf[0] = 0; n->node = -1; n->modified = false; title(w); }
static void insert(Np *n, char c) {
    if (n->len >= NP_CAP - 1) return;
    memmove(n->buf + n->cur + 1, n->buf + n->cur, (size_t)(n->len - n->cur + 1));
    n->buf[n->cur++] = c; n->len++; n->modified = true;
}
static void np_key(Window *w, int key, int mod) {
    Np *n = w->priv; int cols, rows; geom(w, &cols, &rows); layout(n, cols);
    n->menu = 0; n->about = false;
    int r = row_of(n, n->cur), col = n->cur - n->starts[r];
    if (mod & MOD_CTRL) { if (key == 's') save(w); else if (key == 'n') reset(w); return; }
    if (key == K_LEFT) { if (n->cur) n->cur--; }
    else if (key == K_RIGHT) { if (n->cur < n->len) n->cur++; }
    else if (key == K_UP) { if (r > 0) n->cur = MIN(n->starts[r - 1] + col, row_end(n, r - 1)); }
    else if (key == K_DOWN) { if (r + 1 < n->nlines) n->cur = MIN(n->starts[r + 1] + col, row_end(n, r + 1)); }
    else if (key == K_HOME) n->cur = n->starts[r];
    else if (key == K_END) n->cur = row_end(n, r);
    else if (key == K_PGUP) { int t = MAX(0, r - rows); n->cur = MIN(n->starts[t] + col, row_end(n, t)); }
    else if (key == K_PGDN) { int t = MIN(n->nlines - 1, r + rows); n->cur = MIN(n->starts[t] + col, row_end(n, t)); }
    else if (key == K_BACKSPACE) { if (n->cur) { memmove(n->buf + n->cur - 1, n->buf + n->cur, (size_t)(n->len - n->cur + 1)); n->cur--; n->len--; n->modified = true; } }
    else if (key == K_DEL) { if (n->cur < n->len) { memmove(n->buf + n->cur, n->buf + n->cur + 1, (size_t)(n->len - n->cur)); n->len--; n->modified = true; } }
    else if (key == K_ENTER) insert(n, '\n');
    else if (key == K_TAB) { for (int i = 0; i < 4; i++) insert(n, ' '); }
    else if (key >= 32 && key < 127) insert(n, (char)key);
    layout(n, cols); visible(n, rows); title(w);
}
static const char *const menus[3] = { "File", "Edit", "Help" };
static const char *const items[3][3] = { { "New        Ctrl+N", "Save       Ctrl+S", "Exit" }, { "Clear all", 0, 0 }, { "About Notepad", 0, 0 } };
static int mx_of(int m) { int x = 4; for (int i = 0; i < m; i++) x += text_width(&font_ui, menus[i]) + 20; return x; }
static void np_draw(Window *w, int cx, int cy, int cw, int ch) {
    Np *n = w->priv; int cols, rows; geom(w, &cols, &rows); layout(n, cols); visible(n, rows);
    gfx_rect(cx, cy, cw, ch, 0xFFFFFF);
    int ty = cy + MENU_H;
    for (int i = n->top; i < n->nlines && i < n->top + rows + 1; i++) {
        int s = n->starts[i], e = row_end(n, i);
        text_draw_n(&font_mono, cx + 6, ty + (i - n->top) * LH, n->buf + s, e - s, 0x000000);
    }
    int r = row_of(n, n->cur), col = n->cur - n->starts[r];
    if (ui_blink() && r >= n->top && r < n->top + rows + 1) gfx_rect(cx + 6 + col * 8, ty + (r - n->top) * LH + 1, 1, LH - 2, 0x000000);
    if (n->nlines > rows) {
        int th = MAX(20, (ch - MENU_H - STAT_H) * rows / n->nlines), tp = (ch - MENU_H - STAT_H - th) * n->top / MAX(1, n->nlines - rows);
        gfx_rect(cx + cw - 8, ty + tp, 6, th, 0xC8C8C8);
    }
    gfx_rect(cx, cy, cw, MENU_H, 0xFFFFFF);
    for (int i = 0; i < 3; i++) {
        int x = cx + mx_of(i), tw = text_width(&font_ui, menus[i]) + 16;
        if (n->menu == i + 1) gfx_rect(x - 2, cy, tw, MENU_H, 0xCCE8FF); else if (ui_hover(w, x - 2, cy, tw, MENU_H)) gfx_rect(x - 2, cy, tw, MENU_H, 0xE5F3FF);
        text_draw_vc(&font_ui, x + 6, cy, MENU_H, menus[i], 0x000000);
    }
    gfx_rect(cx, cy + ch - STAT_H, cw, STAT_H, 0xF0F0F0); gfx_hline(cx, cy + ch - STAT_H, cw, 0xDDDDDD);
    char s[40]; ksnprintf(s, sizeof s, "Ln %d, Col %d", r + 1, col + 1);
    text_draw_vc(&font_ui, cx + cw - 360, cy + ch - STAT_H, STAT_H, s, 0x333333);
    text_draw_vc(&font_ui, cx + cw - 215, cy + ch - STAT_H, STAT_H, "100%   Windows (CRLF)   UTF-8", 0x333333);
    if (n->menu) {
        int m = n->menu - 1, x = cx + mx_of(m) - 2, cnt = 0; while (cnt < 3 && items[m][cnt]) cnt++;
        gfx_rect(x, cy + MENU_H, 190, cnt * 26 + 6, 0xF2F2F2); gfx_frame(x, cy + MENU_H, 190, cnt * 26 + 6, 0xA0A0A0);
        for (int i = 0; i < cnt; i++) {
            int y = cy + MENU_H + 3 + i * 26;
            if (ui_hover(w, x + 2, y, 186, 26)) gfx_rect(x + 2, y, 186, 26, 0xD6D6D6);
            text_draw_vc(&font_ui, x + 14, y, 26, items[m][i], 0x000000);
        }
    }
    if (n->about) {
        int bx = cx + cw / 2 - 150, by = cy + ch / 2 - 60;
        gfx_rect(bx, by, 300, 120, 0xF0F0F0); gfx_frame(bx, by, 300, 120, g_accent);
        text_draw_c(&font_ui_b, bx, 300, by + 20, "Notepad for Wimboes", 0x000000);
        text_draw_c(&font_ui, bx, 300, by + 48, "Part of the MikeHawkSoft Wimboes OS", 0x333333);
        text_draw_c(&font_ui, bx, 300, by + 90, "(click to close)", 0x777777);
    }
}
static void np_mouse(Window *w, int ev, int x, int y, int btn) {
    Np *n = w->priv; int cols, rows; geom(w, &cols, &rows); layout(n, cols);
    if (ev == M_WHEEL) { n->top = CLAMP(n->top - btn * 3, 0, MAX(0, n->nlines - rows)); return; }
    if (ev != M_DOWN) return;
    if (n->about) { n->about = false; return; }
    if (n->menu) {
        int m = n->menu - 1, mx = mx_of(m) - 2, cnt = 0; while (cnt < 3 && items[m][cnt]) cnt++;
        n->menu = 0;
        if (in_rect(x, y, mx, MENU_H, 190, cnt * 26 + 6)) {
            int i = (y - MENU_H - 3) / 26;
            if (m == 0 && i == 0) reset(w); else if (m == 0 && i == 1) save(w); else if (m == 0 && i == 2) wm_close(w);
            else if (m == 1) { n->len = n->cur = 0; n->buf[0] = 0; n->modified = true; title(w); }
            else if (m == 2) n->about = true;
        }
        return;
    }
    if (y < MENU_H) { for (int i = 0; i < 3; i++) if (in_rect(x, y, mx_of(i) - 2, 0, text_width(&font_ui, menus[i]) + 16, MENU_H)) n->menu = i + 1; return; }
    int r = CLAMP((y - MENU_H) / LH + n->top, 0, n->nlines - 1), c = MAX(0, (x - 6 + 4) / 8);
    n->cur = MIN(n->starts[r] + c, row_end(n, r));
}
static void np_close(Window *w) { Np *n = w->priv; kfree(n->buf); kfree(n); }
Window *notepad_new(int node) {
    int x, y; win_cascade(&x, &y);
    Window *w = wm_create(APP_NOTEPAD, "Untitled - Notepad", ICON_NOTEPAD, x, y, 640, 420);
    if (!w) return 0;
    Np *n = kmalloc(sizeof(Np));
    if (n) n->buf = kmalloc(NP_CAP + 1);
    if (!n || !n->buf) { if (n) kfree(n); wm_close(w); return 0; }
    w->priv = n; w->draw = np_draw; w->mouse = np_mouse; w->key = np_key; w->close = np_close; w->minw = 320;
    n->node = -1;
    if (node >= 0 && g_fs[node].used && !g_fs[node].dir) {
        n->node = node;
        for (int i = 0; i < g_fs[node].size && n->len < NP_CAP - 1; i++) if (g_fs[node].data[i] != '\r') n->buf[n->len++] = g_fs[node].data[i];
    }
    title(w);
    return w;
}
