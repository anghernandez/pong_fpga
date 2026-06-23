#include "game.h"
#include "vga.h"
#include "xil_io.h"

#define BTN_DATA 0x40010000
#define BTN_TRI  0x40010004


#define BTN_C   (1u << 0)
#define BTN_U   (1u << 1)
#define BTN_L   (1u << 2)
#define BTN_R   (1u << 3)
#define BTN_D   (1u << 4)


#define FRAME_TICK_LIMIT 20000

/*static void erase_game(Game *game)
{
    draw_rect(game->player.x, game->player.y,
              game->player.width, game->player.height,
              COLOR_BLACK);

    draw_rect(game->cpu.x, game->cpu.y,
              game->cpu.width, game->cpu.height,
              COLOR_BLACK);

    draw_rect(game->ball.x, game->ball.y,
              game->ball.size, game->ball.size,
              COLOR_BLACK);
}*/

//funcion para borrar ball

/*static void erase_ball_area(int x, int y, int w, int h)
{
    for (int row = 0; row < h; row++) {
        draw_hline(x, y + row, w, COLOR_BLACK);
    }
}*/



/*static void erase_ball_trail(Game *game)
{
    int x0 = game->ball.prev_x;
    int y0 = game->ball.prev_y;
    int x1 = game->ball.x;
    int y1 = game->ball.y;
    int s  = game->ball.size;

    if (x1 > x0) {
        draw_rect(x0, y0, x1 - x0, s, COLOR_BLACK);
    } else if (x1 < x0) {
        draw_rect(x1 + s, y0, x0 - x1, s, COLOR_BLACK);
    }

    if (y1 > y0) {
        draw_hline(x0, y0, s, COLOR_BLACK);
        draw_hline(x0, y0 + 1, s, COLOR_BLACK);
    } else if (y1 < y0) {
        draw_hline(x0, y0 + s - 1, s, COLOR_BLACK);
        draw_hline(x0, y0 + s - 2, s, COLOR_BLACK);
    }
}
*/



static int min_int(int a, int b)
{
    return (a < b) ? a : b;
}

static int max_int(int a, int b)
{
    return (a > b) ? a : b;
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

static void erase_game(Game *game)
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

            game_update(&game, player_up, player_down);

            erase_game(&game);

            //erase_ball_trail(&game);
            erase_ball_union(&game);
            draw_game(&game);
        }
    }

    return 0;
}