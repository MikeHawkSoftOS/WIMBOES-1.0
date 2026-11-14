#include "os.h"
void kmain(void) {
    int w, h; u32 *bb;
    hal_init(&w, &h, &bb);
    mem_init((void *)0x1000000, 64u << 20);
    gfx_init(bb, w, h);
    os_init();
    bool dirty = true;
    for (;;) {
        Event e;
        while (hal_next_event(&e)) { os_event(&e); dirty = true; }
        if (os_tick()) dirty = true;
        if (dirty) { os_render(); hal_present(bb); dirty = false; }
        else hal_idle();
    }
}
