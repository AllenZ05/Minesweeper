# Minesweeper (GeeseSpotter)

A reimagined version of the classic Minesweeper, with a unique twist inspired by the University of Waterloo and its iconic Canadian Geese. Test your strategy and luck in this familiar game with a fun, local theme — clear all the tiles without disturbing a goose!

## Features

- **Point and Click:** Left-click to reveal, right-click to flag, and chord — click a number with the right flags around it to reveal its remaining neighbours — with hover highlights, press-down previews, and hand-drawn flag and goose icons
- **Crisp and Quick:** High-DPI rendering on Retina displays, with keyboard shortcuts (R to restart, P to pause, Esc for the menu)
- **Difficulty Presets:** Five levels, from Beginner (6×6, 5 geese) up to Expert (30×20, 150 geese), plus a Custom mode where you pick the board size and goose count — the window sizes itself to fit the board
- **Best Times:** Your fastest clear of each preset is saved locally and shown in the menu — beat it and the win screen says so
- **First-Reveal Safety:** Your first reveal is never a goose (and when the board has room, neither are its neighbours), so every game starts fair
- **Goose Counter & Timer:** A HUD tracks how many geese are left to flag and times your run — race your best clear. Pausing (or tabbing away) freezes the clock and hides the board, so no sneaky planning
- **Classic Look:** Traditional Minesweeper number colours; losing highlights the geese that got you in red and crosses out misplaced flags, while winning flags every remaining goose

## Tech Stack and Tools used

- **Programming Language:** C++20 (RAII, `std::vector`-backed board, iterative flood fill, `<random>`, `<chrono>`)
- **Graphics:** [raylib](https://www.raylib.com/)
- **Build:** `make`, with assert-based unit tests for the board logic

## Getting Started

1. **Clone** the repository
2. **Install raylib:** `brew install raylib`
3. **Compile:** `make`
4. **Play:** `./geesespotter`

Run the unit tests (optional) with `make test`.

## Project Structure

- `src/board.h` / `src/board.cpp` — pure game logic (cell state, goose placement, reveal/mark rules)
- `src/gui_main.cpp` — raylib front end (menu, rendering, mouse input)
- `tests/board_test.cpp` — unit tests for the board logic

## Timeline

- **Nov 2023 — Main Development:** Full Minesweeper rules (reveal, flag, goose placement, win/loss detection) rendered as a text grid in the terminal and played with typed commands
- **Dec 2023 — Gameplay Improvements:** Added the recursive reveal, so clearing a blank cell opens up the whole empty region like in classic Minesweeper
- **Jan 2024 — Major UI Changes and Code Refactoring:** Restructured the code and overhauled the terminal interface with colours, better board and prompt spacing, and clearer error messages
- **Jul 2026 — Modernization:** Rewrote the core in C++20 with unit tests, replaced the terminal interface with a clickable raylib GUI (difficulty presets, first-reveal safety, goose counter and timer), and added chording, smarter win/loss flag presentation, per-preset best times, and a custom board mode

## Video Demo (original terminal version)

https://github.com/AllenZ05/Minesweeper/assets/124856383/08858cc9-1799-4653-a58b-e5750406ee09
