/* Paint: pencil, brush, eraser, flood fill on a 640x400 canvas. */
#include "os.h"
#define CW 640
#define CH 400
#define TOOLBAR 84
typedef struct { u32 *px; int tool, size; u32 color; bool drawing; int lx, ly; } Paint;
static const u32 pal[20] = { 0x000000, 0x7F7F7F, 0x880015, 0xED1C24, 0xFF7F27, 0xFFF200, 0x22B14C, 0x00A2E8, 0x3F48CC, 0xA349A4,
                             0xFFFFFF, 0xC3C3C3, 0xB97A57, 0xFFAEC9, 0xFFC90E, 0xEFE4B0, 0xB5E61D, 0x99D9EA, 0x7092BE, 0xC8BFE7 };
static const char *const tools[4] = { "Pencil", "Brush", "Eraser", "Fill" };

static void stamp(Paint *p, int x, int y, u32 c) {
    int r = p->tool == 0 ? 0 : p->tool == 2 ? p->size * 2 + 2 : p->size;
    for (int j = -r; j <= r; j++) for (int i = -r; i <= r; i++) {
        int px = x + i, py = y + j;
        if (px >= 0 && py >= 0 && px < CW && py < CH && (r < 3 || i * i + j * j <= r * r + 1)) p->px[py * CW + px] = c;
    }
}
static void stroke(Paint *p, int x0, int y0, int x1, int y1) {
    u32 c = p->tool == 2 ? 0xFFFFFF : p->color;
    int dx = x1 - x0, dy = y1 - y0, n = MAX(dx < 0 ? -dx : dx, dy < 0 ? -dy : dy);
    for (int i = 0; i <= n; i++) stamp(p, n ? x0 + dx * i / n : x0, n ? y0 + dy * i / n : y0, c);
}
static void fill(Paint *p, int sx, int sy) {
    u32 target = p->px[sy * CW + sx];
    if (target == p->color) return;
    int cap = 200000, *st = kmalloc((size_t)cap * sizeof(int)), sp = 0;
    if (!st) return;
    st[sp++] = sy * CW + sx;
    while (sp) {
        int i = st[--sp], x = i % CW, y = i / CW;
        if (p->px[i] != target) continue;
        int l = x, r = x;
        while (l > 0 && p->px[y * CW + l - 1] == target) l--;
        while (r < CW - 1 && p->px[y * CW + r + 1] == target) r++;
        for (int k = l; k <= r; k++) {
            p->px[y * CW + k] = p->color;
            if (y > 0 && p->px[(y - 1) * CW + k] == target && (k == l || p->px[(y - 1) * CW + k - 1] != target) && sp < cap) st[sp++] = (y - 1) * CW + k;
            if (y < CH - 1 && p->px[(y + 1) * CW + k] == target && (k == l || p->px[(y + 1) * CW + k - 1] != target) && sp < cap) st[sp++] = (y + 1) * CW + k;
        }
    }
    kfree(st);
}
static void pt_draw(Window *w, int cx, int cy, int cw, int ch) {
    Paint *p = w->priv;
    gfx_rect(cx, cy, cw, ch, 0xD5DAE3);
    gfx_rect(cx, cy, cw, TOOLBAR, 0xF5F6F7); gfx_hline(cx, cy + TOOLBAR - 1, cw, 0xDADCE0);
    int x = cx + 8;
    for (int i = 0; i < 4; i++) {
        bool hov = ui_hover(w, x, cy + 8, 62, 30);
        gfx_rect(x, cy + 8, 62, 30, p->tool == i ? 0xCCE4F7 : hov ? 0xE5F1FB : 0xE8E8E8); gfx_frame(x, cy + 8, 62, 30, p->tool == i ? g_accent : 0xBBBBBB);
        text_draw_c(&font_ui, x, 62, cy + 8 + (30 - font_ui.h) / 2, tools[i], 0x000000); x += 66;
    }
    ui_button(w, x + 6, cy + 8, 62, 30, "Clear", false);
    text_draw(&font_ui, x + 90, cy + 16, "Size", 0x333333);
    for (int i = 0; i < 4; i++) {
        int bx = x + 130 + i * 30, r = 1 + i * 2;
        gfx_rect(bx, cy + 8, 26, 30, p->size == i + 1 ? 0xCCE4F7 : 0xE8E8E8); gfx_frame(bx, cy + 8, 26, 30, p->size == i + 1 ? g_accent : 0xBBBBBB);
        gfx_circle(bx + 13, cy + 23, r, 0x000000);
    }
    gfx_rect(cx + 8, cy + 46, 30, 30, p->color); gfx_frame(cx + 8, cy + 46, 30, 30, 0x555555);
    for (int i = 0; i < 20; i++) {
        int sx = cx + 50 + (i % 10) * 26, sy = cy + 46 + (i / 10) * 16;
        gfx_rect(sx, sy, 22, 14, pal[i]); gfx_frame(sx, sy, 22, 14, ui_hover(w, sx, sy, 22, 14) ? g_accent : 0x888888);
    }
    int ox = cx + 8, oy = cy + TOOLBAR + 8;
    gfx_rect(ox + 3, oy + 3, CW, CH, 0xB0B6C0);
    for (int j = 0; j < CH; j++) {
        int yy = oy + j; if (yy < cy || yy >= cy + ch) continue;
        for (int i = 0; i < CW; i++) { int xx = ox + i; if (xx >= cx && xx < cx + cw) gfx_pixel(xx, yy, p->px[j * CW + i], 255); }
    }
}
static void pt_mouse(Window *w, int ev, int x, int y, int btn) {
    Paint *p = w->priv; (void)btn;
    int mx = x - 8, my = y - TOOLBAR - 8;
    if (ev == M_UP) { p->drawing = false; return; }
    if (ev == M_MOVE) { if (p->drawing) { stroke(p, p->lx, p->ly, mx, my); p->lx = mx; p->ly = my; } return; }
    if (ev != M_DOWN) return;
    if (y < TOOLBAR) {
        int bx = 8;
        for (int i = 0; i < 4; i++, bx += 66) if (in_rect(x, y, bx, 8, 62, 30)) p->tool = i;
        if (in_rect(x, y, bx + 6, 8, 62, 30)) for (int i = 0; i < CW * CH; i++) p->px[i] = 0xFFFFFF;
        for (int i = 0; i < 4; i++) if (in_rect(x, y, bx + 130 + i * 30, 8, 26, 30)) p->size = i + 1;
        for (int i = 0; i < 20; i++) if (in_rect(x, y, 50 + (i % 10) * 26, 46 + (i / 10) * 16, 22, 14)) { p->color = pal[i]; if (p->tool == 2) p->tool = 1; }
        return;
    }
    if (mx < 0 || my < 0 || mx >= CW || my >= CH) return;
    if (p->tool == 3) { fill(p, mx, my); return; }
    p->drawing = true; p->lx = mx; p->ly = my; stroke(p, mx, my, mx, my);
}
static void pt_close(Window *w) { Paint *p = w->priv; kfree(p->px); kfree(p); }
Window *paint_new(void) {
    int x, y; win_cascade(&x, &y);
    Window *w = wm_create(APP_PAINT, "Untitled - Paint", ICON_PAINT, x, y, 700, 560);
    if (!w) return 0;
    Paint *p = kmalloc(sizeof(Paint));
    if (p) p->px = kmalloc((size_t)CW * CH * 4);
    if (!p || !p->px) { if (p) kfree(p); wm_close(w); return 0; }
    for (int i = 0; i < CW * CH; i++) p->px[i] = 0xFFFFFF;
    p->tool = 1; p->size = 1; p->color = 0x000000;
    w->priv = p; w->draw = pt_draw; w->mouse = pt_mouse; w->close = pt_close; w->minw = 360; w->minh = 260;
    return w;
}
