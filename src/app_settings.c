/* Settings: system info, accent color + wallpaper, user accounts, clock format. */
#include "os.h"
typedef struct { int page; TextInput name, pass; int focus; char msg[48]; } Set;
#define NAV_W 210
static const char *const pages[5] = { "Home", "System", "Personalization", "Accounts", "Time & Language" };
static const char *const desc[5] = { "", "Display, about", "Colors, background", "Your accounts, add users", "Clock format" };
static const u32 accents[8] = { 0x0078D7, 0x0099BC, 0x10893E, 0xF7630C, 0xE74856, 0xE3008C, 0x881798, 0x69797E };
static const char *const wall_names[3] = { "Glow", "Waves", "Dusk" };

static void go(Window *w, int p) { Set *s = w->priv; s->page = p; s->msg[0] = 0; char t[48]; ksnprintf(t, sizeof t, "Settings%s%s", p ? " - " : "", p ? pages[p] : ""); wm_set_title(w, t); }
static void row(int x, int y, const char *k, const char *v) { text_draw(&font_ui, x, y, k, 0x666666); text_draw(&font_ui_b, x + 170, y, v, 0x000000); }
static void st_draw(Window *w, int cx, int cy, int cw, int ch) {
    Set *s = w->priv; char b[64];
    gfx_rect(cx, cy, cw, ch, 0xFFFFFF);
    if (s->page == 0) {
        text_draw(&font_title, cx + 40, cy + 30, "Windows Settings", 0x000000);
        for (int i = 1; i < 5; i++) {
            int x = cx + 40 + ((i - 1) % 2) * 280, y = cy + 90 + ((i - 1) / 2) * 100;
            if (ui_hover(w, x, y, 260, 84)) gfx_rect(x, y, 260, 84, 0xE5F1FB);
            icon_draw(i == 1 ? ICON_PC : i == 2 ? ICON_PAINT : i == 3 ? ICON_USER : ICON_CALENDAR, x + 12, y + 20, 40);
            if (i == 3) { gfx_circle(x + 32, y + 40, 20, 0x5D6D7E); icon_draw(ICON_USER, x + 12, y + 20, 40); }
            text_draw(&font_ui_b, x + 66, y + 22, pages[i], 0x000000);
            text_draw(&font_ui, x + 66, y + 44, desc[i], 0x666666);
        }
        return;
    }
    gfx_rect(cx, cy, NAV_W, ch, 0xF2F2F2);
    for (int i = 0; i < 5; i++) {
        int y = cy + 20 + i * 38; bool cur = s->page == i;
        if (cur) { gfx_rect(cx, y, NAV_W, 38, 0xE0E0E0); gfx_rect(cx, y + 6, 4, 26, g_accent); }
        else if (ui_hover(w, cx, y, NAV_W, 38)) gfx_rect(cx, y, NAV_W, 38, 0xE9E9E9);
        text_draw_vc(&font_ui, cx + 20, y, 38, pages[i], 0x000000);
    }
    int x = cx + NAV_W + 30, y = cy + 24;
    text_draw(&font_title, x, y, pages[s->page], 0x000000); y += 48;
    if (s->page == 1) {
        row(x, y, "Device name", "WIMBOES-PC"); y += 28;
        row(x, y, "Edition", "Wimboes 1.0"); y += 28;
        ksnprintf(b, sizeof b, "%d x %d", g_w, g_h); row(x, y, "Display", b); y += 28;
        u32 sec = hal_ms() / 1000; ksnprintf(b, sizeof b, "%02u:%02u:%02u", sec / 3600, sec / 60 % 60, sec % 60); row(x, y, "Uptime", b); y += 28;
        ksnprintf(b, sizeof b, "%u KB of %u KB used", (u32)(mem_used() / 1024), (u32)(mem_total() / 1024)); row(x, y, "Memory (heap)", b); y += 28;
        row(x, y, "Signed in as", g_user >= 0 ? g_users[g_user].name : "-");
    } else if (s->page == 2) {
        text_draw(&font_ui_b, x, y, "Accent color", 0x000000); y += 28;
        for (int i = 0; i < 8; i++) {
            gfx_rect(x + i * 44, y, 36, 36, accents[i]);
            if (accents[i] == g_accent) gfx_frame(x + i * 44 - 3, y - 3, 42, 42, 0x000000);
            else if (ui_hover(w, x + i * 44, y, 36, 36)) gfx_frame(x + i * 44 - 2, y - 2, 40, 40, 0x888888);
        }
        y += 64; text_draw(&font_ui_b, x, y, "Background", 0x000000); y += 28;
        static const u32 th[3][2] = { { 0x0A3C78, 0x35B5FF }, { 0x005A6A, 0x00D6B0 }, { 0x14141E, 0x6B4FA0 } };
        for (int i = 0; i < 3; i++) {
            gfx_gradient(x + i * 130, y, 118, 74, th[i][0], th[i][1]);
            if (g_wallpaper == i) gfx_frame(x + i * 130 - 3, y - 3, 124, 80, g_accent);
            text_draw_c(&font_ui, x + i * 130, 118, y + 80, wall_names[i], 0x000000);
        }
    } else if (s->page == 3) {
        for (int i = 0; i < MAX_USERS; i++) {
            if (!g_users[i].used) continue;
            avatar_draw(x + 20, y + 20, 20, g_users[i].color);
            text_draw(&font_ui_b, x + 56, y + 4, g_users[i].name, 0x000000);
            text_draw(&font_ui, x + 56, y + 22, g_users[i].admin ? "Administrator" : "Local account", 0x666666);
            if (i != g_user && g_user >= 0 && g_users[g_user].admin) ui_button(w, x + 300, y + 6, 80, 28, "Remove", false);
            y += 46;
        }
        y += 8; text_draw(&font_ui_b, x, y, "Add an account", 0x000000); y += 28;
        s->name.focus = s->focus == 0; s->pass.focus = s->focus == 1; s->pass.pass = true;
        ti_draw(&s->name, x, y, 220, 32, "User name", ui_blink()); ti_draw(&s->pass, x + 232, y, 220, 32, "Password (optional)", ui_blink());
        ui_button(w, x, y + 42, 130, 32, "Add account", true);
        text_draw(&font_ui, x + 144, y + 49, s->msg, 0xB00020);
    } else if (s->page == 4) {
        RtcTime r; hal_rtc(&r);
        ksnprintf(b, sizeof b, "%02d:%02d:%02d  %02d/%02d/%d", r.hour, r.min, r.sec, r.month, r.day, r.year); row(x, y, "Current time", b); y += 40;
        text_draw(&font_ui, x, y + 2, "Use 24-hour clock", 0x000000);
        ui_toggle(w, x + 200, y, g_clock24);
    }
}
static void st_mouse(Window *w, int ev, int x, int y, int btn) {
    Set *s = w->priv; (void)btn;
    if (ev != M_DOWN) return;
    if (s->page == 0) { for (int i = 1; i < 5; i++) if (in_rect(x, y, 40 + ((i - 1) % 2) * 280, 90 + ((i - 1) / 2) * 100, 260, 84)) go(w, i); return; }
    for (int i = 0; i < 5; i++) if (in_rect(x, y, 0, 20 + i * 38, NAV_W, 38)) { go(w, i); return; }
    int bx = NAV_W + 30, by = 72;
    if (s->page == 2) {
        for (int i = 0; i < 8; i++) if (in_rect(x, y, bx + i * 44, by + 28, 36, 36)) g_accent = accents[i];
        for (int i = 0; i < 3; i++) if (in_rect(x, y, bx + i * 130, by + 28 + 64 + 28, 118, 74)) wallpaper_make(i);
    } else if (s->page == 3) {
        int yy = by;
        for (int i = 0; i < MAX_USERS; i++) {
            if (!g_users[i].used) continue;
            if (i != g_user && g_user >= 0 && g_users[g_user].admin && in_rect(x, y, bx + 300, yy + 6, 80, 28)) { user_remove(i); s->msg[0] = 0; return; }
            yy += 46;
        }
        yy += 8 + 28;
        if (in_rect(x, y, bx, yy, 220, 32)) s->focus = 0;
        else if (in_rect(x, y, bx + 232, yy, 220, 32)) s->focus = 1;
        else if (in_rect(x, y, bx, yy + 42, 130, 32)) {
            if (g_user < 0 || !g_users[g_user].admin) kstrcpy(s->msg, "Only administrators can add accounts.", sizeof s->msg);
            else if (user_add(s->name.buf, s->pass.buf, false) >= 0) { ti_clear(&s->name); ti_clear(&s->pass); kstrcpy(s->msg, "Account created.", sizeof s->msg); }
            else kstrcpy(s->msg, "Invalid or duplicate user name.", sizeof s->msg);
        }
    } else if (s->page == 4 && in_rect(x, y, bx + 200, by + 40, 40, 20)) g_clock24 = !g_clock24;
}
static void st_key(Window *w, int key, int mod) {
    Set *s = w->priv;
    if (s->page != 3) return;
    if (key == K_TAB) s->focus ^= 1;
    else if (key == K_ENTER) { if (s->focus == 0) s->focus = 1; else if (g_user >= 0 && g_users[g_user].admin && user_add(s->name.buf, s->pass.buf, false) >= 0) { ti_clear(&s->name); ti_clear(&s->pass); kstrcpy(s->msg, "Account created.", sizeof s->msg); s->focus = 0; } else kstrcpy(s->msg, "Invalid or duplicate user name.", sizeof s->msg); }
    else ti_key(s->focus ? &s->pass : &s->name, key, mod);
}
static void st_close(Window *w) { kfree(w->priv); }
Window *settings_new(int page) {
    int x, y; win_cascade(&x, &y);
    Window *w = wm_create(APP_SETTINGS, "Settings", ICON_GEAR, x, y, 760, 500);
    if (!w) return 0;
    Set *s = kmalloc(sizeof(Set));
    if (!s) { wm_close(w); return 0; }
    w->priv = s; w->draw = st_draw; w->mouse = st_mouse; w->key = st_key; w->close = st_close; w->minw = 560; w->minh = 400;
    go(w, page);
    return w;
}
