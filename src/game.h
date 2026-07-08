#ifndef GAME_H
#define GAME_H

#include "board.h"

#include <chrono>
#include <optional>
#include <string>

class Game {
public:
  void run();

private:
  void new_game();
  void handle_command(const std::string &line);
  void do_show(std::size_t x, std::size_t y);
  void do_mark(std::size_t x, std::size_t y);
  void print_board() const;
  void print_help() const;
  std::string elapsed_string() const;
  std::optional<long long> prompt_number(const std::string &prompt, long long min, long long max);

  std::optional<Board> board_;
  std::chrono::steady_clock::time_point start_time_{};
  bool timer_started_ = false;
  bool quit_ = false;
};

#endif
