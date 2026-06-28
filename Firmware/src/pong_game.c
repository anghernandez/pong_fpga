#include "vga.h"
#include "input.h"

#define BTN_C (1u << 0)

/* Velocidad base controlada por switches:
 *   SW0=0, SW1=0 → normal  (delay 300000)
 *   SW0=1        → rapido  (delay 150000)
 *   SW1=1        → muy rapido (delay 60000)
 */
static int get_delay(void) {
    unsigned int sw = Xil_In32(GPIO_SWITCHES_BASE + GPIO_DATA_REG);
    if (sw & 0x02) return 10000;
    if (sw & 0x01) return 30000;
    return 80000;
}

/* Delay que verifica BTNC cada 10000 ciclos.
 * Retorna 1 si BTNC fue presionado (salir del juego). */
static int interruptible_delay(int total) {
    int chunk = 10000;
    while (total > 0) {
        if (Xil_In32(GPIO_BUTTONS_BASE + GPIO_DATA_REG) & BTN_C)
            return 1;
        volatile int c = (total > chunk) ? chunk : total;
        while (c--);
        total -= chunk;
    }
    return 0;
}

void pong_game_run(void) {
    while (Xil_In32(GPIO_BUTTONS_BASE + GPIO_DATA_REG) & BTN_C);

    int ball_x    = 300;
    int ball_y    = 200;
    int ball_size = 16;
    int dx        = 8;
    int dy        = 8;
    int ball_words = ball_size / 8;

    clear_screen(COLOR_BLACK);

    while (1) {
        // Borrar
        for (int i = 0; i < ball_size; i++) {
            int word_addr = ((ball_y + i) * WORDS_PER_LINE) + (ball_x / 8);
            for (int w = 0; w < ball_words; w++)
                write_word(word_addr + w, 0x00000000);
        }

        // Mover
        ball_x += dx;
        ball_y += dy;

        // Rebotar
        if (ball_x + ball_size >= SCREEN_W) { ball_x = SCREEN_W - ball_size; dx = -dx; }
        if (ball_x <= 0)                     { ball_x = 0;                    dx = -dx; }
        if (ball_y + ball_size >= SCREEN_H)  { ball_y = SCREEN_H - ball_size; dy = -dy; }
        if (ball_y <= 0)                     { ball_y = 0;                    dy = -dy; }

        // Dibujar
        for (int i = 0; i < ball_size; i++) {
            int word_addr = ((ball_y + i) * WORDS_PER_LINE) + (ball_x / 8);
            for (int w = 0; w < ball_words; w++)
                write_word(word_addr + w, 0xFFFFFFFF);
        }

        // Delay interruptible: sale inmediatamente si se presiona BTNC
        if (interruptible_delay(get_delay())) {
            clear_screen(COLOR_BLACK);
            return;
        }
    }
}
