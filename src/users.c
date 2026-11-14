#include "os.h"
User g_users[MAX_USERS]; int g_user = -1;
static const u32 pal[] = { 0x0078D7, 0x10893E, 0xE74856, 0x881798, 0xF7630C, 0x0099BC, 0xE3008C, 0x69797E };

int user_count(void) { int n = 0; for (int i = 0; i < MAX_USERS; i++) n += g_users[i].used; return n; }
int user_nth(int n) { for (int i = 0; i < MAX_USERS; i++) if (g_users[i].used && n-- == 0) return i; return -1; }
int user_find(const char *name) { for (int i = 0; i < MAX_USERS; i++) if (g_users[i].used && !kstricmp(g_users[i].name, name)) return i; return -1; }
int user_docs(int idx) { int h = g_users[idx].home; return h < 0 ? -1 : fs_find(h, "Documents"); }
int user_desktop(int idx) { int h = g_users[idx].home; return h < 0 ? -1 : fs_find(h, "Desktop"); }

int user_add(const char *name, const char *pass, bool admin) {
    if (!name[0] || strlen(name) > 20 || strlen(pass) > 20 || user_find(name) >= 0) return -1;
    int users = fs_find(0, "Users"), home = fs_create(users, name, true);
    if (home < 0) return -1;
    for (int i = 0; i < MAX_USERS; i++) if (!g_users[i].used) {
        User *u = &g_users[i];
        memset(u, 0, sizeof *u);
        u->used = true; u->admin = admin; u->home = home; u->color = pal[i % ARRLEN(pal)];
        kstrcpy(u->name, name, sizeof u->name); kstrcpy(u->pass, pass, sizeof u->pass);
        static const char *dirs[] = { "Desktop", "Documents", "Downloads", "Pictures", "Music" };
        for (int d = 0; d < 5; d++) fs_create(home, dirs[d], true);
        return i;
    }
    fs_delete(home);
    return -1;
}
bool user_remove(int idx) {
    if (idx < 0 || idx >= MAX_USERS || !g_users[idx].used || idx == g_user) return false;
    fs_delete(g_users[idx].home);
    memset(&g_users[idx], 0, sizeof g_users[idx]);
    return true;
}
void users_init(void) {
    memset(g_users, 0, sizeof g_users); g_user = -1;
    int a = user_add("Admin", "", true);
    user_add("Guest", "", false);
    if (a >= 0) {
        int f = fs_create(user_docs(a), "Welcome.txt", false);
        const char *t = "Welcome to Wimboes!\r\n\r\nThis is a tiny OS written from scratch.\r\nTry the Start menu, Paint, the Command Prompt\r\n(type help) and Settings > Accounts.\r\n";
        fs_write(f, t, (int)strlen(t));
    }
}
