/* game.h — 2048 rules, separated from the terminal so they can be tested. */
#ifndef GAME_H
#define GAME_H

#define MOVE_LEFT 0
#define MOVE_RIGHT 1
#define MOVE_UP 2
#define MOVE_DOWN 3

typedef struct {
    int board[16];
    int empty;      /* how many cells are still free   */
    int score;
    int moves;
    int max_tile;
    int won;        /* 2048 reached                    */
    int over;       /* no legal move left              */
    unsigned rng;
} Game;

void game_init(Game *game, unsigned seed);
void game_reset(Game *game);
int game_spawn(Game *game);
int game_move(Game *game, int direction);
int game_can_move(Game *game);

/* internal, exposed for the reversed-direction helpers */
int slide_row_reversed(Game *game, int row, int *gained);
int slide_column_reversed(Game *game, int column, int *gained);

#endif
