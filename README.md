# 2048-terminal

**2048 in the terminal — the rules separated from the front end so they can be tested, ANSI colours, no libraries beyond libc.**

```
$ 2048-terminal
arrows or wasd to move, r to restart, q to quit

2048   score: 52     moves: 12   best tile: 16

      2        ·        ·        ·
      ·        ·        8        ·
      4        2        ·        ·
      ·       16        ·        4
```

The original 2048 is a twenty-line ruleset and a screen full of
presentation. This splits them: `src/game.c` knows how tiles slide, merge
and spawn, and nothing else — no I/O, no terminal, no randomness that
isn't seeded. The front end only draws and reads keys.

## Why 2048-terminal

- **The rules are a library.** Sliding in all four directions, merge-once
  per move, the 2048 win flag, the stuck-board detection and the
  deterministic xorshift RNG all live in `game.c` and are tested
  directly — no terminal required.
- **Merge exactly once per move.** `2 2 2 2` slid left becomes `4 4`, not
  `8`: a tile produced by a merge cannot merge again in the same move.
  The test suite pins this, because it is the rule every reimplementation
  gets wrong at least once.
- **All four directions, including the awkward ones.** Left and up slide
  in place; right and down are handled by mirroring the row/column,
  sliding, and mirroring back — one implementation of the merge rule
  instead of four.
- **Deterministic by seed.** The same seed gives the same board and the
  same demo run, which is what makes the rules testable and the `--demo`
  mode reproducible.
- **Honest end conditions.** Reaching 2048 sets a flag (and the game lets
  you keep going); a board with no legal move ends the game, including
  when the first key you press is the one that proves it.
- **Colours that work in a normal terminal.** Tile colours are 256-colour
  ANSI, matched to the tile value, with a bright inverse for 2048 itself.

## Usage

```sh
2048-terminal            # play (arrows or wasd, r restart, q quit)
2048-terminal --demo 40  # scripted run, prints the board and the score
2048-terminal --version
```

## Development

```sh
make test    # 11 rule tests + demo/version smoke
```

## License

MIT — see [LICENSE](LICENSE).
