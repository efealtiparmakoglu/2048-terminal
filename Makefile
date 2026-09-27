CC      ?= cc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra -Wpedantic

all: 2048-terminal

2048-terminal: src/main.c src/game.c src/game.h
	$(CC) $(CFLAGS) -o $@ src/main.c src/game.c

game_tests: tests/game_tests.c src/game.c src/game.h
	$(CC) $(CFLAGS) -o game_tests tests/game_tests.c src/game.c

test: 2048-terminal game_tests
	./game_tests
	./2048-terminal --demo 40 | grep -q 'best:'
	./2048-terminal --version | grep -q '1.0.0' && echo "e2e: demo + version OK"

clean:
	rm -f 2048-terminal game_tests

.PHONY: all test clean
