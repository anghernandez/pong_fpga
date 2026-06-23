#include "game.h"
#include "config.h"

static void reset_ball(Game *game)
{
    game->ball.prev_x = game->ball.x;
    game->ball.prev_y = game->ball.y;

    game->ball.x = SCREEN_WIDTH / 2;
    game->ball.y = SCREEN_HEIGHT / 2;

    game->ball.size = BALL_SIZE;
    game->ball.vx = BALL_SPEED_X;
    game->ball.vy = BALL_SPEED_Y;
}

/*
void game_reset(Game *game)
{
    game->player.y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    game->cpu.y    = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;

    game->player_score = 0;
    game->cpu_score = 0;

    game->paused = 0;

    reset_ball(game);
}*/


void game_reset(Game *game)
{
    game->player.y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    game->player.prev_y = game->player.y;

    game->cpu.y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    game->cpu.prev_y = game->cpu.y;

    game->player_score = 0;
    game->cpu_score = 0;

    game->paused = 0;
    game->speed = 1; // esto es para velocidad 

    reset_ball(game);


}

void game_init(Game *game)
{
    game->player.x = 20;
    game->player.y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    game->player.width = PADDLE_WIDTH;
    game->player.height = PADDLE_HEIGHT;
    game->player.speed = PADDLE_SPEED;

    game->cpu.x = SCREEN_WIDTH - 20 - PADDLE_WIDTH;
    game->cpu.y = (SCREEN_HEIGHT - PADDLE_HEIGHT) / 2;
    game->cpu.width = PADDLE_WIDTH;
    game->cpu.height = PADDLE_HEIGHT;
    game->cpu.speed = CPU_SPEED;

    game->player_score = 0;
    game->cpu_score = 0;

    // esto es para pausar el juego
    game->paused = 0;  
    game->speed = 1;   
    reset_ball(game);
}

static void move_player(Game *game, int up, int down)
{
    if (up) {
        game->player.y -= game->player.speed;
    }

    if (down) {
        game->player.y += game->player.speed;
    }

    if (game->player.y < 0) {
        game->player.y = 0;
    }

    if (game->player.y + game->player.height > SCREEN_HEIGHT) {
        game->player.y = SCREEN_HEIGHT - game->player.height;
    }
}

static void move_cpu(Game *game)
{
    int cpu_center = game->cpu.y + game->cpu.height / 2;
    int ball_center = game->ball.y + game->ball.size / 2;

    if (ball_center > cpu_center) {
        game->cpu.y += game->cpu.speed;
    } else if (ball_center < cpu_center) {
        game->cpu.y -= game->cpu.speed;
    }

    if (game->cpu.y < 0) {
        game->cpu.y = 0;
    }

    if (game->cpu.y + game->cpu.height > SCREEN_HEIGHT) {
        game->cpu.y = SCREEN_HEIGHT - game->cpu.height;
    }
}

static int rect_collision(int ax, int ay, int aw, int ah,
                          int bx, int by, int bw, int bh)
{
    return ax < bx + bw &&
           ax + aw > bx &&
           ay < by + bh &&
           ay + ah > by;
}



static void update_ball(Game *game)
{
    int steps = game->speed;

    if (steps < 1) steps = 1;
    if (steps > 3) steps = 3;

    for (int i = 0; i < steps; i++) {
        game->ball.x += game->ball.vx;
        game->ball.y += game->ball.vy;

        // Rebote con borde superior
        if (game->ball.y <= 1) {
            game->ball.y = 1;

            if (game->ball.vy < 0) {
                game->ball.vy = -game->ball.vy;
            }
        }

        // Rebote con borde inferior
        if (game->ball.y + game->ball.size >= SCREEN_HEIGHT - 1) {
            game->ball.y = SCREEN_HEIGHT - game->ball.size - 1;

            if (game->ball.vy > 0) {
                game->ball.vy = -game->ball.vy;
            }
        }

        // Colisión con paddle del jugador
        if (rect_collision(game->ball.x, game->ball.y,
                           game->ball.size, game->ball.size,
                           game->player.x, game->player.y,
                           game->player.width, game->player.height)) {

            game->ball.x = game->player.x + game->player.width + 1;

            if (game->ball.vx < 0) {
                game->ball.vx = -game->ball.vx;
            }
        }

        // Colisión con paddle CPU
        if (rect_collision(game->ball.x, game->ball.y,
                           game->ball.size, game->ball.size,
                           game->cpu.x, game->cpu.y,
                           game->cpu.width, game->cpu.height)) {

            game->ball.x = game->cpu.x - game->ball.size - 1;

            if (game->ball.vx > 0) {
                game->ball.vx = -game->ball.vx;
            }
        }

        // Punto para CPU
        if (game->ball.x + game->ball.size < 0) {
            game->cpu_score++;
            reset_ball(game);
            break;
        }

        // Punto para jugador
        if (game->ball.x > SCREEN_WIDTH) {
            game->player_score++;
            reset_ball(game);
            break;
        }
    }
}

void game_update(Game *game, int player_up, int player_down)
{
    game->player.prev_y = game->player.y;
    game->cpu.prev_y = game->cpu.y;

    game->ball.prev_x = game->ball.x;
    game->ball.prev_y = game->ball.y;

    move_player(game, player_up, player_down);
    move_cpu(game);
    update_ball(game);
}