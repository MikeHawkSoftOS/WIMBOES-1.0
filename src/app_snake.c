/* Snake Game */
#include "os.h"

#define GRID_SIZE 20
#define CELL_SIZE 18
#define GAME_W (GRID_SIZE * CELL_SIZE)
#define GAME_H (GRID_SIZE * CELL_SIZE + 60)

typedef struct { int x, y; } Point;

typedef struct {
    Point snake[400];
    int len, dir, score;
    Point food;
    bool alive, paused;
    u32 last_move;
} SnakeState;

static void spawn_food(SnakeState *s) {
    bool ok;
    do {
        s->food.x = (int)(hal_ms() % GRID_SIZE);
        s->food.y = (int)((hal_ms() / 17) % GRID_SIZE);
        ok = true;
        for (int i = 0; i < s->len; i++)
            if (s->snake[i].x == s->food.x && s->snake[i].y == s->food.y) ok = false;
    } while (!ok);
}

static void snake_reset(SnakeState *s) {
    s->len = 3;
    for (int i = 0; i < s->len; i++) { s->snake[i].x = GRID_SIZE / 2 - i; s->snake[i].y = GRID_SIZE / 2; }
    s->dir = 0; s->score = 0; s->alive = true; s->paused = false;
    spawn_food(s); s->last_move = hal_ms();
}

static void snake_draw(Window *w, int cx, int cy, int cw, int ch) {
    (void)w; (void)cw; (void)ch;
    SnakeState *s = w->priv;
    
    /* Game area background */
    gfx_rect(cx, cy, GAME_W, GAME_W, 0x1E1E1E);
    
    /* Grid lines */
    for (int i = 0; i <= GRID_SIZE; i++) {
        int pos = i * CELL_SIZE;
        gfx_hline(cx, cy + pos, GAME_W, 0x2A2A2A);
        gfx_vline(cx + pos, cy, GAME_W, 0x2A2A2A);
    }
    
    /* Food */
    gfx_rect(cx + s->food.x * CELL_SIZE + 2, cy + s->food.y * CELL_SIZE + 2, 
             CELL_SIZE - 4, CELL_SIZE - 4, 0xFF3B30);
    
    /* Snake */
    for (int i = 0; i < s->len; i++) {
        u32 color = i == 0 ? 0x34C759 : col_blend(0x34C759, 0x1E1E1E, i * 100 / s->len);
        gfx_rect(cx + s->snake[i].x * CELL_SIZE + 1, cy + s->snake[i].y * CELL_SIZE + 1,
                 CELL_SIZE - 2, CELL_SIZE - 2, color);
    }
    
    /* Status bar */
    gfx_rect(cx, cy + GAME_W, GAME_W, 60, 0x2C2C2C);
    char buf[32];
    ksnprintf(buf, sizeof buf, "Score: %d", s->score);
    text_draw(&font_ui_b, cx + 10, cy + GAME_W + 10, buf, 0xFFFFFF);
    ksnprintf(buf, sizeof buf, "Length: %d", s->len);
    text_draw(&font_ui, cx + 10, cy + GAME_W + 32, buf, 0xCCCCCC);
    
    if (!s->alive) {
        gfx_darken(cx, cy, GAME_W, GAME_W, 0x000000, 180);
        text_draw_c(&font_title, cx, GAME_W, cy + GAME_W / 2 - 40, "GAME OVER", 0xFF3B30);
        text_draw_c(&font_ui, cx, GAME_W, cy + GAME_W / 2 + 10, "Press SPACE to restart", 0xFFFFFF);
    } else if (s->paused) {
        gfx_darken(cx, cy, GAME_W, GAME_W, 0x000000, 160);
        text_draw_c(&font_title, cx, GAME_W, cy + GAME_W / 2 - 20, "PAUSED", 0xFFFFFF);
        text_draw_c(&font_ui, cx, GAME_W, cy + GAME_W / 2 + 20, "Press P to continue", 0xCCCCCC);
    }
}

void snake_update(void *state) {
    SnakeState *s = state;
    if (!s || !s->alive || s->paused) return;
    
    u32 ms = hal_ms();
    if (ms - s->last_move < 150) return;
    s->last_move = ms;
    
    Point head = s->snake[0];
    if (s->dir == 0) head.x++;
    else if (s->dir == 1) head.y++;
    else if (s->dir == 2) head.x--;
    else if (s->dir == 3) head.y--;
    
    /* Wall collision */
    if (head.x < 0 || head.x >= GRID_SIZE || head.y < 0 || head.y >= GRID_SIZE) {
        s->alive = false; return;
    }
    
    /* Self collision */
    for (int i = 0; i < s->len; i++)
        if (s->snake[i].x == head.x && s->snake[i].y == head.y) { s->alive = false; return; }
    
    /* Move snake */
    for (int i = s->len - 1; i > 0; i--) s->snake[i] = s->snake[i - 1];
    s->snake[0] = head;
    
    /* Food collision */
    if (head.x == s->food.x && head.y == s->food.y) {
        if (s->len < 400) s->len++;
        s->score += 10;
        spawn_food(s);
    }
}

static void snake_key(Window *w, int key, int mod) {
    (void)mod;
    SnakeState *s = w->priv;
    
    if (key == ' ' && !s->alive) { snake_reset(s); return; }
    if (key == 'p' || key == 'P') { s->paused = !s->paused; return; }
    
    if (!s->alive || s->paused) return;
    
    if ((key == K_RIGHT || key == 'd') && s->dir != 2) s->dir = 0;
    else if ((key == K_DOWN || key == 's') && s->dir != 3) s->dir = 1;
    else if ((key == K_LEFT || key == 'a') && s->dir != 0) s->dir = 2;
    else if ((key == K_UP || key == 'w') && s->dir != 1) s->dir = 3;
}

static void snake_close(Window *w) { if (w->priv) kfree(w->priv); }

Window *snake_new(void) {
    SnakeState *s = kmalloc(sizeof(SnakeState));
    if (!s) return 0;
    
    int x, y; win_cascade(&x, &y);
    Window *w = wm_create(8, "Snake", ICON_CALC, x, y, GAME_W, GAME_H);
    if (!w) { kfree(s); return 0; }
    
    snake_reset(s);
    w->priv = s; w->draw = snake_draw; w->key = snake_key;
    w->close = snake_close; w->minw = GAME_W; w->minh = GAME_H;
    return w;
}
