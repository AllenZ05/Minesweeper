# Minesweeper (GeeseSpotter)

A reimagined version of the classic Minesweeper, with a unique twist inspired by the University of Waterloo and its iconic Canadian Geese. Test your strategy and luck in this familiar game with a fun, local theme — clear all the tiles without disturbing a goose!

## Features

- **Difficulty Presets:** Five levels, from Beginner (6×6, 5 geese) up to Expert (30×20, 150 geese) — or build a custom board up to 60×20
- **First-Reveal Safety:** Your first reveal is never a goose (and when the board has room, neither are its neighbours), so every game starts fair
- **Goose Counter & Timer:** Track how many geese are left to find and race your best clear time
- **Quick Commands:** Play with single-line commands like `s 3 4` (show) and `m 3 4` (mark) — no more multi-prompt moves
- **Enhanced UI:** Classic Minesweeper number colours, an aligned grid for large boards, and ANSI-coloured feedback throughout

## Tech Stack and Tools used

- **Programming Language:** C++20 (RAII, `std::vector`-backed board, iterative flood fill, `<random>`, `<chrono>`)
- **Build:** `make`, with assert-based unit tests for the board logic

## Getting Started

1. **Clone** the repository
2. **Compile** the code: `make` (or `g++ -std=c++20 src/*.cpp -o geesespotter`)
3. **Start** the game: `./geesespotter`
4. **Run the tests** (optional): `make test`

## Project Structure

- `src/board.h` / `src/board.cpp` — pure game logic (cell state, goose placement, reveal/mark rules)
- `src/game.h` / `src/game.cpp` — terminal UI, input handling, and game loop
- `src/main.cpp` — entry point
- `tests/board_test.cpp` — unit tests for the board logic

## Timeline

- **Main Development:** Nov 2023
- **Major UI Changes and Code Refactoring:** Jan 2024
- **Modernization (C++20 rewrite, presets, first-reveal safety, timer):** Jul 2026

## Video Demo (original version)

https://github.com/AllenZ05/Minesweeper/assets/124856383/08858cc9-1799-4653-a58b-e5750406ee09
