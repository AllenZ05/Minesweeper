# Minesweeper (GeeseSpotter)

A reimagined version of the classic Minesweeper, with a unique twist inspired by the University of Waterloo and its iconic Canadian Geese. Test your strategy and luck in this familiar game with a fun, local theme — clear all the tiles without disturbing a goose!

## Features

- **Point and Click:** Left-click to reveal, right-click to flag, with hover highlights and hand-drawn flag and goose icons
- **Difficulty Presets:** Five levels, from Beginner (6×6, 5 geese) up to Expert (30×20, 150 geese), with the window sizing itself to fit the board
- **First-Reveal Safety:** Your first reveal is never a goose (and when the board has room, neither are its neighbours), so every game starts fair
- **Goose Counter & Timer:** A HUD tracks how many geese are left to flag and times your run — race your best clear
- **Classic Look:** Traditional Minesweeper number colours, with the goose that got you highlighted in red when you lose

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

- **Main Development:** Nov 2023
- **Major UI Changes and Code Refactoring:** Jan 2024
- **Modernization (C++20 rewrite, presets, first-reveal safety, timer):** Jul 2026
- **Clickable GUI (raylib), replacing the terminal interface:** Jul 2026

## Video Demo (original terminal version)

https://github.com/AllenZ05/Minesweeper/assets/124856383/08858cc9-1799-4653-a58b-e5750406ee09
