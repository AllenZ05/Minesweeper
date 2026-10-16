# Minesweeper — UW Edition

A desktop Minesweeper game inspired by the University of Waterloo: clear the campus, flag the Canadian geese, and beat your personal best.

## Demo

https://github.com/user-attachments/assets/abed9941-ae4a-4830-8f50-09bf21d47cab

[Watch the demo in your browser](https://cdn.jsdelivr.net/gh/AllenZ05/Minesweeper@main/docs/minesweeper-demo.mp4) if the embedded player is unavailable on your phone or tablet.

## Features

- **Classic Minesweeper:** Reveal safe tiles and flag suspected geese. When the flags around a revealed number match its value, click it to reveal the remaining neighbours.
- **Safe openings:** Your first reveal and its neighbours are safe in every preset and custom game.
- **Five difficulties:** Beginner (6×6, 5 geese), Easy (9×9, 10), Medium (16×16, 40), Hard (30×16, 99), and Expert (30×20, 150).
- **Custom boards:** Choose 4–40 columns, 4–20 rows, and your goose count.
- **Personal bests:** Your fastest clear of each difficulty is saved locally.
- **Mouse and keyboard controls:** Play either way, with optional animations and a resizable interface that supports Retina displays.
- **Sound effects:** Soft reveal and flag cues, a win chime, and a goose honk. Toggle sound on or off at any time.
- **Pause and resume:** Pausing or switching windows hides the board and stops the timer.

## Getting Started

On macOS, install [raylib](https://www.raylib.com/) and `pkg-config` with Homebrew, then build and play:

```sh
brew install raylib pkg-config
make
./minesweeper
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
| Toggle sound | S, or the Sound button |

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
