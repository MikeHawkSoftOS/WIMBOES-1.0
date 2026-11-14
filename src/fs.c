/* In-memory filesystem: C:\ with folders and small text files. */
#include "os.h"
FNode g_fs[FS_MAX];

static bool bad_name(const char *n) {
    if (!n[0] || strlen(n) >= FS_NAME) return true;
    for (; *n; n++) if (*n == '\\' || *n == '/' || *n == ':' || *n == '*' || *n == '?' || *n == '"' || *n == '<' || *n == '>' || *n == '|') return true;
    return false;
}
int fs_root(void) { return 0; }
int fs_find(int parent, const char *name) {
    for (int i = 1; i < FS_MAX; i++) if (g_fs[i].used && g_fs[i].parent == parent && !kstricmp(g_fs[i].name, name)) return i;
    return -1;
}
int fs_create(int parent, const char *name, bool dir) {
    if (parent < 0 || !g_fs[parent].used || !g_fs[parent].dir || bad_name(name) || fs_find(parent, name) >= 0) return -1;
    for (int i = 1; i < FS_MAX; i++) if (!g_fs[i].used) {
        memset(&g_fs[i], 0, sizeof g_fs[i]);
        g_fs[i].used = true; g_fs[i].dir = dir; g_fs[i].parent = parent;
        kstrcpy(g_fs[i].name, name, FS_NAME);
        return i;
    }
    return -1;
}
int fs_unique(int parent, const char *base, const char *ext, bool dir) {
    char nm[FS_NAME];
    for (int n = 1; n < 100; n++) {
        if (n == 1) ksnprintf(nm, sizeof nm, "%s%s", base, ext);
        else ksnprintf(nm, sizeof nm, "%s (%d)%s", base, n, ext);
        if (fs_find(parent, nm) < 0) return fs_create(parent, nm, dir);
    }
    return -1;
}
void fs_delete(int node) {
    if (node <= 0 || !g_fs[node].used) return;
    for (int i = 1; i < FS_MAX; i++) if (g_fs[i].used && g_fs[i].parent == node) fs_delete(i);
    if (g_fs[node].data) kfree(g_fs[node].data);
    memset(&g_fs[node], 0, sizeof g_fs[node]);
}
int fs_list(int parent, int *out, int max) {
    int n = 0;
    for (int i = 1; i < FS_MAX && n < max; i++) if (g_fs[i].used && g_fs[i].parent == parent) out[n++] = i;
    for (int i = 1; i < n; i++) {       /* insertion sort: folders first, then name */
        int v = out[i], j = i - 1;
        while (j >= 0) {
            FNode *a = &g_fs[out[j]], *b = &g_fs[v];
            bool after = (a->dir != b->dir) ? (!a->dir && b->dir) : kstricmp(a->name, b->name) > 0;
            if (!after) break;
            out[j + 1] = out[j]; j--;
        }
        out[j + 1] = v;
    }
    return n;
}
bool fs_write(int node, const char *data, int len) {
    if (node <= 0 || !g_fs[node].used || g_fs[node].dir || len < 0 || len > FS_FILE_MAX) return false;
    char *d = kmalloc((size_t)len + 1);
    if (!d) return false;
    memcpy(d, data, (size_t)len); d[len] = 0;
    if (g_fs[node].data) kfree(g_fs[node].data);
    g_fs[node].data = d; g_fs[node].size = len;
    return true;
}
void fs_path(int node, char *buf, int cap) {
    if (node < 0) { kstrcpy(buf, "This PC", cap); return; }
    if (node == 0) { kstrcpy(buf, "C:\\", cap); return; }
    char tmp[160]; fs_path(g_fs[node].parent, tmp, sizeof tmp);
    int l = (int)strlen(tmp);
    ksnprintf(buf, cap, "%s%s%s", tmp, (l && tmp[l - 1] == '\\') ? "" : "\\", g_fs[node].name);
}
bool fs_is_under(int node, int anc) {
    while (node >= 0) { if (node == anc) return true; node = g_fs[node].parent; }
    return false;
}
bool fs_rename(int node, const char *name) {
    if (node <= 0 || bad_name(name)) return false;
    int ex = fs_find(g_fs[node].parent, name);
    if (ex >= 0 && ex != node) return false;
    kstrcpy(g_fs[node].name, name, FS_NAME);
    return true;
}
int fs_resolve(int cwd, const char *path) {
    int cur = cwd;
    if ((path[0] && path[1] == ':') ) { cur = 0; path += 2; }
    if (*path == '\\' || *path == '/') { cur = 0; path++; }
    char part[FS_NAME + 4];
    while (*path) {
        int n = 0;
        while (*path && *path != '\\' && *path != '/') { if (n < (int)sizeof part - 1) part[n++] = *path; path++; }
        part[n] = 0;
        while (*path == '\\' || *path == '/') path++;
        if (!n || !strcmp(part, ".")) continue;
        if (!strcmp(part, "..")) { if (cur > 0) cur = g_fs[cur].parent; continue; }
        if (cur < 0 || !g_fs[cur].dir) return -1;
        cur = fs_find(cur, part);
        if (cur < 0) return -1;
    }
    return cur;
}
void fs_init(void) {
    memset(g_fs, 0, sizeof g_fs);
    g_fs[0].used = true; g_fs[0].dir = true; kstrcpy(g_fs[0].name, "C:", FS_NAME); g_fs[0].parent = -1;
    int win = fs_create(0, "Windows", true);
    int sys = fs_create(win, "System32", true);
    fs_create(win, "Fonts", true);
    int pf = fs_create(0, "Program Files", true);
    fs_create(pf, "Wimboes Apps", true);
    fs_create(0, "Users", true);
    fs_create(0, "$Recycle.Bin", true);
    int k = fs_create(sys, "kernel.sys", false);
    fs_write(k, "MikeHawkSoft Wimboes kernel image\n", 33);
    int rd = fs_create(0, "readme.txt", false);
    const char *t = "Wimboes 1.0\r\n\r\nThis disk lives in RAM. Files you save are kept until you shut down.\r\n";
    fs_write(rd, t, (int)strlen(t));
}

int fs_bin(void) { return fs_find(0, "$Recycle.Bin"); }
bool fs_trash(int node) {
    int bin = fs_bin();
    if (node <= 0 || bin < 0 || node == bin) return false;
    if (fs_is_under(node, bin)) { fs_delete(node); return true; }
    char nm[FS_NAME];
    kstrcpy(nm, g_fs[node].name, sizeof nm);
    for (int n = 2; fs_find(bin, nm) >= 0 && n < 100; n++) ksnprintf(nm, sizeof nm, "%s (%d)", g_fs[node].name, n);
    kstrcpy(g_fs[node].name, nm, FS_NAME);
    g_fs[node].parent = bin;
    return true;
}
