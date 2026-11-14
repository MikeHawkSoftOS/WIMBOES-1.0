/* Window manager: z-order, Windows-10-style frames, drag/resize, focus. */
#include "os.h"

Window g_win[MAX_WIN];
static int order[MAX_WIN], norder, next_id = 1;
static Window *capture;
static struct { int mode, edges; Window *w; int dx, dy, sx, sy, ox, oy, ow, oh; } drag;
static struct { Window *w; int btn; } press;
static struct { Window *w; u32 t; int x, y; } lastdown;

enum { R_NONE, R_CLIENT, R_TITLE, R_MIN, R_MAX, R_CLOSE, R_RESIZE };

static int zindex(Window *w) { for (int i = 0; i < norder; i++) if (&g_win[order[i]] == w) return i; return -1; }
int wm_zcount(void) { return norder; }
Window *wm_zget(int i) { return &g_win[order[i]]; }

Window *wm_create(int app, const char *title, int icon, int x, int y, int w, int h) {
    int slot = -1;
    for (int i = 0; i < MAX_WIN; i++) if (!g_win[i].used) { slot = i; break; }
    if (slot < 0) return 0;
    Window *W = &g_win[slot];
    memset(W, 0, sizeof *W);
    int maxh = g_h - TASKBAR_H;
    w = MIN(w, g_w); h = MIN(h, maxh);
    x = CLAMP(x, 0, g_w - w); y = CLAMP(y, 0, maxh - h);
    W->used = true; W->app = app; W->icon = icon; W->id = next_id++;
    W->x = W->rx = x; W->y = W->ry = y; W->w = W->rw = w; W->h = W->rh = h;
    W->minw = 260; W->minh = 180;
    kstrcpy(W->title, title, sizeof W->title);
    order[norder++] = slot;
    return W;
}
void wm_close(Window *w) {
    int i = zindex(w);
    if (i < 0) return;
    if (w->close) w->close(w);
    for (; i < norder - 1; i++) order[i] = order[i + 1];
    norder--; w->used = false;
    if (capture == w) capture = 0;
    if (drag.w == w) drag.mode = 0;
    if (press.w == w) press.w = 0;
    if (lastdown.w == w) lastdown.w = 0;
}
void wm_close_all(void) { while (norder) wm_close(&g_win[order[norder - 1]]); }
void wm_focus(Window *w) {
    int i = zindex(w);
    if (i < 0) return;
    w->minimized = false;
    for (; i < norder - 1; i++) order[i] = order[i + 1];
    order[norder - 1] = (int)(w - g_win);
}
void wm_minimize(Window *w) { w->minimized = true; }
void wm_toggle_max(Window *w) {
    if (w->maximized) { w->x = w->rx; w->y = w->ry; w->w = w->rw; w->h = w->rh; w->maximized = false; }
    else { w->rx = w->x; w->ry = w->y; w->rw = w->w; w->rh = w->h; w->x = 0; w->y = 0; w->w = g_w; w->h = g_h - TASKBAR_H; w->maximized = true; }
}
Window *wm_top(void) {
    for (int i = norder - 1; i >= 0; i--) if (!g_win[order[i]].minimized) return &g_win[order[i]];
    return 0;
}
int wm_count_app(int app) { int n = 0; for (int i = 0; i < norder; i++) n += g_win[order[i]].app == app; return n; }
void wm_set_title(Window *w, const char *t) { kstrcpy(w->title, t, sizeof w->title); }
Window *wm_hit(int mx, int my) {
    for (int i = norder - 1; i >= 0; i--) {
        Window *w = &g_win[order[i]];
        if (!w->minimized && in_rect(mx, my, w->x, w->y, w->w, w->h)) return w;
    }
    return 0;
}
void wm_update_hover(bool blocked) {
    g_hover_win = (blocked || drag.mode) ? 0 : (capture ? capture : wm_hit(g_mx, g_my));
}

/* ---- geometry ---- */
static int region(Window *w, int mx, int my, int *edges) {
    *edges = 0;
    if (!w->maximized) {
        if (mx < w->x + 5) *edges |= 1;
        if (mx >= w->x + w->w - 5) *edges |= 2;
        if (my < w->y + 4) *edges |= 4;
        if (my >= w->y + w->h - 5) *edges |= 8;
        if (*edges) return R_RESIZE;
    }
    if (my < w->y + TITLE_H) {
        int rx = w->x + w->w - 1 - mx;
        if (rx < 46) return R_CLOSE;
        if (rx < 92) return R_MAX;
        if (rx < 138) return R_MIN;
        return R_TITLE;
    }
    return R_CLIENT;
}

/* ---- drawing ---- */
static void draw_controls(Window *w, bool active) {
    int right = w->x + w->w - 1, y = w->y;
    static const struct { int off, kind; } b[3] = { { 138, R_MIN }, { 92, R_MAX }, { 46, R_CLOSE } };
    for (int i = 0; i < 3; i++) {
        int bx = right - b[i].off;
        bool hov = ui_hover(w, bx, y + 1, 46, TITLE_H - 1) && !(drag.mode);
        u32 fg = active ? 0x000000 : 0x888888;
        if (hov) {
            if (b[i].kind == R_CLOSE) { gfx_rect(bx, y + 1, 46, TITLE_H - 1, 0xE81123); fg = 0xFFFFFF; }
            else gfx_rect(bx, y + 1, 46, TITLE_H - 1, 0xE5E5E5);
        }
        int cx = bx + 23, cy = y + 16;
        if (b[i].kind == R_MIN) gfx_hline(cx - 5, cy, 10, fg);
        else if (b[i].kind == R_MAX) {
            if (w->maximized) { gfx_frame(cx - 5, cy - 3, 8, 8, fg); gfx_hline(cx - 3, cy - 5, 8, fg); gfx_vline(cx + 4, cy - 5, 8, fg); }
            else gfx_frame(cx - 5, cy - 5, 10, 10, fg);
        } else { gfx_line(cx - 5, cy - 5, cx + 4, cy + 4, fg); gfx_line(cx - 5, cy + 4, cx + 4, cy - 5, fg); }
    }
}
static void draw_window(Window *w, bool active) {
    int x = w->x, y = w->y, ww = w->w, hh = w->h;
    if (!w->maximized) {
        for (int d = 1; d <= 9; d++) {          /* cheap soft shadow: concentric rings */
            int a = active ? 34 - d * 3 : 20 - d * 2;
            if (a <= 0) break;
            gfx_rect_a(x - d, y - d + 3, ww + 2 * d, 1, 0x000000, a);
            gfx_rect_a(x - d, y + hh + d + 2, ww + 2 * d, 1, 0x000000, a);
            gfx_rect_a(x - d, y - d + 4, 1, hh + 2 * d - 1, 0x000000, a);
            gfx_rect_a(x + ww + d - 1, y - d + 4, 1, hh + 2 * d - 1, 0x000000, a);
        }
    }
    gfx_rect(x, y, ww, hh, 0xFFFFFF);
    icon_draw(w->icon, x + 9, y + 8, 16);
    char t[64]; text_fit(&font_ui, w->title, ww - 32 - 138 - 8, t, sizeof t);
    text_draw(&font_ui, x + 32, y + (TITLE_H - font_ui.h) / 2, t, active ? 0x000000 : 0x808080);
    draw_controls(w, active);
    int cx = x + 1, cy = y + TITLE_H, cw = ww - 2, ch = hh - TITLE_H - 1;
    Clip o = gfx_clip_push(cx, cy, cw, ch);
    if (w->draw) w->draw(w, cx, cy, cw, ch);
    gfx_clip_pop(o);
    gfx_frame(x, y, ww, hh, active ? g_accent : 0xAAAAAA);
}
void wm_draw_all(void) {
    Window *top = wm_top();
    for (int i = 0; i < norder; i++) {
        Window *w = &g_win[order[i]];
        if (!w->minimized) draw_window(w, w == top);
    }
}

/* ---- input ---- */
static void to_client(Window *w, int mx, int my, int *cx, int *cy) { *cx = mx - (w->x + 1); *cy = my - (w->y + TITLE_H); }

bool wm_mouse_event(const Event *e) {
    int mx = e->x, my = e->y;
    if (e->type == EV_MOVE) {
        if (drag.mode == 1) {
            Window *w = drag.w;
            w->x = CLAMP(mx - drag.dx, 80 - w->w, g_w - 80);
            w->y = CLAMP(my - drag.dy, 0, g_h - TASKBAR_H - 24);
            return true;
        }
        if (drag.mode == 2) {
            Window *w = drag.w; int dx = mx - drag.sx, dy = my - drag.sy;
            int nx = drag.ox, ny = drag.oy, nw = drag.ow, nh = drag.oh;
            if (drag.edges & 2) nw = MAX(w->minw, drag.ow + dx);
            if (drag.edges & 8) nh = MAX(w->minh, drag.oh + dy);
            if (drag.edges & 1) { nw = MAX(w->minw, drag.ow - dx); nx = drag.ox + drag.ow - nw; }
            if (drag.edges & 4) { nh = MAX(w->minh, drag.oh - dy); ny = drag.oy + drag.oh - nh; }
            w->x = nx; w->y = ny; w->w = nw; w->h = nh;
            return true;
        }
        if (capture) {
            int cx, cy; to_client(capture, mx, my, &cx, &cy);
            if (capture->mouse) capture->mouse(capture, M_MOVE, cx, cy, g_mb);
            return true;
        }
        return wm_hit(mx, my) != 0;
    }
    if (e->type == EV_DOWN) {
        Window *w = wm_hit(mx, my);
        if (!w) return false;
        wm_focus(w);
        int edges, r = region(w, mx, my, &edges);
        u32 now = hal_ms();
        bool dbl = lastdown.w == w && now - lastdown.t < 500 && ((mx - lastdown.x) > -6 && (mx - lastdown.x) < 6) && ((my - lastdown.y) > -6 && (my - lastdown.y) < 6);
        lastdown.w = dbl ? 0 : w; lastdown.t = now; lastdown.x = mx; lastdown.y = my;
        if (e->btn == 2) {
            if (r == R_CLIENT && w->mouse) { int cx, cy; to_client(w, mx, my, &cx, &cy); w->mouse(w, M_RIGHT, cx, cy, 2); }
            return true;
        }
        if (r == R_RESIZE) { drag.mode = 2; drag.edges = edges; drag.w = w; drag.sx = mx; drag.sy = my; drag.ox = w->x; drag.oy = w->y; drag.ow = w->w; drag.oh = w->h; }
        else if (r == R_MIN || r == R_MAX || r == R_CLOSE) { press.w = w; press.btn = r; }
        else if (r == R_TITLE) {
            if (dbl) wm_toggle_max(w);
            else if (!w->maximized) { drag.mode = 1; drag.w = w; drag.dx = mx - w->x; drag.dy = my - w->y; }
        } else if (r == R_CLIENT) {
            int cx, cy; to_client(w, mx, my, &cx, &cy);
            capture = w;
            if (w->mouse) { w->mouse(w, M_DOWN, cx, cy, e->btn); if (dbl) w->mouse(w, M_DBL, cx, cy, e->btn); }
        }
        return true;
    }
    if (e->type == EV_UP) {
        bool used = drag.mode != 0 || capture != 0 || press.w != 0;
        drag.mode = 0;
        if (capture) {
            int cx, cy; to_client(capture, mx, my, &cx, &cy);
            Window *w = capture; capture = 0;
            if (w->mouse) w->mouse(w, M_UP, cx, cy, e->btn);
        }
        if (press.w) {
            Window *w = press.w; int edges; press.w = 0;
            if (wm_hit(mx, my) == w && region(w, mx, my, &edges) == press.btn) {
                if (press.btn == R_MIN) wm_minimize(w);
                else if (press.btn == R_MAX) wm_toggle_max(w);
                else wm_close(w);
            }
        }
        return used;
    }
    if (e->type == EV_WHEEL) {
        Window *w = wm_hit(mx, my);
        if (!w) return false;
        int edges;
        if (region(w, mx, my, &edges) == R_CLIENT && w->mouse) { int cx, cy; to_client(w, mx, my, &cx, &cy); w->mouse(w, M_WHEEL, cx, cy, e->key); }
        return true;
    }
    return false;
}
void wm_key_event(int key, int mod) {
    if ((mod & MOD_ALT) && key == K_TAB) {
        for (int i = 0; i < norder; i++) if (!g_win[order[i]].minimized) { if (i < norder - 1 || norder > 1) wm_focus(&g_win[order[i]]); break; }
        return;
    }
    Window *t = wm_top();
    if (!t) return;
    if ((mod & MOD_ALT) && key == K_F4) { wm_close(t); return; }
    if (t->key) t->key(t, key, mod);
}
