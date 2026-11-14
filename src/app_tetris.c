/* Tetris Game */
#include "os.h"

#define BOARD_W 10
#define BOARD_H 20
#define CELL_SIZE 24
#define GAME_W (BOARD_W * CELL_SIZE + 180)
#define GAME_H (BOARD_H * CELL_SIZE + 60)

static const u32 colors[8] = { 0x000000, 0x00F0F0, 0xF0A000, 0x0000F0, 0xF0F000, 0x00F000, 0xF000F0, 0xF00000 };

static const char shapes[7][16] = {
    "0100010001000100", /* I */
    "0000011001100000", /* O */
    "0000011011000000", /* S */
    "0000110001100000", /* Z */
    "0100011001000000", /* T */
    "0100010011000000", /* L */
    "0010001011000000"  /* J */
};

typedef struct {
    int board[BOARD_H][BOARD_W];
    int piece, rot, px, py, next;
    bool alive, paused;
    int score, lines;
    u32 last_drop;
} TetrisState;

static bool check_collision(TetrisState *s, int piece, int rot, int x, int y) {
    for (int j = 0; j < 4; j++) for (int i = 0; i < 4; i++) {
        if (shapes[piece][(rot % 4) * 4 + j * 4 + i] == '1') {
            int bx = x + i, by = y + j;
            if (bx < 0 || bx >= BOARD_W || by >= BOARD_H) return true;
            if (by >= 0 && s->board[by][bx]) return true;
        }
    }
    return false;
}

static void lock_piece(TetrisState *s) {
    for (int j = 0; j < 4; j++) for (int i = 0; i < 4; i++) {
        if (shapes[s->piece][(s->rot % 4) * 4 + j * 4 + i] == '1') {
            int bx = s->px + i, by = s->py + j;
            if (by >= 0 && by < BOARD_H && bx >= 0 && bx < BOARD_W)
                s->board[by][bx] = s->piece + 1;
        }
    }
    
    /* Clear lines */
    int cleared = 0;
    for (int y = BOARD_H - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < BOARD_W; x++) if (!s->board[y][x]) full = false;
        if (full) {
            cleared++;
            for (int yy = y; yy > 0; yy--)
                for (int x = 0; x < BOARD_W; x++) s->board[yy][x] = s->board[yy - 1][x];
            for (int x = 0; x < BOARD_W; x++) s->board[0][x] = 0;
            y++;
        }
    }
    s->lines += cleared;
    s->score += cleared * cleared * 100;
}

static void spawn_piece(TetrisState *s) {
    s->piece = s->next; s->next = (int)((hal_ms() / 13) % 7);
    s->rot = 0; s->px = BOARD_W / 2 - 2; s->py = -1;
    if (check_collision(s, s->piece, s->rot, s->px, s->py)) s->alive = false;
}

static void tetris_reset(TetrisState *s) {
    for (int y = 0; y < BOARD_H; y++) for (int x = 0; x < BOARD_W; x++) s->board[y][x] = 0;
    s->next = (int)((hal_ms() / 7) % 7);
    spawn_piece(s);
    s->score = 0; s->lines = 0; s->alive = true; s->paused = false; s->last_drop = hal_ms();
}

static void tetris_draw(Window *w, int cx, int cy, int cw, int ch) {
    (void)w; (void)cw; (void)ch;
    TetrisState *s = w->priv;
    
    /* Board background */
    gfx_rect(cx, cy, BOARD_W * CELL_SIZE, BOARD_H * CELL_SIZE, 0x1E1E1E);
    
    /* Grid */
    for (int y = 0; y <= BOARD_H; y++) gfx_hline(cx, cy + y * CELL_SIZE, BOARD_W * CELL_SIZE, 0x2A2A2A);
    for (int x = 0; x <= BOARD_W; x++) gfx_vline(cx + x * CELL_SIZE, cy, BOARD_H * CELL_SIZE, 0x2A2A2A);
    
    /* Board cells */
    for (int y = 0; y < BOARD_H; y++) for (int x = 0; x < BOARD_W; x++) {
        if (s->board[y][x]) {
            gfx_rect(cx + x * CELL_SIZE + 1, cy + y * CELL_SIZE + 1, 
                    CELL_SIZE - 2, CELL_SIZE - 2, colors[s->board[y][x]]);
        }
    }
    
    /* Current piece */
    if (s->alive) {
        for (int j = 0; j < 4; j++) for (int i = 0; i < 4; i++) {
            if (shapes[s->piece][(s->rot % 4) * 4 + j * 4 + i] == '1') {
                int bx = s->px + i, by = s->py + j;
                if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H) {
                    gfx_rect(cx + bx * CELL_SIZE + 1, cy + by * CELL_SIZE + 1,
                            CELL_SIZE - 2, CELL_SIZE - 2, colors[s->piece + 1]);
                }
            }
        }
    }
    
    /* Info panel */
    int px = cx + BOARD_W * CELL_SIZE + 20;
    text_draw(&font_ui_b, px, cy + 10, "Next", 0xFFFFFF);
    gfx_rect(px, cy + 35, 100, 100, 0x2C2C2C);
    for (int j = 0; j < 4; j++) for (int i = 0; i < 4; i++) {
        if (shapes[s->next][j * 4 + i] == '1') {
            gfx_rect(px + 20 + i * 20, cy + 55 + j * 20, 18, 18, colors[s->next + 1]);
        }
    }
    
    char buf[32];
    ksnprintf(buf, sizeof buf, "Score: %d", s->score);
    text_draw(&font_ui_b, px, cy + 160, buf, 0xFFFFFF);
    ksnprintf(buf, sizeof buf, "Lines: %d", s->lines);
    text_draw(&font_ui, px, cy + 185, buf, 0xCCCCCC);
    
    text_draw(&font_small, px, cy + 230, "Controls:", 0x888888);
    text_draw(&font_small, px, cy + 250, "← → : Move", 0x888888);
    text_draw(&font_small, px, cy + 268, "↑   : Rotate", 0x888888);
    text_draw(&font_small, px, cy + 286, "↓   : Drop", 0x888888);
    text_draw(&font_small, px, cy + 304, "P   : Pause", 0x888888);
    
    /* Overlays */
    if (!s->alive) {
        gfx_darken(cx, cy, BOARD_W * CELL_SIZE, BOARD_H * CELL_SIZE, 0x000000, 180);
        text_draw_c(&font_title, cx, BOARD_W * CELL_SIZE, cy + BOARD_H * CELL_SIZE / 2 - 40, 
                    "GAME OVER", 0xFF3B30);
        text_draw_c(&font_ui, cx, BOARD_W * CELL_SIZE, cy + BOARD_H * CELL_SIZE / 2 + 10,
                    "Press SPACE to restart", 0xFFFFFF);
    } else if (s->paused) {
        gfx_darken(cx, cy, BOARD_W * CELL_SIZE, BOARD_H * CELL_SIZE, 0x000000, 160);
        text_draw_c(&font_title, cx, BOARD_W * CELL_SIZE, cy + BOARD_H * CELL_SIZE / 2 - 20,
                    "PAUSED", 0xFFFFFF);
    }
    
    /* Status bar */
    gfx_rect(cx, cy + BOARD_H * CELL_SIZE, GAME_W, 60, 0x2C2C2C);
    ksnprintf(buf, sizeof buf, "Score: %d   Lines: %d", s->score, s->lines);
    text_draw(&font_ui_b, cx + 10, cy + BOARD_H * CELL_SIZE + 20, buf, 0xFFFFFF);
}

void tetris_update(void *state) {
    TetrisState *s = state;
    if (!s || !s->alive || s->paused) return;
    
    u32 ms = hal_ms();
    if (ms - s->last_drop < 500) return;
    s->last_drop = ms;
    
    if (!check_collision(s, s->piece, s->rot, s->px, s->py + 1)) {
        s->py++;
    } else {
        lock_piece(s);
        spawn_piece(s);
    }
}

static void tetris_key(Window *w, int key, int mod) {
    (void)mod;
    TetrisState *s = w->priv;
    
    if (key == ' ' && !s->alive) { tetris_reset(s); return; }
    if (key == 'p' || key == 'P') { s->paused = !s->paused; return; }
    
    if (!s->alive || s->paused) return;
    
    if (key == K_LEFT && !check_collision(s, s->piece, s->rot, s->px - 1, s->py)) s->px--;
    else if (key == K_RIGHT && !check_collision(s, s->piece, s->rot, s->px + 1, s->py)) s->px++;
    else if (key == K_DOWN) {
        while (!check_collision(s, s->piece, s->rot, s->px, s->py + 1)) s->py++;
        s->last_drop = hal_ms();
    }
    else if (key == K_UP) {
        if (!check_collision(s, s->piece, s->rot + 1, s->px, s->py)) s->rot++;
    }
}

static void tetris_close(Window *w) { if (w->priv) kfree(w->priv); }

Window *tetris_new(void) {
    TetrisState *s = kmalloc(sizeof(TetrisState));
    if (!s) return 0;
    
    int x, y; win_cascade(&x, &y);
    Window *w = wm_create(9, "Tetris", ICON_CALC, x, y, GAME_W, GAME_H);
    if (!w) { kfree(s); return 0; }
    
    tetris_reset(s);
    w->priv = s; w->draw = tetris_draw; w->key = tetris_key;
    w->close = tetris_close; w->minw = GAME_W; w->minh = GAME_H;
    return w;
}
