#ifndef GAME_H
#define GAME_H


typedef struct {
    int x;
    int y;
    int prev_y;
    int width;
    int height;
    int speed;
} Paddle;

typedef struct {
    int x;
    int y;
    int prev_x;
    int prev_y;
    int size;
    int vx;
    int vy;
} Ball;


typedef struct {
    Paddle player;
    Paddle cpu;
    Ball ball;
    int player_score;
    int cpu_score;
} Game;

void game_init(Game *game);
void game_update(Game *game, int player_up, int player_down);




#endif