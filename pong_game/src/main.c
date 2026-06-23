#include "game.h"
#include "vga.h"
#include "xil_io.h"

#define SW_DATA 0x40020000
#define SW_TRI 0x40020004


#define BTN_DATA 0x40010000
#define BTN_TRI  0x40010004

#define BTN_C   (1u << 0)
#define BTN_U   (1u << 1)
#define BTN_L   (1u << 2)
#define BTN_R   (1u << 3)
#define BTN_D   (1u << 4)



#define FRAME_TICK_LIMIT 20000


static int min_int(int a, int b)
{
    return (a < b) ? a : b;
}

static int max_int(int a, int b)
{
    return (a > b) ? a : b;
}


static int get_speed_from_switches(void)
{
    unsigned int sw = Xil_In32(SW_DATA);

    if (sw & (1 << 4)) return 3; // rápida
    if (sw & (1 << 3)) return 2; // media
    if (sw & (1 << 2)) return 1; // lenta

    return 1;
}

static void erase_ball_union(Game *game)
{
    int margin = 2;

    int x0 = game->ball.prev_x - margin;
    int y0 = game->ball.prev_y - margin;
    int x1 = game->ball.x - margin;
    int y1 = game->ball.y - margin;

    int left   = min_int(x0, x1);
    int top    = min_int(y0, y1);
    int right  = max_int(x0, x1) + game->ball.size + margin * 2;
    int bottom = max_int(y0, y1) + game->ball.size + margin * 2;

    for (int y = top; y < bottom; y++) {
        draw_hline(left, y, right - left, COLOR_BLACK);
    }
}

static void erase_incremental(Game *game)
{
    // Player paddle: borrar solo la parte que dejó atrás
    if (game->player.y > game->player.prev_y) {
        draw_rect(game->player.x, game->player.prev_y,
                  game->player.width, game->player.y - game->player.prev_y,
                  COLOR_BLACK);
    } else if (game->player.y < game->player.prev_y) {
        draw_rect(game->player.x, game->player.y + game->player.height,
                  game->player.width, game->player.prev_y - game->player.y,
                  COLOR_BLACK);
    }

    // CPU paddle
    if (game->cpu.y > game->cpu.prev_y) {
        draw_rect(game->cpu.x, game->cpu.prev_y,
                  game->cpu.width, game->cpu.y - game->cpu.prev_y,
                  COLOR_BLACK);
    } else if (game->cpu.y < game->cpu.prev_y) {
        draw_rect(game->cpu.x, game->cpu.y + game->cpu.height,
                  game->cpu.width, game->cpu.prev_y - game->cpu.y,
                  COLOR_BLACK);
    }




}


static void erase_full_game(Game *game)
{
    draw_rect(game->player.x, game->player.y,
              game->player.width, game->player.height,
              COLOR_BLACK);

    draw_rect(game->cpu.x, game->cpu.y,
              game->cpu.width, game->cpu.height,
              COLOR_BLACK);

    draw_rect(game->ball.x - 2, game->ball.y - 2,
              game->ball.size + 4, game->ball.size + 4,
              COLOR_BLACK);
}


static void draw_game(Game *game)
{
    draw_rect(game->player.x, game->player.y,
              game->player.width, game->player.height,
              COLOR_WHITE);

    draw_rect(game->cpu.x, game->cpu.y,
              game->cpu.width, game->cpu.height,
              COLOR_WHITE);

    draw_rect(game->ball.x, game->ball.y,
              game->ball.size, game->ball.size,
              COLOR_RED_LIGHT);
}

int main(void)
{
    Game game;
    unsigned int frame_counter = 0;

    game_init(&game);
    Xil_Out32(BTN_TRI, 0xFFFFFFFF);
    Xil_Out32(SW_TRI,  0xFFFFFFFF);

    int prev_pause_btn = 0;
    int prev_reset_btn = 0; 
    clear_screen(COLOR_BLACK);
   // draw_border(COLOR_WHITE);
    draw_game(&game);

while (1) {
    frame_counter++;


    



    if (frame_counter >= FRAME_TICK_LIMIT) {
        frame_counter = 0;

        unsigned int btn = Xil_In32(BTN_DATA);

        int player_up   = (btn & BTN_U) ? 1 : 0;
        int player_down = (btn & BTN_D) ? 1 : 0;

        int pause_btn = (btn & BTN_L) ? 1 : 0;
        int reset_btn = (btn & BTN_C) ? 1 : 0;

        if (reset_btn && !prev_reset_btn) {
            erase_full_game(&game);
            erase_incremental(&game);
            erase_ball_union(&game);

            game_reset(&game);

            draw_game(&game);

            prev_reset_btn = reset_btn;
            continue;
        }

        prev_reset_btn = reset_btn;

        if (pause_btn && !prev_pause_btn) {
            game.paused = !game.paused;
        }

        prev_pause_btn = pause_btn;

      if (!game.paused) {
        game.speed = get_speed_from_switches();

        game_update(&game, player_up, player_down);

        erase_incremental(&game);
        erase_ball_union(&game);

        draw_game(&game);

        }
    }
}
    return 0;
}


