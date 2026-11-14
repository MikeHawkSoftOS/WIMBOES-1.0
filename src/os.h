/* Wimboes - shared header. Everything above hal_* is hardware independent and
 * is also compiled natively by test/host_main.c so the UI can be tested. */
#ifndef OS_H
#define OS_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t i32;
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

/* ---------------------------------------------------------------- ui */
extern u32 g_accent;
extern bool g_clock24;
enum { ICON_FOLDER, ICON_PC, ICON_BIN, ICON_NOTEPAD, ICON_CALC, ICON_PAINT, ICON_TERM, ICON_GEAR,
       ICON_TASKMGR, ICON_FILE, ICON_DRIVE, ICON_USER, ICON_POWER, ICON_LOGO, ICON_SEARCH,
       ICON_DOCS, ICON_PLUS, ICON_BACK, ICON_UP, ICON_NEWFOLDER, ICON_TRASH, ICON_CALENDAR };
void icon_draw(int kind, int x, int y, int size);
void avatar_draw(int cx, int cy, int r, u32 color);
void logo_draw(int x, int y, int size, u32 color);
bool in_rect(int px, int py, int x, int y, int w, int h);
extern int g_mx, g_my, g_mb;
struct Window;
extern struct Window *g_hover_win;
bool ui_hover(struct Window *w, int x, int y, int bw, int bh);
bool ui_button(struct Window *w, int x, int y, int bw, int bh, const char *label, bool primary);
bool ui_toggle(struct Window *w, int x, int y, bool on);
typedef struct { char buf[32]; int len; bool pass; bool focus; } TextInput;
void ti_clear(TextInput *t);
bool ti_key(TextInput *t, int key, int mod);             /* true if consumed */
void ti_draw(TextInput *t, int x, int y, int w, int h, const char *placeholder, bool blink);

/* ---------------------------------------------------------------- window manager */
#define MAX_WIN 24
#define TITLE_H 31
#define TASKBAR_H 40
enum { M_MOVE, M_DOWN, M_UP, M_DBL, M_WHEEL, M_RIGHT };
typedef struct Window {
    bool used, minimized, maximized;
    int x, y, w, h, rx, ry, rw, rh, minw, minh;
    char title[64]; int icon, app, id;
    void *priv;
    void (*draw)(struct Window *, int cx, int cy, int cw, int ch);
    void (*mouse)(struct Window *, int ev, int x, int y, int btn);   /* client-relative */
    void (*key)(struct Window *, int key, int mod);
    void (*close)(struct Window *);
} Window;
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
void os_signout(void);
void os_shutdown(bool restart);
bool os_running(void);
#endif
