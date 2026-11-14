/* Task Manager: lists open windows, End task, heap usage. */
#include "os.h"
typedef struct { int sel_id; } Tm;
static void tm_draw(Window *w, int cx, int cy, int cw, int ch) {
    Tm *t = w->priv; char b[64];
    gfx_rect(cx, cy, cw, ch, 0xFFFFFF);
    gfx_rect(cx, cy, cw, 30, 0xF5F6F7); gfx_hline(cx, cy + 29, cw, 0xDADCE0);
    text_draw_vc(&font_ui, cx + 40, cy, 30, "Name", 0x555555); text_draw_vc(&font_ui, cx + cw - 200, cy, 30, "Status", 0x555555); text_draw_vc(&font_ui, cx + cw - 90, cy, 30, "PID", 0x555555);
    int y = cy + 30;
    for (int i = wm_zcount() - 1; i >= 0; i--, y += 28) {
        Window *x = wm_zget(i);
        if (x->id == t->sel_id) gfx_rect(cx, y, cw, 28, 0xCCE8FF); else if (ui_hover(w, cx, y, cw, 28)) gfx_rect(cx, y, cw, 28, 0xE5F3FF);
        icon_draw(g_apps[x->app].icon, cx + 12, y + 5, 18);
        char nm[48]; text_fit(&font_ui, x->title, cw - 280, nm, sizeof nm);
        text_draw_vc(&font_ui, cx + 40, y, 28, nm, 0x000000);
        text_draw_vc(&font_ui, cx + cw - 200, y, 28, x->minimized ? "Minimized" : "Running", 0x555555);
        ksnprintf(b, sizeof b, "%d", 1000 + x->id); text_draw_vc(&font_ui, cx + cw - 90, y, 28, b, 0x555555);
    }
    gfx_rect(cx, cy + ch - 44, cw, 44, 0xF5F6F7); gfx_hline(cx, cy + ch - 44, cw, 0xDADCE0);
    ksnprintf(b, sizeof b, "Windows: %d    Heap: %u / %u KB", wm_zcount(), (u32)(mem_used() / 1024), (u32)(mem_total() / 1024));
    text_draw_vc(&font_ui, cx + 12, cy + ch - 44, 44, b, 0x333333);
    ui_button(w, cx + cw - 110, cy + ch - 36, 98, 28, "End task", false);
}
static void tm_mouse(Window *w, int ev, int x, int y, int btn) {
    Tm *t = w->priv; (void)btn;
    if (ev != M_DOWN) return;
    int cw = w->w - 2, ch = w->h - TITLE_H - 1;
    if (in_rect(x, y, cw - 110, ch - 36, 98, 28)) {
        for (int i = 0; i < wm_zcount(); i++) if (wm_zget(i)->id == t->sel_id) { wm_close(wm_zget(i)); t->sel_id = 0; return; }
        return;
    }
    int row = (y - 30) / 28;
    if (y >= 30 && row >= 0 && row < wm_zcount()) t->sel_id = wm_zget(wm_zcount() - 1 - row)->id;
}
static void tm_close(Window *w) { kfree(w->priv); }
Window *taskmgr_new(void) {
    int x, y; win_cascade(&x, &y);
    Window *w = wm_create(APP_TASKMGR, "Task Manager", ICON_TASKMGR, x, y, 560, 400);
    if (!w) return 0;
    Tm *t = kmalloc(sizeof(Tm));
    if (!t) { wm_close(w); return 0; }
    w->priv = t; w->draw = tm_draw; w->mouse = tm_mouse; w->close = tm_close; w->minw = 400; w->minh = 240;
    return w;
}
