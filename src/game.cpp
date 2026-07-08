#include "game.h"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

const std::string kReset = "\033[0m";
const std::string kBrightRed = "\033[1;91m";
const std::string kGreen = "\033[32m";
const std::string kYellow = "\033[33m";
const std::string kBrightYellow = "\033[93m";
const std::string kBlue = "\033[34m";
const std::string kCyan = "\033[36m";
const std::string kGrey = "\033[90m";
const std::string kBold = "\033[1m";

// Classic Minesweeper number colours, indexed by cell value 1-8.
const char *kValueColours[] = {
    "",        "\033[94m", "\033[32m", "\033[91m", "\033[34m",
    "\033[31m", "\033[36m", "\033[35m", "\033[90m",
};

std::string cell_string(const Board &board, std::size_t x, std::size_t y) {
  if (board.is_hidden(x, y)) {
    if (board.is_marked(x, y)) {
      return kBrightYellow + "M" + kReset;
    }
    return kGrey + "#" + kReset;
  }
  const int value = board.value(x, y);
  if (value == Board::goose_value) {
    return kBrightRed + "G" + kReset;
  }
  if (value == 0) {
    return kGrey + "." + kReset;
  }
  return kValueColours[value] + std::to_string(value) + kReset;
}

} // namespace

void Game::run() {
  std::cout << '\n'
            << kBold << "===========================================\n"
            << "   GeeseSpotter - Minesweeper, UW Edition\n"
            << "===========================================" << kReset << '\n'
            << "Clear the field without disturbing a goose!\n";

  new_game();
  std::string line;
  while (!quit_) {
    std::cout << '\n'
              << kGrey << "(s x y = show, m x y = mark, n = new game, h = help, q = quit)"
              << kReset << "\n> ";
    if (!std::getline(std::cin, line)) {
      break;
    }
    handle_command(line);
  }
  std::cout << kCyan << "\nThanks for playing GeeseSpotter!" << kReset << '\n';
}

void Game::new_game() {
  timer_started_ = false;
  std::cout << '\n'
            << kBold << "Choose a difficulty:" << kReset << '\n'
            << "  1) Beginner ( 6 x 6,    5 geese)\n"
            << "  2) Easy     ( 9 x 9,   10 geese)\n"
            << "  3) Medium   (16 x 16,  40 geese)\n"
            << "  4) Hard     (30 x 16,  99 geese)\n"
            << "  5) Expert   (30 x 20, 150 geese)\n"
            << "  6) Custom\n"
            << "  q) Quit\n";

  while (true) {
    std::cout << "> ";
    std::string line;
    if (!std::getline(std::cin, line)) {
      quit_ = true;
      return;
    }
    std::istringstream iss(line);
    std::string choice;
    iss >> choice;
    if (choice == "1") {
      board_.emplace(6, 6, 5);
    } else if (choice == "2") {
      board_.emplace(9, 9, 10);
    } else if (choice == "3") {
      board_.emplace(16, 16, 40);
    } else if (choice == "4") {
      board_.emplace(30, 16, 99);
    } else if (choice == "5") {
      board_.emplace(30, 20, 150);
    } else if (choice == "6") {
      const auto width = prompt_number("Board width (1-60): ", 1, Board::max_width);
      if (!width) {
        quit_ = true;
        return;
      }
      const auto height = prompt_number("Board height (1-20): ", 1, Board::max_height);
      if (!height) {
        quit_ = true;
        return;
      }
      const long long max_geese = *width * *height - 1;
      const auto geese =
          prompt_number("Number of geese (0-" + std::to_string(max_geese) + "): ", 0, max_geese);
      if (!geese) {
        quit_ = true;
        return;
      }
      board_.emplace(static_cast<std::size_t>(*width), static_cast<std::size_t>(*height),
                     static_cast<unsigned int>(*geese));
    } else if (choice == "q" || choice == "Q") {
      quit_ = true;
      return;
    } else {
      std::cout << kYellow << "Please enter 1-6, or q to quit." << kReset << '\n';
      continue;
    }
    break;
  }

  print_board();
}

std::optional<long long> Game::prompt_number(const std::string &prompt, long long min,
                                             long long max) {
  while (true) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
      return std::nullopt;
    }
    std::istringstream iss(line);
    long long value = 0;
    if (iss >> value && value >= min && value <= max) {
      return value;
    }
    std::cout << kYellow << "Please enter a number between " << min << " and " << max << "."
              << kReset << '\n';
  }
}

void Game::handle_command(const std::string &line) {
  std::istringstream iss(line);
  std::string command;
  if (!(iss >> command)) {
    return;
  }

  const char action = static_cast<char>(std::tolower(static_cast<unsigned char>(command[0])));
  if (command.size() == 1 && (action == 's' || action == 'm')) {
    long long x = 0, y = 0;
    if (!(iss >> x >> y) || x < 0 || y < 0 || static_cast<std::size_t>(x) >= board_->width() ||
        static_cast<std::size_t>(y) >= board_->height()) {
      std::cout << kYellow << "Usage: " << action << " <x> <y>  with x in 0-"
                << board_->width() - 1 << " and y in 0-" << board_->height() - 1 << "." << kReset
                << '\n';
      return;
    }
    if (action == 's') {
      do_show(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
    } else {
      do_mark(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
    }
  } else if (command == "n" || command == "N") {
    new_game();
  } else if (command == "q" || command == "Q") {
    quit_ = true;
  } else if (command == "h" || command == "H" || command == "?") {
    print_help();
  } else {
    std::cout << kYellow << "Unknown command. Type h for help." << kReset << '\n';
  }
}

void Game::do_show(std::size_t x, std::size_t y) {
  const RevealResult result = board_->reveal(x, y);
  if (!timer_started_ && (result == RevealResult::Revealed || result == RevealResult::Goose)) {
    timer_started_ = true;
    start_time_ = std::chrono::steady_clock::now();
  }

  switch (result) {
  case RevealResult::Marked:
    std::cout << kYellow << "That cell is marked. Unmark it first (m " << x << " " << y << ")."
              << kReset << '\n';
    break;
  case RevealResult::AlreadyRevealed:
    std::cout << kYellow << "That cell is already revealed." << kReset << '\n';
    break;
  case RevealResult::Goose:
    board_->reveal_all_geese();
    print_board();
    std::cout << kBrightRed << "        _\n"
              << "    >(o)__   HONK! You disturbed a goose!\n"
              << "     (___/   Game over after " << elapsed_string() << "." << kReset << '\n';
    new_game();
    break;
  case RevealResult::Revealed:
    print_board();
    if (board_->is_won()) {
      std::cout << kGreen << kBold << "Congratulations! You cleared the field in "
                << elapsed_string() << "!" << kReset << '\n';
      new_game();
    }
    break;
  }
}

void Game::do_mark(std::size_t x, std::size_t y) {
  if (board_->toggle_mark(x, y) == MarkResult::AlreadyRevealed) {
    std::cout << kYellow << "That cell is already revealed, so it cannot be marked." << kReset
              << '\n';
    return;
  }
  print_board();
}

void Game::print_board() const {
  const Board &board = *board_;

  std::cout << "\n    ";
  for (std::size_t x = 0; x < board.width(); ++x) {
    std::cout << std::setw(2) << x << ' ';
  }
  std::cout << '\n';

  for (std::size_t y = 0; y < board.height(); ++y) {
    std::cout << std::setw(3) << y << ' ';
    for (std::size_t x = 0; x < board.width(); ++x) {
      std::cout << ' ' << cell_string(board, x, y) << ' ';
    }
    std::cout << '\n';
  }

  const long long geese_left =
      static_cast<long long>(board.num_geese()) - static_cast<long long>(board.num_marked());
  std::cout << '\n' << kBlue << "Geese left: " << geese_left << kReset;
  if (timer_started_) {
    std::cout << kGrey << "   |   Time: " << elapsed_string() << kReset;
  }
  std::cout << '\n';
}

void Game::print_help() const {
  std::cout << '\n'
            << kBold << "Commands:" << kReset << '\n'
            << "  s <x> <y>   reveal the cell at column x, row y\n"
            << "  m <x> <y>   mark/unmark a cell as a suspected goose\n"
            << "  n           start a new game\n"
            << "  h           show this help\n"
            << "  q           quit\n"
            << kBold << "Symbols:" << kReset << "  " << kGrey << "#" << kReset << " hidden   "
            << kGrey << "." << kReset << " empty   " << kValueColours[1] << "1" << kReset << "-"
            << kValueColours[8] << "8" << kReset << " adjacent geese   " << kBrightYellow << "M"
            << kReset << " marked   " << kBrightRed << "G" << kReset << " goose\n"
            << "Your first reveal is always safe. Good luck!\n";
}

std::string Game::elapsed_string() const {
  if (!timer_started_) {
    return "0s";
  }
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::steady_clock::now() - start_time_)
                           .count();
  if (seconds >= 60) {
    return std::to_string(seconds / 60) + "m " + std::to_string(seconds % 60) + "s";
  }
  return std::to_string(seconds) + "s";
}
