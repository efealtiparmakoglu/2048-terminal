/* game_tests.c — the 2048 rules, with a seeded RNG so every run matches. */
#include <stdio.h>
#include <string.h>

#include "../src/game.h"

static int tests_run = 0;

#define FAIL(...)                                                             \
    do {                                                                      \
        printf("FAIL %s:%d: ", __FILE__, __LINE__);                           \
        printf(__VA_ARGS__);                                                  \
        printf("\n");                                                         \
        return 1;                                                             \
    } while (0)

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond))                                                          \
            FAIL("check failed: %s", #cond);                                  \
    } while (0)

static void test_reset(void) { tests_run++; }

static Game seeded(void) {
    Game game;
    game_init(&game, 99);
    return game;
}

static int test_initial_board(void) {
    Game game = seeded();
    int filled = 0;
    for (int i = 0; i < 16; i++)
        if (game.board[i] != 0)
            filled++;
    if (filled != 2) FAIL("starting tiles = %d, want 2", filled);
    if (game.empty != 14) FAIL("empty = %d, want 14", game.empty);
    for (int i = 0; i < 16; i++) {
        if (game.board[i] != 0 && game.board[i] != 2 && game.board[i] != 4)
            FAIL("tile %d has value %d", i, game.board[i]);
    }
    if (game.score != 0) FAIL("score should start at 0");
    test_reset();
    return 0;
}

static int test_same_seed_same_board(void) {
    Game a = seeded();
    Game b = seeded();
    if (memcmp(a.board, b.board, sizeof a.board) != 0)
        FAIL("the same seed must produce the same board");
    test_reset();
    return 0;
}

static int test_left_merges(void) {
    Game game = seeded();
    memset(game.board, 0, sizeof game.board);
    game.empty = 14;
    game.board[0] = 2; game.board[1] = 2; game.board[3] = 4;
    int moved = game_move(&game, MOVE_LEFT);
    if (!moved) FAIL("the move should count as a change");
    /* 2,2,4 -> 4,4 -> then the new tile spawns somewhere */
    if (game.board[0] != 4) FAIL("tiles did not merge: %d %d", game.board[0], game.board[1]);
    if (game.score != 4) FAIL("score = %d, want 4", game.score);
    test_reset();
    return 0;
}

static int test_merge_is_once_per_move(void) {
    Game game = seeded();
    memset(game.board, 0, sizeof game.board);
    game.empty = 16;
    game.board[0] = 2; game.board[1] = 2; game.board[2] = 2; game.board[3] = 2;
    game_move(&game, MOVE_LEFT);
    /* 2,2,2,2 -> 4,4 -> the first pair merges, the second cannot reuse it */
    if (game.board[0] != 4) FAIL("first cell = %d, want 4", game.board[0]);
    if (game.board[1] != 4) FAIL("second cell = %d, want 4", game.board[1]);
    if (game.score != 8) FAIL("score = %d, want 8", game.score);
    test_reset();
    return 0;
}

static int test_no_move_is_not_counted(void) {
    Game game = seeded();
    memset(game.board, 0, sizeof game.board);
    game.empty = 16;
    for (int i = 0; i < 15; i++)
        game.board[i] = (i % 3 == 0) ? 2 : 4;
    game.board[15] = 2;
    int moves = game.moves;
    /* fill the board so nothing can slide */
    for (int i = 0; i < 16; i++)
        game.board[i] = ((i / 4 + i % 4) % 2 == 0) ? 2 : 4; /* checkerboard */
    if (game_move(&game, MOVE_LEFT)) FAIL("a full unmergeable board must not move");
    if (game.moves != moves) FAIL("moves counter changed");
    test_reset();
    return 0;
}

static int test_directions_all_merge(void) {
    struct { int direction; int index; } cases[4] = {
        {MOVE_LEFT, 0}, {MOVE_RIGHT, 3}, {MOVE_UP, 0}, {MOVE_DOWN, 3},
    };
    for (int c = 0; c < 4; c++) {
        Game game = seeded();
        memset(game.board, 0, sizeof game.board);
        game.empty = 15;
        if (cases[c].direction == MOVE_LEFT || cases[c].direction == MOVE_RIGHT)
            game.board[0] = 2, game.board[1] = 2;
        else
            game.board[0] = 2, game.board[4] = 2;
        int before = game.score;
        game_move(&game, cases[c].direction);
        if (game.score <= before)
            FAIL("direction %d did not merge", cases[c].direction);
    }
    test_reset();
    return 0;
}

static int test_right_pushes_to_the_end(void) {
    Game game = seeded();
    memset(game.board, 0, sizeof game.board);
    game.empty = 15;
    game.board[0] = 2;
    game.board[1] = 4;
    game_move(&game, MOVE_RIGHT);
    if (game.board[2] != 2 || game.board[3] != 4)
        FAIL("right slide produced %d %d", game.board[2], game.board[3]);
    test_reset();
    return 0;
}

static int test_win_is_recorded(void) {
    Game game = seeded();
    memset(game.board, 0, sizeof game.board);
    game.empty = 15;
    game.board[0] = 1024;
    game.board[1] = 1024;
    game_move(&game, MOVE_LEFT);
    if (game.max_tile != 2048) FAIL("max tile = %d, want 2048", game.max_tile);
    if (!game.won) FAIL("winning should set the won flag");
    test_reset();
    return 0;
}

static int test_game_over_detection(void) {
    Game game = seeded();
    memset(game.board, 0, sizeof game.board);
    game.empty = 0;
    for (int i = 0; i < 16; i++)
        game.board[i] = ((i / 4 + i % 4) % 2 == 0) ? 2 : 4; /* checkerboard */
    if (game_can_move(&game)) FAIL("the board is not stuck");
    game_move(&game, MOVE_LEFT);
    if (!game.over) FAIL("moving on a stuck board should end the game");
    test_reset();
    return 0;
}

static int test_spawn_respects_free_cells(void) {
    Game game = seeded();
    game.empty = 1;
    memset(game.board, 0, sizeof game.board);
    game.board[0] = 2;
    game.empty = 15;
    for (int i = 1; i < 16; i++)
        game.board[i] = 0;
    game.empty = 15;
    int filled = 0;
    for (int i = 0; i < 16; i++)
        if (game.board[i])
            filled++;
    if (filled != 1) FAIL("setup failed, filled = %d", filled);
    test_reset();
    return 0;
}

static int test_demo_run_is_stable(void) {
    Game a, b;
    game_init(&a, 12345);
    game_init(&b, 12345);
    for (int i = 0; i < 30; i++) {
        game_move(&a, i % 4);
        game_move(&b, i % 4);
    }
    if (a.score != b.score) FAIL("same seed, different scores: %d vs %d", a.score, b.score);
    if (a.moves != b.moves) FAIL("move counts differ");
    test_reset();
    return 0;
}

int main(void) {
    int fails = 0;
    fails += test_initial_board();
    fails += test_same_seed_same_board();
    fails += test_left_merges();
    fails += test_merge_is_once_per_move();
    fails += test_no_move_is_not_counted();
    fails += test_directions_all_merge();
    fails += test_right_pushes_to_the_end();
    fails += test_win_is_recorded();
    fails += test_game_over_detection();
    fails += test_spawn_respects_free_cells();
    fails += test_demo_run_is_stable();
    if (fails) {
        printf("game_tests: %d test function(s) FAILED\n", fails);
        return 1;
    }
    printf("game_tests: all %d test groups passed\n", tests_run);
    return 0;
}
