/* 32-bit software renderer: rects, alpha, AA circles, anti-aliased text. */
#include "os.h"

u32 *g_bb; int g_w, g_h;
static Clip clip;

void gfx_init(u32 *bb, int w, int h) { g_bb = bb; g_w = w; g_h = h; gfx_clip_reset(); }
void gfx_clip_reset(void) { clip.x0 = 0; clip.y0 = 0; clip.x1 = g_w; clip.y1 = g_h; }
Clip gfx_clip_push(int x, int y, int w, int h) {
    Clip old = clip;
    clip.x0 = MAX(clip.x0, x); clip.y0 = MAX(clip.y0, y);
    clip.x1 = MIN(clip.x1, x + w); clip.y1 = MIN(clip.y1, y + h);
    if (clip.x1 < clip.x0) clip.x1 = clip.x0;
    if (clip.y1 < clip.y0) clip.y1 = clip.y0;
    return old;
}
void gfx_clip_pop(Clip c) { clip = c; }

static inline int mix(int d, int s, int a) { int t = s * a + d * (255 - a); return (t + 1 + (t >> 8)) >> 8; }
u32 col_blend(u32 d, u32 s, int a) {
    if (a <= 0) return d;
    if (a >= 255) return s;
    return (u32)(mix((d >> 16) & 255, (s >> 16) & 255, a) << 16 | mix((d >> 8) & 255, (s >> 8) & 255, a) << 8 |
                 mix(d & 255, s & 255, a));
}
u32 col_lighten(u32 c, int a) { return col_blend(c, 0xFFFFFF, a); }
u32 col_darken(u32 c, int a) { return col_blend(c, 0x000000, a); }

void gfx_pixel(int x, int y, u32 c, int a) {
    if (x < clip.x0 || x >= clip.x1 || y < clip.y0 || y >= clip.y1) return;
    u32 *p = &g_bb[y * g_w + x];
    *p = col_blend(*p, c, a);
}
void gfx_rect(int x, int y, int w, int h, u32 c) {
    int x0 = MAX(x, clip.x0), y0 = MAX(y, clip.y0), x1 = MIN(x + w, clip.x1), y1 = MIN(y + h, clip.y1);
    for (int j = y0; j < y1; j++) { u32 *p = &g_bb[j * g_w]; for (int i = x0; i < x1; i++) p[i] = c; }
}
void gfx_rect_a(int x, int y, int w, int h, u32 c, int a) {
    if (a >= 255) { gfx_rect(x, y, w, h, c); return; }
    int x0 = MAX(x, clip.x0), y0 = MAX(y, clip.y0), x1 = MIN(x + w, clip.x1), y1 = MIN(y + h, clip.y1);
    for (int j = y0; j < y1; j++) { u32 *p = &g_bb[j * g_w]; for (int i = x0; i < x1; i++) p[i] = col_blend(p[i], c, a); }
}
void gfx_darken(int x, int y, int w, int h, u32 tint, int a) { gfx_rect_a(x, y, w, h, tint, a); }
void gfx_hline(int x, int y, int w, u32 c) { gfx_rect(x, y, w, 1, c); }
void gfx_vline(int x, int y, int h, u32 c) { gfx_rect(x, y, 1, h, c); }
void gfx_frame(int x, int y, int w, int h, u32 c) {
    gfx_rect(x, y, w, 1, c); gfx_rect(x, y + h - 1, w, 1, c);
    gfx_rect(x, y, 1, h, c); gfx_rect(x + w - 1, y, 1, h, c);
}
void gfx_gradient(int x, int y, int w, int h, u32 top, u32 bot) {
    for (int j = 0; j < h; j++) gfx_rect(x, y + j, w, 1, col_blend(top, bot, h > 1 ? j * 255 / (h - 1) : 0));
}
void gfx_line(int x0, int y0, int x1, int y1, u32 c) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (;;) {
        gfx_pixel(x0, y0, c, 255);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
static u32 isqrt(u32 n) {
    u32 r = 0, bit = 1u << 30;
    while (bit > n) bit >>= 2;
    while (bit) { if (n >= r + bit) { n -= r + bit; r = (r >> 1) + bit; } else r >>= 1; bit >>= 2; }
    return r;
}
static int cov(int rr16, int d16) { return CLAMP(rr16 - d16 + 8, 0, 16); }
void gfx_ring(int cx, int cy, int r, int thick, u32 c) {
    for (int y = cy - r - 1; y <= cy + r + 1; y++)
        for (int x = cx - r - 1; x <= cx + r + 1; x++) {
            int dx = x - cx, dy = y - cy;
            int d = (int)isqrt((u32)(dx * dx + dy * dy) * 256);
            int a = cov(r * 16, d) - (thick > 0 ? cov((r - thick) * 16, d) : 0);
            if (a > 0) gfx_pixel(x, y, c, a * 255 / 16);
        }
}
void gfx_circle(int cx, int cy, int r, u32 c) { gfx_ring(cx, cy, r, 0, c); }

/* ---- text ---- */
static int gidx(char ch) { int i = (u8)ch - 32; return (i < 0 || i > 94) ? '?' - 32 : i; }
int text_width(const Font *f, const char *s) {
    int x16 = 0;
    for (; *s; s++) x16 += f->adv[gidx(*s)];
    return (x16 + 8) >> 4;
}
void text_draw_n(const Font *f, int x, int y, const char *s, int n, u32 c) {
    if (y + f->h <= clip.y0 || y >= clip.y1) return;
    int x16 = x << 4;
    for (int k = 0; k < n && s[k]; k++) {
        int idx = gidx(s[k]), px = (x16 + 8) >> 4;
        x16 += f->adv[idx];
        int w = f->gw[idx], h = f->gh[idx];
        if (!w) continue;
        int gx = px + f->gx[idx], gy = y + f->gy[idx];
        if (gx >= clip.x1 || gx + w <= clip.x0 || gy >= clip.y1 || gy + h <= clip.y0) continue;
        const u8 *b = f->bits + f->off[idx];
        for (int j = 0; j < h; j++) {
            int yy = gy + j;
            if (yy < clip.y0 || yy >= clip.y1) continue;
            for (int i = 0; i < w; i++) {
                int xx = gx + i, a = b[j * w + i];
                if (!a || xx < clip.x0 || xx >= clip.x1) continue;
                u32 *p = &g_bb[yy * g_w + xx];
                *p = a >= 250 ? c : col_blend(*p, c, a);
            }
        }
    }
}
void text_draw(const Font *f, int x, int y, const char *s, u32 c) { text_draw_n(f, x, y, s, 1 << 20, c); }
void text_draw_r(const Font *f, int right, int y, const char *s, u32 c) { text_draw(f, right - text_width(f, s), y, s, c); }
void text_draw_c(const Font *f, int x, int w, int y, const char *s, u32 c) { text_draw(f, x + (w - text_width(f, s)) / 2, y, s, c); }
void text_draw_vc(const Font *f, int x, int y, int h, const char *s, u32 c) { text_draw(f, x, y + (h - f->h) / 2, s, c); }
void text_fit(const Font *f, const char *s, int maxw, char *out, int cap) {
    kstrcpy(out, s, cap);
    if (text_width(f, out) <= maxw) return;
    int n = (int)strlen(out);
    while (n > 0) {
        n--;
        out[n] = 0;
        char tmp[96]; ksnprintf(tmp, sizeof tmp, "%s...", out);
        if (text_width(f, tmp) <= maxw) { kstrcpy(out, tmp, cap); return; }
    }
}
