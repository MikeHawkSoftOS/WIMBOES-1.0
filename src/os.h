/* Wimboes - shared header. Everything above hal_* is hardware independent and
 * is also compiled natively by test/host_main.c so the UI can be tested. */
#ifndef OS_H
#define OS_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int16_t i16; typedef int32_t i32;
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define CLAMP(v,lo,hi) ((v)<(lo)?(lo):((v)>(hi)?(hi):(v)))
#define ARRLEN(a) ((int)(sizeof(a)/sizeof((a)[0])))

/* ---------------------------------------------------------------- lib */
#ifdef HOST
#include <string.h>
#else
void *memcpy(void *d, const void *s, size_t n);
void *memmove(void *d, const void *s, size_t n);
void *memset(void *d, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
#endif
int  ksnprintf(char *buf, int cap, const char *fmt, ...);
void kstrcpy(char *dst, const char *src, int cap);      /* always terminates */
int  kstricmp(const char *a, const char *b);
bool kcontains_ci(const char *hay, const char *needle);
int  katoi(const char *s);

void  mem_init(void *base, size_t size);
void *kmalloc(size_t n);                                 /* zeroed */
void  kfree(void *p);
size_t mem_used(void);
size_t mem_total(void);

/* ---------------------------------------------------------------- hal */
enum { EV_NONE, EV_MOVE, EV_DOWN, EV_UP, EV_KEY, EV_WHEEL };
enum { MOD_SHIFT = 1, MOD_CTRL = 2, MOD_ALT = 4 };
enum { K_BACKSPACE = 8, K_TAB = 9, K_ENTER = 10, K_ESC = 27, K_DEL = 127,
       K_UP = 0x100, K_DOWN, K_LEFT, K_RIGHT, K_HOME, K_END, K_PGUP, K_PGDN, K_F4, K_F5, K_WIN };
typedef struct { int type, x, y, btn, key, mod; } Event;   /* btn: 1=L 2=R 4=M, wheel: key=+-1 */
typedef struct { int year, month, day, hour, min, sec, wday; } RtcTime;

void hal_init(int *w, int *h, u32 **backbuf);
bool hal_next_event(Event *e);
u32  hal_ms(void);
void hal_present(const u32 *bb);
void hal_rtc(RtcTime *t);
void hal_idle(void);
void hal_poweroff(void);
void hal_reboot(void);

/* Disk I/O (ATA PIO mode) */
bool hal_disk_read(u32 sector, void *buf, int count);   /* read count sectors */
bool hal_disk_write(u32 sector, const void *buf, int count);

/* ---------------------------------------------------------------- font / gfx */
typedef struct {
    int h, ascent, mono_w;
    const uint16_t *adv;           /* 1/16 px */
    const uint8_t *gw, *gh; const int8_t *gx, *gy; const uint32_t *off; const uint8_t *bits;
} Font;
extern const Font font_ui, font_ui_b, font_small, font_title, font_big, font_mono;

typedef struct { int x0, y0, x1, y1; } Clip;
extern u32 *g_bb; extern int g_w, g_h;
void gfx_init(u32 *bb, int w, int h);
Clip gfx_clip_push(int x, int y, int w, int h);          /* intersect with current */
void gfx_clip_pop(Clip c);
void gfx_clip_reset(void);
void gfx_pixel(int x, int y, u32 c, int a);
void gfx_rect(int x, int y, int w, int h, u32 c);
void gfx_rect_a(int x, int y, int w, int h, u32 c, int a);
void gfx_frame(int x, int y, int w, int h, u32 c);
void gfx_hline(int x, int y, int w, u32 c);
void gfx_vline(int x, int y, int h, u32 c);
void gfx_gradient(int x, int y, int w, int h, u32 top, u32 bottom);
void gfx_circle(int cx, int cy, int r, u32 c);            /* anti-aliased */
void gfx_ring(int cx, int cy, int r, int thick, u32 c);
void gfx_line(int x0, int y0, int x1, int y1, u32 c);
void gfx_darken(int x, int y, int w, int h, u32 tint, int a);  /* acrylic-ish */
void gfx_rrect(int x, int y, int w, int h, int r, u32 c);              /* filled rounded rect, AA corners */
void gfx_rrect_a(int x, int y, int w, int h, int r, u32 c, int a);     /* alpha version */
void gfx_rrect_frame(int x, int y, int w, int h, int r, u32 c);        /* rounded rect frame/border */
void gfx_rrect_frame_a(int x, int y, int w, int h, int r, u32 c, int a); /* alpha frame */
void gfx_shadow(int x, int y, int w, int h, int r, int spread, u32 color, int alpha); /* drop shadow */
u32  col_blend(u32 dst, u32 src, int a);
u32  col_lighten(u32 c, int a);                           /* toward white */
u32  col_darken(u32 c, int a);                            /* toward black */
int  text_width(const Font *f, const char *s);
void text_draw(const Font *f, int x, int y, const char *s, u32 c);
void text_draw_n(const Font *f, int x, int y, const char *s, int n, u32 c);
void text_draw_r(const Font *f, int right, int y, const char *s, u32 c);
void text_draw_c(const Font *f, int x, int w, int y, const char *s, u32 c);
void text_draw_vc(const Font *f, int x, int y, int h, const char *s, u32 c);
void text_fit(const Font *f, const char *s, int maxw, char *out, int cap);  /* ellipsis */

/* ---------------------------------------------------------------- themes & customization */
#define THEME_NAME_MAX 32
#define THEME_MAX 16
typedef struct {
    bool used;
    char name[THEME_NAME_MAX];
    u32 accent;
    u32 bg_primary, bg_secondary, bg_tertiary;
    u32 fg_primary, fg_secondary, fg_muted;
    u32 border, border_focus;
    u32 hover, active, selection;
    u32 window_bg, titlebar_bg, titlebar_fg, titlebar_fg_inactive;
    u32 taskbar_bg, startmenu_bg, startmenu_rail;
    u32 shadow;
    int wallpaper;
    int shell_style;
    int cursor_theme;
    int ui_scale;        /* 100, 125, 150, 175, 200 */
    bool animations;
    bool transparency;
    int rounding;        /* corner radius 0-16 */
} Theme;
extern Theme g_themes[THEME_MAX];
extern int g_theme;       /* active theme index */
extern bool g_reduced_motion;
void theme_init(void);
void theme_load(int idx);
void theme_save_current(void);
void theme_load_saved(void);
void theme_apply_colors_public(void);
bool theme_parse_json(const char *json, Theme *t);
void theme_to_json(const Theme *t, char *buf, int cap);

/* ---------------------------------------------------------------- widgets / gadgets */
#define WIDGET_MAX 32
#define WIDGET_NAME_MAX 24
enum { WIDGET_CLOCK, WIDGET_SYSMON, WIDGET_NOTES, WIDGET_CALENDAR, WIDGET_WEATHER, WIDGET_LAUNCHER, WIDGET_CUSTOM };
typedef struct Widget Widget;
struct Widget {
    bool used;
    int type;
    char name[WIDGET_NAME_MAX];
    int x, y, w, h;
    int min_w, min_h, max_w, max_h;
    bool pinned;          /* stay on top of desktop icons */
    bool locked;          /* prevent moving */
    void *data;           /* type-specific data */
    void (*draw)(Widget *, int cx, int cy, int cw, int ch);
    void (*mouse)(Widget *, int ev, int x, int y, int btn);
    void (*key)(Widget *, int key, int mod);
    void (*tick)(Widget *);
    void (*close)(Widget *);
};
extern Widget g_widgets[WIDGET_MAX];
Widget *widget_create(int type, int x, int y);
void widget_close(Widget *w);
void widget_draw_all(void);
bool widget_mouse_event(const Event *e);
void widget_key_event(int key, int mod);
void widget_tick_all(void);
void widgets_save(void);
void widgets_load(void);

/* ---------------------------------------------------------------- window manager (forward decls for desktop/snap) */
#define MAX_WIN 24
#define TITLE_H 31
#define TASKBAR_H 40
enum { M_MOVE, M_DOWN, M_UP, M_DBL, M_WHEEL, M_RIGHT };
typedef struct Window Window;

/* ---------------------------------------------------------------- virtual desktops */
#define DESKTOP_MAX 8
typedef struct {
    bool used;
    char name[24];
    int wallpaper;
    int widget_ids[WIDGET_MAX];
    int nwidgets;
} Desktop;
extern Desktop g_desktops[DESKTOP_MAX];
extern int g_desktop;     /* active desktop index */
void desktops_init(void);
void desktop_switch(int idx);
void desktop_new(void);
void desktop_close(int idx);
void desktop_move_win(Window *w, int target);
int  desktop_of_win(Window *w);
void desktops_save(void);
void desktops_load(void);

/* ---------------------------------------------------------------- window tiling / snap */
enum { SNAP_NONE, SNAP_LEFT, SNAP_RIGHT, SNAP_TOP, SNAP_BOTTOM, SNAP_TOPLEFT, SNAP_TOPRIGHT, SNAP_BOTTOMLEFT, SNAP_BOTTOMRIGHT, SNAP_MAXIMIZE };
typedef struct { int zone; u32 hint_start; } SnapState;
extern SnapState g_snap;
void snap_update(Window *w, int mx, int my);
int  snap_zone_at(int mx, int my);
void snap_apply(Window *w, int zone);
void snap_draw_hints(void);

/* Window struct definition (moved up so desktop/snap can use it) */
struct Window {
    bool used, minimized, maximized;
    int x, y, w, h, rx, ry, rw, rh, minw, minh;
    char title[64]; int icon, app, id;
    void *priv;
    /* Animation state */
    float anim_x, anim_y, anim_w, anim_h, anim_alpha;
    u32 anim_start;
    bool animating;
    void (*draw)(struct Window *, int cx, int cy, int cw, int ch);
    void (*mouse)(struct Window *, int ev, int x, int y, int btn);   /* client-relative */
    void (*key)(struct Window *, int key, int mod);
    void (*close)(struct Window *);
};

/* ---------------------------------------------------------------- command palette / launcher */
#define CMD_MAX 64
typedef struct { const char *name; const char *desc; int icon; void (*action)(void *arg); void *arg; } CmdItem;
extern CmdItem g_commands[CMD_MAX];
extern int g_ncmds;
extern bool g_palette_open;
extern char g_palette_query[64];
extern int g_palette_qlen;
extern int g_palette_sel;
void palette_init(void);
void palette_toggle(void);
void palette_draw(void);
void palette_key(int key, int mod);
void palette_mouse(int mx, int my, int btn, int ev);
void palette_register(const char *name, const char *desc, int icon, void (*action)(void *arg), void *arg);

/* ---------------------------------------------------------------- notifications / toasts */
#define TOAST_MAX 16
#define TOAST_TEXT_MAX 128
typedef struct { bool used; u32 start; int duration; u32 color; char text[TOAST_TEXT_MAX]; int icon; int y_anim; } Toast;
extern Toast g_toasts[TOAST_MAX];
void toast_show(const char *text, int duration_ms, u32 color, int icon);
void toast_draw(void);
void toast_tick(void);

/* ---------------------------------------------------------------- startup apps */
#define STARTUP_MAX 16
typedef struct { int app; int arg; bool enabled; } StartupEntry;
extern StartupEntry g_startup[STARTUP_MAX];
extern int g_nstartup;
void startup_init(void);
void startup_run_all(void);
void startup_add(int app, int arg);
void startup_remove(int idx);
void startup_save(void);
void startup_load(void);

/* ---------------------------------------------------------------- cursor themes */
#define CURSOR_MAX 8
#define CURSOR_FRAMES 16
typedef struct { bool used; char name[24]; const char *frames[CURSOR_FRAMES]; int nframes; int hot_x, hot_y; } CursorTheme;
extern CursorTheme g_cursors[CURSOR_MAX];
extern int g_cursor_theme;
void cursors_init(void);
void cursor_draw_themed(void);

/* ---------------------------------------------------------------- ram filesystem */
#define FS_MAX 160
#define FS_NAME 28
#define FS_FILE_MAX 8192
#define FS_THISPC (-2)
typedef struct { bool used, dir; char name[FS_NAME]; int parent; char *data; int size; } FNode;
extern FNode g_fs[FS_MAX];
void fs_init(void);
int  fs_root(void);
int  fs_find(int parent, const char *name);
int  fs_create(int parent, const char *name, bool dir);  /* -1 on failure/exists */
int  fs_unique(int parent, const char *base, const char *ext, bool dir);
void fs_delete(int node);                                /* recursive */
int  fs_list(int parent, int *out, int max);             /* folders first, then A-Z */
bool fs_write(int node, const char *data, int len);
void fs_path(int node, char *buf, int cap);              /* C:\Users\x */
int  fs_resolve(int cwd, const char *path);              /* relative or absolute */
bool fs_rename(int node, const char *name);
bool fs_is_under(int node, int ancestor);
bool fs_trash(int node);                                 /* move to Recycle Bin (or delete if already there) */
int  fs_bin(void);
bool fs_save(void);                                      /* save filesystem to disk */
bool fs_load(void);                                      /* load filesystem from disk */
/* Saves are deferred: changes only mark the data dirty; os_tick flushes it a moment later, so a click or
   keystroke never waits on disk I/O. */
void persist_mark(void);
void persist_clear(void);
void persist_flush(bool force);

/* ---------------------------------------------------------------- users */
#define MAX_USERS 8
typedef struct { bool used, admin; char name[24]; char pass[24]; u32 color; int home; } User;
extern User g_users[MAX_USERS]; extern int g_user;       /* g_user = signed-in index, -1 none */
void users_init(void);
int  user_add(const char *name, const char *pass, bool admin);
bool user_remove(int idx);
int  user_count(void);
int  user_find(const char *name);
int  user_nth(int n);
int  user_docs(int idx);  int user_desktop(int idx);
bool users_save(void);                                   /* save users to disk */
bool users_load(void);                                   /* load users from disk */

/* ---------------------------------------------------------------- ui */
extern u32 g_accent;
extern bool g_clock24;
enum { ICON_FOLDER, ICON_PC, ICON_BIN, ICON_NOTEPAD, ICON_CALC, ICON_PAINT, ICON_TERM, ICON_GEAR,
       ICON_TASKMGR, ICON_FILE, ICON_DRIVE, ICON_USER, ICON_POWER, ICON_LOGO, ICON_SEARCH,
       ICON_DOCS, ICON_PLUS, ICON_BACK, ICON_UP, ICON_NEWFOLDER, ICON_TRASH, ICON_CALENDAR, ICON_GAME,
       /* System */
       ICON_DISPLAY, ICON_SOUND, ICON_NOTIFICATIONS, ICON_POWER_BATTERY, ICON_STORAGE, ICON_ABOUT,
       ICON_THEMES, ICON_LOCK_SCREEN, ICON_START, ICON_TASKBAR_SETTINGS, ICON_APPS, ICON_DEFAULT_APPS,
       ICON_STARTUP, ICON_OFFLINE_MAPS,
       /* Accounts */
       ICON_YOUR_INFO, ICON_SIGNIN_OPTIONS, ICON_FAMILY,
       /* Backup */
       ICON_BACKUP,
       /* Time & Language */
       ICON_DATE_TIME, ICON_LANGUAGE, ICON_TYPING, ICON_SPEECH,
       /* Gaming */
       ICON_GAMING, ICON_CAPTURES, ICON_GAME_BAR,
       /* Accessibility */
       ICON_ACCESSIBILITY, ICON_VISION, ICON_HEARING, ICON_INTERACTION,
       /* Privacy & Security */
       ICON_PRIVACY, ICON_APP_PERMISSIONS, ICON_WINDOWS_SECURITY, ICON_FIND_DEVICE,
       ICON_BITLOCKER, ICON_WINDOWS_UPDATE, ICON_UPDATE_HISTORY, ICON_ADVANCED_OPTIONS,
       /* Common UI */
       ICON_CHEVRON_RIGHT, ICON_CHEVRON_DOWN, ICON_MORE_VERT, ICON_REFRESH,
       ICON_ADD, ICON_REMOVE, ICON_EDIT, ICON_SETTINGS_GEAR };
void icon_draw(int kind, int x, int y, int size);
void avatar_draw(int cx, int cy, int r, u32 color);
void logo_draw(int x, int y, int size, u32 color);
bool in_rect(int px, int py, int x, int y, int w, int h);
extern int g_mx, g_my, g_mb;
extern Window *g_hover_win;
bool ui_hover(Window *w, int x, int y, int bw, int bh);
bool ui_button(Window *w, int x, int y, int bw, int bh, const char *label, bool primary);
bool ui_toggle(Window *w, int x, int y, bool on);
typedef struct { char buf[32]; int len; bool pass; bool focus; } TextInput;
void ti_clear(TextInput *t);
bool ti_key(TextInput *t, int key, int mod);             /* true if consumed */
void ti_draw(TextInput *t, int x, int y, int w, int h, const char *placeholder, bool blink);

/* ---------------------------------------------------------------- window manager */
extern Window g_win[MAX_WIN];
Window *wm_create(int app, const char *title, int icon, int x, int y, int w, int h);
void wm_close(Window *w);
void wm_focus(Window *w);
void wm_minimize(Window *w);
void wm_toggle_max(Window *w);
Window *wm_top(void);
int  wm_count_app(int app);
void wm_set_title(Window *w, const char *t);
void wm_draw_all(void);
bool wm_mouse_event(const Event *e);                     /* true = consumed by a window */
void wm_key_event(int key, int mod);
Window *wm_hit(int x, int y);
void wm_update_hover(bool blocked);
void wm_close_all(void);
int  wm_zcount(void);
Window *wm_zget(int i);                                  /* 0 = bottom */
bool ui_blink(void);

/* ---------------------------------------------------------------- apps / desktop */
typedef struct { const char *name; int icon; bool pinned; } AppDef;
enum { APP_EXPLORER, APP_TERMINAL, APP_NOTEPAD, APP_CALC, APP_PAINT, APP_SETTINGS, APP_TASKMGR, APP_COUNT };
extern const AppDef g_apps[APP_COUNT];
Window *app_launch(int app, int arg);                    /* arg: fs node or -1 */
void app_explorer_open(Window *w, int node);
Window *explorer_new(int node);
Window *notepad_new(int node);
Window *calc_new(void);
Window *paint_new(void);
Window *terminal_new(void);
Window *settings_new(int page);
Window *taskmgr_new(void);
void os_init(void);
void os_event(const Event *e);
void os_render(void);
bool os_tick(void);                                      /* true => needs redraw */
void win_cascade(int *x, int *y);
void wallpaper_make(int kind);
extern int g_wallpaper;
extern int g_shell_style;
void os_signout(void);
void os_shutdown(bool restart);
bool os_running(void);
#endif
