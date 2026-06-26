#include "vga.h"
#include "font.h"
#include "sd_image.h"
#include "input.h"
#include "ui_text.h"
#include <string.h>

void pong_paddles_run(void);
int  pong_mp_handshake(int is_master);
void pong_mp_master_run(void);
void pong_mp_slave_run(void);

/* ============================================================================
 * MAQUINA DE ESTADOS
 *
 *   MENU              --SW0--> GAME          (dos jugadores, una board)
 *   MENU              --SW1--> MP_ROLE_SELECT
 *   MP_ROLE_SELECT           : SW15=ON=Maestro / SW15=OFF=Esclavo
 *                      --BTNC--> MP_HANDSHAKE (rol se bloquea aqui)
 *   MP_HANDSHAKE             : SPI handshake bloqueante
 *                      --OK---> MP_GAME
 *                      --FAIL-> MENU
 *   MP_GAME                  : juego SPI segun rol
 *                      --fin--> MENU
 *   GAME                     : pong local dos jugadores
 *                      --fin--> MENU
 * ============================================================================ */

typedef enum {
    STATE_MENU,
    STATE_MP_ROLE_SELECT,
    STATE_MP_HANDSHAKE,
    STATE_MP_GAME,
    STATE_GAME
} GameState;

#define SW0_BIT   0
#define SW1_BIT   1
#define SW15_BIT  15
#define BTNC_BIT  0

static int bit_is_set(unsigned int reg, int bit) {
    return (reg >> bit) & 0x1;
}

#define IMG_BYTES        (480 * 480 / 2)
#define MENU_CACHE_ADDR  0x81100000
#define TWO_CACHE_ADDR   0x81200000

static unsigned char * const menu_cache = (unsigned char *)MENU_CACHE_ADDR;
static unsigned char * const two_cache  = (unsigned char *)TWO_CACHE_ADDR;
static int menu_ok = 0;
static int two_ok  = 0;

static void preload_images(void) {
    if (sd_image_load("MENU.BIN") == 0) {
        memcpy(menu_cache, sd_image_get_buffer(), IMG_BYTES);
        menu_ok = 1;
    }
    if (sd_image_load("FIELD.BIN") == 0) {
        memcpy(two_cache, sd_image_get_buffer(), IMG_BYTES);
        two_ok = 1;
    }
}

static void draw_state_menu(void) {
    if (menu_ok)
        draw_image_centered(menu_cache);
    else
        clear_screen(COLOR_MAGENTA_LIGHT);
    ui_draw_text_centered(390, "\xBF EN QUE MODO QUIERES JUGAR?", COLOR_RED_LIGHT);
    ui_draw_text_centered(440, "SW0(DOS JUGADORES)     SW1(MULTIJUGADOR)", COLOR_CYAN_LIGHT);
}

static void draw_state_mp_role_select(void) {
    if (two_ok)
        draw_image_centered(two_cache);
    else
        clear_screen(COLOR_BLACK);
    ui_draw_text_centered(200, "MODO MULTIJUGADOR", COLOR_CYAN_LIGHT);
    ui_draw_text_centered(240, "SW15 ON : MAESTRO     SW15 OFF : ESCLAVO", COLOR_WHITE);
    ui_draw_text_centered(300, "ESCLAVO: presiona BTNC primero", COLOR_YELLOW);
    ui_draw_text_centered(340, "MAESTRO: espera y luego presiona BTNC", COLOR_RED_LIGHT);
}

int main(void) {
    input_init();
    sd_image_init();

    preload_images();

    GameState state      = STATE_MENU;
    GameState prev_state = (GameState)(-1);
    int       is_master  = 0;

    int sw0_prev = 0, sw1_prev = 0, btnc_prev = 0;

    while (1) {
        if (state != prev_state) {

            /* Estados bloqueantes: llaman a una funcion y vuelven al menu */
            if (state == STATE_GAME) {
                pong_paddles_run();
                state      = STATE_MENU;
                prev_state = (GameState)(-1);
                continue;
            }
            if (state == STATE_MP_HANDSHAKE) {
                int ok = pong_mp_handshake(is_master);
                state      = ok ? STATE_MP_GAME : STATE_MENU;
                prev_state = (GameState)(-1);
                continue;
            }
            if (state == STATE_MP_GAME) {
                if (is_master) pong_mp_master_run();
                else           pong_mp_slave_run();
                state      = STATE_MENU;
                prev_state = (GameState)(-1);
                continue;
            }

            /* Estados de pantalla: solo dibujan y esperan input */
            switch (state) {
                case STATE_MENU:            draw_state_menu();            break;
                case STATE_MP_ROLE_SELECT:  draw_state_mp_role_select();  break;
                default: break;
            }
            prev_state = state;
        }

        unsigned int sw  = input_read_switches();
        unsigned int btn = input_read_buttons();

        int sw0_now  = bit_is_set(sw,  SW0_BIT);
        int sw1_now  = bit_is_set(sw,  SW1_BIT);
        int btnc_now = bit_is_set(btn, BTNC_BIT);

        int sw0_edge  = sw0_now  && !sw0_prev;
        int sw1_edge  = sw1_now  && !sw1_prev;
        int btnc_edge = btnc_now && !btnc_prev;

        sw0_prev  = sw0_now;
        sw1_prev  = sw1_now;
        btnc_prev = btnc_now;

        switch (state) {
            case STATE_MENU:
                if      (sw0_edge) state = STATE_GAME;
                else if (sw1_edge) state = STATE_MP_ROLE_SELECT;
                break;
            case STATE_MP_ROLE_SELECT:
                if (btnc_edge) {
                    /* Leer SW15 directo del GPIO — input_read_switches devuelve
                     * flancos ascendentes, no niveles; SW15 estable daria 0 */
                    unsigned int sw_raw = Xil_In32(GPIO_SWITCHES_BASE + GPIO_DATA_REG);
                    is_master = (sw_raw >> SW15_BIT) & 1;
                    state     = STATE_MP_HANDSHAKE;
                }
                break;
            default: break;
        }
    }

    return 0;
}