/* Host-side harness: runs the real shell/apps against a fake HAL and dumps screenshots (PPM). */
#include "os.h"
#include <stdio.h>
#include <stdlib.h>

static u32 fake_ms; static u32 *bb; static bool off;
void hal_init(int *w, int *h, u32 **b) { (void)w; (void)h; (void)b; }
bool hal_next_event(Event *e) { (void)e; return false; }
u32 hal_ms(void) { return fake_ms; }
void hal_present(const u32 *b) { (void)b; }
void hal_rtc(RtcTime *t) { *t = (RtcTime){ 2026, 11, 14, 15, 45, 30, 6 }; }
void hal_idle(void) {}
void hal_poweroff(void) { off = true; }
void hal_reboot(void) { off = true; }

static void ev(int type, int x, int y, int btn, int key, int mod) { Event e = { type, x, y, btn, key, mod }; os_event(&e); }
static void move(int x, int y) { ev(EV_MOVE, x, y, 0, 0, 0); }
static void click(int x, int y) { move(x, y); ev(EV_DOWN, x, y, 1, 0, 0); ev(EV_UP, x, y, 1, 0, 0); fake_ms += 50; }
static void rclick(int x, int y) { move(x, y); ev(EV_DOWN, x, y, 2, 0, 0); ev(EV_UP, x, y, 2, 0, 0); fake_ms += 50; }
static void dbl(int x, int y) { click(x, y); fake_ms += 100; click(x, y); }
static void drag(int x0, int y0, int x1, int y1) { move(x0, y0); ev(EV_DOWN, x0, y0, 1, 0, 0); for (int i = 1; i <= 10; i++) move(x0 + (x1 - x0) * i / 10, y0 + (y1 - y0) * i / 10); ev(EV_UP, x1, y1, 1, 0, 0); fake_ms += 50; }
static void key(int k, int mod) { ev(EV_KEY, 0, 0, 0, k, mod); }
static void type(const char *s) { for (; *s; s++) key(*s == '\n' ? K_ENTER : *s, 0); }
static void adv(u32 ms) { for (u32 t = 0; t < ms; t += 50) { fake_ms += 50; os_tick(); } }
static void shot(const char *name) {
    os_render();
    char p[96]; snprintf(p, sizeof p, "/tmp/shots/%s.ppm", name);
    FILE *f = fopen(p, "wb"); fprintf(f, "P6\n%d %d\n255\n", g_w, g_h);
    for (int i = 0; i < g_w * g_h; i++) { u32 c = bb[i]; fputc(c >> 16, f); fputc(c >> 8, f); fputc(c, f); }
    fclose(f);
}
int main(void) {
    system("mkdir -p /tmp/shots");
    int W = 1024, H = 768;
    bb = calloc((size_t)W * H, 4); gfx_init(bb, W, H);
    mem_init(malloc(64 << 20), 64 << 20);
    os_init();
    adv(1000); shot("00_boot");
    adv(2000); shot("01_login");
    click(W / 2, H / 2 + 62 + 18);                 /* Sign in as Admin (no password) */
    adv(500); shot("02_welcome");
    adv(1500); shot("03_desktop");
    click(24, H - 20); shot("04_start");
    type("note"); shot("05_start_search");
    key(K_ENTER, 0); adv(100);
    type("Hello from Wimboes!\nThis is Notepad running inside a toy OS.\n"); key('s', MOD_CTRL); adv(600); shot("06_notepad");
    app_launch(APP_EXPLORER, -1); adv(100); dbl(120, 130);  shot("07_explorer");
    app_launch(APP_CALC, -1); type("12*3.5="); shot("08_calc");
    app_launch(APP_TERMINAL, -1); type("help\ndir\nwhoami\necho hi there > t.txt\ntype t.txt\nbogus\n"); shot("09_terminal");
    app_launch(APP_PAINT, -1); drag(100, 250, 300, 330); shot("10_paint");
    app_launch(APP_SETTINGS, 3); shot("11_settings_accounts");
    app_launch(APP_SETTINGS, 2); shot("12_settings_personalize");
    app_launch(APP_TASKMGR, -1); shot("13_taskmgr");
    rclick(700, 100); shot("14_ctx");
    click(10, 700); click(400, 400); shot("15_busy");
    os_signout(); adv(100); shot("16_signout");
    printf("done off=%d heap=%zu/%zu\n", off, mem_used(), mem_total());
    return 0;
}
