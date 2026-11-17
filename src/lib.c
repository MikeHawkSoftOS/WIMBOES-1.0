/* Hardware independent helpers shared by kernel and host test build. */
#include "os.h"

/* ------------------------------------------------------------ global theme state */
Theme g_themes[THEME_MAX];
int g_theme = 0;
bool g_reduced_motion = false;

/* ------------------------------------------------------------ theme implementation */

/* Helper: copy a theme's colors to active globals */
static void theme_apply_colors(const Theme *t) {
    g_accent = t->accent;
    g_wallpaper = t->wallpaper;
    g_shell_style = t->shell_style;
    /* g_reduced_motion is handled separately; g_clock24 is separate from theme */
}

/* Initialize built-in themes */
void theme_init(void) {
    memset(g_themes, 0, sizeof g_themes);

    /* Theme 0: Windows Light */
    {
        Theme *t = &g_themes[0];
        t->used = true;
        kstrcpy(t->name, "Windows Light", sizeof t->name);
        t->accent = 0x0078D7;
        t->bg_primary = 0xFFFFFF;
        t->bg_secondary = 0xF3F3F3;
        t->bg_tertiary = 0xE1E1E1;
        t->fg_primary = 0x000000;
        t->fg_secondary = 0x333333;
        t->fg_muted = 0x6B6B6B;
        t->border = 0xADADAD;
        t->border_focus = 0x0078D7;
        t->hover = 0xE5F1FB;
        t->active = 0xCCE4F7;
        t->selection = 0x0078D7;
        t->window_bg = 0xFFFFFF;
        t->titlebar_bg = 0xFFFFFF;
        t->titlebar_fg = 0x000000;
        t->titlebar_fg_inactive = 0x808080;
        t->taskbar_bg = 0xFFFFFF;
        t->startmenu_bg = 0xF3F3F3;
        t->startmenu_rail = 0xE1E1E1;
        t->shadow = 0x000000;
        t->wallpaper = 0;
        t->shell_style = 1;
        t->cursor_theme = 0;
        t->ui_scale = 100;
        t->animations = true;
        t->transparency = true;
        t->rounding = 8;
    }

    /* Theme 1: Windows Dark */
    {
        Theme *t = &g_themes[1];
        t->used = true;
        kstrcpy(t->name, "Windows Dark", sizeof t->name);
        t->accent = 0x0078D7;
        t->bg_primary = 0x1B1B1B;
        t->bg_secondary = 0x2D2D2D;
        t->bg_tertiary = 0x3C3C3C;
        t->fg_primary = 0xFFFFFF;
        t->fg_secondary = 0xCCCCCC;
        t->fg_muted = 0x888888;
        t->border = 0x4A4A4A;
        t->border_focus = 0x0078D7;
        t->hover = 0x3E3E42;
        t->active = 0x0E639C;
        t->selection = 0x264F78;
        t->window_bg = 0x1E1E1E;
        t->titlebar_bg = 0x2D2D2D;
        t->titlebar_fg = 0xFFFFFF;
        t->titlebar_fg_inactive = 0x888888;
        t->taskbar_bg = 0x17191E;
        t->startmenu_bg = 0x1B1B1B;
        t->startmenu_rail = 0x000000;
        t->shadow = 0x000000;
        t->wallpaper = 2;
        t->shell_style = 1;
        t->cursor_theme = 0;
        t->ui_scale = 100;
        t->animations = true;
        t->transparency = true;
        t->rounding = 8;
    }

    /* Theme 2: High Contrast #1 (Black bg, yellow fg) */
    {
        Theme *t = &g_themes[2];
        t->used = true;
        kstrcpy(t->name, "High Contrast #1", sizeof t->name);
        t->accent = 0xFFFF00;
        t->bg_primary = 0x000000;
        t->bg_secondary = 0x000000;
        t->bg_tertiary = 0x1A1A1A;
        t->fg_primary = 0xFFFF00;
        t->fg_secondary = 0xFFFF00;
        t->fg_muted = 0xAAAA00;
        t->border = 0xFFFF00;
        t->border_focus = 0xFFFFFF;
        t->hover = 0x333300;
        t->active = 0x555500;
        t->selection = 0xFFFF00;
        t->window_bg = 0x000000;
        t->titlebar_bg = 0x000000;
        t->titlebar_fg = 0xFFFF00;
        t->titlebar_fg_inactive = 0x888800;
        t->taskbar_bg = 0x000000;
        t->startmenu_bg = 0x000000;
        t->startmenu_rail = 0x000000;
        t->shadow = 0xFFFF00;
        t->wallpaper = 2;
        t->shell_style = 1;
        t->cursor_theme = 0;
        t->ui_scale = 100;
        t->animations = false;
        t->transparency = false;
        t->rounding = 0;
    }

    /* Theme 3: High Contrast #2 (White bg, black fg) */
    {
        Theme *t = &g_themes[3];
        t->used = true;
        kstrcpy(t->name, "High Contrast #2", sizeof t->name);
        t->accent = 0x0000FF;
        t->bg_primary = 0xFFFFFF;
        t->bg_secondary = 0xFFFFFF;
        t->bg_tertiary = 0xEEEEEE;
        t->fg_primary = 0x000000;
        t->fg_secondary = 0x000000;
        t->fg_muted = 0x333333;
        t->border = 0x000000;
        t->border_focus = 0x0000FF;
        t->hover = 0xE0E0E0;
        t->active = 0xCCCCCC;
        t->selection = 0x0000FF;
        t->window_bg = 0xFFFFFF;
        t->titlebar_bg = 0xFFFFFF;
        t->titlebar_fg = 0x000000;
        t->titlebar_fg_inactive = 0x555555;
        t->taskbar_bg = 0xFFFFFF;
        t->startmenu_bg = 0xFFFFFF;
        t->startmenu_rail = 0xEEEEEE;
        t->shadow = 0x000000;
        t->wallpaper = 0;
        t->shell_style = 1;
        t->cursor_theme = 0;
        t->ui_scale = 100;
        t->animations = false;
        t->transparency = false;
        t->rounding = 0;
    }

    /* Theme 4: Custom (user modifiable copy of current) */
    {
        Theme *t = &g_themes[4];
        t->used = true;
        kstrcpy(t->name, "Custom", sizeof t->name);
        t->accent = 0x0078D7;
        t->bg_primary = 0xFFFFFF;
        t->bg_secondary = 0xF3F3F3;
        t->bg_tertiary = 0xE1E1E1;
        t->fg_primary = 0x000000;
        t->fg_secondary = 0x333333;
        t->fg_muted = 0x6B6B6B;
        t->border = 0xADADAD;
        t->border_focus = 0x0078D7;
        t->hover = 0xE5F1FB;
        t->active = 0xCCE4F7;
        t->selection = 0x0078D7;
        t->window_bg = 0xFFFFFF;
        t->titlebar_bg = 0xFFFFFF;
        t->titlebar_fg = 0x000000;
        t->titlebar_fg_inactive = 0x808080;
        t->taskbar_bg = 0xFFFFFF;
        t->startmenu_bg = 0xF3F3F3;
        t->startmenu_rail = 0xE1E1E1;
        t->shadow = 0x000000;
        t->wallpaper = 0;
        t->shell_style = 1;
        t->cursor_theme = 0;
        t->ui_scale = 100;
        t->animations = true;
        t->transparency = false;
        t->rounding = 0;
    }
}

/* Load theme by index and apply it */
void theme_load(int idx) {
    if (idx < 0 || idx >= THEME_MAX || !g_themes[idx].used) return;

    Theme *t = &g_themes[idx];
    theme_apply_colors(t);
    g_theme = idx;
    g_reduced_motion = !t->animations;
}

/* Persist current theme index to disk (sector 2080) */
#define THEME_DISK_SECTOR 2080

void theme_save_current(void) {
    char buf[512];
    memset(buf, 0, sizeof buf);
    buf[0] = 'W'; buf[1] = 'T'; buf[2] = 'H'; buf[3] = 1;  /* magic + version */
    *(int *)(buf + 4) = g_theme;
    hal_disk_write(THEME_DISK_SECTOR, buf, 1);
}

/* Load theme index from disk and apply it */
void theme_load_saved(void) {
    char buf[512];
    if (!hal_disk_read(THEME_DISK_SECTOR, buf, 1)) return;
    if (buf[0] != 'W' || buf[1] != 'T' || buf[2] != 'W') return;
    int idx = *(int *)(buf + 4);
    if (idx >= 0 && idx < THEME_MAX && g_themes[idx].used) {
        theme_load(idx);
    }
}

/* Parse JSON theme (stub - not implemented) */
bool theme_parse_json(const char *json, Theme *t) {
    (void)json; (void)t;
    return false;
}

/* Export theme to JSON (stub - not implemented) */
void theme_to_json(const Theme *t, char *buf, int cap) {
    (void)t;
    if (cap > 0) buf[0] = 0;
}

/* Public helper to apply active theme's colors to globals */
void theme_apply_colors_public(void) {
    if (g_theme >= 0 && g_theme < THEME_MAX && g_themes[g_theme].used) {
        theme_apply_colors(&g_themes[g_theme]);
        g_reduced_motion = !g_themes[g_theme].animations;
    }
}

void kstrcpy(char *dst, const char *src, int cap) {
    int i = 0;
    if (cap <= 0) return;
    for (; i < cap - 1 && src[i]; i++) dst[i] = src[i];
    dst[i] = 0;
}
static int lc(int c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
int kstricmp(const char *a, const char *b) {
    while (*a && lc((u8)*a) == lc((u8)*b)) { a++; b++; }
    return lc((u8)*a) - lc((u8)*b);
}
bool kcontains_ci(const char *hay, const char *needle) {
    int n = (int)strlen(needle), h = (int)strlen(hay);
    if (!n) return true;
    for (int i = 0; i + n <= h; i++) {
        int j = 0;
        while (j < n && lc((u8)hay[i + j]) == lc((u8)needle[j])) j++;
        if (j == n) return true;
    }
    return false;
}
int katoi(const char *s) {
    int v = 0, neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return neg ? -v : v;
}

typedef struct { char *p; int n, cap; } Out;
static void put(Out *o, char c) { if (o->n < o->cap - 1) o->p[o->n] = c; o->n++; }
static void putnum(Out *o, u32 v, int base, bool neg, int width, bool zero, bool left) {
    char tmp[12]; int n = 0;
    if (!v) tmp[n++] = '0';
    while (v) { tmp[n++] = "0123456789abcdef"[v % base]; v /= base; }
    int len = n + (neg ? 1 : 0);
    int pad = width > len ? width - len : 0;
    if (!left && !zero) while (pad--) put(o, ' ');
    if (neg) put(o, '-');
    if (!left && zero) while (pad--) put(o, '0');
    while (n) put(o, tmp[--n]);
    if (left) while (pad--) put(o, ' ');
}
int ksnprintf(char *buf, int cap, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    Out o = { buf, 0, cap };
    for (; *fmt; fmt++) {
        if (*fmt != '%') { put(&o, *fmt); continue; }
        fmt++;
        bool zero = false, left = false; int width = 0;
        if (*fmt == '-') { left = true; fmt++; }
        if (*fmt == '0') { zero = true; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') width = width * 10 + (*fmt++ - '0');
        switch (*fmt) {
        case 'd': { int v = va_arg(ap, int); putnum(&o, v < 0 ? (u32)(-(v + 1)) + 1u : (u32)v, 10, v < 0, width, zero, left); break; }
        case 'u': putnum(&o, va_arg(ap, u32), 10, false, width, zero, left); break;
        case 'x': putnum(&o, va_arg(ap, u32), 16, false, width, zero, left); break;
        case 'c': put(&o, (char)va_arg(ap, int)); break;
        case 's': { const char *s = va_arg(ap, const char *); if (!s) s = "(null)";
            int len = (int)strlen(s), pad = width > len ? width - len : 0;
            if (!left) while (pad--) put(&o, ' ');
            while (*s) put(&o, *s++);
            if (left) while (pad--) put(&o, ' ');
            break; }
        case '%': put(&o, '%'); break;
        case 0: fmt--; break;
        default: put(&o, *fmt);
        }
    }
    va_end(ap);
    if (cap > 0) buf[o.n < cap ? o.n : cap - 1] = 0;
    return o.n;
}

/* ------------------------------------------------------------ toast notifications */

Toast g_toasts[TOAST_MAX];

void toast_show(const char *text, int duration_ms, u32 color, int icon) {
    if (!text) return;

    /* Find a free slot */
    int slot = -1;
    for (int i = 0; i < TOAST_MAX; i++) {
        if (!g_toasts[i].used) {
            slot = i;
            break;
        }
    }
    /* If full, evict the oldest */
    if (slot == -1) {
        u32 oldest = hal_ms();
        for (int i = 0; i < TOAST_MAX; i++) {
            if (g_toasts[i].used && g_toasts[i].start < oldest) {
                oldest = g_toasts[i].start;
                slot = i;
            }
        }
    }
    if (slot == -1) return;

    Toast *t = &g_toasts[slot];
    t->used = true;
    t->start = hal_ms();
    t->duration = duration_ms > 0 ? duration_ms : 3000;
    t->color = color ? color : g_accent;
    t->icon = icon;
    t->y_anim = 0;
    kstrcpy(t->text, text, TOAST_TEXT_MAX);
}

void toast_tick(void) {
    u32 now = hal_ms();
    for (int i = 0; i < TOAST_MAX; i++) {
        Toast *t = &g_toasts[i];
        if (!t->used) continue;

        /* Slide in animation (first ~150ms) */
        if (t->y_anim < 30) {
            int elapsed = (int)(now - t->start);
            if (elapsed < 150) {
                t->y_anim = (30 * elapsed) / 150;
            } else {
                t->y_anim = 30;
            }
        }

        /* Check expiry */
        if ((int)(now - t->start) >= t->duration) {
            t->used = false;
        }
    }
}

void toast_draw(void) {
    u32 now = hal_ms();
    int margin = 16;
    int toast_w = 320;
    int toast_h = 50;
    int start_x = g_w - toast_w - margin;
    int start_y = g_h - TASKBAR_H - margin;

    for (int i = 0; i < TOAST_MAX; i++) {
        Toast *t = &g_toasts[i];
        if (!t->used) continue;

        int y = start_y - i * (toast_h + 8) + (30 - t->y_anim);

        /* Background with rounded corners and shadow */
        gfx_shadow(start_x, y, toast_w, toast_h, 8, 12, 0x000000, 60);
        gfx_rrect(start_x, y, toast_w, toast_h, 8, t->color);
        gfx_rrect_frame(start_x, y, toast_w, toast_h, 8, col_lighten(t->color, 40));

        /* Icon */
        if (t->icon >= 0) {
            icon_draw(t->icon, start_x + 12, y + (toast_h - 24) / 2, 24);
        }

        /* Text */
        int text_x = start_x + (t->icon >= 0 ? 48 : 16);
        int text_w = toast_w - (text_x - start_x) - 16;
        char fit[TOAST_TEXT_MAX];
        text_fit(&font_ui, t->text, text_w, fit, sizeof(fit));
        text_draw(&font_ui, text_x, y + (toast_h - font_ui.h) / 2, fit, 0xFFFFFF);
    }
}
