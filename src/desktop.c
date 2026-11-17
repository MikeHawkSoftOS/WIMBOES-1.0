/* The shell: boot splash, login, desktop, taskbar, Start menu, shutdown. */
#include "os.h"

enum { S_BOOT, S_SETUP, S_LOGIN, S_WELCOME, S_DESKTOP, S_SHUTDOWN, S_HALTED };
static int state; static u32 t0; static bool restart_req;
static u32 *wall; int g_wallpaper;
static RtcTime rtc; static u32 rtc_ms = ~0u;
static u32 last_ph;
static u32 state_t; static int prev_state = -1;           /* when the current screen was entered */
static bool anim_busy; static int anim_dt; static u32 last_anim;
static int tb_off, sm_off, sm_prog;                       /* taskbar slide-in, start menu slide */
static u32 *wall_blur; static u32 login_shake_t;
int g_shell_style = 1; /* 0 macOS-like, 1 Windows 11, 2 Windows 10 */
static int sm_scroll;
static int approach(int cur, int tgt, int ms) {
    int d = tgt - cur; if (!d) return cur;
    int st = d * anim_dt / ms; if (!st) st = d > 0 ? 1 : -1;
    if ((d > 0 && st > d) || (d < 0 && st < d)) st = d;
    if (cur + st != tgt) anim_busy = true;
    return cur + st;
}

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
static u32 isqrt(u32 n) {
    u32 r = 0, bit = 1u << 30;
    while (bit > n) bit >>= 2;
    while (bit) { if (n >= r + bit) { n -= r + bit; r = (r >> 1) + bit; } else r >>= 1; bit >>= 2; }
    return r;
}

/* anti-aliased translucent rounded rectangle */
static void rrect_a(int x, int y, int w, int h, int r, u32 c, int a) {
    if (r * 2 > h) r = h / 2;
    if (r * 2 > w) r = w / 2;
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) {
        int dx = i < r ? r - 1 - i : (i > w - 1 - r ? i - (w - r) : -1);
        int dy = j < r ? r - 1 - j : (j > h - 1 - r ? j - (h - r) : -1);
        int al = a;
        if (dx >= 0 && dy >= 0) {
            int d16 = (int)isqrt((u32)(dx * dx + dy * dy) * 256);
            int cv = CLAMP(r * 16 - d16 + 8, 0, 16);
            al = a * cv / 16;
        }
        if (al > 0) gfx_pixel(x + i, y + j, c, al);
    }
}

/* blurred copy of the wallpaper for the login screen */
static void blur_pass(const u32 *s, u32 *d, int w, int h, int horiz) {
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
        int r = 0, g = 0, b = 0;
        for (int k = -2; k <= 2; k++) {
            int xx = horiz ? CLAMP(x + k, 0, w - 1) : x, yy = horiz ? y : CLAMP(y + k, 0, h - 1);
            u32 c = s[yy * w + xx]; r += (int)(c >> 16 & 255); g += (int)(c >> 8 & 255); b += (int)(c & 255);
        }
        d[y * w + x] = (u32)((r / 5) << 16 | (g / 5) << 8 | (b / 5));
    }
}
static int lerp8(int a, int b, int f) { return a + (b - a) * f / 256; }
static void wallpaper_blur(u32 *src) {
    int w = g_w, h = g_h, sw = w / 8 + 1, sh = h / 8 + 1;
    if (!wall_blur) wall_blur = kmalloc((size_t)w * h * 4);
    u32 *s = kmalloc((size_t)sw * sh * 4), *t = kmalloc((size_t)sw * sh * 4);
    if (!wall_blur || !s || !t) { if (wall_blur) { kfree(wall_blur); wall_blur = 0; } kfree(s); kfree(t); return; }
    for (int sy = 0; sy < sh; sy++) for (int sx = 0; sx < sw; sx++) {
        int r = 0, g = 0, b = 0, n = 0;
        for (int j = 0; j < 8; j += 2) for (int i = 0; i < 8; i += 2) {
            u32 c = src[MIN(sy * 8 + j, h - 1) * w + MIN(sx * 8 + i, w - 1)];
            r += (int)(c >> 16 & 255); g += (int)(c >> 8 & 255); b += (int)(c & 255); n++;
        }
        s[sy * sw + sx] = (u32)((r / n) << 16 | (g / n) << 8 | (b / n));
    }
    for (int n = 0; n < 3; n++) { blur_pass(s, t, sw, sh, 1); blur_pass(t, s, sw, sh, 0); }
    for (int y = 0; y < h; y++) {
        int sy = y >> 3, fy = (y & 7) * 32, sy1 = MIN(sy + 1, sh - 1);
        for (int x = 0; x < w; x++) {
            int sx = x >> 3, fx = (x & 7) * 32, sx1 = MIN(sx + 1, sw - 1);
            u32 a = s[sy * sw + sx], b = s[sy * sw + sx1], c = s[sy1 * sw + sx], e = s[sy1 * sw + sx1];
            u32 out = 0;
            for (int sh2 = 16; sh2 >= 0; sh2 -= 8) {
                int top = lerp8((int)(a >> sh2 & 255), (int)(b >> sh2 & 255), fx);
                int bot = lerp8((int)(c >> sh2 & 255), (int)(e >> sh2 & 255), fx);
                out |= (u32)lerp8(top, bot, fy) << sh2;
            }
            wall_blur[y * w + x] = out;
        }
    }
    kfree(s); kfree(t);
}
void wallpaper_make(int kind) {
    if (!wall) wall = kmalloc((size_t)g_w * g_h * 4);
    if (!wall) return;
    g_wallpaper = kind;
    int w = g_w, h = g_h;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            u32 c;
            if (kind == 0) {
                /* Clean slate: a restrained midnight gradient, no glowing logo. */
                int t = (y * 150 / MAX(1, h)) + (x * 35 / MAX(1, w));
                int r = 18 + t * 10 / 255, g = 27 + t * 22 / 255, b = 43 + t * 38 / 255;
                int wave = (int)isqrt((u32)((x - w * 3 / 4) * (x - w * 3 / 4) +
                                            (y - h / 3) * (y - h / 3)));
                int glow = MAX(0, 70 - wave * 70 / MAX(1, w));
                r = CLAMP(r + glow / 3, 0, 255); g = CLAMP(g + glow / 2, 0, 255);
                b = CLAMP(b + glow, 0, 255);
                c = (u32)(r << 16 | g << 8 | b);
            }
            else if (kind == 1) {
                /* macOS style - purple/pink gradient */
                int t = (y * 200 / h + x * 55 / w);
                int r = 100 + t * 100 / 255;
                int g = 50 + t * 80 / 255;
                int b = 150 + t * 80 / 255;
                
                /* Noise texture */
                int noise = ((x * 127 + y * 311) & 255) - 128;
                r = CLAMP(r + noise / 32, 0, 255);
                g = CLAMP(g + noise / 32, 0, 255);
                b = CLAMP(b + noise / 32, 0, 255);
                
                c = (u32)(r << 16 | g << 8 | b);
            }
            else {
                /* Dark theme */
                int dx = x - w / 2, dy = y - h / 2;
                int d = MAX((dx < 0 ? -dx : dx), (dy < 0 ? -dy : dy));
                int i = d < w / 2 ? (w / 2 - d) * 50 / (w / 2) : 0;
                
                int r = 14 + i / 3;
                int g = 14 + i / 3; 
                int b = 24 + i / 2;
                c = (u32)(r << 16 | g << 8 | b);
            }
            wall[y * w + x] = c;
        }
    }
    wallpaper_blur(wall);
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

/* Setup wizard state */
static TextInput setup_name, setup_pass, setup_pass2;
static int setup_step;  /* 0=welcome, 1=name, 2=password, 3=complete */
static bool setup_is_admin;

static void start_reset(void);
static void desktop_reset(void);

void os_init(void) {
    fs_init(); users_init();

    /* Initialize theme system */
    theme_init();

    /* Try to load saved data from disk */
    bool loaded_users = users_load();
    bool loaded_fs = fs_load();
    if (loaded_users && loaded_fs) persist_clear();      /* memory now matches the disk: nothing to write back */

    /* Load saved theme */
    theme_load_saved();

    wallpaper_make(g_wallpaper);
    
    /* Check if we need setup (no users or failed to load) */
    if (user_count() == 0 || !loaded_users) {
        state = S_BOOT; t0 = hal_ms();
        setup_step = 0; 
        setup_is_admin = true;
        setup_name.focus = true; setup_pass.pass = true; setup_pass2.pass = true;
        ti_clear(&setup_name); ti_clear(&setup_pass); ti_clear(&setup_pass2);
    } else {
        sel_user = user_nth(0);
        state = S_BOOT; t0 = hal_ms();
    }
    
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
    memcpy(g_bb, wall_blur ? wall_blur : wall, (size_t)g_w * g_h * 4);
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

/* ------------------------------------------------------------ Windows 11-style Setup Wizard */
static void render_setup(void) {
    gfx_rect(0, 0, g_w, g_h, 0xF3F3F3);
    
    int cx = g_w / 2, cy = g_h / 2;
    
    if (setup_step == 0) {
        /* Welcome screen */
        logo_draw(cx - 60, 100, 120, g_accent);
        text_draw_c(&font_title, 0, g_w, 260, "Welcome to Wimboes", 0x000000);
        text_draw_c(&font_ui, 0, g_w, 310, "Let's set up your device", 0x666666);
        
        bool next_btn = ui_button(0, cx - 80, cy + 80, 160, 44, "Get Started", true);
        if (g_mb & 1 && next_btn) { setup_step = 1; g_mb = 0; }
    }
    else if (setup_step == 1) {
        /* Name input */
        text_draw_c(&font_title, 0, g_w, 150, "Who's going to use this device?", 0x000000);
        text_draw_c(&font_ui, 0, g_w, 200, "Create an account to personalize your experience", 0x666666);
        
        setup_name.focus = true;
        ti_draw(&setup_name, cx - 180, 280, 360, 48, "Your name", ui_blink());
        
        text_draw(&font_small, cx - 180, 340, "Administrator account", 0x666666);
        bool toggle_clicked = ui_toggle(0, cx + 240, 335, setup_is_admin);
        if ((g_mb & 1) && toggle_clicked) { setup_is_admin = !setup_is_admin; g_mb = 0; }
        
        bool next_btn = ui_button(0, cx + 60, cy + 120, 120, 44, "Next", true);
        bool back_btn = ui_button(0, cx - 180, cy + 120, 120, 44, "Back", false);
        
        if ((g_mb & 1) && next_btn && setup_name.len > 0) { setup_step = 2; g_mb = 0; }
        if ((g_mb & 1) && back_btn) { setup_step = 0; g_mb = 0; }
    }
    else if (setup_step == 2) {
        /* Password input */
        text_draw_c(&font_title, 0, g_w, 150, "Create a password", 0x000000);
        text_draw_c(&font_ui, 0, g_w, 200, "Or leave blank for no password", 0x666666);
        
        setup_pass.focus = !setup_pass2.focus;
        ti_draw(&setup_pass, cx - 180, 260, 360, 48, "Password", ui_blink());
        ti_draw(&setup_pass2, cx - 180, 320, 360, 48, "Confirm password", ui_blink());
        
        bool match = strcmp(setup_pass.buf, setup_pass2.buf) == 0;
        bool can_continue = (setup_pass.len == 0 && setup_pass2.len == 0) || (match && setup_pass.len > 0);
        
        if (!match && setup_pass2.len > 0) {
            text_draw_c(&font_small, 0, g_w, 380, "Passwords don't match", 0xE81123);
        }
        
        bool next_btn = ui_button(0, cx + 60, cy + 140, 120, 44, "Create", !can_continue);
        bool back_btn = ui_button(0, cx - 180, cy + 140, 120, 44, "Back", false);
        
        if ((g_mb & 1) && next_btn && can_continue) { setup_step = 3; g_mb = 0; }
        if ((g_mb & 1) && back_btn) { setup_step = 1; g_mb = 0; }
    }
    else if (setup_step == 3) {
        /* Completing setup */
        logo_draw(cx - 48, 180, 96, g_accent);
        text_draw_c(&font_title, 0, g_w, 310, "Setting things up...", 0x000000);
        text_draw_c(&font_ui, 0, g_w, 360, "This will only take a moment", 0x666666);
        spinner(cx, 440, 24);
    }
}

static void setup_event(const Event *e) {
    if (e->type == EV_KEY) {
        if (setup_step == 1) {
            if (e->key == K_ENTER && setup_name.len > 0) setup_step = 2;
            else ti_key(&setup_name, e->key, e->mod);
        } else if (setup_step == 2) {
            if (e->key == K_TAB) {
                setup_pass.focus = !setup_pass.focus;
                setup_pass2.focus = !setup_pass2.focus;
            } else if (e->key == K_ENTER) {
                bool match = strcmp(setup_pass.buf, setup_pass2.buf) == 0;
                if ((setup_pass.len == 0 && setup_pass2.len == 0) || (match && setup_pass.len > 0)) {
                    setup_step = 3;
                }
            } else {
                if (setup_pass.focus) ti_key(&setup_pass, e->key, e->mod);
                else ti_key(&setup_pass2, e->key, e->mod);
            }
        }
    }
}

/* ------------------------------------------------------------ Modern Windows 11 Login */
#define LG_PW_W 240
#define LG_PW_H 34
static int lg_av_cy(void) { return g_h / 2 - 50; }
static int lg_name_y(void) { return lg_av_cy() + 46 + 14; }
static int lg_pw_y(void) { return lg_name_y() + font_title.h + 18; }
static int lg_user_x(int i) { return g_w / 2 + i * 100 - (user_count() - 1) * 50; }
static int lg_user_y(void) { return g_h - 150; }
static int lg_btn_x(int i) { return g_w / 2 - 118 + i * 124; }
static int lg_btn_y(void) { return g_h - 64; }
static int lg_shake(void) {                      /* wrong-password wobble */
    if (!login_shake_t) return 0;
    int dt = (int)(hal_ms() - login_shake_t); if (dt > 500) return 0;
    int p = dt % 160, tri = p < 40 ? p : p < 120 ? 80 - p : p - 160;
    return tri * 14 * (500 - dt) / (40 * 500);
}
static void render_login(void) {
    memcpy(g_bb, wall_blur ? wall_blur : wall, (size_t)g_w * g_h * 4);
    gfx_rect_a(0, 0, g_w, g_h, 0x000000, 45);
    User *u = &g_users[sel_user];
    int cx = g_w / 2; char b[48];
    ksnprintf(b, sizeof b, "%s, %s %d", wd_names[weekday(rtc.year, rtc.month, rtc.day)], mon_names[(rtc.month - 1 + 12) % 12], rtc.day);
    text_draw_c(&font_title, 0, g_w, 40, b, 0xFFFFFF);
    fmt_time(b, sizeof b);
    text_draw_c(&font_big, 0, g_w, 40 + font_title.h + 6, b, 0xFFFFFF);

    avatar_draw(cx, lg_av_cy(), 46, u->color);
    text_draw_c(&font_title, 0, g_w, lg_name_y(), u->name, 0xFFFFFF);

    int px = cx - LG_PW_W / 2 + lg_shake(), py = lg_pw_y();
    if (u->pass[0]) {
        pw.focus = true;
        rrect_a(px, py, LG_PW_W, LG_PW_H, LG_PW_H / 2, 0xFFFFFF, 55);
        if (pw.len == 0) text_draw_vc(&font_ui, px + 18, py, LG_PW_H, "Enter Password", 0xE4ECF5);
        int n = MIN(pw.len, 14);
        for (int k = 0; k < n; k++) gfx_circle(px + 20 + k * 12, py + LG_PW_H / 2, 3, 0xFFFFFF);
        if (pw.len == 0 && ui_blink()) gfx_vline(px + 16, py + 9, LG_PW_H - 18, 0xFFFFFF);
        if (pw.len > 0) {
            int bx = px + LG_PW_W - 18, by = py + LG_PW_H / 2;
            gfx_circle(bx, by, 11, in_rect(g_mx, g_my, px + LG_PW_W - 30, py, 30, LG_PW_H) ? 0xFFFFFF : 0xDCE6F2);
            gfx_line(bx - 5, by, bx + 5, by, 0x2C3E58);
            gfx_line(bx + 1, by - 4, bx + 5, by, 0x2C3E58); gfx_line(bx + 1, by + 4, bx + 5, by, 0x2C3E58);
        }
    } else {
        bool hov = in_rect(g_mx, g_my, px, py, LG_PW_W, LG_PW_H);
        rrect_a(px, py, LG_PW_W, LG_PW_H, LG_PW_H / 2, 0xFFFFFF, hov ? 85 : 55);
        text_draw_c(&font_ui, px, LG_PW_W, py + (LG_PW_H - font_ui.h) / 2, "Click to sign in", 0xFFFFFF);
    }
    if (login_err[0]) text_draw_c(&font_small, 0, g_w, py + LG_PW_H + 14, login_err, 0xFFC4C4);

    if (user_count() > 1) for (int i = 0; i < user_count(); i++) {
        int idx = user_nth(i), x = lg_user_x(i), y = lg_user_y();
        avatar_draw(x, y, 20, g_users[idx].color);
        if (idx == sel_user) gfx_ring(x, y, 25, 2, 0xFFFFFF);
        text_draw_c(&font_small, x - 50, 100, y + 30, g_users[idx].name, 0xFFFFFF);
    }
    for (int i = 0; i < 2; i++) {
        int bx = lg_btn_x(i), by = lg_btn_y();
        rrect_a(bx, by, 112, 32, 16, 0xFFFFFF, in_rect(g_mx, g_my, bx, by, 112, 32) ? 80 : 38);
        text_draw_c(&font_ui, bx, 112, by + (32 - font_ui.h) / 2, i ? "Shut Down" : "Restart", 0xFFFFFF);
    }
    int el = (int)(hal_ms() - state_t);
    if (el < 600) { gfx_rect_a(0, 0, g_w, g_h, 0x000000, 255 - el * 255 / 600); anim_busy = true; }
}
static void login_attempt(void) {
    User *u = &g_users[sel_user];
    if (!strcmp(u->pass, pw.buf)) {
        g_user = sel_user; state = S_WELCOME; t0 = hal_ms(); ti_clear(&pw); login_err[0] = 0; login_shake_t = 0;
    } else {
        kstrcpy(login_err, "Incorrect password. Try again.", sizeof login_err); ti_clear(&pw);
        login_shake_t = hal_ms() | 1;
    }
}
static void login_event(const Event *e) {
    int cx = g_w / 2;
    if (e->type == EV_KEY) {
        if (e->key == K_ENTER) login_attempt();
        else if (ti_key(&pw, e->key, e->mod)) login_err[0] = 0;
        return;
    }
    if (e->type != EV_DOWN || e->btn != 1) return;
    for (int i = 0; i < 2; i++) if (in_rect(e->x, e->y, lg_btn_x(i), lg_btn_y(), 112, 32)) { os_shutdown(i == 1); return; }
    if (user_count() > 1) for (int i = 0; i < user_count(); i++)
        if (in_rect(e->x, e->y, lg_user_x(i) - 40, lg_user_y() - 26, 80, 72)) {
            sel_user = user_nth(i); ti_clear(&pw); login_err[0] = 0; return;
        }
    int px = cx - LG_PW_W / 2, py = lg_pw_y();
    if (g_users[sel_user].pass[0]) { if (pw.len > 0 && in_rect(e->x, e->y, px + LG_PW_W - 30, py, 30, LG_PW_H)) login_attempt(); }
    else if (in_rect(e->x, e->y, px, py, LG_PW_W, LG_PW_H)) login_attempt();
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

    /* Get theme colors */
    Theme *t = (g_theme >= 0 && g_theme < THEME_MAX && g_themes[g_theme].used) ? &g_themes[g_theme] : 0;
    u32 menu_bg = t ? t->bg_primary : 0xF2F2F2;
    u32 menu_border = t ? t->border : 0xA0A0A0;
    u32 hover_bg = t ? t->hover : 0xD6D6D6;
    u32 fg_primary = t ? t->fg_primary : 0x000000;
    u32 shadow = t ? t->shadow : 0x000000;
    int rounding = t ? t->rounding : 8;  /* Context menus always have at least 8px rounding */

    /* Shadow */
    gfx_shadow(ctx.x, ctx.y, w, h, rounding, 8, shadow, 60);

    /* Background with rounded corners */
    gfx_rrect(ctx.x, ctx.y, w, h, rounding, menu_bg);
    gfx_rrect_frame(ctx.x, ctx.y, w, h, rounding, menu_border);

    for (int i = 0; i < ctx.n; i++) {
        int y = ctx.y + 4 + i * 30;
        bool hov = in_rect(g_mx, g_my, ctx.x + 2, y, w - 4, 30);
        if (hov) {
            int item_rounding = 6;
            if (i == 0 && i == ctx.n - 1) item_rounding = rounding;
            else if (i == 0) item_rounding = rounding;
            else if (i == ctx.n - 1) item_rounding = rounding;
            gfx_rrect(ctx.x + 2, y, w - 4, 30, item_rounding, hover_bg);
        }
        /* Icon based on action */
        int icon = ICON_FILE;
        if (ctx.act[i] == CA_REFRESH) icon = ICON_REFRESH;
        else if (ctx.act[i] == CA_NEWFOLDER) icon = ICON_NEWFOLDER;
        else if (ctx.act[i] == CA_NEWTEXT) icon = ICON_NOTEPAD;
        else if (ctx.act[i] == CA_PERSONALIZE) icon = ICON_THEMES;
        else if (ctx.act[i] == CA_OPEN) icon = ICON_FOLDER;
        else if (ctx.act[i] == CA_DELETE) icon = ICON_TRASH;
        else if (ctx.act[i] == CA_TERMINAL) icon = ICON_TERM;
        else if (ctx.act[i] == CA_EMPTYBIN) icon = ICON_BIN;
        icon_draw(icon, ctx.x + 8, y + 7, 16);
        text_draw_vc(&font_ui, ctx.x + 32, y, 30, ctx.label[i], fg_primary);
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
static int sm_y(void) { return g_h - TASKBAR_H - sm_h() + sm_off; }
static void start_reset(void) { start_open = false; sm_scroll = 0; sm_query[0] = 0; sm_qlen = 0; sm_popup = 0; }
static void desktop_reset(void) { dsel = -1; ctx.open = false; dclick_i = -1; }

typedef struct { int app, y, h; char hdr; } SMEnt;       /* app = -1 for a header row */
static int sm_build(SMEnt *e) {
    int idx[APP_COUNT], m = 0, n = 0, y = sm_y() + 8 - sm_scroll;
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

    /* Get theme colors */
    Theme *t = (g_theme >= 0 && g_theme < THEME_MAX && g_themes[g_theme].used) ? &g_themes[g_theme] : 0;
    u32 startmenu_bg = t ? t->startmenu_bg : 0x1B1B1B;
    u32 startmenu_rail = t ? t->startmenu_rail : 0x000000;
    u32 accent = g_accent;
    u32 fg_primary = t ? t->fg_primary : 0xFFFFFF;
    u32 fg_secondary = t ? t->fg_secondary : 0xCCCCCC;
    u32 fg_muted = t ? t->fg_muted : 0x888888;
    u32 hover_bg = t ? t->hover : 0x3C3C3C;
    u32 border = t ? t->border : 0x4A4A4A;
    u32 shadow = t ? t->shadow : 0x000000;
    int rounding = t ? t->rounding : 0;
    bool transparency = t ? t->transparency : false;
    int menu_alpha = transparency ? 240 : 255;

    /* Draw start menu background with shadow and rounded corners */
    if (rounding > 0) {
        /* Shadow */
        gfx_shadow(0, sy, SM_W, sh, rounding, 12, shadow, 80);
        /* Background */
        gfx_rrect_a(0, sy, SM_W, sh, rounding, startmenu_bg, menu_alpha);
        gfx_rrect_frame_a(0, sy, SM_W, sh, rounding, border, menu_alpha);
        /* Rail background */
        gfx_rrect_a(0, sy, SM_RAIL, sh, rounding, startmenu_rail, menu_alpha);
    } else {
        gfx_rect_a(0, sy, SM_W, sh, startmenu_bg, menu_alpha);
        gfx_rect_a(0, sy, SM_RAIL, sh, startmenu_rail, menu_alpha);
    }

    /* Hamburger menu lines */
    for (int i = 0; i < 3; i++) gfx_rect(15, sy + 17 + i * 6, 18, 2, fg_muted);

    static const int ricon[4] = { ICON_POWER, ICON_GEAR, ICON_DOCS, ICON_USER };
    for (int k = 0; k < 4; k++) {
        int y = rail_y(k);
        if (in_rect(g_mx, g_my, 0, y, SM_RAIL, 48) && !sm_popup) {
            if (rounding > 0) gfx_rrect_a(0, y, SM_RAIL, 48, 8, 0xFFFFFF, 30);
            else gfx_rect_a(0, y, SM_RAIL, 48, 0xFFFFFF, 30);
        }
        if (k == 3) avatar_draw(24, y + 24, 12, g_user >= 0 ? g_users[g_user].color : 0x777777);
        else icon_draw(k == 1 ? ICON_GEAR : ricon[k], 12, y + 12, 24);
    }

    SMEnt e[APP_COUNT * 2 + 2]; int n = sm_build(e);
    Clip list_clip = gfx_clip_push(SM_RAIL, sy + 4, SM_APPS_W, sh - 8);
    if (!n) text_draw(&font_ui, SM_RAIL + 16, sy + 20, "No results found.", fg_muted);
    for (int i = 0; i < n; i++) {
        if (e[i].app < 0) {
            char hd[16]; if (e[i].hdr == '*') kstrcpy(hd, "Best match", sizeof hd); else { hd[0] = e[i].hdr; hd[1] = 0; }
            text_draw_vc(&font_ui_b, SM_RAIL + 14, e[i].y, 28, hd, fg_secondary);
        } else {
            if (in_rect(g_mx, g_my, SM_RAIL, e[i].y, SM_APPS_W, 40) && !sm_popup) {
                if (rounding > 0) gfx_rrect_a(SM_RAIL, e[i].y, SM_APPS_W, 40, 8, hover_bg, 255);
                else gfx_rect(SM_RAIL, e[i].y, SM_APPS_W, 40, hover_bg);
            }
            icon_draw(g_apps[e[i].app].icon, SM_RAIL + 14, e[i].y + 8, 24);
            text_draw_vc(&font_ui, SM_RAIL + 50, e[i].y, 40, g_apps[e[i].app].name, fg_primary);
        }
    }
    gfx_clip_pop(list_clip);

    text_draw(&font_ui_b, 316, sy + 16, "Productivity", fg_secondary);
    text_draw(&font_ui_b, 316, sy + 270, "Explore", fg_secondary);

    for (int i = 0; i < ARRLEN(tiles); i++) {
        int x, y, w; tile_rect(i, &x, &y, &w);
        bool hov = in_rect(g_mx, g_my, x, y, w, 100) && !sm_popup;
        u32 tile_bg = tiles[i].app < 0 ? accent : (hov ? col_lighten(accent, 20) : accent);
        if (rounding > 0) {
            gfx_rrect(x, y, w, 100, 12, tile_bg);
            if (hov) gfx_rrect_frame(x, y, w, 100, 12, 0xDDDDDD);
        } else {
            gfx_rect(x, y, w, 100, tile_bg);
            if (hov) gfx_frame(x, y, w, 100, 0xDDDDDD);
        }
        if (tiles[i].app < 0) {
            int wd = weekday(rtc.year, rtc.month, rtc.day);
            char b[24]; ksnprintf(b, sizeof b, "%d", rtc.day);
            text_draw(&font_ui_b, x + 10, y + 10, wd_names[wd], fg_primary);
            text_draw(&font_big, x + 10, y + 28, b, fg_primary);
            ksnprintf(b, sizeof b, "%s %d", mon_names[(rtc.month - 1 + 12) % 12], rtc.year);
            text_draw(&font_small, x + 10, y + 78, b, fg_primary);
            icon_draw(ICON_CALENDAR, x + w - 50, y + 20, 36);
        } else {
            icon_draw(g_apps[tiles[i].app].icon, x + (w - 40) / 2, y + 16, 40);
            { char tn[32]; text_fit(&font_small, g_apps[tiles[i].app].name, w - 12, tn, sizeof tn); text_draw(&font_small, x + 8, y + 100 - 22, tn, fg_primary); }
        }
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
    else if (e->key == K_BACKSPACE) { if (sm_qlen) sm_query[--sm_qlen] = 0; sm_scroll = 0; }
    else if (e->key >= 32 && e->key < 127 && !(e->mod & MOD_CTRL) && sm_qlen < 28) { sm_query[sm_qlen++] = (char)e->key; sm_query[sm_qlen] = 0; sm_scroll = 0; }
}

/* ------------------------------------------------------------ taskbar */
static int tb_search_w(void) { return MIN(260, g_w / 4); }
static int tb_slots(int *apps) {
    int n = 0;
    for (int a = 0; a < APP_COUNT; a++) if (g_apps[a].pinned) apps[n++] = a;
    for (int a = 0; a < APP_COUNT; a++) if (!g_apps[a].pinned && wm_count_app(a)) apps[n++] = a;
    return n;
}
static int tb_apps_x(void) {
    if (g_shell_style == 2) return 48 + tb_search_w() + 8;
    int apps[APP_COUNT], n = tb_slots(apps);
    int group = n * 48;
    return MAX(48 + tb_search_w() + 8, (g_w - group) / 2);
}
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
static int tb_hv[APP_COUNT + 2], tb_iw[APP_COUNT + 2];
static void taskbar_draw(void) {
    int ty = g_h - TASKBAR_H + tb_off;

    /* Get theme colors */
    Theme *t = (g_theme >= 0 && g_theme < THEME_MAX && g_themes[g_theme].used) ? &g_themes[g_theme] : 0;
    u32 taskbar_bg = t ? t->taskbar_bg : (g_shell_style == 0 ? 0xF5F5F5 : (g_shell_style == 1 ? 0x17191E : 0x0C0C0C));
    u32 accent = g_accent;
    u32 fg_primary = t ? t->fg_primary : 0xFFFFFF;
    u32 fg_muted = t ? t->fg_muted : 0x888888;
    u32 border = t ? t->border : 0x4A4A4A;
    int rounding = t ? t->rounding : 0;
    bool transparency = t ? t->transparency : false;

    int taskbar_alpha = transparency ? 230 : 255;

    if (g_shell_style == 0) {
        /* macOS style - centered pill */
        int pill_w = 48 + tb_search_w() + 8 + 48 * tb_slots((int[APP_COUNT]){0}) + 100;
        int pill_x = (g_w - pill_w) / 2;
        if (rounding > 0) {
            gfx_rrect_a(pill_x, ty + 3, pill_w, TASKBAR_H - 6, rounding, taskbar_bg, taskbar_alpha);
            gfx_rrect_frame_a(pill_x, ty + 3, pill_w, TASKBAR_H - 6, rounding, border, taskbar_alpha);
        } else {
            gfx_rect_a(pill_x, ty + 3, pill_w, TASKBAR_H - 6, taskbar_bg, taskbar_alpha);
            gfx_frame(pill_x, ty + 3, pill_w, TASKBAR_H - 6, border);
        }
    } else if (g_shell_style == 1) {
        /* Windows 11 style - centered with rounded corners */
        if (rounding > 0) {
            /* Floating taskbar with rounded corners */
            int tb_margin = 8;
            int tb_w = g_w - tb_margin * 2;
            int tb_h = TASKBAR_H - 4;
            int tb_x = tb_margin;
            int tb_y = ty + 2;
            gfx_shadow(tb_x, tb_y, tb_w, tb_h, rounding, 8, 0x000000, 60);
            gfx_rrect_a(tb_x, tb_y, tb_w, tb_h, rounding, taskbar_bg, taskbar_alpha);
            gfx_rrect_frame_a(tb_x, tb_y, tb_w, tb_h, rounding, border, taskbar_alpha);
        } else {
            gfx_rect_a(0, ty, g_w, TASKBAR_H, taskbar_bg, taskbar_alpha);
        }
    } else {
        /* Windows 10 style - full width */
        gfx_rect_a(0, ty, g_w, TASKBAR_H, taskbar_bg, taskbar_alpha);
    }

    bool sh = in_rect(g_mx, g_my, 0, ty, 48, TASKBAR_H);
    int sv = tb_hv[APP_COUNT] = approach(tb_hv[APP_COUNT], sh ? 255 : 0, 110);

    /* Start button */
    int start_x = (g_shell_style == 1) ? (g_w / 2 - 24) : 0;
    int start_w = 48;
    if (rounding > 0 && g_shell_style == 1) {
        if (start_open) {
            gfx_rrect(start_x, ty, start_w, TASKBAR_H, rounding, col_lighten(accent, 60));
        } else if (sv) {
            gfx_rrect_a(start_x, ty, start_w, TASKBAR_H, rounding, 0xFFFFFF, sv * 22 / 255);
        }
    } else {
        if (start_open) gfx_rect(start_x, ty, start_w, TASKBAR_H, 0x333333); else if (sv) gfx_rect_a(start_x, ty, start_w, TASKBAR_H, 0xFFFFFF, sv * 22 / 255);
    }
    logo_draw(start_x + 14, ty + 10, 20, g_shell_style == 0 ? accent : (sh && !start_open ? accent : fg_primary));

    /* Search box */
    int sx = start_x + start_w, sw = tb_search_w();
    int search_bg = start_open ? 0xFFFFFF : (transparency ? 0xFFFFFF : 0xE6E6E6);
    int search_alpha = start_open ? 255 : (transparency ? 180 : 255);
    if (rounding > 0) {
        gfx_rrect_a(sx, ty + 4, sw, 32, 16, search_bg, search_alpha);
    } else {
        gfx_rect(sx, ty + 4, sw, 32, search_bg);
    }
    tb_magnifier(sx + 8, ty + 11);
    if (start_open && sm_qlen) text_draw_vc(&font_ui, sx + 36, ty + 4, 32, sm_query, 0x000000);
    else text_draw_vc(&font_ui, sx + 36, ty + 4, 32, "Type here to search", fg_muted);
    if (start_open && ui_blink()) gfx_rect(sx + 36 + (sm_qlen ? text_width(&font_ui, sm_query) : 0), ty + 11, 1, 18, 0x000000);

    /* App buttons */
    int apps[APP_COUNT], n = tb_slots(apps); Window *top = wm_top();
    for (int i = 0; i < n; i++) {
        int x = tb_apps_x() + i * 48, cnt = wm_count_app(apps[i]);
        bool act = top && top->app == apps[i], hov = in_rect(g_mx, g_my, x, ty, 48, TASKBAR_H) && !start_open && !ctx.open;
        int hv = tb_hv[i] = approach(tb_hv[i], hov ? 255 : 0, 110);
        int al = (act ? 40 : 0) + hv * 25 / 255;
        if (al) {
            if (rounding > 0) gfx_rrect_a(x, ty, 48, TASKBAR_H, 8, 0xFFFFFF, al);
            else gfx_rect_a(x, ty, 48, TASKBAR_H, 0xFFFFFF, al);
        }
        icon_draw(g_apps[apps[i]].icon, x + 12, ty + 8 - hv * 2 / 255, 24);
        int iw = tb_iw[i] = approach(tb_iw[i], cnt ? (act ? 44 : 24) : 0, 100);
        if (iw > 0) gfx_rect(x + (48 - iw) / 2, ty + TASKBAR_H - 3, iw, 2, act ? 0x8CCBFF : 0x76B9ED);
    }

    /* Clock/date */
    char tm[16], dt[16]; fmt_time(tm, sizeof tm); fmt_date(dt, sizeof dt);
    int cw = MAX(text_width(&font_small, tm), text_width(&font_small, dt)) + 22, cx = g_w - 6 - cw;
    if (in_rect(g_mx, g_my, cx, ty, cw, TASKBAR_H)) {
        if (rounding > 0) gfx_rrect_a(cx, ty, cw, TASKBAR_H, 8, 0xFFFFFF, 25);
        else gfx_rect_a(cx, ty, cw, TASKBAR_H, 0xFFFFFF, 25);
    }
    text_draw_c(&font_small, cx, cw, ty + 5, tm, fg_primary);
    text_draw_c(&font_small, cx, cw, ty + 21, dt, fg_primary);

    /* System tray icons (network, volume, battery, action center) */
    int ix = cx - 100;
    u32 tray_fg = fg_primary;
    gfx_line(ix + 6, ty + 18, ix + 10, ty + 14, tray_fg); gfx_line(ix + 10, ty + 14, ix + 14, ty + 18, tray_fg);
    for (int b = 0; b < 4; b++) gfx_rect(ix + 36 + b * 4, ty + 24 - b * 3, 2, 3 + b * 3, tray_fg);
    gfx_rect(ix + 72, ty + 16, 4, 7, tray_fg); gfx_line(ix + 76, ty + 16, ix + 80, ty + 12, tray_fg); gfx_line(ix + 76, ty + 22, ix + 80, ty + 26, tray_fg); gfx_vline(ix + 80, ty + 12, 15, tray_fg);
    gfx_vline(g_w - 6, ty + 2, TASKBAR_H - 4, border);
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
    u32 now = hal_ms(); anim_busy = false;
    anim_dt = (last_anim && now - last_anim < 100) ? (int)(now - last_anim) : 16; last_anim = now;
    int tgt = start_open ? 1000 : 0;
    if (sm_prog < tgt) sm_prog = MIN(tgt, sm_prog + anim_dt * 1000 / 200);
    else if (sm_prog > tgt) sm_prog = MAX(tgt, sm_prog - anim_dt * 1000 / 150);
    if (sm_prog != tgt) anim_busy = true;
    sm_off = (1000 - sm_prog * (2000 - sm_prog) / 1000) * sm_h() / 1000;
    int el = (int)(now - state_t);
    if (el < 450) { int p = el * 1000 / 450; tb_off = TASKBAR_H * (1000 - p * (2000 - p) / 1000) / 1000; anim_busy = true; } else tb_off = 0;
    memcpy(g_bb, wall, (size_t)g_w * g_h * 4);
    desk_draw();
    wm_update_hover(start_open || ctx.open || g_my >= g_h - TASKBAR_H);
    wm_draw_all();
    taskbar_draw();
    if (start_open || sm_prog > 0) { Clip c = gfx_clip_push(0, 0, g_w, g_h - TASKBAR_H); start_draw(); gfx_clip_pop(c); }
    if (ctx.open) ctx_draw();
    if (el < 500) { gfx_rect_a(0, 0, g_w, g_h, 0x000000, 255 - el * 255 / 500); anim_busy = true; }
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
    if (e->type == EV_WHEEL) {
        if (start_open && in_rect(mx, my, SM_RAIL, sm_y(), SM_APPS_W, sm_h())) {
            SMEnt ent[APP_COUNT * 2 + 2]; int n = sm_build(ent);
            int content = n ? ent[n - 1].y + ent[n - 1].h - (sm_y() + 8 - sm_scroll) : 0;
            int max_scroll = MAX(0, content - (sm_h() - 16));
            sm_scroll = CLAMP(sm_scroll - e->key * 44, 0, max_scroll);
        } else if (!start_open) wm_mouse_event(e);
        return;
    }
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
    if (state == S_SETUP) setup_event(e);
    else if (state == S_LOGIN) login_event(e);
    else if (state == S_DESKTOP) desktop_event(e);
}
void os_render(void) {
    gfx_clip_reset(); rtc_refresh(); anim_busy = false;
    if (state != prev_state) { prev_state = state; state_t = hal_ms(); }
    switch (state) {
    case S_BOOT: render_boot(); break;
    case S_SETUP: render_setup(); break;
    case S_LOGIN: render_login(); break;
    case S_WELCOME: render_welcome(); break;
    case S_DESKTOP: render_desktop(); break;
    case S_SHUTDOWN: render_shutdown(); break;
    default: render_halted(); break;
    }
    if (state == S_SETUP || state == S_LOGIN || state == S_DESKTOP) cursor_draw();
}
bool os_tick(void) {
    u32 ms = hal_ms(); bool d = false;
    static u32 last_save = 0;
    
    switch (state) {
    case S_BOOT: 
        if (ms - t0 > 2600) { 
            if (user_count() == 0) state = S_SETUP;
            else state = S_LOGIN;
            d = true; 
        } 
        break;
    case S_SETUP:
        if (setup_step == 3) {
            /* Complete setup after short delay */
            if (ms - t0 > 1500 || t0 == 0) {
                /* Create the user */
                int idx = user_add(setup_name.buf, setup_pass.buf, setup_is_admin);
                if (idx >= 0) {
                    /* Save new user to disk */
                    users_save();
                    fs_save();
                    
                    g_user = idx;
                    state = S_WELCOME;
                    t0 = hal_ms();
                    d = true;
                }
            } else if (t0 == 0) {
                t0 = ms; /* Start completion timer */
            }
        }
        break;
    case S_WELCOME: if (ms - t0 > 1500) { state = S_DESKTOP; desktop_reset(); d = true; } break;
    case S_SHUTDOWN: 
        if (ms - t0 > 1800) { 
            /* Save everything before shutdown */
            users_save();
            fs_save();
            
            if (restart_req) hal_reboot(); 
            else hal_poweroff(); 
            state = S_HALTED; 
            d = true; 
        } 
        break;
    default: break;
    }
    
    /* Deferred saves (flushed ~1.5 s after the last change) + a full save every 30 s on the desktop.
       Both are instant no-ops when there is no ATA disk, and neither ever runs inside an input event. */
    persist_flush(false);
    if (state == S_DESKTOP && ms - last_save > 30000) {
        persist_clear();
        users_save();
        fs_save();
        last_save = ms;
    }
    
    if (anim_busy || (state == S_LOGIN && login_shake_t && ms - login_shake_t < 520)) d = true;
    u32 ph = (state == S_BOOT || state == S_WELCOME || state == S_SHUTDOWN || state == S_SETUP) ? ms / 60 : ms / 500;
    if (ph != last_ph) { last_ph = ph; d = true; }
    
    /* Update game windows */
    if (state == S_DESKTOP) {
        for (int i = 0; i < wm_zcount(); i++) {
            Window *w = wm_zget(i);
            if (!w->minimized && w->animating) {
                d = true;
                /* Game-specific updates */
            }
        }
    }
    
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
