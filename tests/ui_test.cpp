// Exercises the real controller and renderer with deterministic input.
// Requires a desktop/OpenGL context: `make test-ui`.
#include "ui.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <deque>
#include <filesystem>
#include <iostream>
#include <set>

namespace {
struct Input {
  Vector2 mouse{-100, -100};
  bool pressed = false;
  bool released = false;
  bool down = false;
  bool right = false;
  bool focused = true;
  std::set<int> keys;
  std::deque<int> characters;
} input;
}
// Executable definitions override the shared library's input queries. Rendering
// still uses raylib, so these tests exercise the same layouts and hit targets.
bool IsWindowFocused() { return input.focused; }
Vector2 GetMousePosition() { return input.mouse; }
bool IsMouseButtonPressed(int button) { return button == MOUSE_BUTTON_LEFT ? input.pressed : input.right; }
bool IsMouseButtonReleased(int button) { return button == MOUSE_BUTTON_LEFT && input.released; }
bool IsMouseButtonDown(int button) { return button == MOUSE_BUTTON_LEFT && input.down; }
bool IsKeyPressed(int code) { return input.keys.contains(code); }
bool IsKeyPressedRepeat(int) { return false; }
bool IsKeyDown(int code) { return input.keys.contains(code); }
int GetCharPressed() {
  if (input.characters.empty()) return 0;
  const int ch = input.characters.front(); input.characters.pop_front(); return ch;
}

namespace {
void tick(UI &ui, Game &game) { ui.update(game); input = {}; }
void keypress(UI &ui, Game &game, int key) { input.keys.insert(key); tick(ui, game); }
void click(UI &ui, Game &game, float x, float y) {
  input.mouse = {x, y}; input.pressed = input.down = true; tick(ui, game);
  input.mouse = {x, y}; input.released = true; tick(ui, game);
}
void type(UI &ui, Game &game, const char *value) {
  for (const char *p = value; *p; ++p) input.characters.push_back(*p);
  tick(ui, game);
  keypress(ui, game, KEY_ENTER);
}
void capture(UI &ui, Game &game, const std::filesystem::path &directory, const char *name) {
  for (int i = 0; i < 18; ++i) {
    tick(ui, game);
    BeginDrawing(); ui.draw(game); EndDrawing();
  }
  Image screen = LoadImageFromScreen();
  if (game.phase == Phase::Menu) {
    const Color pixel = GetImageColor(screen, screen.width * 50 / 640, screen.height * 650 / 780);
    assert(pixel.r == 255 && pixel.g == 255 && pixel.b == 255); // Custom button reaches its expected Retina position.
  }
  if (!directory.empty()) {
    assert(ExportImage(screen, (directory / name).string().c_str()));
  }
  UnloadImage(screen);
}
}

int main(int argc, char **argv) {
  SetTraceLogLevel(LOG_WARNING);
  SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
  InitWindow(640, 780, "Mineseeker interface checks");
  SetTargetFPS(60);
  const auto temporary = std::filesystem::temp_directory_path() /
      ("mineseeker-ui-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(temporary);
  const std::filesystem::path screenshots = argc > 1 ? argv[1] : "";
  if (!screenshots.empty()) std::filesystem::create_directories(screenshots);
  {
    Game game(temporary / "records");
    UI ui;
    tick(ui, game);
    capture(ui, game, screenshots, "menu.png");
    // A button only commits when released over the same control.
    input.mouse = {300, 340}; input.pressed = input.down = true; tick(ui, game);
    input.mouse = {15, 15}; input.released = true; tick(ui, game);
    assert(game.phase == Phase::Menu);
    keypress(ui, game, KEY_DOWN);
    keypress(ui, game, KEY_UP);
    keypress(ui, game, KEY_ENTER);
    assert(game.phase == Phase::Playing && game.preset_index == 1);
    assert(!game.timer_started); // Selecting a preset never leaks a board click.
    // The exact right/bottom edge lies outside the board; these must never
    // become out-of-bounds cell coordinates.
    click(ui, game, 500, 216);
    click(ui, game, 160, 556);
    assert(!game.timer_started);
    input.mouse = {160, 216}; input.right = true; tick(ui, game);
    assert(game.board->is_marked(0, 0));
    click(ui, game, 160, 216);
    assert(!game.timer_started);
    input.mouse = {160, 216}; input.right = true; tick(ui, game);
    assert(!game.board->is_marked(0, 0));
    input.mouse = {15, 15}; input.pressed = input.down = true; tick(ui, game);
    input.mouse = {160, 216}; input.released = true; tick(ui, game);
    assert(!game.timer_started); // A press from outside the board cannot reveal.
    click(ui, game, 160, 216);
    assert(game.timer_started && !game.board->is_hidden(0, 0));
    keypress(ui, game, KEY_R);
    keypress(ui, game, KEY_F);
    assert(game.board->is_marked(4, 4));
    keypress(ui, game, KEY_SPACE);
    assert(!game.timer_started);
    keypress(ui, game, KEY_F);
    keypress(ui, game, KEY_SPACE);
    assert(game.timer_started && !game.board->is_hidden(4, 4));
    capture(ui, game, screenshots, "playing.png");
    keypress(ui, game, KEY_P);
    assert(game.paused);
    capture(ui, game, screenshots, "paused.png");
    keypress(ui, game, KEY_P);
    assert(!game.paused);
    keypress(ui, game, KEY_H);
    assert(game.paused);
    capture(ui, game, screenshots, "help.png");
    // The guide separates game rules from keyboard mappings, with tabs that
    // can themselves be reached using the keyboard.
    keypress(ui, game, KEY_TAB);
    keypress(ui, game, KEY_TAB);
    keypress(ui, game, KEY_ENTER);
    capture(ui, game, screenshots, "help-keyboard.png");
    keypress(ui, game, KEY_M);
    keypress(ui, game, KEY_ESCAPE);
    assert(game.phase == Phase::Playing && game.paused);
    keypress(ui, game, KEY_P);
    assert(!game.paused);
    // Winning keeps the board visible and offers keyboard replay.
    for (std::size_t y = 0; y < game.board->height(); ++y)
      for (std::size_t x = 0; x < game.board->width(); ++x)
        if (game.board->value(x, y) != Board::goose_value) game.reveal(x, y, GetTime());
    tick(ui, game);
    assert(game.phase == Phase::Won && game.new_best);
    capture(ui, game, screenshots, "won.png");
    keypress(ui, game, KEY_ENTER);
    assert(game.phase == Phase::Playing && !game.timer_started);
    game.reveal(4, 4, GetTime());
    bool lost = false;
    for (std::size_t y = 0; y < game.board->height() && !lost; ++y)
      for (std::size_t x = 0; x < game.board->width() && !lost; ++x)
        if (game.board->value(x, y) == Board::goose_value) { game.reveal(x, y, GetTime()); lost = true; }
    tick(ui, game);
    assert(game.phase == Phase::Lost);
    capture(ui, game, screenshots, "lost.png");
    keypress(ui, game, KEY_ESCAPE);
    click(ui, game, 320, 651);
    assert(game.phase == Phase::Custom);
    click(ui, game, 459, 272); type(ui, game, "4");
    click(ui, game, 459, 364); type(ui, game, "4");
    click(ui, game, 459, 456); type(ui, game, "999");
    capture(ui, game, screenshots, "custom.png");
    click(ui, game, 320, 651);
    assert(game.board->width() == 4 && game.board->height() == 4 && game.board->num_geese() == 7);
    assert(!game.timer_started);
    capture(ui, game, screenshots, "small.png");
    keypress(ui, game, KEY_SPACE);
    assert(game.phase == Phase::Won && !game.new_best);
    capture(ui, game, screenshots, "small-won.png");
    keypress(ui, game, KEY_ESCAPE);
    click(ui, game, 320, 651);
    click(ui, game, 459, 272); type(ui, game, "40");
    click(ui, game, 459, 364); type(ui, game, "20");
    click(ui, game, 459, 456); type(ui, game, "150");
    click(ui, game, 320, 651);
    assert(game.board->width() == 40 && game.board->height() == 20);
    assert(GetScreenWidth() <= GetMonitorWidth(GetCurrentMonitor()) - 70);
    assert(GetScreenHeight() <= GetMonitorHeight(GetCurrentMonitor()) - 130);
    keypress(ui, game, KEY_SPACE);
    capture(ui, game, screenshots, "wide.png");
    input.focused = false; tick(ui, game);
    assert(game.paused);
    tick(ui, game);
    assert(game.paused); // Regaining focus never resumes unexpectedly.
    keypress(ui, game, KEY_P);
    assert(!game.paused);
    SetWindowSize(700, 560);
    tick(ui, game);
    keypress(ui, game, KEY_RIGHT);
    keypress(ui, game, KEY_RIGHT);
    capture(ui, game, screenshots, "resized.png");
    // Hit testing must follow the same scale and letterbox offset as rendering.
    bool checked = false;
    const float zoom = std::min(GetScreenWidth() / 1664.0f, GetScreenHeight() / 1188.0f);
    for (std::size_t y = 0; y < game.board->height() && !checked; ++y)
      for (std::size_t x = 0; x < game.board->width() && !checked; ++x)
        if (game.board->is_hidden(x, y) && !game.board->is_marked(x, y)) {
          input.mouse = {(GetScreenWidth() - 1664 * zoom) / 2 + (32 + x * 40 + 20) * zoom,
                         (GetScreenHeight() - 1188 * zoom) / 2 + (196 + y * 40 + 20) * zoom};
          input.right = true;
          tick(ui, game);
          assert(game.board->is_marked(x, y));
          checked = true;
        }
    assert(checked);
  }
  CloseWindow();
  std::filesystem::remove_all(temporary);
  std::cout << "All interface interaction and rendering checks passed.\n";
}
