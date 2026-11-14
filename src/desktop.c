/* The shell: boot splash, login, desktop, taskbar, Start menu, shutdown. */
#include "os.h"

enum { S_BOOT, S_LOGIN, S_WELCOME, S_DESKTOP, S_SHUTDOWN, S_HALTED };
static int state; static u32 t0; static bool restart_req;
static u32 *wall; int g_wallpaper;
static RtcTime rtc; static u32 rtc_ms = ~0u;
static u32 last_ph;

const AppDef g_apps[APP_COUNT] = {
    [APP_EXPLORER] = { "File Explorer", ICON_FOLDER, true },
    [APP_TERMINAL] = { "Command Prompt", ICON_TERM, true },
    [APP_NOTEPAD] = { "Notepad", ICON_NOTEPAD, true },
    [APP_CALC] = { "Calculator", ICON_CALC, false },
    [APP_PAINT] = { "Paint", ICON_PAINT, false },
    [APP_SETTINGS] = { "Settings", ICON_GEAR, false },
    [APP_TASKMGR] = { "Task Manager", ICON_TASKMGR, false },
};

static const char *const wd_names[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char *const mon_names[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
static int weekday(int y, int m, int d) {
    static const int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (m < 3) y--;
    return (y + y / 4 - y / 100 + y / 400 + t[(m - 1) % 12] + d) % 7;
}
static void rtc_refresh(void) {
    u32 ms = hal_ms();
    if (rtc_ms == ~0u || ms - rtc_ms > 400) { hal_rtc(&rtc); rtc_ms = ms; }
}
static void fmt_time(char *b, int cap) {
    if (g_clock24) ksnprintf(b, cap, "%02d:%02d", rtc.hour, rtc.min);
    else { int h = rtc.hour % 12; if (!h) h = 12; ksnprintf(b, cap, "%d:%02d %s", h, rtc.min, rtc.hour >= 12 ? "PM" : "AM"); }
}
static void fmt_date(char *b, int cap) { ksnprintf(b, cap, "%d/%d/%d", rtc.month, rtc.day, rtc.year); }

void win_cascade(int *x, int *y) {
    static int n;
    *x = 70 + (n % 8) * 32; *y = 30 + (n % 8) * 28; n++;
}

/* ------------------------------------------------------------ wallpaper */
void wallpaper_make(int kind) {
    if (!wall) wall = kmalloc((size_t)g_w * g_h * 4);
    if (!wall) return;
    g_wallpaper = kind;
    int w = g_w, h = g_h;
    int gx = w * 60 / 100, gy = h * 42 / 100, R = h * 90 / 100, S = h * 30 / 100, gap = h / 120 + 3;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            u32 c;
            if (kind == 0) {
                int dx = x - gx, dy = y - gy, ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
                int d = MAX(ax, ay) + MIN(ax, ay) / 2;
                int i = d < R ? (R - d) * 255 / R : 0; i = i * i / 255;
                int t = x * 100 / w + y * 60 / h;                       /* 0..160 */
                u32 base = (u32)((2 + t / 30) << 16 | (18 + t * 24 / 100) << 8 | (58 + t * 55 / 100));
                c = col_blend(base, 0x35B5FF, i * 215 / 255);
                int u = dx, v = dy + dx * 16 / 100, au = u < 0 ? -u : u, av = v < 0 ? -v : v;
                if (au > gap / 2 && av > gap / 2 && au < S && av < S * 80 / 100)
                    c = col_blend(c, 0xFFFFFF, 40 + i / 5 + (u < 0 ? 0 : 10));
            } else if (kind == 1) {
                int tri = ((x * 2 + y) & 255); tri = tri < 128 ? tri : 255 - tri;
                int t = y * 255 / h;
                c = (u32)((0 + t / 12) << 16 | (80 + t * 80 / 255) << 8 | (95 + t * 40 / 255));
                int wave = h * 55 / 100 + tri / 3 + (x * 40 / w);
                if (y > wave) c = col_blend(c, 0x00D6B0, 55);
                if (y > wave + h / 6) c = col_blend(c, 0x003B4A, 70);
            } else {
                int dx = x - w / 2, dy = y - h / 2, ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
                int d = MAX(ax, ay) + MIN(ax, ay) / 2, i = d < w / 2 ? (w / 2 - d) * 255 / (w / 2) : 0;
                c = col_blend((u32)(0x14 << 16 | 0x14 << 8 | 0x1E), 0x6B4FA0, i / 3);
            }
            wall[y * w + x] = c;
        }
    }
}

/* ------------------------------------------------------------ cursor */
static const char *const cur[19] = {
    "X           ", "XX          ", "X.X         ", "X..X        ", "X...X       ", "X....X      ", "X.....X     ", "X......X    ",
    "X.......X   ", "X........X  ", "X.....XXXXX ", "X..X..X     ", "X.X X..X    ", "XX  X..X    ", "X    X..X   ", "     X..X   ",
    "      X..X  ", "      X..X  ", "       XX   " };
static void cursor_draw(void) {
    for (int j = 0; j < 19; j++) for (int i = 0; i < 12; i++) {
        char c = cur[j][i];
        if (c == 'X') gfx_pixel(g_mx + i, g_my + j, 0x000000, 255);
        else if (c == '.') gfx_pixel(g_mx + i, g_my + j, 0xFFFFFF, 255);
    }
}

/* ------------------------------------------------------------ spinner */
static void spinner(int cx, int cy, int scale) {
    static const int sn[12] = { 0, 50, 87, 100, 87, 50, 0, -50, -87, -100, -87, -50 };
    int head = (int)((hal_ms() / 80) % 12);
    for (int t = 5; t >= 0; t--) {
        int idx = ((head - t) % 12 + 12) % 12;
        int x = cx + sn[idx] * scale / 100, y = cy - sn[(idx + 3) % 12] * scale / 100;
        gfx_circle(x, y, MAX(2, (6 - t) * scale / 24), 0xFFFFFF);
    }
}

/* ------------------------------------------------------------ session state */
static TextInput pw;
static int sel_user;
static char login_err[48];
static bool power_popup;                 /* login screen power menu */

static void start_reset(void);
static void desktop_reset(void);

void os_init(void) {
    fs_init(); users_init();
    wallpaper_make(0);
    sel_user = user_nth(0);
    state = S_BOOT; t0 = hal_ms();
    pw.pass = true; pw.focus = true;
}
bool os_running(void) { return state != S_HALTED; }

void os_signout(void) {
    wm_close_all(); start_reset(); desktop_reset();
    g_user = -1; ti_clear(&pw); login_err[0] = 0; power_popup = false;
    sel_user = user_nth(0); state = S_LOGIN;
}
void os_shutdown(bool restart) { wm_close_all(); start_reset(); restart_req = restart; state = S_SHUTDOWN; t0 = hal_ms(); }

/* ------------------------------------------------------------ boot / welcome / shutdown screens */
static void render_boot(void) {
    gfx_rect(0, 0, g_w, g_h, 0x000000);
    logo_draw(g_w / 2 - 48, g_h / 2 - 110, 96, 0x2E9BFF);
    spinner(g_w / 2, g_h / 2 + 90, 22);
    text_draw_c(&font_small, 0, g_w, g_h - 40, "MikeHawkSoft", 0x8A8A8A);
}
static void render_wall_dark(int a) {
    memcpy(g_bb, wall, (size_t)g_w * g_h * 4);
    gfx_rect_a(0, 0, g_w, g_h, 0x000000, a);
}
static void render_welcome(void) {
    render_wall_dark(110);
    User *u = &g_users[g_user];
    avatar_draw(g_w / 2, g_h / 2 - 90, 64, u->color);
    text_draw_c(&font_big, 0, g_w, g_h / 2 - 5, u->name, 0xFFFFFF);
    text_draw_c(&font_ui, 0, g_w, g_h / 2 + 48, "Welcome", 0xFFFFFF);
    spinner(g_w / 2, g_h / 2 + 110, 18);
}
static void render_shutdown(void) {
    gfx_rect(0, 0, g_w, g_h, 0x0A3A78);
    text_draw_c(&font_title, 0, g_w, g_h / 2 - 12, restart_req ? "Restarting" : "Shutting down", 0xFFFFFF);
    spinner(g_w / 2, g_h / 2 + 60, 20);
}
static void render_halted(void) {
    gfx_rect(0, 0, g_w, g_h, 0x000000);
    text_draw_c(&font_title, 0, g_w, g_h / 2 - 12, "It's now safe to turn off your computer.", 0xFF9A1F);
}

/* ------------------------------------------------------------ login */
static int lg_cx(void) { return g_w / 2; }
static int lg_field_y(void) { return g_h / 2 + 62; }
static int lg_row_y(int i) { return g_h - 24 - (user_count() - i) * 60; }
static void render_login(void) {
    render_wall_dark(95);
    User *u = &g_users[sel_user];
    int cx = lg_cx();
    avatar_draw(cx, g_h / 2 - 90, 64, u->color);
    text_draw_c(&font_big, 0, g_w, g_h / 2 - 5, u->name, 0xFFFFFF);
    int fy = lg_field_y();
    if (u->pass[0]) {
        pw.focus = true;
        ti_draw(&pw, cx - 130, fy, 260, 36, "Password", ui_blink());
        gfx_rect(cx + 130, fy, 36, 36, g_accent);
        icon_draw(ICON_BACK, cx + 136, fy + 6, 24);           /* dark arrow -> lighten below */
        gfx_rect(cx + 130, fy, 36, 36, g_accent);
        gfx_line(cx + 139, fy + 18, cx + 157, fy + 18, 0xFFFFFF);
        gfx_line(cx + 150, fy + 11, cx + 157, fy + 18, 0xFFFFFF); gfx_line(cx + 150, fy + 25, cx + 157, fy + 18, 0xFFFFFF);
    } else {
        bool hov = in_rect(g_mx, g_my, cx - 60, fy, 120, 36);
        gfx_rect(cx - 60, fy, 120, 36, hov ? col_lighten(g_accent, 20) : g_accent);
        text_draw_c(&font_ui, cx - 60, 120, fy + (36 - font_ui.h) / 2, "Sign in", 0xFFFFFF);
    }
    if (login_err[0]) text_draw_c(&font_ui, 0, g_w, fy + 50, login_err, 0xFFFFFF);
    for (int n = 0, i = 0; n < MAX_USERS; n++) {
        if (!g_users[n].used) continue;
        int y = lg_row_y(i++);
        bool hov = in_rect(g_mx, g_my, 16, y, 250, 52);
        if (n == sel_user) gfx_rect_a(16, y, 250, 52, 0xFFFFFF, 40);
        else if (hov) gfx_rect_a(16, y, 250, 52, 0xFFFFFF, 20);
        avatar_draw(46, y + 26, 20, g_users[n].color);
        text_draw_vc(&font_title, 78, y, 52, g_users[n].name, 0xFFFFFF);
    }
    bool ph = in_rect(g_mx, g_my, g_w - 64, g_h - 64, 40, 40);
    if (ph || power_popup) gfx_rect_a(g_w - 64, g_h - 64, 40, 40, 0xFFFFFF, 30);
    icon_draw(ICON_POWER, g_w - 56, g_h - 56, 24);
    if (power_popup) {
        int px = g_w - 64 - 130, py = g_h - 64 - 76;
        gfx_rect(px, py, 170, 72, 0x2B2B2B);
        const char *it[2] = { "Shut down", "Restart" };
        for (int i = 0; i < 2; i++) {
            if (in_rect(g_mx, g_my, px, py + i * 36, 170, 36)) gfx_rect(px, py + i * 36, 170, 36, 0x404040);
            text_draw_vc(&font_ui, px + 14, py + i * 36, 36, it[i], 0xFFFFFF);
        }
    }
}
static void login_attempt(void) {
    User *u = &g_users[sel_user];
    if (!strcmp(u->pass, pw.buf)) {
        g_user = sel_user; state = S_WELCOME; t0 = hal_ms(); ti_clear(&pw); login_err[0] = 0;
    } else { kstrcpy(login_err, "The password is incorrect. Try again.", sizeof login_err); ti_clear(&pw); }
}
static void login_event(const Event *e) {
    int cx = lg_cx(), fy = lg_field_y();
    if (e->type == EV_KEY) {
        if (e->key == K_ENTER) login_attempt();
        else if (e->key == K_ESC) power_popup = false;
        else if (ti_key(&pw, e->key, e->mod)) login_err[0] = 0;
        return;
    }
    if (e->type != EV_DOWN || e->btn != 1) return;
    int px = g_w - 64 - 130, py = g_h - 64 - 76;
    if (power_popup && in_rect(e->x, e->y, px, py, 170, 72)) { os_shutdown((e->y - py) >= 36); return; }
    power_popup = in_rect(e->x, e->y, g_w - 64, g_h - 64, 40, 40) ? !power_popup : false;
    if (g_users[sel_user].pass[0] ? in_rect(e->x, e->y, cx + 130, fy, 36, 36) : in_rect(e->x, e->y, cx - 60, fy, 120, 36)) { login_attempt(); return; }
    for (int n = 0, i = 0; n < MAX_USERS; n++) {
        if (!g_users[n].used) continue;
        if (in_rect(e->x, e->y, 16, lg_row_y(i++), 250, 52)) { sel_user = n; ti_clear(&pw); login_err[0] = 0; }
    }
}

/* ------------------------------------------------------------ desktop icons + context menu */
typedef struct { int kind, node; } DItem;                 /* 0 This PC, 1 Recycle Bin, 2 fs node */
static DItem ditems[48]; static int nditems, dsel = -1;
static u32 dclick_t; static int dclick_i = -1;

static void desk_collect(void) {
    nditems = 0;
    ditems[nditems++] = (DItem){ 0, FS_THISPC };
    ditems[nditems++] = (DItem){ 1, fs_bin() };
    if (g_user >= 0) {
        int list[40], n = fs_list(user_desktop(g_user), list, 40);
        for (int i = 0; i < n; i++) ditems[nditems++] = (DItem){ 2, list[i] };
    }
}
static int desk_rows(void) { return MAX(1, (g_h - TASKBAR_H - 8) / 92); }
static void desk_pos(int i, int *x, int *y) { int r = desk_rows(); *x = 6 + (i / r) * 86; *y = 6 + (i % r) * 92; }
static int desk_hit(int mx, int my) {
    for (int i = 0; i < nditems; i++) { int x, y; desk_pos(i, &x, &y); if (in_rect(mx, my, x, y, 86, 90)) return i; }
    return -1;
}
static const char *ditem_label(const DItem *d) { return d->kind == 0 ? "This PC" : d->kind == 1 ? "Recycle Bin" : g_fs[d->node].name; }
static int ditem_icon(const DItem *d) { return d->kind == 0 ? ICON_PC : d->kind == 1 ? ICON_BIN : g_fs[d->node].dir ? ICON_FOLDER : ICON_FILE; }
static void desk_open(const DItem *d) {
    if (d->kind == 0) app_launch(APP_EXPLORER, FS_THISPC);
    else if (d->kind == 1 || g_fs[d->node].dir) app_launch(APP_EXPLORER, d->node);
    else app_launch(APP_NOTEPAD, d->node);
}
static void desk_draw(void) {
    desk_collect();
    for (int i = 0; i < nditems; i++) {
        int x, y; desk_pos(i, &x, &y);
        bool hov = !g_hover_win && in_rect(g_mx, g_my, x, y, 86, 90) && g_my < g_h - TASKBAR_H;
        if (i == dsel) { gfx_rect_a(x, y, 86, 90, g_accent, 90); gfx_frame(x, y, 86, 90, col_lighten(g_accent, 30)); }
        else if (hov) gfx_rect_a(x, y, 86, 90, 0xFFFFFF, 35);
        icon_draw(ditem_icon(&ditems[i]), x + 23, y + 8, 40);
        char t[FS_NAME + 4]; text_fit(&font_ui, ditem_label(&ditems[i]), 80, t, sizeof t);
        text_draw_c(&font_ui, x + 1, 86, y + 56, t, 0x000000);
        text_draw_c(&font_ui, x, 86, y + 55, t, 0xFFFFFF);
    }
}

enum { CA_NONE, CA_REFRESH, CA_NEWFOLDER, CA_NEWTEXT, CA_PERSONALIZE, CA_OPEN, CA_DELETE, CA_TERMINAL, CA_EMPTYBIN };
static struct { bool open; int x, y, n, act[6], item; const char *label[6]; } ctx;
static void ctx_add(int act, const char *l) { ctx.act[ctx.n] = act; ctx.label[ctx.n++] = l; }
static void ctx_show(int x, int y) { ctx.open = true; ctx.x = MIN(x, g_w - 232); ctx.y = MIN(y, g_h - TASKBAR_H - ctx.n * 30 - 10); }
static void ctx_draw(void) {
    int w = 230, h = ctx.n * 30 + 8;
    gfx_rect_a(ctx.x - 1, ctx.y + 3, w + 3, h + 3, 0x000000, 40);
    gfx_rect(ctx.x, ctx.y, w, h, 0xF2F2F2); gfx_frame(ctx.x, ctx.y, w, h, 0xA0A0A0);
    for (int i = 0; i < ctx.n; i++) {
        int y = ctx.y + 4 + i * 30;
        if (in_rect(g_mx, g_my, ctx.x + 2, y, w - 4, 30)) gfx_rect(ctx.x + 2, y, w - 4, 30, 0xD6D6D6);
        text_draw_vc(&font_ui, ctx.x + 32, y, 30, ctx.label[i], 0x000000);
    }
}
static void ctx_run(int act) {
    int dn = g_user >= 0 ? user_desktop(g_user) : -1;
    switch (act) {
    case CA_NEWFOLDER: fs_unique(dn, "New folder", "", true); break;
    case CA_NEWTEXT: fs_unique(dn, "New Text Document", ".txt", false); break;
    case CA_PERSONALIZE: app_launch(APP_SETTINGS, 2); break;
    case CA_TERMINAL: app_launch(APP_TERMINAL, -1); break;
    case CA_OPEN: if (ctx.item >= 0 && ctx.item < nditems) desk_open(&ditems[ctx.item]); break;
    case CA_DELETE: if (ctx.item >= 0 && ctx.item < nditems && ditems[ctx.item].kind == 2) fs_trash(ditems[ctx.item].node); dsel = -1; break;
    case CA_EMPTYBIN: { int bin = fs_bin(), l[40], n = fs_list(bin, l, 40); for (int i = 0; i < n; i++) fs_delete(l[i]); break; }
    default: break;
    }
}

/* ------------------------------------------------------------ start menu */
#define SM_W 640
#define SM_RAIL 48
#define SM_APPS_W 258
static bool start_open; static char sm_query[32]; static int sm_qlen, sm_popup;   /* 0 none, 1 user, 2 power */
static int sm_h(void) { return MIN(560, g_h - TASKBAR_H - 4); }
static int sm_y(void) { return g_h - TASKBAR_H - sm_h(); }
static void start_reset(void) { start_open = false; sm_query[0] = 0; sm_qlen = 0; sm_popup = 0; }
static void desktop_reset(void) { dsel = -1; ctx.open = false; dclick_i = -1; }

typedef struct { int app, y, h; char hdr; } SMEnt;       /* app = -1 for a header row */
static int sm_build(SMEnt *e) {
    int idx[APP_COUNT], m = 0, n = 0, y = sm_y() + 8;
    for (int a = 0; a < APP_COUNT; a++) if (!sm_qlen || kcontains_ci(g_apps[a].name, sm_query)) idx[m++] = a;
    for (int i = 1; i < m; i++) { int v = idx[i], j = i - 1; while (j >= 0 && kstricmp(g_apps[idx[j]].name, g_apps[v].name) > 0) { idx[j + 1] = idx[j]; j--; } idx[j + 1] = v; }
    char last = 0;
    for (int i = 0; i < m; i++) {
        char l = sm_qlen ? '*' : g_apps[idx[i]].name[0];
        if (l != last) { e[n++] = (SMEnt){ -1, y, 28, l }; y += 28; last = l; }
        e[n++] = (SMEnt){ idx[i], y, 40, 0 }; y += 40;
    }
    return n;
}
static const struct { int app, col, row, wc, grp; } tiles[] = {
    { -1, 0, 0, 2, 0 }, { APP_NOTEPAD, 2, 0, 1, 0 }, { APP_EXPLORER, 0, 1, 1, 0 }, { APP_CALC, 1, 1, 1, 0 }, { APP_PAINT, 2, 1, 1, 0 },
    { APP_TERMINAL, 0, 0, 1, 1 }, { APP_TASKMGR, 1, 0, 1, 1 }, { APP_SETTINGS, 2, 0, 1, 1 } };
static void tile_rect(int i, int *x, int *y, int *w) {
    *x = 316 + tiles[i].col * 104; *y = (tiles[i].grp ? sm_y() + 298 : sm_y() + 44) + tiles[i].row * 104;
    *w = tiles[i].wc * 100 + (tiles[i].wc - 1) * 4;
}
static int rail_y(int k) { return sm_y() + sm_h() - 48 * (k + 1); }      /* 0 power, 1 settings, 2 docs, 3 user */

static void start_draw(void) {
    int sy = sm_y(), sh = sm_h();
    gfx_rect_a(0, sy, SM_W, sh, 0x1B1B1B, 240);
    gfx_rect_a(0, sy, SM_RAIL, sh, 0x000000, 70);
    for (int i = 0; i < 3; i++) gfx_rect(15, sy + 17 + i * 6, 18, 2, 0xFFFFFF);
    static const int ricon[4] = { ICON_POWER, ICON_GEAR, ICON_DOCS, ICON_USER };
    for (int k = 0; k < 4; k++) {
        int y = rail_y(k);
        if (in_rect(g_mx, g_my, 0, y, SM_RAIL, 48) && !sm_popup) gfx_rect_a(0, y, SM_RAIL, 48, 0xFFFFFF, 30);
        if (k == 3) avatar_draw(24, y + 24, 12, g_user >= 0 ? g_users[g_user].color : 0x777777);
        else icon_draw(k == 1 ? ICON_GEAR : ricon[k], 12, y + 12, 24);
    }
    SMEnt e[APP_COUNT * 2 + 2]; int n = sm_build(e);
    if (!n) text_draw(&font_ui, SM_RAIL + 16, sy + 20, "No results found.", 0xBBBBBB);
    for (int i = 0; i < n; i++) {
        if (e[i].app < 0) {
            char hd[16]; if (e[i].hdr == '*') kstrcpy(hd, "Best match", sizeof hd); else { hd[0] = e[i].hdr; hd[1] = 0; }
            text_draw_vc(&font_ui_b, SM_RAIL + 14, e[i].y, 28, hd, 0xFFFFFF);
        } else {
            if (in_rect(g_mx, g_my, SM_RAIL, e[i].y, SM_APPS_W, 40) && !sm_popup) gfx_rect(SM_RAIL, e[i].y, SM_APPS_W, 40, 0x3C3C3C);
            icon_draw(g_apps[e[i].app].icon, SM_RAIL + 14, e[i].y + 8, 24);
            text_draw_vc(&font_ui, SM_RAIL + 50, e[i].y, 40, g_apps[e[i].app].name, 0xFFFFFF);
        }
    }
    text_draw(&font_ui_b, 316, sy + 16, "Productivity", 0xE6E6E6);
    text_draw(&font_ui_b, 316, sy + 270, "Explore", 0xE6E6E6);
    for (int i = 0; i < ARRLEN(tiles); i++) {
        int x, y, w; tile_rect(i, &x, &y, &w);
        bool hov = in_rect(g_mx, g_my, x, y, w, 100) && !sm_popup;
        gfx_rect(x, y, w, 100, g_accent);
        if (tiles[i].app < 0) {
            int wd = weekday(rtc.year, rtc.month, rtc.day);
            char b[24]; ksnprintf(b, sizeof b, "%d", rtc.day);
            text_draw(&font_ui_b, x + 10, y + 10, wd_names[wd], 0xFFFFFF);
            text_draw(&font_big, x + 10, y + 28, b, 0xFFFFFF);
            ksnprintf(b, sizeof b, "%s %d", mon_names[(rtc.month - 1 + 12) % 12], rtc.year);
            text_draw(&font_small, x + 10, y + 78, b, 0xFFFFFF);
            icon_draw(ICON_CALENDAR, x + w - 50, y + 20, 36);
        } else {
            icon_draw(g_apps[tiles[i].app].icon, x + (w - 40) / 2, y + 16, 40);
            { char tn[32]; text_fit(&font_small, g_apps[tiles[i].app].name, w - 12, tn, sizeof tn); text_draw(&font_small, x + 8, y + 100 - 22, tn, 0xFFFFFF); }
        }
        if (hov) gfx_frame(x, y, w, 100, 0xDDDDDD);
    }
    if (sm_popup) {
        static const char *const up[2] = { "Change account settings", "Sign out" }, *const pp[2] = { "Shut down", "Restart" };
        const char *const *it = sm_popup == 1 ? up : pp;
        int py = rail_y(sm_popup == 1 ? 3 : 0) - 2 * 40 + 48;
        gfx_rect(4, py, 230, 80, 0x2B2B2B); gfx_frame(4, py, 230, 80, 0x555555);
        for (int i = 0; i < 2; i++) {
            if (in_rect(g_mx, g_my, 4, py + i * 40, 230, 40)) gfx_rect(5, py + i * 40, 228, 40, 0x434343);
            text_draw_vc(&font_ui, 18, py + i * 40, 40, it[i], 0xFFFFFF);
        }
    }
}
static void start_launch(int app, int arg) { Window *w = app_launch(app, arg); (void)w; start_reset(); }
static bool start_click(int mx, int my) {
    int sy = sm_y();
    if (sm_popup) {
        int py = rail_y(sm_popup == 1 ? 3 : 0) - 2 * 40 + 48;
        if (in_rect(mx, my, 4, py, 230, 80)) {
            int i = (my - py) / 40, p = sm_popup;
            start_reset();
            if (p == 1) { if (i == 0) app_launch(APP_SETTINGS, 3); else os_signout(); }
            else os_shutdown(i == 1);
            return true;
        }
        sm_popup = 0;
        if (mx < SM_RAIL) return true;
    }
    if (mx < SM_RAIL) {
        for (int k = 0; k < 4; k++) if (in_rect(mx, my, 0, rail_y(k), SM_RAIL, 48)) {
            if (k == 0) sm_popup = 2;
            else if (k == 1) start_launch(APP_SETTINGS, 0);
            else if (k == 2) start_launch(APP_EXPLORER, g_user >= 0 ? user_docs(g_user) : FS_THISPC);
            else sm_popup = 1;
        }
        return true;
    }
    SMEnt e[APP_COUNT * 2 + 2]; int n = sm_build(e);
    for (int i = 0; i < n; i++) if (e[i].app >= 0 && in_rect(mx, my, SM_RAIL, e[i].y, SM_APPS_W, 40)) { start_launch(e[i].app, -1); return true; }
    for (int i = 0; i < ARRLEN(tiles); i++) {
        int x, y, w; tile_rect(i, &x, &y, &w);
        if (tiles[i].app >= 0 && in_rect(mx, my, x, y, w, 100)) { start_launch(tiles[i].app, -1); return true; }
    }
    (void)sy;
    return true;
}
static void start_key(const Event *e) {
    if (e->key == K_ESC) { if (sm_popup) sm_popup = 0; else start_reset(); }
    else if (e->key == K_ENTER) { SMEnt en[APP_COUNT * 2 + 2]; int n = sm_build(en); for (int i = 0; i < n; i++) if (en[i].app >= 0 && sm_qlen) { start_launch(en[i].app, -1); return; } }
    else if (e->key == K_BACKSPACE) { if (sm_qlen) sm_query[--sm_qlen] = 0; }
    else if (e->key >= 32 && e->key < 127 && !(e->mod & MOD_CTRL) && sm_qlen < 28) { sm_query[sm_qlen++] = (char)e->key; sm_query[sm_qlen] = 0; }
}

/* ------------------------------------------------------------ taskbar */
static int tb_search_w(void) { return MIN(260, g_w / 4); }
static int tb_slots(int *apps) {
    int n = 0;
    for (int a = 0; a < APP_COUNT; a++) if (g_apps[a].pinned) apps[n++] = a;
    for (int a = 0; a < APP_COUNT; a++) if (!g_apps[a].pinned && wm_count_app(a)) apps[n++] = a;
    return n;
}
static int tb_apps_x(void) { return 48 + tb_search_w() + 8; }
static void tb_app_click(int app) {
    int n = wm_count_app(app);
    if (!n) { app_launch(app, -1); return; }
    Window *top = wm_top(), *first = 0, *last = 0;
    for (int i = 0; i < wm_zcount(); i++) if (wm_zget(i)->app == app) { if (!first) first = wm_zget(i); last = wm_zget(i); }
    if (n == 1) { if (last == top) wm_minimize(last); else wm_focus(last); }
    else if (top && top->app == app) wm_focus(first);
    else wm_focus(last);
}
static void tb_magnifier(int x, int y) { gfx_ring(x + 9, y + 9, 6, 2, 0x333333); gfx_line(x + 13, y + 13, x + 19, y + 19, 0x333333); gfx_line(x + 14, y + 13, x + 20, y + 19, 0x333333); }
static void taskbar_draw(void) {
    int ty = g_h - TASKBAR_H;
    gfx_rect_a(0, ty, g_w, TASKBAR_H, 0x0C0C0C, 232);
    bool sh = in_rect(g_mx, g_my, 0, ty, 48, TASKBAR_H);
    if (start_open) gfx_rect(0, ty, 48, TASKBAR_H, 0x333333); else if (sh) gfx_rect(0, ty, 48, TASKBAR_H, 0x2E2E2E);
    logo_draw(14, ty + 10, 20, sh && !start_open ? g_accent : 0xFFFFFF);
    int sx = 48, sw = tb_search_w();
    gfx_rect(sx, ty + 4, sw, 32, start_open ? 0xFFFFFF : 0xE6E6E6);
    tb_magnifier(sx + 8, ty + 11);
    if (start_open && sm_qlen) text_draw_vc(&font_ui, sx + 36, ty + 4, 32, sm_query, 0x000000);
    else text_draw_vc(&font_ui, sx + 36, ty + 4, 32, "Type here to search", 0x4A4A4A);
    if (start_open && ui_blink()) gfx_rect(sx + 36 + (sm_qlen ? text_width(&font_ui, sm_query) : 0), ty + 11, 1, 18, 0x000000);
    int apps[APP_COUNT], n = tb_slots(apps); Window *top = wm_top();
    for (int i = 0; i < n; i++) {
        int x = tb_apps_x() + i * 48, cnt = wm_count_app(apps[i]);
        bool act = top && top->app == apps[i], hov = in_rect(g_mx, g_my, x, ty, 48, TASKBAR_H) && !start_open && !ctx.open;
        if (act) gfx_rect_a(x, ty, 48, TASKBAR_H, 0xFFFFFF, 40); else if (hov) gfx_rect_a(x, ty, 48, TASKBAR_H, 0xFFFFFF, 25);
        icon_draw(g_apps[apps[i]].icon, x + 12, ty + 8, 24);
        if (cnt) gfx_rect(x + (act ? 2 : 12), ty + TASKBAR_H - 3, act ? 44 : 24, 2, act ? 0x8CCBFF : 0x76B9ED);
    }
    char tm[16], dt[16]; fmt_time(tm, sizeof tm); fmt_date(dt, sizeof dt);
    int cw = MAX(text_width(&font_small, tm), text_width(&font_small, dt)) + 22, cx = g_w - 6 - cw;
    if (in_rect(g_mx, g_my, cx, ty, cw, TASKBAR_H)) gfx_rect_a(cx, ty, cw, TASKBAR_H, 0xFFFFFF, 25);
    text_draw_c(&font_small, cx, cw, ty + 5, tm, 0xFFFFFF);
    text_draw_c(&font_small, cx, cw, ty + 21, dt, 0xFFFFFF);
    int ix = cx - 100;                                   /* tray glyphs: chevron, network, speaker */
    gfx_line(ix + 6, ty + 18, ix + 10, ty + 14, 0xFFFFFF); gfx_line(ix + 10, ty + 14, ix + 14, ty + 18, 0xFFFFFF);
    for (int b = 0; b < 4; b++) gfx_rect(ix + 36 + b * 4, ty + 24 - b * 3, 2, 3 + b * 3, 0xFFFFFF);
    gfx_rect(ix + 72, ty + 16, 4, 7, 0xFFFFFF); gfx_line(ix + 76, ty + 16, ix + 80, ty + 12, 0xFFFFFF); gfx_line(ix + 76, ty + 22, ix + 80, ty + 26, 0xFFFFFF); gfx_vline(ix + 80, ty + 12, 15, 0xFFFFFF);
    gfx_vline(g_w - 6, ty + 2, TASKBAR_H - 4, 0x555555);
}
static void taskbar_click(int mx, int my) {
    (void)my;
    int cw = 0; (void)cw;
    if (mx < 48) { if (start_open) start_reset(); else { start_open = true; sm_popup = 0; } return; }
    if (mx < 48 + tb_search_w()) { start_open = true; sm_popup = 0; return; }
    if (mx >= g_w - 6) { for (int i = 0; i < wm_zcount(); i++) wm_minimize(wm_zget(i)); start_reset(); return; }
    int apps[APP_COUNT], n = tb_slots(apps);
    for (int i = 0; i < n; i++) if (mx >= tb_apps_x() + i * 48 && mx < tb_apps_x() + i * 48 + 48) { start_reset(); tb_app_click(apps[i]); return; }
    start_reset();
}

/* ------------------------------------------------------------ desktop state */
static void render_desktop(void) {
    memcpy(g_bb, wall, (size_t)g_w * g_h * 4);
    desk_draw();
    wm_update_hover(start_open || ctx.open || g_my >= g_h - TASKBAR_H);
    wm_draw_all();
    taskbar_draw();
    if (start_open) start_draw();
    if (ctx.open) ctx_draw();
}
static void desktop_event(const Event *e) {
    int mx = e->x, my = e->y;
    if (e->type == EV_KEY) {
        if (e->key == K_WIN || (e->key == K_ESC && (e->mod & MOD_CTRL))) { if (start_open) start_reset(); else start_open = true; return; }
        if (start_open) { start_key(e); return; }
        if (ctx.open) { if (e->key == K_ESC) ctx.open = false; return; }
        wm_key_event(e->key, e->mod);
        return;
    }
    if (e->type == EV_MOVE || e->type == EV_UP) { wm_mouse_event(e); return; }
    if (e->type == EV_WHEEL) { if (!start_open) wm_mouse_event(e); return; }
    if (e->type != EV_DOWN) return;
    if (ctx.open) {
        bool inside = in_rect(mx, my, ctx.x, ctx.y, 230, ctx.n * 30 + 8);
        ctx.open = false;
        if (inside) { int i = (my - ctx.y - 4) / 30; if (i >= 0 && i < ctx.n) ctx_run(ctx.act[i]); }
        return;
    }
    if (my >= g_h - TASKBAR_H) { if (e->btn == 1) taskbar_click(mx, my); return; }
    if (start_open) {
        if (in_rect(mx, my, 0, sm_y(), SM_W, sm_h())) start_click(mx, my); else start_reset();
        return;
    }
    if (wm_mouse_event(e)) { dsel = -1; return; }
    int i = desk_hit(mx, my);
    ctx.n = 0; ctx.item = i;
    if (e->btn == 2) {
        if (i >= 0) {
            dsel = i; ctx_add(CA_OPEN, "Open");
            if (ditems[i].kind == 2) ctx_add(CA_DELETE, "Delete");
            if (ditems[i].kind == 1) ctx_add(CA_EMPTYBIN, "Empty Recycle Bin");
        } else {
            dsel = -1; ctx_add(CA_REFRESH, "Refresh"); ctx_add(CA_NEWFOLDER, "New folder"); ctx_add(CA_NEWTEXT, "New text document");
            ctx_add(CA_TERMINAL, "Open Command Prompt here"); ctx_add(CA_PERSONALIZE, "Personalize");
        }
        ctx_show(mx, my);
        return;
    }
    u32 now = hal_ms();
    if (i >= 0 && i == dclick_i && now - dclick_t < 500) { desk_open(&ditems[i]); dclick_i = -1; }
    else { dclick_i = i; dclick_t = now; }
    dsel = i;
}

/* ------------------------------------------------------------ entry points */
void os_event(const Event *e) {
    if (e->type == EV_MOVE || e->type == EV_DOWN || e->type == EV_UP) { g_mx = e->x; g_my = e->y; }
    if (e->type == EV_DOWN) g_mb |= e->btn;
    if (e->type == EV_UP) g_mb &= ~e->btn;
    if (state == S_LOGIN) login_event(e);
    else if (state == S_DESKTOP) desktop_event(e);
}
void os_render(void) {
    gfx_clip_reset(); rtc_refresh();
    switch (state) {
    case S_BOOT: render_boot(); break;
    case S_LOGIN: render_login(); break;
    case S_WELCOME: render_welcome(); break;
    case S_DESKTOP: render_desktop(); break;
    case S_SHUTDOWN: render_shutdown(); break;
    default: render_halted(); break;
    }
    if (state == S_LOGIN || state == S_DESKTOP) cursor_draw();
}
bool os_tick(void) {
    u32 ms = hal_ms(); bool d = false;
    switch (state) {
    case S_BOOT: if (ms - t0 > 2600) { state = S_LOGIN; d = true; } break;
    case S_WELCOME: if (ms - t0 > 1500) { state = S_DESKTOP; desktop_reset(); d = true; } break;
    case S_SHUTDOWN: if (ms - t0 > 1800) { if (restart_req) hal_reboot(); else hal_poweroff(); state = S_HALTED; d = true; } break;
    default: break;
    }
    u32 ph = (state == S_BOOT || state == S_WELCOME || state == S_SHUTDOWN) ? ms / 60 : ms / 500;
    if (ph != last_ph) { last_ph = ph; d = true; }
    return d;
}
Window *app_launch(int app, int arg) {
    Window *w = 0;
    switch (app) {
    case APP_EXPLORER: w = explorer_new(arg == -1 ? FS_THISPC : arg); break;
    case APP_NOTEPAD: w = notepad_new(arg); break;
    case APP_CALC: w = calc_new(); break;
    case APP_PAINT: w = paint_new(); break;
    case APP_TERMINAL: w = terminal_new(); break;
    case APP_SETTINGS: w = settings_new(arg < 0 ? 0 : arg); break;
    case APP_TASKMGR: w = taskmgr_new(); break;
    }
    if (w) wm_focus(w);
    return w;
}
