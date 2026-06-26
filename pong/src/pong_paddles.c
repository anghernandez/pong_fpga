#include "vga.h"
#include "input.h"
#include "ui_text.h"

/* ─── Configuracion ──────────────────────────────────────────────────────────── */
#define PAD_W      8      /* 1 word VGA, word-aligned                           */
#define PAD_H      60
#define PAD_SPEED  10
#define PLAYER_X   24    /* 3×8 px, word-aligned                                */
#define CPU_X      608   /* 76×8 px, word-aligned                               */

/*
 * BALL_VX debe ser multiplo de 8 para que bx permanezca siempre word-aligned.
 * Eso permite dibujar y borrar la bola con write_word directo, sin draw_pixel.
 * BALL_SZ = 8 → 1 word por fila → draw y erase muy rapido y limpio.
 */
#define BALL_SZ    8     /* 1 word de ancho                                     */
#define BALL_VX    8     /* multiplo de 8: bx siempre alineado                  */
#define BALL_VY    5

#define SCORE_Y    8
#define TOP_WALL   18    /* score ocupa y=8..15; margen de 2px */
#define WIN_SCORE  7

/* Ticks por frame — aumentar para ir mas lento, bajar para ir mas rapido. */
#define TICK_NORMAL   7500U

#define BTN_C (1u << 0)   /* BTNC: salir al menu              */
#define BTN_U (1u << 1)   /* BTNU: jugador 2 arriba           */
#define BTN_L (1u << 2)   /* BTNL: jugador 1 arriba           */
#define BTN_R (1u << 3)   /* BTNR: jugador 2 abajo            */
#define BTN_D (1u << 4)   /* BTND: jugador 1 abajo            */

/* ─── Utilidades ─────────────────────────────────────────────────────────────── */
static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }
static int iclamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/* ─── Dibujo ─────────────────────────────────────────────────────────────────── */

/*
 * La bola siempre tiene x word-aligned (bx % 8 == 0) y BALL_SZ=8 (1 word).
 * Dibujo y borrado se hacen con write_word directo: cero draw_pixel, cero RMW.
 */
static void draw_ball_sq(int bx, int by, unsigned char color) {
    unsigned int w    = color_to_word(color);
    int          wx   = bx / 8;
    for (int r = 0; r < BALL_SZ; r++)
        write_word((by + r) * WORDS_PER_LINE + wx, w);
}

/*
 * Borra la union de la posicion anterior y nueva de la bola.
 * Como bx siempre es multiplo de 8, left y right tambien lo son
 * → solo write_word, sin draw_pixel ni read-modify-write.
 */
static void erase_ball_area(int px, int py, int nx, int ny) {
    int left  = imin(px, nx);
    int right = imax(px, nx) + BALL_SZ;
    int top   = imax(imin(py, ny) - 1, 0);
    int bot   = imin(imax(py, ny) + BALL_SZ + 1, SCREEN_H);
    int words = (right - left) / 8;
    for (int row = top; row < bot; row++) {
        int base = row * WORDS_PER_LINE + left / 8;
        for (int w = 0; w < words; w++)
            write_word(base + w, 0);
    }
}

/* Paletas en x word-aligned, ancho 1 word → draw_rect exacto */
static void draw_paddle_rect(int x, int y, unsigned char color) {
    draw_rect(x, y, PAD_W, PAD_H, color);
}

static void erase_paddle_strip(int x, int new_y, int old_y) {
    if (new_y > old_y)
        draw_rect(x, old_y, PAD_W, new_y - old_y, COLOR_BLACK);
    else if (new_y < old_y)
        draw_rect(x, new_y + PAD_H, PAD_W, old_y - new_y, COLOR_BLACK);
}

static void draw_score(int ps, int cs) {
    char buf[14];
    buf[0]  = 'J'; buf[1]  = '1'; buf[2]  = ':'; buf[3]  = ' ';
    buf[4]  = '0' + (ps > 9 ? 9 : ps);
    buf[5]  = ' '; buf[6]  = ' '; buf[7]  = ' ';
    buf[8]  = 'J'; buf[9]  = '2'; buf[10] = ':'; buf[11] = ' ';
    buf[12] = '0' + (cs > 9 ? 9 : cs);
    buf[13] = 0;
    ui_draw_text_centered(SCORE_Y, buf, COLOR_GRAY_LIGHT);
}

static void draw_field(int py, int cy, int bx, int by, int ps, int cs) {
    clear_screen(COLOR_BLACK);
    draw_score(ps, cs);
    draw_paddle_rect(PLAYER_X, py, COLOR_YELLOW);
    draw_paddle_rect(CPU_X,    cy, COLOR_CYAN_LIGHT);
    draw_ball_sq(bx, by, COLOR_WHITE);
}

/* ─── Logica ─────────────────────────────────────────────────────────────────── */
static void move_player_pad(int *y, int up, int down) {
    if (up)   *y -= PAD_SPEED;
    if (down) *y += PAD_SPEED;
    *y = iclamp(*y, 0, SCREEN_H - PAD_H);
}

/* Retorna 0=juego sigue, 1=J1 anoto, -1=J2 anoto */
static int step_ball(int *bx, int *by, int *vx, int *vy,
                     int player_y, int cpu_y) {
    *bx += *vx;
    *by += *vy;

    if (*by <= TOP_WALL) {
        *by = TOP_WALL;
        if (*vy < 0) *vy = -*vy;
    }
    if (*by + BALL_SZ >= SCREEN_H - 1) {
        *by = SCREEN_H - BALL_SZ - 1;
        if (*vy > 0) *vy = -*vy;
    }

    if (*vx < 0 &&
        *bx <= PLAYER_X + PAD_W && *bx + BALL_SZ > PLAYER_X &&
        *by + BALL_SZ > player_y && *by < player_y + PAD_H) {
        *bx = PLAYER_X + PAD_W;   /* 32 = 4×8, word-aligned */
        *vx = -*vx;
    }

    if (*vx > 0 &&
        *bx + BALL_SZ >= CPU_X && *bx < CPU_X + PAD_W &&
        *by + BALL_SZ > cpu_y && *by < cpu_y + PAD_H) {
        *bx = CPU_X - BALL_SZ;    /* 600 = 75×8, word-aligned */
        *vx = -*vx;
    }

    if (*bx + BALL_SZ <= 0) return -1;
    if (*bx >= SCREEN_W)    return  1;
    return 0;
}

/* ─── Punto de entrada ───────────────────────────────────────────────────────── */
void pong_paddles_run(void) {
    while (Xil_In32(GPIO_BUTTONS_BASE + GPIO_DATA_REG) & BTN_C);

    /*
     * frame_count persiste entre llamadas (static).
     * Incrementa cada frame → actua como fuente de pseudo-aleatoriedad
     * para la direccion vertical de saque.
     */
    static unsigned int frame_count = 0;

    int player_y = (SCREEN_H - PAD_H) / 2;
    int cpu_y    = (SCREEN_H - PAD_H) / 2;

    /* bx inicial: (640-8)/2 = 316, redondeado a multiplo de 8 → 312 */
    int bx = ((SCREEN_W - BALL_SZ) / 2) & ~7;
    int by = (SCREEN_H - BALL_SZ) / 2;
    int vx = BALL_VX;
    int vy = ((frame_count >> 2) & 1) ? BALL_VY : -BALL_VY;
    int ps = 0, cs = 0;

    int prev_player_y = player_y, prev_cpu_y = cpu_y;
    int prev_bx = bx, prev_by = by;
    int prev_exit = 0;

    draw_field(player_y, cpu_y, bx, by, ps, cs);

    volatile unsigned int tick = 0;

    while (1) {
        tick++;
        if (tick < TICK_NORMAL) continue;
        tick = 0;
        frame_count++;

        /* ── Lectura de entrada ── */
        unsigned int btn = Xil_In32(GPIO_BUTTONS_BASE + GPIO_DATA_REG);

        int exit_now  = (btn & BTN_C) ? 1 : 0;
        /* J1: BTNL=arriba  BTND=abajo */
        int p1_up   = (btn & BTN_L) ? 1 : 0;
        int p1_down = (btn & BTN_D) ? 1 : 0;
        /* J2: BTNU=arriba  BTNR=abajo */
        int p2_up   = (btn & BTN_U) ? 1 : 0;
        int p2_down = (btn & BTN_R) ? 1 : 0;

        if (exit_now && !prev_exit) {
            clear_screen(COLOR_BLACK);
            return;
        }
        prev_exit = exit_now;

        /* ── Actualizacion del juego ── */
        prev_player_y = player_y;
        prev_cpu_y    = cpu_y;
        prev_bx = bx; prev_by = by;

        move_player_pad(&player_y, p1_up, p1_down);
        move_player_pad(&cpu_y,    p2_up, p2_down);

        int r = step_ball(&bx, &by, &vx, &vy, player_y, cpu_y);

        if (r != 0) {
            if (r ==  1) ps++;
            if (r == -1) cs++;

            /* Saque con direccion vertical aleatoria segun frame_count */
            bx = ((SCREEN_W - BALL_SZ) / 2) & ~7;
            by = (SCREEN_H - BALL_SZ) / 2;
            vx = (r == 1) ? BALL_VX : -BALL_VX;
            vy = ((frame_count >> 2) & 1) ? BALL_VY : -BALL_VY;
            prev_bx = bx; prev_by = by;

            draw_field(player_y, cpu_y, bx, by, ps, cs);

            if (ps >= WIN_SCORE || cs >= WIN_SCORE) {
                if (ps >= WIN_SCORE)
                    ui_draw_text_centered(220, "GANO JUGADOR 1!", COLOR_YELLOW);
                else
                    ui_draw_text_centered(220, "GANO JUGADOR 2!", COLOR_CYAN_LIGHT);
                ui_draw_text_centered(260, "PRESIONA BTNC PARA SALIR", COLOR_WHITE);
                while (!(Xil_In32(GPIO_BUTTONS_BASE + GPIO_DATA_REG) & BTN_C));
                while (  Xil_In32(GPIO_BUTTONS_BASE + GPIO_DATA_REG) & BTN_C);
                clear_screen(COLOR_BLACK);
                return;
            }
        } else {
            /* ── Renderizado incremental ── */
            erase_paddle_strip(PLAYER_X, player_y, prev_player_y);
            erase_paddle_strip(CPU_X,    cpu_y,    prev_cpu_y);
            erase_ball_area(prev_bx, prev_by, bx, by);
            draw_paddle_rect(PLAYER_X, player_y, COLOR_YELLOW);
            draw_paddle_rect(CPU_X,    cpu_y,    COLOR_CYAN_LIGHT);
            draw_ball_sq(bx, by, COLOR_WHITE);
        }
    }
}