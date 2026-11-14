/* Command Prompt with a small set of cmd.exe-style commands. */
#include "os.h"
#define T_LINES 200
#define T_COLS 100
#define T_HIST 8
typedef struct { char lines[T_LINES][T_COLS]; int nl; char in[T_COLS]; int inlen; int cwd; char hist[T_HIST][T_COLS]; int nh, hp; } Term;

static void newline(Term *t) {
    if (t->nl == T_LINES) { memmove(t->lines[0], t->lines[1], (size_t)(T_LINES - 1) * T_COLS); t->nl--; }
    t->lines[t->nl][0] = 0; t->nl++;
}
static void out(Term *t, const char *s) {
    if (!t->nl) newline(t);
    for (; *s; s++) {
        if (*s == '\n') { newline(t); continue; }
        if (*s == '\r') continue;
        char *l = t->lines[t->nl - 1]; int n = (int)strlen(l);
        if (n >= T_COLS - 2) { newline(t); l = t->lines[t->nl - 1]; n = 0; }
        l[n] = *s; l[n + 1] = 0;
    }
}
static void outln(Term *t, const char *s) { out(t, s); newline(t); }
static void prompt_str(Term *t, char *b, int cap) { char p[100]; fs_path(t->cwd, p, sizeof p); ksnprintf(b, cap, "%s>", p); }

static void launch_with_file(Term *t, int app, const char *arg) {
    int node = -1;
    if (*arg) { node = fs_resolve(t->cwd, arg); if (node < 0 && app == APP_NOTEPAD) { outln(t, "The system cannot find the file specified."); return; } }
    app_launch(app, node);
}
static void run(Window *w, Term *t, char *line) {
    while (*line == ' ') line++;
    int l = (int)strlen(line); while (l && line[l - 1] == ' ') line[--l] = 0;
    char *args = line; while (*args && *args != ' ') args++;
    char cmd[24]; int cl = (int)(args - line); if (cl > 23) cl = 23;
    memcpy(cmd, line, (size_t)cl); cmd[cl] = 0;
    while (*args == ' ') args++;
    char b[160];
    if (!cmd[0]) return;
    if (!kstricmp(cmd, "help")) {
        outln(t, "Commands: help ver cls echo dir cd mkdir type del rd ren whoami users");
        outln(t, "          date time hostname exit logoff shutdown restart");
        outln(t, "Apps:     notepad [file]  calc  paint  explorer  settings  taskmgr");
        outln(t, "Tip: echo hello > note.txt creates a file.");
    } else if (!kstricmp(cmd, "ver")) { outln(t, ""); outln(t, "Wimboes [Version 1.0.0]"); }
    else if (!kstricmp(cmd, "cls")) t->nl = 0;
    else if (!kstricmp(cmd, "echo")) {
        char *gt = args; while (*gt && *gt != '>') gt++;
        if (*gt == '>') {
            *gt = 0; char *f = gt + 1; while (*f == ' ') f++;
            int al = (int)strlen(args); while (al && args[al - 1] == ' ') args[--al] = 0;
            int node = fs_resolve(t->cwd, f);
            if (node < 0) node = fs_create(t->cwd, f, false);
            if (node < 0 || g_fs[node].dir) outln(t, "Access is denied.");
            else { char c[T_COLS + 2]; ksnprintf(c, sizeof c, "%s\r\n", args); fs_write(node, c, (int)strlen(c)); }
        } else outln(t, args);
    } else if (!kstricmp(cmd, "dir")) {
        int node = *args ? fs_resolve(t->cwd, args) : t->cwd, list[FS_MAX];
        if (node < 0 || !g_fs[node].dir) { outln(t, "File Not Found"); return; }
        fs_path(node, b, sizeof b); char h[170]; ksnprintf(h, sizeof h, " Directory of %s", b); outln(t, ""); outln(t, h); outln(t, "");
        int n = fs_list(node, list, FS_MAX), files = 0, dirs = 0;
        for (int i = 0; i < n; i++) {
            FNode *f = &g_fs[list[i]];
            if (f->dir) { dirs++; ksnprintf(b, sizeof b, "    <DIR>         %s", f->name); }
            else { files++; ksnprintf(b, sizeof b, "%9d     %s", f->size, f->name); }
            outln(t, b);
        }
        ksnprintf(b, sizeof b, "        %d File(s)   %d Dir(s)", files, dirs); outln(t, b);
    } else if (!kstricmp(cmd, "cd") || !kstricmp(cmd, "chdir")) {
        if (!*args) { fs_path(t->cwd, b, sizeof b); outln(t, b); return; }
        int node = fs_resolve(t->cwd, args);
        if (node < 0 || !g_fs[node].dir) outln(t, "The system cannot find the path specified."); else t->cwd = node;
    } else if (!kstricmp(cmd, "mkdir") || !kstricmp(cmd, "md")) {
        if (!*args || fs_create(t->cwd, args, true) < 0) outln(t, "A subdirectory or file already exists, or the name is invalid.");
    } else if (!kstricmp(cmd, "type")) {
        int node = *args ? fs_resolve(t->cwd, args) : -1;
        if (node < 0 || g_fs[node].dir) outln(t, "The system cannot find the file specified.");
        else { if (g_fs[node].data) out(t, g_fs[node].data); newline(t); }
    } else if (!kstricmp(cmd, "del") || !kstricmp(cmd, "rd") || !kstricmp(cmd, "rmdir")) {
        int node = *args ? fs_resolve(t->cwd, args) : -1;
        if (node <= 0 || fs_is_under(t->cwd, node)) outln(t, "The system cannot find the file specified.");
        else fs_delete(node);
    } else if (!kstricmp(cmd, "ren") || !kstricmp(cmd, "rename")) {
        char *sp = args; while (*sp && *sp != ' ') sp++;
        if (!*sp) { outln(t, "The syntax of the command is incorrect."); return; }
        *sp++ = 0; while (*sp == ' ') sp++;
        int node = fs_resolve(t->cwd, args);
        if (node <= 0 || !fs_rename(node, sp)) outln(t, "The system cannot find the file specified, or the new name is invalid.");
    } else if (!kstricmp(cmd, "whoami")) outln(t, g_user >= 0 ? g_users[g_user].name : "nobody");
    else if (!kstricmp(cmd, "hostname")) outln(t, "WIMBOES-PC");
    else if (!kstricmp(cmd, "users")) { for (int i = 0; i < MAX_USERS; i++) if (g_users[i].used) { ksnprintf(b, sizeof b, "  %s%s", g_users[i].name, g_users[i].admin ? "  (administrator)" : ""); outln(t, b); } }
    else if (!kstricmp(cmd, "date")) { RtcTime r; hal_rtc(&r); ksnprintf(b, sizeof b, "The current date is: %02d/%02d/%d", r.month, r.day, r.year); outln(t, b); }
    else if (!kstricmp(cmd, "time")) { RtcTime r; hal_rtc(&r); ksnprintf(b, sizeof b, "The current time is: %02d:%02d:%02d", r.hour, r.min, r.sec); outln(t, b); }
    else if (!kstricmp(cmd, "exit")) wm_close(w);
    else if (!kstricmp(cmd, "logoff")) os_signout();
    else if (!kstricmp(cmd, "shutdown")) os_shutdown(false);
    else if (!kstricmp(cmd, "restart")) os_shutdown(true);
    else if (!kstricmp(cmd, "notepad")) launch_with_file(t, APP_NOTEPAD, args);
    else if (!kstricmp(cmd, "calc")) app_launch(APP_CALC, -1);
    else if (!kstricmp(cmd, "paint")) app_launch(APP_PAINT, -1);
    else if (!kstricmp(cmd, "settings")) app_launch(APP_SETTINGS, 0);
    else if (!kstricmp(cmd, "taskmgr")) app_launch(APP_TASKMGR, -1);
    else if (!kstricmp(cmd, "explorer")) app_launch(APP_EXPLORER, *args ? fs_resolve(t->cwd, args) : t->cwd);
    else { ksnprintf(b, sizeof b, "'%s' is not recognized as an internal or external command.", cmd); outln(t, b); }
}
static void tm_key(Window *w, int key, int mod) {
    Term *t = w->priv;
    if (mod & MOD_CTRL) { if (key == 'c') { out(t, t->in); outln(t, "^C"); t->in[0] = 0; t->inlen = 0; } return; }
    if (key == K_ENTER) {
        char p[110]; prompt_str(t, p, sizeof p);
        if (!t->nl) newline(t);
        out(t, p); outln(t, t->in);
        char line[T_COLS]; kstrcpy(line, t->in, sizeof line);
        if (line[0] && (!t->nh || strcmp(t->hist[(t->nh - 1) % T_HIST], line))) { kstrcpy(t->hist[t->nh % T_HIST], line, T_COLS); t->nh++; }
        t->hp = t->nh; t->in[0] = 0; t->inlen = 0;
        run(w, t, line);
    } else if (key == K_BACKSPACE) { if (t->inlen) t->in[--t->inlen] = 0; }
    else if (key == K_UP) { if (t->hp > MAX(0, t->nh - T_HIST)) { t->hp--; kstrcpy(t->in, t->hist[t->hp % T_HIST], T_COLS); t->inlen = (int)strlen(t->in); } }
    else if (key == K_DOWN) { if (t->hp < t->nh) { t->hp++; if (t->hp == t->nh) t->in[0] = 0; else kstrcpy(t->in, t->hist[t->hp % T_HIST], T_COLS); t->inlen = (int)strlen(t->in); } }
    else if (key >= 32 && key < 127 && t->inlen < T_COLS - 20) { t->in[t->inlen++] = (char)key; t->in[t->inlen] = 0; }
}
static void tm_draw(Window *w, int cx, int cy, int cw, int ch) {
    Term *t = w->priv;
    gfx_rect(cx, cy, cw, ch, 0x0C0C0C);
    int rows = (ch - 6) / 17, first = MAX(0, t->nl - (rows - 1));
    int y = cy + 4;
    for (int i = first; i < t->nl; i++, y += 17) text_draw(&font_mono, cx + 6, y, t->lines[i], 0xCCCCCC);
    char p[110]; prompt_str(t, p, sizeof p);
    int x = cx + 6; text_draw(&font_mono, x, y, p, 0xCCCCCC); x += (int)strlen(p) * 8;
    text_draw(&font_mono, x, y, t->in, 0xCCCCCC); x += t->inlen * 8;
    if (ui_blink()) gfx_rect(x, y + 13, 8, 2, 0xCCCCCC);
}
static void tm_mouse(Window *w, int ev, int x, int y, int btn) { (void)w; (void)ev; (void)x; (void)y; (void)btn; }
static void tm_close(Window *w) { kfree(w->priv); }
Window *terminal_new(void) {
    int x, y; win_cascade(&x, &y);
    Window *w = wm_create(APP_TERMINAL, "Command Prompt", ICON_TERM, x, y, 680, 400);
    if (!w) return 0;
    Term *t = kmalloc(sizeof(Term));
    if (!t) { wm_close(w); return 0; }
    t->cwd = g_user >= 0 ? g_users[g_user].home : 0;
    w->priv = t; w->draw = tm_draw; w->key = tm_key; w->mouse = tm_mouse; w->close = tm_close; w->minw = 320; w->minh = 140;
    outln(t, "Wimboes [Version 1.0.0]"); outln(t, "(c) MikeHawkSoft. Type 'help' for commands."); outln(t, "");
    return w;
}
