// Clickable GUI front end for GeeseSpotter, built with raylib.
// Reuses the same Board logic as the terminal version.
#include "board.h"
#include "raylib.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int kCell = 36;
constexpr int kHud = 56;
constexpr int kMenuWidth = 460;
constexpr int kMenuHeight = 640;
constexpr int kMinWindowWidth = 340;
constexpr int kMinCustomSize = 4;
constexpr int kMaxCustomWidth = 40; // Board allows 60, but wider wouldn't fit on screen
constexpr int kMaxCustomHeight = static_cast<int>(Board::max_height);

struct Preset {
  const char *name;
  std::size_t width;
  std::size_t height;
  unsigned int geese;
};

constexpr Preset kPresets[] = {
    {"Beginner", 6, 6, 5},   {"Easy", 9, 9, 10},      {"Medium", 16, 16, 40},
    {"Hard", 30, 16, 99},    {"Expert", 30, 20, 150},
};

enum class Phase { Menu, Custom, Playing, Won, Lost };

// Best clear per preset, in seconds (0 = none yet), kept in a dotfile so
// they survive restarts. Custom games are not tracked.
std::string best_times_path() {
  const char *home = std::getenv("HOME");
  return home ? std::string(home) + "/.geesespotter_best" : std::string(".geesespotter_best");
}

std::array<double, 5> load_best_times() {
  std::array<double, 5> best{};
  std::ifstream file(best_times_path());
  std::string name;
  double seconds = 0.0;
  while (file >> name >> seconds) {
    for (int i = 0; i < 5; ++i) {
      if (name == kPresets[i].name && seconds > 0.0) {
        best[i] = seconds;
      }
    }
  }
  return best;
}

void save_best_times(const std::array<double, 5> &best) {
  std::ofstream file(best_times_path());
  for (int i = 0; i < 5; ++i) {
    if (best[i] > 0.0) {
      file << kPresets[i].name << ' ' << best[i] << '\n';
    }
  }
}

const Color kBackground = {235, 237, 241, 255};
const Color kHudBar = {45, 55, 72, 255};
const Color kHiddenTile = {156, 168, 186, 255};
const Color kHiddenHover = {177, 189, 207, 255};
const Color kRevealedTile = {226, 229, 234, 255};
const Color kGridLine = {120, 131, 148, 255};
const Color kFlagRed = {211, 47, 47, 255};
const Color kWinGreen = {102, 187, 106, 255};
const Color kNumberColors[9] = {
    BLANK,
    {25, 118, 210, 255},  // 1 blue
    {56, 142, 60, 255},   // 2 green
    {211, 47, 47, 255},   // 3 red
    {21, 45, 155, 255},   // 4 dark blue
    {123, 31, 27, 255},   // 5 dark red
    {0, 131, 143, 255},   // 6 teal
    {123, 31, 162, 255},  // 7 purple
    {84, 90, 99, 255},    // 8 grey
};

struct App {
  Phase phase = Phase::Menu;
  std::optional<Board> board;
  Preset preset = kPresets[1];
  int preset_index = 1; // index into kPresets, -1 for custom games
  std::array<double, 5> best_times = load_best_times();
  bool new_best = false;
  int custom_width = 16;
  int custom_height = 12;
  int custom_geese = 40;
  double stepper_next_fire = 0.0; // hold-to-repeat for the +/- buttons
  int stepper_fires = 0;
  int editing_row = -1; // custom screen: row whose value is being typed
  std::string edit_buffer;
  bool timer_started = false;
  double start_time = 0.0;
  double final_time = 0.0;
  bool paused = false;
  double paused_elapsed = 0.0; // run time frozen while paused
  bool press_on_board = false; // left button went down on a cell
  std::vector<std::pair<int, int>> boom_cells; // geese that ended the game
};

bool is_boom_cell(const App &app, std::size_t x, std::size_t y) {
  return std::find(app.boom_cells.begin(), app.boom_cells.end(),
                   std::pair{static_cast<int>(x), static_cast<int>(y)}) != app.boom_cells.end();
}

void draw_text_centered(const char *text, float cx, float cy, int size, Color color) {
  const int width = MeasureText(text, size);
  DrawText(text, static_cast<int>(cx) - width / 2, static_cast<int>(cy) - size / 2, size, color);
}

bool button(Rectangle rect, const char *label, int font_size, const char *sublabel = nullptr) {
  const bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
  DrawRectangleRounded(rect, 0.25f, 8, hover ? kHiddenHover : kHiddenTile);
  const float cx = rect.x + rect.width / 2;
  const float cy = rect.y + rect.height / 2;
  if (sublabel != nullptr) {
    draw_text_centered(label, cx, cy - 8, font_size, WHITE);
    draw_text_centered(sublabel, cx, cy + 12, 14, Fade(WHITE, 0.75f));
  } else {
    draw_text_centered(label, cx, cy, font_size, WHITE);
  }
  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Stepper button: fires once on click, then repeats while held. One shared
// timer is enough since only one button can be held at a time.
bool repeat_button(App &app, Rectangle rect, const char *label) {
  const bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
  DrawRectangleRounded(rect, 0.25f, 8, hover ? kHiddenHover : kHiddenTile);
  draw_text_centered(label, rect.x + rect.width / 2, rect.y + rect.height / 2, 20, WHITE);
  if (!hover || !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    return false;
  }
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    app.stepper_next_fire = GetTime() + 0.4;
    app.stepper_fires = 0;
    return true;
  }
  if (GetTime() >= app.stepper_next_fire) {
    ++app.stepper_fires; // speed up the longer the button is held
    app.stepper_next_fire = GetTime() + (app.stepper_fires > 20 ? 0.015 : 0.06);
    return true;
  }
  return false;
}

void draw_flag(float cx, float cy) {
  const Color pole = {60, 66, 78, 255};
  DrawLineEx({cx - 2, cy - 9}, {cx - 2, cy + 9}, 2.0f, pole);
  DrawLineEx({cx - 7, cy + 9}, {cx + 4, cy + 9}, 2.0f, pole);
  // Triangle vertices must be counter-clockwise for raylib to render them.
  DrawTriangle({cx - 2, cy - 9}, {cx - 2, cy - 1}, {cx + 8, cy - 5}, kFlagRed);
}

void draw_cross(float cx, float cy) {
  const Color dark = {40, 44, 52, 255};
  DrawLineEx({cx - 9, cy - 9}, {cx + 9, cy + 9}, 3.0f, dark);
  DrawLineEx({cx - 9, cy + 9}, {cx + 9, cy - 9}, 3.0f, dark);
}

void draw_goose(float cx, float cy) {
  const Color body_brown = {121, 101, 79, 255};
  DrawEllipse(static_cast<int>(cx) + 3, static_cast<int>(cy) + 4, 9.0f, 6.0f, body_brown);
  DrawLineEx({cx - 2, cy + 3}, {cx - 6, cy - 7}, 4.0f, BLACK); // neck
  DrawCircle(static_cast<int>(cx) - 6, static_cast<int>(cy) - 8, 4.0f, BLACK); // head
  DrawTriangle({cx - 9, cy - 10}, {cx - 14, cy - 8}, {cx - 9, cy - 6}, ORANGE); // beak
  DrawCircle(static_cast<int>(cx) - 5, static_cast<int>(cy) - 5, 1.5f, WHITE); // chin patch
  DrawCircle(static_cast<int>(cx) - 7, static_cast<int>(cy) - 9, 1.0f, WHITE); // eye
}

void draw_cell(const App &app, std::size_t x, std::size_t y, int px, int py, bool hovered,
               bool pressed) {
  const Board &board = *app.board;
  const Rectangle rect = {static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(kCell), static_cast<float>(kCell)};
  const float cx = px + kCell / 2.0f;
  const float cy = py + kCell / 2.0f;

  if (board.is_hidden(x, y)) {
    if (pressed) {
      // Held down: show as a flat empty tile, classic Minesweeper style.
      DrawRectangleRec(rect, kRevealedTile);
      DrawRectangleLinesEx(rect, 1.0f, Fade(kGridLine, 0.35f));
    } else {
      DrawRectangleRec(rect, hovered && app.phase == Phase::Playing ? kHiddenHover : kHiddenTile);
      DrawRectangleLinesEx(rect, 1.0f, kGridLine);
    }
    if (board.is_marked(x, y)) {
      draw_flag(cx, cy);
      // A flag on a safe cell was wrong: cross it out once the game is lost.
      if (app.phase == Phase::Lost && board.value(x, y) != Board::goose_value) {
        draw_cross(cx, cy);
      }
    }
    return;
  }

  const int value = board.value(x, y);
  const bool boom = is_boom_cell(app, x, y);
  DrawRectangleRec(rect, boom ? Fade(kFlagRed, 0.45f) : kRevealedTile);
  DrawRectangleLinesEx(rect, 1.0f, Fade(kGridLine, 0.35f));
  if (value == Board::goose_value) {
    draw_goose(cx, cy);
  } else if (value > 0) {
    draw_text_centered(TextFormat("%d", value), cx, cy, 22, kNumberColors[value]);
  }
}

double current_elapsed(const App &app) {
  if (!app.timer_started) {
    return 0.0;
  }
  if (app.phase != Phase::Playing) {
    return app.final_time;
  }
  return app.paused ? app.paused_elapsed : GetTime() - app.start_time;
}

void set_paused(App &app, bool paused) {
  if (app.paused == paused) {
    return;
  }
  app.paused = paused;
  if (app.timer_started) {
    if (paused) {
      app.paused_elapsed = GetTime() - app.start_time;
    } else {
      app.start_time = GetTime() - app.paused_elapsed; // resume where we left off
    }
  }
}

const char *format_seconds(int seconds) {
  if (seconds >= 60) {
    return TextFormat("%dm %02ds", seconds / 60, seconds % 60);
  }
  return TextFormat("%ds", seconds);
}

const char *time_text(const App &app) {
  return format_seconds(static_cast<int>(current_elapsed(app)));
}

void start_game(App &app, const Preset &preset, int preset_index) {
  app.preset = preset;
  app.preset_index = preset_index;
  app.board.emplace(preset.width, preset.height, preset.geese);
  app.phase = Phase::Playing;
  app.timer_started = false;
  app.final_time = 0.0;
  app.new_best = false;
  app.paused = false;
  app.paused_elapsed = 0.0;
  app.press_on_board = false;
  app.boom_cells.clear();
  const int board_width = static_cast<int>(preset.width) * kCell;
  SetWindowSize(std::max(board_width, kMinWindowWidth),
                static_cast<int>(preset.height) * kCell + kHud);
}

void go_to_menu(App &app) {
  app.phase = Phase::Menu;
  app.board.reset();
  app.press_on_board = false;
  SetWindowSize(kMenuWidth, kMenuHeight);
}

void update_menu(App &app) {
  draw_text_centered("GeeseSpotter", kMenuWidth / 2.0f, 64, 44, kHudBar);
  draw_text_centered("Minesweeper - UW Edition", kMenuWidth / 2.0f, 104, 20,
                     Fade(kHudBar, 0.65f));

  for (int i = 0; i < 5; ++i) {
    const Preset &preset = kPresets[i];
    const Rectangle rect = {80, 150.0f + i * 64, 300, 48};
    const char *label = TextFormat("%s  (%dx%d, %d geese)", preset.name,
                                   static_cast<int>(preset.width),
                                   static_cast<int>(preset.height), preset.geese);
    const char *best =
        app.best_times[i] > 0.0
            ? TextFormat("Best: %s", format_seconds(static_cast<int>(app.best_times[i])))
            : nullptr;
    if (button(rect, label, 18, best)) {
      start_game(app, preset, i);
      return;
    }
  }
  const char *custom_sub = TextFormat("%dx%d, %d geese", app.custom_width, app.custom_height,
                                      app.custom_geese);
  if (button({80, 150.0f + 5 * 64, 300, 48}, "Custom", 18, custom_sub)) {
    app.phase = Phase::Custom;
    return;
  }

  draw_text_centered("Left-click: reveal    Right-click: flag", kMenuWidth / 2.0f, 548, 18,
                     Fade(kHudBar, 0.65f));
  draw_text_centered("Click a satisfied number to reveal its neighbours", kMenuWidth / 2.0f, 574,
                     16, Fade(kHudBar, 0.5f));
  draw_text_centered("R: restart    P: pause    Esc: menu", kMenuWidth / 2.0f, 598, 16,
                     Fade(kHudBar, 0.5f));
}

void update_custom(App &app) {
  struct Row {
    const char *label;
    int *value;
    int min;
    int max;
  };
  const int max_geese = app.custom_width * app.custom_height - 9;
  const Row rows[] = {
      {"Width", &app.custom_width, kMinCustomSize, kMaxCustomWidth},
      {"Height", &app.custom_height, kMinCustomSize, kMaxCustomHeight},
      {"Geese", &app.custom_geese, 1, max_geese},
  };
  const auto value_rect = [](int i) { return Rectangle{280, 150.0f + i * 70, 60, 40}; };

  const auto commit_edit = [&] {
    if (app.editing_row >= 0 && !app.edit_buffer.empty()) {
      const Row &row = rows[app.editing_row];
      *row.value = std::clamp(std::stoi(app.edit_buffer), row.min, row.max);
    }
    app.editing_row = -1;
    app.edit_buffer.clear();
  };

  if (IsKeyPressed(KEY_ESCAPE)) {
    if (app.editing_row >= 0) { // first Esc cancels the edit, second leaves
      app.editing_row = -1;
      app.edit_buffer.clear();
    } else {
      app.phase = Phase::Menu;
      return;
    }
  }
  if (app.editing_row >= 0) {
    for (int ch = GetCharPressed(); ch != 0; ch = GetCharPressed()) {
      if (ch >= '0' && ch <= '9' && app.edit_buffer.size() < 3) {
        app.edit_buffer.push_back(static_cast<char>(ch));
      }
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !app.edit_buffer.empty()) {
      app.edit_buffer.pop_back();
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
      commit_edit();
    }
    // Clicking anywhere else applies the typed value.
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(GetMousePosition(), value_rect(app.editing_row))) {
      commit_edit();
    }
  }

  draw_text_centered("Custom Game", kMenuWidth / 2.0f, 64, 40, kHudBar);

  for (int i = 0; i < 3; ++i) {
    const Row &row = rows[i];
    const float row_y = 150.0f + i * 70;
    DrawText(row.label, 80, static_cast<int>(row_y) + 10, 20, kHudBar);
    if (repeat_button(app, {240, row_y, 40, 40}, "-")) {
      *row.value = std::max(*row.value - 1, row.min);
    }

    const Rectangle vrect = value_rect(i);
    const bool editing = app.editing_row == i;
    DrawRectangleRounded(vrect, 0.25f, 8, editing ? WHITE : Fade(WHITE, 0.55f));
    DrawRectangleRoundedLinesEx(vrect, 0.25f, 8, 1.0f, Fade(kGridLine, editing ? 1.0f : 0.6f));
    const float vx = vrect.x + vrect.width / 2;
    const float vy = vrect.y + vrect.height / 2;
    if (editing) {
      const bool blink = static_cast<int>(GetTime() * 2.5) % 2 == 0;
      draw_text_centered(TextFormat("%s%s", app.edit_buffer.c_str(), blink ? "_" : " "), vx, vy,
                         22, kHudBar);
      DrawText(TextFormat("%d to %d", row.min, row.max), 396, static_cast<int>(row_y) + 13, 14,
               Fade(kHudBar, 0.55f));
    } else {
      draw_text_centered(TextFormat("%d", *row.value), vx, vy, 22, kHudBar);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          CheckCollisionPointRec(GetMousePosition(), vrect)) {
        app.editing_row = i;
        app.edit_buffer.clear();
      }
    }

    if (repeat_button(app, {340, row_y, 40, 40}, "+")) {
      *row.value = std::min(*row.value + 1, row.max);
    }
  }
  // Shrinking the board can leave too many geese: keep the count in range.
  // The cap leaves the 3x3 around the first click clear, so every custom
  // game keeps full first-reveal safety.
  app.custom_geese = std::clamp(app.custom_geese, 1, max_geese);

  if (button({80, 420, 300, 48}, "Start", 18)) {
    // A width or height typed just now can shrink the goose cap this frame.
    app.custom_geese = std::clamp(app.custom_geese, 1, app.custom_width * app.custom_height - 9);
    const Preset custom{"Custom", static_cast<std::size_t>(app.custom_width),
                        static_cast<std::size_t>(app.custom_height),
                        static_cast<unsigned int>(app.custom_geese)};
    start_game(app, custom, -1);
    return;
  }
  if (button({80, 484, 300, 48}, "Back", 18)) {
    app.editing_row = -1;
    app.edit_buffer.clear();
    app.phase = Phase::Menu;
    return;
  }
  draw_text_centered("Click a value to type it in", kMenuWidth / 2.0f, 566, 16,
                     Fade(kHudBar, 0.5f));
}

// After a losing chord on (x, y): every goose it revealed sits next to it.
void collect_chord_booms(App &app, std::size_t x, std::size_t y) {
  const Board &board = *app.board;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      const long long nx = static_cast<long long>(x) + dx;
      const long long ny = static_cast<long long>(y) + dy;
      if (nx < 0 || ny < 0 || nx >= static_cast<long long>(board.width()) ||
          ny >= static_cast<long long>(board.height())) {
        continue;
      }
      const auto ux = static_cast<std::size_t>(nx);
      const auto uy = static_cast<std::size_t>(ny);
      if (!board.is_hidden(ux, uy) && board.value(ux, uy) == Board::goose_value) {
        app.boom_cells.emplace_back(static_cast<int>(ux), static_cast<int>(uy));
      }
    }
  }
}

void handle_board_click(App &app, std::size_t x, std::size_t y) {
  Board &board = *app.board;
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    board.toggle_mark(x, y);
    return;
  }
  // Reveals fire on release (so holding the button previews the press), and
  // only when the press also started on the board.
  if (!IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || !app.press_on_board) {
    return;
  }

  // Hidden cell: reveal it. Revealed number: try to chord.
  const bool was_hidden = board.is_hidden(x, y);
  const RevealResult result = was_hidden ? board.reveal(x, y) : board.chord(x, y);
  if (!app.timer_started && (result == RevealResult::Revealed || result == RevealResult::Goose)) {
    app.timer_started = true;
    app.start_time = GetTime();
  }
  if (result == RevealResult::Goose) {
    app.final_time = GetTime() - app.start_time;
    if (was_hidden) {
      app.boom_cells.emplace_back(static_cast<int>(x), static_cast<int>(y));
    } else {
      collect_chord_booms(app, x, y);
    }
    board.reveal_unmarked_geese();
    app.phase = Phase::Lost;
  } else if (result == RevealResult::Revealed && board.is_won()) {
    app.final_time = app.timer_started ? GetTime() - app.start_time : 0.0;
    board.mark_all_geese();
    if (app.preset_index >= 0) {
      double &best = app.best_times[app.preset_index];
      if (best <= 0.0 || app.final_time < best) {
        best = app.final_time;
        save_best_times(app.best_times);
        app.new_best = true;
      }
    }
    app.phase = Phase::Won;
  }
}

void update_game(App &app) {
  if (IsKeyPressed(KEY_R)) {
    start_game(app, app.preset, app.preset_index);
    return;
  }
  if (IsKeyPressed(KEY_ESCAPE)) {
    go_to_menu(app);
    return;
  }
  if (app.phase == Phase::Playing) {
    if (IsKeyPressed(KEY_P)) {
      set_paused(app, !app.paused);
    }
    // Losing focus pauses too, so tabbing away neither burns the clock nor
    // grants free planning time.
    if (!app.paused && app.timer_started && !IsWindowFocused()) {
      set_paused(app, true);
    }
  }

  const Board &board = *app.board;
  const int window_width = GetScreenWidth();
  const int offset_x = (window_width - static_cast<int>(board.width()) * kCell) / 2;
  const int offset_y = kHud;

  // Which cell is the mouse over?
  const Vector2 mouse = GetMousePosition();
  const int mx = static_cast<int>(mouse.x) - offset_x;
  const int my = static_cast<int>(mouse.y) - offset_y;
  int hover_x = -1;
  int hover_y = -1;
  if (mx >= 0 && my >= 0 && mx / kCell < static_cast<int>(board.width()) &&
      my / kCell < static_cast<int>(board.height())) {
    hover_x = mx / kCell;
    hover_y = my / kCell;
  }

  if (app.phase == Phase::Playing && !app.paused && hover_x >= 0) {
    handle_board_click(app, static_cast<std::size_t>(hover_x), static_cast<std::size_t>(hover_y));
  }
  // Track whether the press started on the board: without this, clicking a
  // menu button would reveal whichever cell ends up under the cursor once
  // the window resizes to the board.
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    app.press_on_board = app.phase == Phase::Playing && !app.paused && hover_x >= 0;
  } else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    app.press_on_board = false;
  }

  if (app.paused) {
    // Cover the board so pausing can't be used to plan ahead.
    const int window_height = GetScreenHeight();
    DrawRectangle(0, kHud, window_width, window_height - kHud, kHiddenTile);
    const float mid_y = kHud + (window_height - kHud) / 2.0f;
    draw_text_centered("Paused", window_width / 2.0f, mid_y - 12, 32, kHudBar);
    draw_text_centered("P to resume", window_width / 2.0f, mid_y + 20, 16,
                       Fade(kHudBar, 0.7f));
  } else {
    // While the left button is held, preview the press: the target cell shows
    // as revealed, or all of a number's hidden neighbours for a chord.
    const bool press_active = app.phase == Phase::Playing && app.press_on_board &&
                              hover_x >= 0 && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const bool press_is_chord =
        press_active && !board.is_hidden(static_cast<std::size_t>(hover_x),
                                         static_cast<std::size_t>(hover_y));

    for (std::size_t y = 0; y < board.height(); ++y) {
      for (std::size_t x = 0; x < board.width(); ++x) {
        const bool hovered =
            static_cast<int>(x) == hover_x && static_cast<int>(y) == hover_y;
        bool pressed = false;
        if (press_active && board.is_hidden(x, y) && !board.is_marked(x, y)) {
          const int dx = static_cast<int>(x) - hover_x;
          const int dy = static_cast<int>(y) - hover_y;
          pressed = press_is_chord ? (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) : hovered;
        }
        draw_cell(app, x, y, offset_x + static_cast<int>(x) * kCell,
                  offset_y + static_cast<int>(y) * kCell, hovered, pressed);
      }
    }
  }

  // HUD bar
  DrawRectangle(0, 0, window_width, kHud, kHudBar);
  const Rectangle menu_button = {window_width - 78.0f, 12, 66, 32};
  if (app.phase == Phase::Playing) {
    const long long geese_left = static_cast<long long>(board.num_geese()) -
                                 static_cast<long long>(board.num_marked());
    DrawText(TextFormat("Geese: %lld", geese_left), 14, 18, 20, RAYWHITE);
    const Rectangle pause_button = {window_width - 150.0f, 12, 64, 32};
    const char *time = time_text(app);
    DrawText(time, static_cast<int>(pause_button.x) - 10 - MeasureText(time, 20), 18, 20,
             Fade(RAYWHITE, 0.7f));
    if (button(pause_button, app.paused ? "Resume" : "Pause", 14)) {
      set_paused(app, !app.paused);
    }
    if (button(menu_button, "Menu", 16)) {
      go_to_menu(app);
    }
    return;
  }

  // Game over: message on the left, Again/Menu buttons on the right
  if (app.phase == Phase::Won) {
    DrawText(TextFormat("Cleared in %s!", time_text(app)), 14, 10, 18, kWinGreen);
    if (app.new_best) {
      DrawText("New best!", 14, 32, 14, kWinGreen);
    } else if (app.preset_index >= 0 && app.best_times[app.preset_index] > 0.0) {
      DrawText(TextFormat("Best: %s",
                          format_seconds(static_cast<int>(app.best_times[app.preset_index]))),
               14, 32, 14, Fade(RAYWHITE, 0.6f));
    }
  } else {
    DrawText("HONK! Game over", 14, 19, 18, {255, 138, 128, 255});
  }
  const Rectangle again_button = {window_width - 150.0f, 12, 64, 32};
  if (button(again_button, "Again", 16)) {
    start_game(app, app.preset, app.preset_index);
    return;
  }
  if (button(menu_button, "Menu", 16)) {
    go_to_menu(app);
  }
}

} // namespace

int main() {
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  InitWindow(kMenuWidth, kMenuHeight, "GeeseSpotter");
  SetTargetFPS(60);
  SetExitKey(KEY_NULL); // only quit via the window close button

  App app;
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(kBackground);
    if (app.phase == Phase::Menu) {
      update_menu(app);
    } else if (app.phase == Phase::Custom) {
      update_custom(app);
    } else {
      update_game(app);
    }
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
