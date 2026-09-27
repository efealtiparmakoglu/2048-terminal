/* game.c — the 2048 rules: sliding, merging, spawning, scoring. No I/O,
 * so the whole game is testable without a terminal. */
#include "game.h"

#include <stdlib.h>
#include <string.h>

void game_init(Game *game, unsigned seed) {
    memset(game, 0, sizeof *game);
    game->score = 0;
    game->moves = 0;
    game->won = 0;
    game->over = 0;
    game->rng = seed ? seed : 1u;
    game_reset(game);
}

void game_reset(Game *game) {
    memset(game->board, 0, sizeof game->board);
    game->empty = 16;
    game->max_tile = 0;
    game_spawn(game);
    game_spawn(game);
}

static unsigned next_random(Game *game) {
    /* xorshift32: deterministic given the seed, good enough for a game */
    unsigned x = game->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    game->rng = x ? x : 1u;
    return game->rng;
}

int game_spawn(Game *game) {
    if (game->empty == 0)
        return -1;
    int index = (int)(next_random(game) % 16u);
    while (game->board[index] != 0) {
        index = (int)(next_random(game) % 16u);
        if (game->empty == 0)
            return -1;
    }
    game->board[index] = (next_random(game) % 10u < 7u) ? 2 : 4;
    game->empty--;
    if (game->board[index] > game->max_tile)
        game->max_tile = game->board[index];
    return index;
}

static int slide_row(Game *game, int row, int *gained) {
    int values[4];
    int count = 0;
    for (int column = 0; column < 4; column++) {
        if (game->board[row * 4 + column] != 0)
            values[count++] = game->board[row * 4 + column];
    }
    int output[4] = {0, 0, 0, 0};
    int written = 0;
    for (int i = 0; i < count; i++) {
        if (i + 1 < count && values[i] == values[i + 1]) {
            int merged = values[i] * 2;
            output[written++] = merged;
            *gained += merged;
            if (merged > game->max_tile)
                game->max_tile = merged;
            if (merged == 2048)
                game->won = 1;
            i++;
        } else {
            output[written++] = values[i];
        }
    }
    int changed = 0;
    for (int column = 0; column < 4; column++) {
        if (game->board[row * 4 + column] != output[column])
            changed = 1;
        game->board[row * 4 + column] = output[column];
    }
    return changed;
}

static int slide_column(Game *game, int column, int *gained) {
    int values[4];
    int count = 0;
    for (int row = 0; row < 4; row++) {
        if (game->board[row * 4 + column] != 0)
            values[count++] = game->board[row * 4 + column];
    }
    int output[4] = {0, 0, 0, 0};
    int written = 0;
    for (int i = 0; i < count; i++) {
        if (i + 1 < count && values[i] == values[i + 1]) {
            int merged = values[i] * 2;
            output[written++] = merged;
            *gained += merged;
            if (merged > game->max_tile)
                game->max_tile = merged;
            if (merged == 2048)
                game->won = 1;
            i++;
        } else {
            output[written++] = values[i];
        }
    }
    int changed = 0;
    for (int row = 0; row < 4; row++) {
        if (game->board[row * 4 + column] != output[row])
            changed = 1;
        game->board[row * 4 + column] = output[row];
    }
    return changed;
}

int game_move(Game *game, int direction) {
    if (game->over)
        return 0;
    int gained = 0;
    int changed = 0;
    switch (direction) {
    case MOVE_LEFT:
        for (int row = 0; row < 4; row++)
            changed |= slide_row(game, row, &gained);
        break;
    case MOVE_RIGHT:
        for (int row = 0; row < 4; row++)
            changed |= slide_row_reversed(game, row, &gained);
        break;
    case MOVE_UP:
        for (int column = 0; column < 4; column++)
            changed |= slide_column(game, column, &gained);
        break;
    case MOVE_DOWN:
        for (int column = 0; column < 4; column++)
            changed |= slide_column_reversed(game, column, &gained);
        break;
    default:
        return 0;
    }
    if (!changed) {
        /* nothing slid — if the board is stuck, the game is over even
         * though this keystroke did not count as a move */
        if (!game_can_move(game))
            game->over = 1;
        return 0;
    }
    game->score += gained;
    game->moves++;
    game_spawn(game);
    if (!game_can_move(game))
        game->over = 1;
    return 1;
}

int slide_row_reversed(Game *game, int row, int *gained) {
    int mirror[4];
    for (int i = 0; i < 4; i++)
        mirror[i] = game->board[row * 4 + (3 - i)];
    Game flipped = *game;
    for (int i = 0; i < 4; i++)
        flipped.board[row * 4 + i] = mirror[i];
    int changed = slide_row(&flipped, row, gained);
    for (int i = 0; i < 4; i++)
        game->board[row * 4 + (3 - i)] = flipped.board[row * 4 + i];
    game->max_tile = flipped.max_tile;
    game->won = flipped.won;
    return changed;
}

int slide_column_reversed(Game *game, int column, int *gained) {
    int mirror[4];
    for (int i = 0; i < 4; i++)
        mirror[i] = game->board[(3 - i) * 4 + column];
    Game flipped = *game;
    for (int i = 0; i < 4; i++)
        flipped.board[i * 4 + column] = mirror[i];
    int changed = slide_column(&flipped, column, gained);
    for (int i = 0; i < 4; i++)
        game->board[(3 - i) * 4 + column] = flipped.board[i * 4 + column];
    game->max_tile = flipped.max_tile;
    game->won = flipped.won;
    return changed;
}

int game_can_move(Game *game) {
    for (int i = 0; i < 16; i++) {
        if (game->board[i] == 0)
            return 1;
        int row = i / 4, column = i % 4;
        if (column < 3 && game->board[i] == game->board[i + 1])
            return 1;
        if (row < 3 && game->board[i] == game->board[i + 4])
            return 1;
    }
    return 0;
}
