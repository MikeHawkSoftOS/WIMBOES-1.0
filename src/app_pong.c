/* Pong Game */
#include "os.h"

#define GAME_W 600
#define GAME_H 420
#define PADDLE_W 12
#define PADDLE_H 80
#define BALL_SIZE 12

typedef struct {
    int p1y, p2y;
    float bx, by, bvx, bvy;
    int score1, score2;
    bool paused;
    u32 last_update;
} PongState;

static void pong_reset_ball(PongState *s) {
    s->bx = GAME_W / 2.0f; s->by = GAME_H / 2.0f;
    float angle = ((hal_ms() % 120) - 60) * 0.02f;
    s->bvx = ((hal_ms() % 2) ? 1.0f : -1.0f) * 4.0f;
    s->bvy = angle * 2.0f;
}

static void pong_reset(PongState *s) {
    s->p1y = s->p2y = (GAME_H - PADDLE_H) / 2;
    pong_reset_ball(s);
    s->score1 = s->score2 = 0;
    s->paused = false; s->last_update = hal_ms();
}

void pong_update(void *state) {
    PongState *s = state;
    if (!s || s->paused) return;
    
    u32 ms = hal_ms();
    float dt = (ms - s->last_update) / 16.0f;
    if (dt > 3.0f) dt = 1.0f;
    s->last_update = ms;
    
    /* Ball movement */
    s->bx += s->bvx * dt;
    s->by += s->bvy * dt;
    
    /* Ball collision with top/bottom */
    if (s->by < 0) { s->by = 0; s->bvy = -s->bvy; }
    if (s->by > GAME_H - BALL_SIZE) { s->by = GAME_H - BALL_SIZE; s->bvy = -s->bvy; }
    
    /* Ball collision with paddles */
    if (s->bx <= PADDLE_W && s->by + BALL_SIZE >= s->p1y && s->by <= s->p1y + PADDLE_H) {
        s->bx = PADDLE_W; s->bvx = -s->bvx * 1.05f;
        s->bvy += ((s->by + BALL_SIZE / 2) - (s->p1y + PADDLE_H / 2)) * 0.1f;
    }
    if (s->bx >= GAME_W - PADDLE_W - BALL_SIZE && s->by + BALL_SIZE >= s->p2y && s->by <= s->p2y + PADDLE_H) {
        s->bx = GAME_W - PADDLE_W - BALL_SIZE; s->bvx = -s->bvx * 1.05f;
        s->bvy += ((s->by + BALL_SIZE / 2) - (s->p2y + PADDLE_H / 2)) * 0.1f;
    }
    
    /* AI for player 2 */
    int target = (int)(s->by + BALL_SIZE / 2 - PADDLE_H / 2);
    if (s->p2y < target - 2) s->p2y += 3;
    else if (s->p2y > target + 2) s->p2y -= 3;
    s->p2y = CLAMP(s->p2y, 0, GAME_H - PADDLE_H);
    
    /* Scoring */
    if (s->bx < 0) { s->score2++; pong_reset_ball(s); }
    if (s->bx > GAME_W) { s->score1++; pong_reset_ball(s); }
}

static void pong_draw(Window *w, int cx, int cy, int cw, int ch) {
    (void)w; (void)cw; (void)ch;
    PongState *s = w->priv;
    
    /* Background */
    gfx_rect(cx, cy, GAME_W, GAME_H, 0x000000);
    
    /* Center line */
    for (int y = 0; y < GAME_H; y += 20)
        gfx_rect(cx + GAME_W / 2 - 2, cy + y, 4, 10, 0x444444);
    
    /* Paddles */
    gfx_rect(cx, cy + s->p1y, PADDLE_W, PADDLE_H, 0xFFFFFF);
    gfx_rect(cx + GAME_W - PADDLE_W, cy + s->p2y, PADDLE_W, PADDLE_H, 0xFFFFFF);
    
    /* Ball */
    gfx_rect(cx + (int)s->bx, cy + (int)s->by, BALL_SIZE, BALL_SIZE, 0xFFFFFF);
    
    /* Scores */
    char buf[16];
    ksnprintf(buf, sizeof buf, "%d", s->score1);
    text_draw_c(&font_big, cx, GAME_W / 2 - 40, cy + 30, buf, 0x666666);
    ksnprintf(buf, sizeof buf, "%d", s->score2);
    text_draw_c(&font_big, cx + GAME_W / 2 + 40, GAME_W / 2 - 40, cy + 30, buf, 0x666666);
    
    /* Status bar */
    gfx_rect(cx, cy + GAME_H, GAME_W, 60, 0x1E1E1E);
    text_draw(&font_ui, cx + 10, cy + GAME_H + 10, "Controls: W/S or Arrow Keys", 0xCCCCCC);
    text_draw(&font_ui, cx + 10, cy + GAME_H + 32, "P: Pause", 0x888888);
    
    if (s->paused) {
        gfx_darken(cx, cy, GAME_W, GAME_H, 0x000000, 160);
        text_draw_c(&font_title, cx, GAME_W, cy + GAME_H / 2 - 20, "PAUSED", 0xFFFFFF);
        text_draw_c(&font_ui, cx, GAME_W, cy + GAME_H / 2 + 20, "Press P to continue", 0xCCCCCC);
    }
}

static void pong_key(Window *w, int key, int mod) {
    (void)mod;
    PongState *s = w->priv;
    
    if (key == 'p' || key == 'P') { s->paused = !s->paused; s->last_update = hal_ms(); return; }
    if (key == ' ') { pong_reset(s); return; }
    
    if (!s->paused) {
        if (key == 'w' || key == 'W' || key == K_UP) s->p1y = MAX(0, s->p1y - 20);
        if (key == 's' || key == 'S' || key == K_DOWN) s->p1y = MIN(GAME_H - PADDLE_H, s->p1y + 20);
    }
}

static void pong_close(Window *w) { if (w->priv) kfree(w->priv); }

Window *pong_new(void) {
    PongState *s = kmalloc(sizeof(PongState));
    if (!s) return 0;
    
    int x, y; win_cascade(&x, &y);
    Window *w = wm_create(10, "Pong", ICON_PAINT, x, y, GAME_W, GAME_H + 60);
    if (!w) { kfree(s); return 0; }
    
    pong_reset(s);
    w->priv = s; w->draw = pong_draw; w->key = pong_key;
    w->close = pong_close; w->minw = GAME_W; w->minh = GAME_H + 60;
    return w;
}
