#ifndef GAME_H
#define GAME_H

#include "board.h"

#include <array>
#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

struct Preset {
  const char *name;
  std::size_t width;
  std::size_t height;
  unsigned int geese;
};

inline constexpr std::array<Preset, 5> kPresets = {{
    {"Beginner", 6, 6, 5}, {"Easy", 9, 9, 10}, {"Medium", 16, 16, 40},
    {"Hard", 30, 16, 99}, {"Expert", 30, 20, 150},
}};

enum class Phase { Menu, Custom, Playing, Won, Lost };
std::filesystem::path best_times_path();

// Game state has no graphics/input dependencies. Callers supply a monotonic clock.
struct Game {
  explicit Game(std::filesystem::path records_path = best_times_path());
  void start(const Preset &choice, int index);
  void menu();
  void reveal(std::size_t x, std::size_t y, double now);
  void flag(std::size_t x, std::size_t y);
  void pause(bool value, double now);
  double elapsed(double now) const;
  bool save_records();

  Phase phase = Phase::Menu;
  std::optional<Board> board;
  Preset preset = kPresets[1];
  int preset_index = 1;
  std::array<double, kPresets.size()> best_times{};
  bool new_best = false;
  bool records_saved = true;
  bool timer_started = false;
  bool paused = false;
  std::vector<std::pair<std::size_t, std::size_t>> boom_cells;

private:
  std::filesystem::path records_path_;
  double start_time_ = 0;
  double paused_elapsed_ = 0;
  double final_time_ = 0;
};

#endif
