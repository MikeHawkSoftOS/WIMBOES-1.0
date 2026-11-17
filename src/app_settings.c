/* Settings: comprehensive Windows-like settings app with multiple pages. */
#include "os.h"
typedef struct { int page; TextInput name, pass; int focus; char msg[48]; } Set;
#define NAV_W 210
static const char *const pages[17] = {
    "Home", "System", "Personalization", "Accounts", "Time & Language",
    "Display", "Sound", "Notifications", "Power", "Storage",
    "Themes", "Colors", "Start", "Taskbar", "Startup Apps",
    "Accessibility", "Privacy & Security"
};
static const char *const desc[17] = {
    "", "Display, about", "Colors, background", "Your accounts, add users", "Clock format",
    "Scale, layout, multiple displays", "Output, input, volume mixer", "Manage notifications", "Battery, sleep, power mode", "Drives, storage sense",
    "Select a theme", "Accent color, custom colors", "Start menu settings", "Taskbar behaviors", "Apps that start automatically",
    "Vision, hearing, interaction", "Privacy, security, Windows Update"
};
static const u32 accents[8] = { 0x0078D7, 0x0099BC, 0x10893E, 0xF7630C, 0xE74856, 0xE3008C, 0x881798, 0x69797E };
static const char *const wall_names[3] = { "Midnight", "Aurora", "Dusk" };
static const char *const shell_names[2] = { "Windows 11 (centered)", "Windows 10 (left-aligned)" };

static void go(Window *w, int p) { Set *s = w->priv; s->page = p; s->msg[0] = 0; char t[48]; ksnprintf(t, sizeof t, "Settings%s%s", p ? " - " : "", p ? pages[p] : ""); wm_set_title(w, t); }
static void row(int x, int y, const char *k, const char *v) { text_draw(&font_ui, x, y, k, 0x666666); text_draw(&font_ui_b, x + 170, y, v, 0x000000); }
static void st_draw(Window *w, int cx, int cy, int cw, int ch) {
    Set *s = w->priv; char b[64];
    gfx_rect(cx, cy, cw, ch, 0xFFFFFF);
    if (s->page == 0) {
        text_draw(&font_title, cx + 40, cy + 30, "Windows Settings", 0x000000);
        for (int i = 1; i < 17; i++) {
            int col = (i - 1) % 3;
            int row_idx = (i - 1) / 3;
            int x = cx + 40 + col * 240;
            int y = cy + 90 + row_idx * 100;
            if (ui_hover(w, x, y, 220, 84)) gfx_rect(x, y, 220, 84, 0xE5F1FB);
            int icon = ICON_PC;
            if (i == 1) icon = ICON_PC;
            else if (i == 2) icon = ICON_PAINT;
            else if (i == 3) icon = ICON_USER;
            else if (i == 4) icon = ICON_CALENDAR;
            else if (i == 5) icon = ICON_DISPLAY;
            else if (i == 6) icon = ICON_SOUND;
            else if (i == 7) icon = ICON_NOTIFICATIONS;
            else if (i == 8) icon = ICON_POWER_BATTERY;
            else if (i == 9) icon = ICON_STORAGE;
            else if (i == 10) icon = ICON_THEMES;
            else if (i == 11) icon = ICON_THEMES;
            else if (i == 12) icon = ICON_START;
            else if (i == 13) icon = ICON_TASKBAR_SETTINGS;
            else if (i == 14) icon = ICON_STARTUP;
            else if (i == 15) icon = ICON_ACCESSIBILITY;
            else if (i == 16) icon = ICON_PRIVACY;
            icon_draw(icon, x + 12, y + 20, 40);
            text_draw(&font_ui_b, x + 66, y + 22, pages[i], 0x000000);
            text_draw(&font_ui, x + 66, y + 44, desc[i], 0x666666);
        }
        return;
    }
    gfx_rect(cx, cy, NAV_W, ch, 0xF2F2F2);
    for (int i = 0; i < 17; i++) {
        int y = cy + 20 + i * 38; bool cur = s->page == i;
        if (cur) { gfx_rect(cx, y, NAV_W, 38, 0xE0E0E0); gfx_rect(cx, y + 6, 4, 26, g_accent); }
        else if (ui_hover(w, cx, y, NAV_W, 38)) gfx_rect(cx, y, NAV_W, 38, 0xE9E9E9);
        text_draw_vc(&font_ui, cx + 20, y, 38, pages[i], 0x000000);
    }
    int x = cx + NAV_W + 30, y = cy + 24;
    text_draw(&font_title, x, y, pages[s->page], 0x000000); y += 48;

    if (s->page == 1) { /* System */
        row(x, y, "Device name", "WIMBOES-PC"); y += 28;
        row(x, y, "Edition", "Wimboes 1.0"); y += 28;
        ksnprintf(b, sizeof b, "%d x %d", g_w, g_h); row(x, y, "Display", b); y += 28;
        u32 sec = hal_ms() / 1000; ksnprintf(b, sizeof b, "%02u:%02u:%02u", sec / 3600, sec / 60 % 60, sec % 60); row(x, y, "Uptime", b); y += 28;
        ksnprintf(b, sizeof b, "%u KB of %u KB used", (u32)(mem_used() / 1024), (u32)(mem_total() / 1024)); row(x, y, "Memory (heap)", b); y += 28;
        row(x, y, "Signed in as", g_user >= 0 ? g_users[g_user].name : "-");
    } else if (s->page == 2) { /* Personalization */
        text_draw(&font_ui_b, x, y, "Accent color", 0x000000); y += 28;
        for (int i = 0; i < 8; i++) {
            gfx_rect(x + i * 44, y, 36, 36, accents[i]);
            if (accents[i] == g_accent) gfx_frame(x + i * 44 - 3, y - 3, 42, 42, 0x000000);
            else if (ui_hover(w, x + i * 44, y, 36, 36)) gfx_frame(x + i * 44 - 2, y - 2, 40, 40, 0x888888);
        }
        y += 64; text_draw(&font_ui_b, x, y, "Desktop & login background", 0x000000); y += 28;
        static const u32 th[3][2] = { { 0x0A3C78, 0x35B5FF }, { 0x005A6A, 0x00D6B0 }, { 0x14141E, 0x6B4FA0 } };
        for (int i = 0; i < 3; i++) {
            gfx_gradient(x + i * 130, y, 118, 74, th[i][0], th[i][1]);
            if (g_wallpaper == i) gfx_frame(x + i * 130 - 3, y - 3, 124, 80, g_accent);
            text_draw_c(&font_ui, x + i * 130, 118, y + 80, wall_names[i], 0x000000);
        }
        y += 112;
        text_draw(&font_ui_b, x, y, "Desktop style", 0x000000); y += 28;
        for (int i = 0; i < 2; i++) {
            int sx = x + i * 130;
            gfx_rect(sx, y, 118, 48, g_shell_style == i ? 0xE3F0FC : 0xF0F0F0);
            gfx_frame(sx, y, 118, 48, g_shell_style == i ? g_accent : 0xD0D0D0);
            text_draw_c(&font_ui, sx, 118, y + 16, shell_names[i], 0x222222);
        }
        text_draw(&font_small, x, y + 58, "Changes the taskbar layout.", 0x666666);
    } else if (s->page == 3) { /* Accounts */
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
    } else if (s->page == 4) { /* Time & Language */
        RtcTime r; hal_rtc(&r);
        ksnprintf(b, sizeof b, "%02d:%02d:%02d  %02d/%02d/%d", r.hour, r.min, r.sec, r.month, r.day, r.year); row(x, y, "Current time", b); y += 40;
        text_draw(&font_ui, x, y + 2, "Use 24-hour clock", 0x000000);
        ui_toggle(w, x + 200, y, g_clock24);
        y += 36;
        text_draw(&font_ui, x, y + 2, "Language", 0x000000);
        ui_button(w, x + 200, y, 120, 28, "English (US)", false);
    } else if (s->page == 5) { /* Display */
        text_draw(&font_ui_b, x, y, "Scale & layout", 0x000000); y += 28;
        row(x, y, "Display resolution", "1920 x 1080 (Recommended)"); y += 28;
        row(x, y, "Scale", "100% (Recommended)"); y += 28;
        row(x, y, "Orientation", "Landscape"); y += 40;
        text_draw(&font_ui_b, x, y, "Multiple displays", 0x000000); y += 28;
        row(x, y, "Display 1", "WIMBOES Monitor"); y += 28;
        ui_button(w, x, y, 120, 32, "Identify", false);
    } else if (s->page == 6) { /* Sound */
        text_draw(&font_ui_b, x, y, "Output", 0x000000); y += 28;
        row(x, y, "Output device", "WIMBOES Audio Device"); y += 28;
        text_draw(&font_ui_b, x, y, "Master volume", 0x000000); y += 28;
        gfx_rect(x, y, 200, 16, 0xE1E1E1);
        gfx_rect(x, y, 150, 16, g_accent);
        y += 40;
        text_draw(&font_ui_b, x, y, "Input", 0x000000); y += 28;
        row(x, y, "Input device", "WIMBOES Microphone"); y += 28;
        text_draw(&font_ui_b, x, y, "Microphone volume", 0x000000); y += 28;
        gfx_rect(x, y, 200, 16, 0xE1E1E1);
        gfx_rect(x, y, 100, 16, g_accent);
        y += 40;
        ui_button(w, x, y, 140, 32, "Volume mixer", false);
    } else if (s->page == 7) { /* Notifications */
        text_draw(&font_ui_b, x, y, "Notifications", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Get notifications from apps and other senders", 0x666666); y += 28;
        ui_toggle(w, x, y, true); y += 36;
        text_draw(&font_ui_b, x, y, "Notification settings", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Show notifications on the lock screen", 0x666666);
        ui_toggle(w, x + 300, y, true); y += 32;
        text_draw(&font_ui, x, y, "Show reminders and incoming calls on the lock screen", 0x666666);
        ui_toggle(w, x + 300, y, false); y += 32;
        text_draw(&font_ui, x, y, "Do not disturb mode", 0x666666);
        ui_toggle(w, x + 300, y, false);
    } else if (s->page == 8) { /* Power */
        text_draw(&font_ui_b, x, y, "Power mode", 0x000000); y += 28;
        row(x, y, "Power mode", "Balanced"); y += 28;
        text_draw(&font_ui, x, y, "Screen and sleep", 0x000000); y += 28;
        row(x, y, "On battery power, turn off screen after", "5 minutes"); y += 28;
        row(x, y, "When plugged in, turn off screen after", "10 minutes"); y += 28;
        row(x, y, "On battery power, put device to sleep after", "15 minutes"); y += 28;
        row(x, y, "When plugged in, put device to sleep after", "30 minutes"); y += 40;
        text_draw(&font_ui_b, x, y, "Battery saver", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Turn battery saver on automatically at", 0x666666); y += 28;
        row(x, y, "Battery saver threshold", "20%"); y += 40;
        ui_button(w, x, y, 140, 32, "Battery usage", false);
    } else if (s->page == 9) { /* Storage */
        text_draw(&font_ui_b, x, y, "Local storage", 0x000000); y += 28;
        row(x, y, "C: (WIMBOES)", "8.5 GB free of 20 GB"); y += 28;
        text_draw(&font_ui_b, x, y, "Storage sense", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Automatically free up space", 0x666666);
        ui_toggle(w, x + 300, y, false); y += 36;
        text_draw(&font_ui, x, y, "Delete temporary files", 0x666666);
        ui_toggle(w, x + 300, y, true); y += 36;
        text_draw(&font_ui, x, y, "Run Storage Sense", 0x666666);
        ui_toggle(w, x + 300, y, true);
    } else if (s->page == 10) { /* Themes */
        text_draw(&font_ui_b, x, y, "Select a theme", 0x000000); y += 28;
        for (int i = 0; i < 5; i++) {
            if (!g_themes[i].used) continue;
            int sx = x + (i % 3) * 180;
            int sy = y + (i / 3) * 130;
            gfx_rect(sx, sy, 160, 110, 0xF2F2F2);
            if (i == g_theme) gfx_frame(sx, sy, 160, 110, g_accent);
            gfx_rect(sx + 10, sy + 10, 140, 70, g_themes[i].window_bg);
            gfx_rect(sx + 10, sy + 10, 140, 24, g_themes[i].titlebar_bg);
            text_draw_c(&font_ui, sx, 160, sy + 84, g_themes[i].name, 0x000000);
        }
    } else if (s->page == 11) { /* Colors */
        text_draw(&font_ui_b, x, y, "Choose your mode", 0x000000); y += 28;
        static const char *modes[2] = { "Light", "Dark" };
        for (int i = 0; i < 2; i++) {
            int sx = x + i * 130;
            gfx_rect(sx, y, 118, 48, (g_theme == i) ? 0xE3F0FC : 0xF0F0F0);
            gfx_frame(sx, y, 118, 48, (g_theme == i) ? g_accent : 0xD0D0D0);
            text_draw_c(&font_ui, sx, 118, y + 16, modes[i], 0x222222);
        }
        y += 72;
        text_draw(&font_ui_b, x, y, "Accent color", 0x000000); y += 28;
        for (int i = 0; i < 8; i++) {
            gfx_rect(x + i * 44, y, 36, 36, accents[i]);
            if (accents[i] == g_accent) gfx_frame(x + i * 44 - 3, y - 3, 42, 42, 0x000000);
            else if (ui_hover(w, x + i * 44, y, 36, 36)) gfx_frame(x + i * 44 - 2, y - 2, 40, 40, 0x888888);
        }
        y += 64;
        text_draw(&font_ui_b, x, y, "Custom color", 0x000000); y += 28;
        ui_button(w, x, y, 140, 32, "View colors", false);
        y += 52;
        text_draw(&font_ui_b, x, y, "Transparency effects", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Enable transparency for taskbar, Start, and action center", 0x666666);
        ui_toggle(w, x + 300, y, g_themes[g_theme].transparency); y += 36;
        text_draw(&font_ui_b, x, y, "Rounded corners", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Enable rounded corners for windows and menus", 0x666666);
        ui_toggle(w, x + 300, y, g_themes[g_theme].rounding > 0);
    } else if (s->page == 12) { /* Start */
        text_draw(&font_ui_b, x, y, "Start menu settings", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Show recently added apps", 0x666666);
        ui_toggle(w, x + 300, y, true); y += 32;
        text_draw(&font_ui, x, y, "Show most used apps", 0x666666);
        ui_toggle(w, x + 300, y, true); y += 32;
        text_draw(&font_ui, x, y, "Show recently opened items in Start, Jump Lists, and File Explorer", 0x666666);
        ui_toggle(w, x + 300, y, true); y += 32;
        text_draw(&font_ui, x, y, "Show account", 0x666666);
        ui_toggle(w, x + 300, y, true); y += 32;
        text_draw(&font_ui, x, y, "Show settings", 0x666666);
        ui_toggle(w, x + 300, y, true); y += 32;
        text_draw(&font_ui, x, y, "Show power", 0x666666);
        ui_toggle(w, x + 300, y, true);
    } else if (s->page == 13) { /* Taskbar */
        text_draw(&font_ui_b, x, y, "Taskbar behaviors", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Taskbar alignment", 0x666666); y += 28;
        static const char *aligns[2] = { "Center (Windows 11)", "Left (Windows 10)" };
        for (int i = 0; i < 2; i++) {
            int sx = x + i * 180;
            gfx_rect(sx, y, 160, 36, (g_shell_style == i) ? 0xE3F0FC : 0xF0F0F0);
            gfx_frame(sx, y, 160, 36, (g_shell_style == i) ? g_accent : 0xD0D0D0);
            text_draw_c(&font_ui, sx, 160, y + 10, aligns[i], 0x222222);
        }
        y += 60;
        text_draw(&font_ui, x, y, "Automatically hide the taskbar", 0x666666);
        ui_toggle(w, x + 300, y, false); y += 32;
        text_draw(&font_ui, x, y, "Show badges on taskbar buttons", 0x666666);
        ui_toggle(w, x + 300, y, true); y += 32;
        text_draw(&font_ui, x, y, "Show taskbar on all displays", 0x666666);
        ui_toggle(w, x + 300, y, false);
    } else if (s->page == 14) { /* Startup Apps */
        text_draw(&font_ui_b, x, y, "Startup apps", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Apps that run at startup", 0x666666); y += 36;
        for (int a = 0; a < APP_COUNT; a++) {
            if (g_apps[a].pinned) {
                icon_draw(g_apps[a].icon, x, y + 4, 24);
                text_draw(&font_ui_b, x + 32, y + 4, g_apps[a].name, 0x000000);
                text_draw(&font_ui, x + 32, y + 22, "Enabled", 0x0078D7);
                ui_toggle(w, x + 250, y + 4, true);
                y += 36;
            }
        }
        text_draw(&font_ui, x, y, "No other startup apps configured", 0x666666);
    } else if (s->page == 15) { /* Accessibility */
        text_draw(&font_ui_b, x, y, "Vision", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Text size", 0x666666); y += 28;
        gfx_rect(x, y, 200, 16, 0xE1E1E1);
        gfx_rect(x, y, 100, 16, g_accent);
        y += 40;
        text_draw(&font_ui, x, y, "Visual effects", 0x666666);
        ui_toggle(w, x + 200, y, g_themes[g_theme].animations); y += 32;
        text_draw(&font_ui, x, y, "Transparency effects", 0x666666);
        ui_toggle(w, x + 200, y, g_themes[g_theme].transparency); y += 32;
        text_draw(&font_ui, x, y, "Show animations", 0x666666);
        ui_toggle(w, x + 200, y, g_themes[g_theme].animations); y += 32;
        text_draw(&font_ui_b, x, y, "Hearing", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Mono audio", 0x666666);
        ui_toggle(w, x + 200, y, false); y += 32;
        text_draw(&font_ui, x, y, "Flash screen during notifications", 0x666666);
        ui_toggle(w, x + 200, y, false); y += 40;
        text_draw(&font_ui_b, x, y, "Interaction", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Mouse pointer size", 0x666666); y += 28;
        gfx_rect(x, y, 200, 16, 0xE1E1E1);
        gfx_rect(x, y, 100, 16, g_accent);
        y += 40;
        text_draw(&font_ui, x, y, "Text cursor thickness", 0x666666);
        ui_toggle(w, x + 200, y, false);
    } else if (s->page == 16) { /* Privacy & Security */
        text_draw(&font_ui_b, x, y, "Windows Security", 0x000000); y += 28;
        ui_button(w, x, y, 140, 32, "Open Windows Security", false); y += 52;
        text_draw(&font_ui_b, x, y, "Privacy", 0x000000); y += 28;
        text_draw(&font_ui, x, y, "Let apps access your location", 0x666666);
        ui_toggle(w, x + 300, y, false); y += 32;
        text_draw(&font_ui, x, y, "Let apps access your camera", 0x666666);
        ui_toggle(w, x + 300, y, false); y += 32;
        text_draw(&font_ui, x, y, "Let apps access your microphone", 0x666666);
        ui_toggle(w, x + 300, y, false); y += 32;
        text_draw(&font_ui, x, y, "Send diagnostic data to Microsoft", 0x666666);
        ui_toggle(w, x + 300, y, false); y += 52;
        text_draw(&font_ui_b, x, y, "Windows Update", 0x000000); y += 28;
        row(x, y, "Status", "You're up to date"); y += 28;
        ui_button(w, x, y, 140, 32, "Check for updates", false);
    }
}
static void st_mouse(Window *w, int ev, int x, int y, int btn) {
    Set *s = w->priv; (void)btn;
    if (ev != M_DOWN) return;
    if (s->page == 0) { for (int i = 1; i < 17; i++) { int col = (i - 1) % 3; int row_idx = (i - 1) / 3; int xx = 40 + col * 240; int yy = 90 + row_idx * 100; if (in_rect(x, y, xx, yy, 220, 84)) { go(w, i); return; } } return; }
    for (int i = 0; i < 17; i++) if (in_rect(x, y, 0, 20 + i * 38, NAV_W, 38)) { go(w, i); return; }
    int bx = NAV_W + 30, by = 72;
    if (s->page == 2) { /* Personalization */
        for (int i = 0; i < 8; i++) if (in_rect(x, y, bx + i * 44, by + 28, 36, 36)) {
            g_accent = accents[i];
            if (g_theme == 4) { g_themes[4].accent = accents[i]; theme_save_current(); }
        }
        for (int i = 0; i < 3; i++) if (in_rect(x, y, bx + i * 130, by + 28 + 64 + 28, 118, 74)) {
            wallpaper_make(i);
            if (g_theme == 4) { g_themes[4].wallpaper = i; theme_save_current(); }
        }
        int sty = by + 28 + 64 + 28 + 74 + 66;
        for (int i = 0; i < 2; i++) if (in_rect(x, y, bx + i * 130, sty, 118, 48)) {
            g_shell_style = i;
            if (g_theme == 4) { g_themes[4].shell_style = i; theme_save_current(); }
        }
    } else if (s->page == 3) { /* Accounts */
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
    } else if (s->page == 4 && in_rect(x, y, bx + 200, by + 40, 40, 20)) { /* Time & Language */
        g_clock24 = !g_clock24;
    } else if (s->page == 5) { /* Display - no interactive elements yet */ }
    else if (s->page == 6) { /* Sound - volume sliders could be added */ }
    else if (s->page == 7) { /* Notifications - toggles */ }
    else if (s->page == 8) { /* Power - options */ }
    else if (s->page == 9) { /* Storage - toggles */ }
    else if (s->page == 10) { /* Themes - click to select theme */
        for (int i = 0; i < 5; i++) {
            if (!g_themes[i].used) continue;
            int col = i % 3, row = i / 3;
            int sx = bx + col * 180, sy = by + 28 + row * 130;
            if (in_rect(x, y, sx, sy, 160, 110)) {
                theme_load(i);
                theme_save_current();
                theme_apply_colors_public();
                return;
            }
        }
    } else if (s->page == 11) { /* Colors */
        for (int i = 0; i < 2; i++) {
            int sx = bx + i * 130;
            if (in_rect(x, y, sx, by + 28, 118, 48)) {
                theme_load(i);
                theme_save_current();
                theme_apply_colors_public();
                return;
            }
        }
        for (int i = 0; i < 8; i++) if (in_rect(x, y, bx + i * 44, by + 28 + 72 + 28, 36, 36)) {
            g_accent = accents[i];
            if (g_theme == 4) { g_themes[4].accent = accents[i]; theme_save_current(); }
        }
        if (in_rect(x, y, bx + 300, by + 28 + 72 + 28 + 64 + 28 + 36, 40, 20)) {
            g_themes[g_theme].transparency = !g_themes[g_theme].transparency;
            theme_save_current();
            theme_apply_colors_public();
        }
        if (in_rect(x, y, bx + 300, by + 28 + 72 + 28 + 64 + 28 + 36 + 36 + 28, 40, 20)) {
            g_themes[g_theme].rounding = g_themes[g_theme].rounding ? 0 : 8;
            theme_save_current();
            theme_apply_colors_public();
        }
    } else if (s->page == 12) { /* Start - toggles */ }
    else if (s->page == 13) { /* Taskbar */
        for (int i = 0; i < 2; i++) {
            int sx = bx + i * 180;
            if (in_rect(x, y, sx, by + 28 + 28, 160, 36)) {
                g_shell_style = i;
                if (g_theme == 4) { g_themes[4].shell_style = i; theme_save_current(); }
                return;
            }
        }
    } else if (s->page == 14) { /* Startup Apps - toggles */ }
    else if (s->page == 15) { /* Accessibility */
        if (in_rect(x, y, bx + 200, by + 28 + 40 + 32, 40, 20)) {
            g_themes[g_theme].animations = !g_themes[g_theme].animations;
            g_reduced_motion = !g_themes[g_theme].animations;
            theme_save_current();
            theme_apply_colors_public();
        }
        if (in_rect(x, y, bx + 200, by + 28 + 40 + 32 + 32, 40, 20)) {
            g_themes[g_theme].transparency = !g_themes[g_theme].transparency;
            theme_save_current();
            theme_apply_colors_public();
        }
        if (in_rect(x, y, bx + 200, by + 28 + 40 + 32 + 32 + 32, 40, 20)) {
            g_themes[g_theme].animations = !g_themes[g_theme].animations;
            g_reduced_motion = !g_themes[g_theme].animations;
            theme_save_current();
            theme_apply_colors_public();
        }
    } else if (s->page == 16) { /* Privacy & Security - toggles */ }
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
