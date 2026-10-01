#include "game.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

std::filesystem::path best_times_path() {
  // Preserve records from the original GeeseSpotter release.
  const char *home = std::getenv("HOME");
  return home ? std::filesystem::path(home) / ".geesespotter_best" : ".geesespotter_best";
}

Game::Game(std::filesystem::path records_path) : records_path_(std::move(records_path)) {
  std::ifstream file(records_path_);
  std::string line;
  while (std::getline(file, line)) {
    std::istringstream record(line);
    std::string name;
    double seconds;
    if (!(record >> name >> seconds) || !std::isfinite(seconds) || seconds <= 0 || seconds > 1e9) {
      continue;
    }
    for (std::size_t i = 0; i < kPresets.size(); ++i) {
      if (name == kPresets[i].name) best_times[i] = seconds;
    }
  }
}

bool Game::save_records() {
  // Write beside the destination, then rename, so an interrupted write never
  // truncates the user's previous records. Failed writes remain visible in UI.
  auto temporary = records_path_;
  temporary += ".tmp." + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  std::ofstream file(temporary);
  file << std::setprecision(12);
  for (std::size_t i = 0; i < kPresets.size(); ++i) {
    if (best_times[i] > 0) file << kPresets[i].name << ' ' << best_times[i] << '\n';
  }
  file.close();
  std::error_code error;
  if (file) std::filesystem::rename(temporary, records_path_, error);
  records_saved = static_cast<bool>(file) && !error;
  if (!records_saved) std::filesystem::remove(temporary, error);
  return records_saved;
}

void Game::start(const Preset &choice, int index) {
  preset = choice;
  preset_index = index;
  board.emplace(preset.width, preset.height, preset.geese);
  phase = Phase::Playing;
  timer_started = paused = new_best = false;
  start_time_ = paused_elapsed_ = final_time_ = 0;
  boom_cells.clear();
}

void Game::menu() {
  phase = Phase::Menu;
  board.reset();
  paused = false;
}

double Game::elapsed(double now) const {
  if (!timer_started) return 0;
  if (phase != Phase::Playing) return final_time_;
  return paused ? paused_elapsed_ : std::max(0.0, now - start_time_);
}

void Game::pause(bool value, double now) {
  if (phase != Phase::Playing || value == paused) return;
  if (value) paused_elapsed_ = elapsed(now);
  else if (timer_started) start_time_ = now - paused_elapsed_;
  paused = value;
}

GameEvent Game::flag(std::size_t x, std::size_t y) {
  if (phase != Phase::Playing || paused || !board) return GameEvent::None;
  return board->toggle_mark(x, y) == MarkResult::Toggled ? GameEvent::Flagged : GameEvent::None;
}

GameEvent Game::reveal(std::size_t x, std::size_t y, double now) {
  if (phase != Phase::Playing || paused || !board) return GameEvent::None;
  const auto result = board->is_hidden(x, y) ? board->reveal(x, y) : board->chord(x, y);
  if (result != RevealResult::Revealed && result != RevealResult::Goose) return GameEvent::None;
  if (!timer_started) {
    timer_started = true;
    start_time_ = now;
  }
  if (result == RevealResult::Goose) {
    for (std::size_t by = 0; by < board->height(); ++by) {
      for (std::size_t bx = 0; bx < board->width(); ++bx) {
        if (!board->is_hidden(bx, by) && board->value(bx, by) == Board::goose_value)
          boom_cells.emplace_back(bx, by);
      }
    }
    final_time_ = elapsed(now);
    board->reveal_unmarked_geese();
    phase = Phase::Lost;
    return GameEvent::Lost;
  } else if (board->is_won()) {
    final_time_ = elapsed(now);
    board->mark_all_geese();
    phase = Phase::Won;
    if (preset_index >= 0 && preset_index < static_cast<int>(best_times.size())) {
      double &best = best_times[preset_index];
      const double recorded = std::max(0.001, final_time_);
      if (best == 0 || recorded < best) {
        best = recorded;
        new_best = true;
        save_records();
      }
    }
    return GameEvent::Won;
  }
  return GameEvent::Revealed;
}
