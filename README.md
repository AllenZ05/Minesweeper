# Mineseeker — UW Edition

A desktop Minesweeper game inspired by the University of Waterloo: clear the campus, flag the Canadian geese, and beat your personal best.

## Features

- **Classic Minesweeper:** Reveal safe tiles and flag suspected geese. When the flags around a revealed number match its value, click it to reveal the remaining neighbours.
- **Safe openings:** Your first reveal and its neighbours are safe in every preset and custom game.
- **Five difficulties:** Beginner (6×6, 5 geese), Easy (9×9, 10), Medium (16×16, 40), Hard (30×16, 99), and Expert (30×20, 150).
- **Custom boards:** Choose 4–40 columns, 4–20 rows, and your goose count.
- **Personal bests:** Your fastest clear of each difficulty is saved locally.
- **Mouse and keyboard controls:** Play either way, with optional animations and a resizable interface that supports Retina displays.
- **Pause and resume:** Pausing or switching windows hides the board and stops the timer.

## Getting Started

On macOS, install [raylib](https://www.raylib.com/) and `pkg-config` with Homebrew, then build and play:

```sh
brew install raylib pkg-config
make
./mineseeker
```

Built with C++20 and raylib 6. Keep the `assets` directory beside the executable if you move the game.

## Controls

| Action | Mouse / keyboard |
| --- | --- |
| Reveal or chord | Left-click, or Space / Enter on the focused cell |
| Toggle a flag | Right-click, or F on the focused cell |
| Move between cells | Arrow keys |
| Move between controls | Tab / Shift+Tab; Up / Down also work in menus |
| Activate a control | Enter / Space |
| Restart | R |
| Pause / resume | P |
| Return to the menu | Esc |
| Open controls / help | H / F1 |
| Toggle animations | M, or the Motion button in the menu |

In Custom, click a value to type it. Enter applies the change; Esc cancels it.

## Development

```sh
make test       # Board rules, game state, timing, and saved-record tests
make test-ui    # Input and rendering checks; requires a desktop/OpenGL context
```

- `src/board.h` / `src/board.cpp` — board rules, placement, and iterative flood fill
- `src/game.h` / `src/game.cpp` — game phases, timing, pause, results, and record persistence
- `src/ui.h` / `src/ui.cpp` — layout, rendering, animation, and mouse/keyboard interaction
- `src/gui_main.cpp` — window setup and application loop
- `tests/` — board, game-state, and interface regression tests
