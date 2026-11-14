/* File Explorer: sidebar, address bar, details view over the RAM filesystem. */
#include "os.h"

typedef struct { int cur, sel, scroll, hist[16], nhist, list[FS_MAX], n; } Exp;
#define TOP_H 72
#define SIDE_W 170
#define HDR_H 24
#define ROW_H 24

static const char *label_of(int node) { return node == 0 ? "Local Disk (C:)" : g_fs[node].name; }
static void refresh(Exp *e) {
    e->n = 0;
    if (e->cur == FS_THISPC) {
        if (g_user >= 0) { int tmp[FS_MAX], n = fs_list(g_users[g_user].home, tmp, FS_MAX); for (int i = 0; i < n && e->n < FS_MAX - 1; i++) e->list[e->n++] = tmp[i]; }
        e->list[e->n++] = 0;
        return;
    }
    int tmp[FS_MAX], n = fs_list(e->cur, tmp, FS_MAX);
    for (int i = 0; i < n; i++) if (!(e->cur == 0 && g_fs[tmp[i]].name[0] == '$')) e->list[e->n++] = tmp[i];
}
static void go(Window *w, int node, bool push) {
    Exp *e = w->priv;
    if (push && e->nhist < 16) e->hist[e->nhist++] = e->cur;
    e->cur = node; e->sel = -1; e->scroll = 0; refresh(e);
    char p[96]; fs_path(node, p, sizeof p);
    char t[110]; ksnprintf(t, sizeof t, "%s - File Explorer", node == FS_THISPC ? "This PC" : node == 0 ? "Local Disk (C:)" : g_fs[node].name);
    wm_set_title(w, t);
}
void app_explorer_open(Window *w, int node) { go(w, node, true); }
static void up(Window *w) { Exp *e = w->priv; if (e->cur == FS_THISPC) return; go(w, e->cur == 0 ? FS_THISPC : g_fs[e->cur].parent, true); }
static void open_item(Window *w, int node) {
    if (g_fs[node].dir) go(w, node, true); else app_launch(APP_NOTEPAD, node);
}
static void delete_sel(Window *w) {
    Exp *e = w->priv;
    if (e->sel < 0 || e->sel >= e->n || e->cur == FS_THISPC) return;
    fs_trash(e->list[e->sel]); refresh(e); e->sel = -1;
}
static const char *type_of(int node) {
    if (node == 0) return "Local Disk";
    if (g_fs[node].dir) return "File folder";
    int l = (int)strlen(g_fs[node].name);
    if (l > 4 && !kstricmp(g_fs[node].name + l - 4, ".txt")) return "Text Document";
    return "File";
}
static int side_items(int *nodes, const char **labels) {
    int n = 0;
    if (g_user >= 0) {
        static const char *nm[] = { "Desktop", "Documents", "Downloads", "Pictures" };
        for (int i = 0; i < 4; i++) { int f = fs_find(g_users[g_user].home, nm[i]); if (f >= 0) { nodes[n] = f; labels[n++] = nm[i]; } }
    }
    nodes[n] = FS_THISPC; labels[n++] = "This PC";
    nodes[n] = 0; labels[n++] = "Local Disk (C:)";
    return n;
}
static void ex_draw(Window *w, int cx, int cy, int cw, int ch) {
    Exp *e = w->priv;
    gfx_rect(cx, cy, cw, ch, 0xFFFFFF);
    gfx_rect(cx, cy, cw, TOP_H, 0xF5F6F7); gfx_hline(cx, cy + TOP_H - 1, cw, 0xDADCE0);
    bool hb = ui_hover(w, cx + 6, cy + 8, 28, 28);
    if (hb) gfx_rect(cx + 6, cy + 8, 28, 28, 0xDDE9F5);
    icon_draw(ICON_BACK, cx + 8, cy + 10, 24);
    if (ui_hover(w, cx + 38, cy + 8, 28, 28)) gfx_rect(cx + 38, cy + 8, 28, 28, 0xDDE9F5);
    icon_draw(ICON_UP, cx + 40, cy + 10, 24);
    gfx_rect(cx + 72, cy + 9, cw - 80, 26, 0xFFFFFF); gfx_frame(cx + 72, cy + 9, cw - 80, 26, 0xC8C8C8);
    char p[110], t[110]; fs_path(e->cur, p, sizeof p); text_fit(&font_ui, p, cw - 100, t, sizeof t);
    text_draw_vc(&font_ui, cx + 82, cy + 9, 26, t, 0x000000);
    const char *btn[2] = { "New folder", "Delete" }; int bx = cx + 8;
    for (int i = 0; i < 2; i++) {
        int bw = text_width(&font_ui, btn[i]) + 44;
        if (ui_hover(w, bx, cy + 40, bw, 28)) gfx_rect(bx, cy + 40, bw, 28, 0xDDE9F5);
        icon_draw(i ? ICON_TRASH : ICON_NEWFOLDER, bx + 8, cy + 44, 20);
        text_draw_vc(&font_ui, bx + 34, cy + 40, 28, btn[i], 0x000000);
        bx += bw + 6;
    }
    /* sidebar */
    int nodes[8]; const char *labels[8]; int sn = side_items(nodes, labels);
    gfx_vline(cx + SIDE_W - 1, cy + TOP_H, ch - TOP_H, 0xE6E6E6);
    text_draw(&font_ui_b, cx + 12, cy + TOP_H + 10, "Quick access", 0x1A1A1A);
    for (int i = 0; i < sn; i++) {
        int y = cy + TOP_H + 34 + i * 26 + (i >= sn - 2 ? 12 : 0);
        bool cur = nodes[i] == e->cur;
        if (cur) gfx_rect(cx + 2, y, SIDE_W - 6, 26, 0xCCE8FF);
        else if (ui_hover(w, cx + 2, y, SIDE_W - 6, 26)) gfx_rect(cx + 2, y, SIDE_W - 6, 26, 0xE5F3FF);
        icon_draw(nodes[i] == FS_THISPC ? ICON_PC : nodes[i] == 0 ? ICON_DRIVE : ICON_FOLDER, cx + 12, y + 5, 16);
        text_draw_vc(&font_ui, cx + 36, y, 26, labels[i], 0x000000);
    }
    /* list */
    int lx = cx + SIDE_W, lw = cw - SIDE_W, ly = cy + TOP_H;
    int c2 = lx + lw - 230, c3 = lx + lw - 80;
    text_draw_vc(&font_ui, lx + 40, ly, HDR_H, "Name", 0x555555);
    text_draw_vc(&font_ui, c2, ly, HDR_H, "Type", 0x555555);
    text_draw_vc(&font_ui, c3, ly, HDR_H, "Size", 0x555555);
    gfx_hline(lx, ly + HDR_H - 1, lw, 0xE6E6E6);
    int rows = (ch - TOP_H - HDR_H - 24) / ROW_H;
    Clip o = gfx_clip_push(lx, ly + HDR_H, lw, ch - TOP_H - HDR_H - 24);
    for (int i = e->scroll; i < e->n && i < e->scroll + rows + 1; i++) {
        int y = ly + HDR_H + (i - e->scroll) * ROW_H, node = e->list[i];
        if (i == e->sel) { gfx_rect(lx + 2, y, lw - 4, ROW_H, 0xCCE8FF); gfx_frame(lx + 2, y, lw - 4, ROW_H, 0x99D1FF); }
        else if (ui_hover(w, lx + 2, y, lw - 4, ROW_H)) gfx_rect(lx + 2, y, lw - 4, ROW_H, 0xE5F3FF);
        icon_draw(node == 0 ? ICON_DRIVE : g_fs[node].dir ? ICON_FOLDER : ICON_FILE, lx + 12, y + 4, 16);
        char nm[48]; text_fit(&font_ui, label_of(node), c2 - lx - 50, nm, sizeof nm);
        text_draw_vc(&font_ui, lx + 36, y, ROW_H, nm, 0x000000);
        text_draw_vc(&font_ui, c2, y, ROW_H, type_of(node), 0x555555);
        if (!g_fs[node].dir) { char sz[24]; ksnprintf(sz, sizeof sz, "%d B", g_fs[node].size); text_draw_vc(&font_ui, c3, y, ROW_H, sz, 0x555555); }
    }
    gfx_clip_pop(o);
    if (!e->n) text_draw_c(&font_ui, lx, lw, ly + 60, "This folder is empty.", 0x777777);
    char st[40]; ksnprintf(st, sizeof st, "%d item%s", e->n, e->n == 1 ? "" : "s");
    gfx_rect(cx, cy + ch - 24, cw, 24, 0xF5F6F7);
    text_draw_vc(&font_ui, cx + 10, cy + ch - 24, 24, st, 0x444444);
}
static void ex_mouse(Window *w, int ev, int x, int y, int btn) {
    Exp *e = w->priv; (void)btn;
    if (ev == M_WHEEL) { e->scroll = CLAMP(e->scroll - btn * 3, 0, MAX(0, e->n - 3)); return; }
    if (ev != M_DOWN && ev != M_DBL) return;
    int cw = w->w - 2, ch = w->h - TITLE_H - 1;
    if (ev == M_DOWN && y < TOP_H) {
        if (in_rect(x, y, 6, 8, 28, 28) && e->nhist) { go(w, e->hist[--e->nhist], false); return; }
        if (in_rect(x, y, 38, 8, 28, 28)) { up(w); return; }
        if (y >= 40) {
            int bx = 8;
            for (int i = 0; i < 2; i++) {
                int bw = text_width(&font_ui, i ? "Delete" : "New folder") + 44;
                if (in_rect(x, y, bx, 40, bw, 28)) {
                    if (i == 0 && e->cur >= 0) { fs_unique(e->cur, "New folder", "", true); refresh(e); }
                    else if (i == 1) delete_sel(w);
                }
                bx += bw + 6;
            }
        }
        return;
    }
    if (x < SIDE_W) {
        int nodes[8]; const char *labels[8]; int sn = side_items(nodes, labels);
        for (int i = 0; i < sn; i++) { int yy = TOP_H + 34 + i * 26 + (i >= sn - 2 ? 12 : 0); if (in_rect(x, y, 2, yy, SIDE_W - 6, 26)) go(w, nodes[i], true); }
        return;
    }
    int row = (y - TOP_H - HDR_H) / ROW_H + e->scroll;
    if (y < TOP_H + HDR_H || y > ch - 24 || row < 0 || row >= e->n) { if (ev == M_DOWN) e->sel = -1; return; }
    (void)cw;
    if (ev == M_DOWN) e->sel = row;
    else open_item(w, e->list[row]);
}
static void ex_key(Window *w, int key, int mod) {
    Exp *e = w->priv; (void)mod;
    if (key == K_DOWN) e->sel = MIN(e->n - 1, e->sel + 1);
    else if (key == K_UP) e->sel = MAX(0, e->sel - 1);
    else if (key == K_ENTER && e->sel >= 0 && e->sel < e->n) open_item(w, e->list[e->sel]);
    else if (key == K_DEL) delete_sel(w);
    else if (key == K_BACKSPACE) up(w);
    else if (key == K_F5) refresh(e);
    int rows = (w->h - TITLE_H - 1 - TOP_H - HDR_H - 24) / ROW_H;
    if (e->sel >= 0) { if (e->sel < e->scroll) e->scroll = e->sel; if (e->sel >= e->scroll + rows) e->scroll = e->sel - rows + 1; }
}
static void ex_close(Window *w) { kfree(w->priv); }
Window *explorer_new(int node) {
    int x, y; win_cascade(&x, &y);
    Window *w = wm_create(APP_EXPLORER, "File Explorer", ICON_FOLDER, x, y, 760, 480);
    if (!w) return 0;
    w->priv = kmalloc(sizeof(Exp)); w->draw = ex_draw; w->mouse = ex_mouse; w->key = ex_key; w->close = ex_close;
    w->minw = 440; w->minh = 260;
    ((Exp *)w->priv)->cur = FS_THISPC; ((Exp *)w->priv)->sel = -1;
    go(w, node, false);
    return w;
}
