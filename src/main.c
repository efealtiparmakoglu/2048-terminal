/* main.c — terminal front end: ANSI colours, no external libraries. */
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static struct termios saved;
static int raw_enabled = 0;

static void raw_on(void) {
    struct termios raw;
    if (tcgetattr(STDIN_FILENO, &saved) != 0)
        return;
    raw = saved;
    raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1; /* decisecond timeout, so we can poll */
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    raw_enabled = 1;
}

static void raw_off(void) {
    if (raw_enabled)
        tcsetattr(STDIN_FILENO, TCSANOW, &saved);
    raw_enabled = 0;
}

static const char *color_for(int value) {
    switch (value) {
    case 2: return "\033[97;47m";
    case 4: return "\033[97;43m";
    case 8: return "\033[38;5;208m";
    case 16: return "\033[38;5;202m";
    case 32: return "\033[38;5;166m";
    case 64: return "\033[38;5;220m";
    case 128: return "\033[38;5;196m";
    case 256: return "\033[97;41m";
    case 512: return "\033[97;45m";
    case 1024: return "\033[97;44m";
    case 2048: return "\033[30;107m";
    default: return "\033[38;5;231m";
    }
}

static void draw(const Game *game, int interactive) {
    if (interactive)
        printf("\033[H\033[2J"); /* home + clear */
    printf("\033[1m2048\033[0m   score: %-6d moves: %-4d best tile: %d\n\n",
           game->score, game->moves, game->max_tile);
    for (int row = 0; row < 4; row++) {
        for (int column = 0; column < 4; column++) {
            int value = game->board[row * 4 + column];
            if (value == 0) {
                printf("   \033[38;5;240m  ·  \033[0m");
            } else {
                char label[8];
                snprintf(label, sizeof label, "%4d", value);
                printf("   %s%s \033[0m", color_for(value), label);
            }
        }
        printf("\n");
        if (row < 3)
            printf("\n");
    }
    if (game->won)
        printf("\n\033[1;32m2048 reached! Keep going for a higher score.\033[0m\n");
    if (game->over)
        printf("\n\033[1;31mNo moves left. Final score: %d\033[0m\n", game->score);
}

static int play(void) {
    Game game;
    game_init(&game, (unsigned)time(NULL));
    raw_on();
    puts("arrows or wasd to move, r to restart, q to quit");

    for (;;) {
        draw(&game, 1);
        char key = 0;
        if (read(STDIN_FILENO, &key, 1) != 1)
            continue;
        if (key == 0x1b) { /* arrow escape sequence */
            char seq[2] = {0, 0};
            if (read(STDIN_FILENO, &seq[0], 1) != 1)
                continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1)
                continue;
            switch (seq[1]) {
            case 'A': game_move(&game, MOVE_UP); break;
            case 'B': game_move(&game, MOVE_DOWN); break;
            case 'C': game_move(&game, MOVE_RIGHT); break;
            case 'D': game_move(&game, MOVE_LEFT); break;
            default: break;
            }
            continue;
        }
        switch (key) {
        case 'a': case 'h': game_move(&game, MOVE_LEFT); break;
        case 'd': case 'l': game_move(&game, MOVE_RIGHT); break;
        case 'w': case 'k': game_move(&game, MOVE_UP); break;
        case 's': case 'j': game_move(&game, MOVE_DOWN); break;
        case 'r': game_reset(&game); break;
        case 'q': case 0x03: raw_off(); return 0;
        default: break;
        }
    }
}

/* --demo: play a fixed number of scripted moves, then print the board.
 * Used by the test target and handy for eyeballing the rules. */
static int demo(int moves) {
    Game game;
    game_init(&game, 12345);
    int direction = MOVE_LEFT;
    for (int i = 0; i < moves; i++) {
        direction = (direction + 1) % 4;
        game_move(&game, direction);
    }
    draw(&game, 0);
    printf("moves played: %d, score: %d, best: %d\n", game.moves, game.score,
           game.max_tile);
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--demo") == 0) {
        return demo(argc > 2 ? atoi(argv[2]) : 20);
    }
    if (argc > 1 && (strcmp(argv[1], "--version") == 0)) {
        puts("2048-terminal 1.0.0");
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        puts("2048-terminal 1.0.0 — 2048 in the terminal\n"
             "usage: 2048-terminal [--demo N] [--version] [--help]");
        return 0;
    }
    return play();
}
