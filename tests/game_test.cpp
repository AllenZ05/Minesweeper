#include "game.h"

#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>

namespace {
bool near(double a, double b) { return std::abs(a - b) < 0.00001; }

void test_clock_and_pause(const std::filesystem::path &path) {
  Game game(path);
  game.start(kPresets[2], 2);
  assert(!game.timer_started && game.elapsed(50) == 0);
  assert(game.flag(0, 0) == GameEvent::Flagged);
  assert(!game.timer_started);
  assert(game.reveal(0, 0, 50) == GameEvent::None); // A marked cell cannot start the timer.
  assert(!game.timer_started);
  game.reveal(8, 8, 100);
  assert(game.timer_started && game.phase == Phase::Playing);
  assert(near(game.elapsed(105), 5));
  game.pause(true, 110);
  assert(game.paused && near(game.elapsed(300), 10));
  assert(game.reveal(0, 1, 300) == GameEvent::None);
  assert(game.board->is_hidden(0, 0));
  const auto marked = game.board->num_marked();
  game.flag(0, 0);
  assert(game.board->num_marked() == marked);
  game.pause(false, 400);
  assert(near(game.elapsed(405), 15));
  game.start(game.preset, game.preset_index); // Restart also supports an aliased preset.
  assert(!game.paused && !game.timer_started && game.elapsed(900) == 0);
}

void test_results_and_saved_records(const std::filesystem::path &path) {
  Game game(path);
  game.start({"Beginner", 3, 3, 1}, 0);
  assert(game.reveal(1, 1, 100) == GameEvent::Revealed);
  assert(game.reveal(1, 1, 101) == GameEvent::None); // Unsatisfied chord is silent.
  for (std::size_t y = 0; y < 3; ++y)
    for (std::size_t x = 0; x < 3; ++x)
      if (game.board->value(x, y) == Board::goose_value) game.flag(x, y);
  assert(game.reveal(1, 1, 112.5) == GameEvent::Won); // A winning chord has only one outcome.
  assert(game.phase == Phase::Won && game.new_best && game.records_saved);
  assert(near(game.elapsed(999), 12.5));
  Game loaded(path);
  assert(near(loaded.best_times[0], 12.5));
  const auto marked = game.board->num_marked();
  game.flag(0, 0);
  assert(game.board->num_marked() == marked);
  game.reveal(0, 0, 1000);
  assert(game.phase == Phase::Won);

  game.start({"Beginner", 3, 3, 1}, 0);
  game.reveal(1, 1, 200);
  std::size_t gx = 0, gy = 0;
  for (std::size_t y = 0; y < 3; ++y)
    for (std::size_t x = 0; x < 3; ++x)
      if (game.board->value(x, y) == Board::goose_value) { gx = x; gy = y; }
  assert(game.reveal(gx, gy, 211) == GameEvent::Lost);
  assert(game.phase == Phase::Lost && !game.new_best);
  assert(game.boom_cells.size() == 1 && game.boom_cells[0] == std::make_pair(gx, gy));
  assert(near(game.elapsed(999), 11));
  assert(near(game.best_times[0], 12.5));
  game.pause(true, 300);
  assert(!game.paused);
  game.menu();
  assert(!game.board && game.phase == Phase::Menu);
}

void test_custom_and_instant_win(const std::filesystem::path &path) {
  Game game(path);
  game.start({"Custom", 2, 1, 1}, -1);
  game.reveal(0, 0, 1);
  assert(game.phase == Phase::Won && !game.new_best);
  assert(!std::filesystem::exists(path));
  game.start({"Beginner", 2, 1, 1}, 0);
  game.reveal(0, 0, 1);
  assert(game.phase == Phase::Won && game.new_best);
  Game loaded(path);
  assert(loaded.best_times[0] > 0); // Zero-duration win is distinct from no record.
}

void test_record_errors_and_recovery(const std::filesystem::path &directory) {
  const auto path = directory / "missing" / "records";
  Game game(path);
  game.best_times[0] = 2.5;
  assert(!game.save_records());
  assert(!game.records_saved && game.best_times[0] == 2.5);
  std::filesystem::create_directory(directory / "missing");
  assert(game.save_records());
  assert(game.records_saved);
  std::ofstream file(path);
  file << "corrupt record\nEasy -2\nBeginner 3.25\nMedium 1e100\nHard 80.5\n";
  file.close();
  Game loaded(path);
  assert(loaded.best_times[0] == 3.25 && loaded.best_times[1] == 0);
  assert(loaded.best_times[2] == 0 && loaded.best_times[3] == 80.5);
  loaded.best_times[0] = 1.5;
  assert(loaded.save_records());
  Game replaced(path);
  assert(replaced.best_times[0] == 1.5 && replaced.best_times[3] == 80.5);
  // A failed rename must leave the previous destination and remove its temp file.
  const auto protected_path = directory / "directory-not-file";
  std::filesystem::create_directory(protected_path);
  Game blocked(protected_path);
  blocked.best_times[0] = 9;
  assert(!blocked.save_records());
  assert(std::filesystem::is_directory(protected_path));
  for (const auto &entry : std::filesystem::directory_iterator(directory))
    assert(entry.path().filename().string().find(".tmp.") == std::string::npos);
}
} // namespace

int main() {
  const auto directory = std::filesystem::temp_directory_path() /
      ("minesweeper-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(directory);
  test_clock_and_pause(directory / "clock");
  test_results_and_saved_records(directory / "records");
  test_custom_and_instant_win(directory / "custom");
  test_record_errors_and_recovery(directory);
  std::filesystem::remove_all(directory);
  std::cout << "All game state and record tests passed.\n";
}
