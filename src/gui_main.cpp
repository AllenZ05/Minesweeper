// Clickable GUI front end for GeeseSpotter, built with raylib.
// Reuses the same Board logic as the terminal version.
#include "board.h"
#include "raylib.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace {

constexpr int kCell = 36;
constexpr int kHud = 56;
constexpr int kMenuWidth = 460;
constexpr int kMenuHeight = 560;
constexpr int kMinWindowWidth = 340;

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

enum class Phase { Menu, Playing, Won, Lost };

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
  bool timer_started = false;
  double start_time = 0.0;
  double final_time = 0.0;
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

bool button(Rectangle rect, const char *label, int font_size) {
  const bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
  DrawRectangleRounded(rect, 0.25f, 8, hover ? kHiddenHover : kHiddenTile);
  draw_text_centered(label, rect.x + rect.width / 2, rect.y + rect.height / 2, font_size, WHITE);
  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
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
  return app.phase == Phase::Playing ? GetTime() - app.start_time : app.final_time;
}

const char *time_text(const App &app) {
  const int seconds = static_cast<int>(current_elapsed(app));
  if (seconds >= 60) {
    return TextFormat("%dm %02ds", seconds / 60, seconds % 60);
  }
  return TextFormat("%ds", seconds);
}

void start_game(App &app, const Preset &preset) {
  app.preset = preset;
  app.board.emplace(preset.width, preset.height, preset.geese);
  app.phase = Phase::Playing;
  app.timer_started = false;
  app.final_time = 0.0;
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
    if (button(rect, label, 18)) {
      start_game(app, preset);
      return;
    }
  }

  draw_text_centered("Left-click: reveal    Right-click: flag", kMenuWidth / 2.0f, 490, 18,
                     Fade(kHudBar, 0.65f));
  draw_text_centered("Click a satisfied number to reveal its neighbours", kMenuWidth / 2.0f, 516,
                     16, Fade(kHudBar, 0.5f));
  draw_text_centered("R: restart    Esc: menu", kMenuWidth / 2.0f, 540, 16, Fade(kHudBar, 0.5f));
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
    app.phase = Phase::Won;
  }
}

void update_game(App &app) {
  if (IsKeyPressed(KEY_R)) {
    start_game(app, app.preset);
    return;
  }
  if (IsKeyPressed(KEY_ESCAPE)) {
    go_to_menu(app);
    return;
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

  if (app.phase == Phase::Playing && hover_x >= 0) {
    handle_board_click(app, static_cast<std::size_t>(hover_x), static_cast<std::size_t>(hover_y));
  }
  // Track whether the press started on the board: without this, clicking a
  // menu button would reveal whichever cell ends up under the cursor once
  // the window resizes to the board.
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    app.press_on_board = app.phase == Phase::Playing && hover_x >= 0;
  } else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    app.press_on_board = false;
  }

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

  // HUD bar
  DrawRectangle(0, 0, window_width, kHud, kHudBar);
  const Rectangle menu_button = {window_width - 78.0f, 12, 66, 32};
  if (app.phase == Phase::Playing) {
    const long long geese_left = static_cast<long long>(board.num_geese()) -
                                 static_cast<long long>(board.num_marked());
    DrawText(TextFormat("Geese: %lld", geese_left), 14, 18, 20, RAYWHITE);
    const char *time = time_text(app);
    DrawText(time, static_cast<int>(menu_button.x) - 10 - MeasureText(time, 20), 18, 20,
             Fade(RAYWHITE, 0.7f));
    if (button(menu_button, "Menu", 16)) {
      go_to_menu(app);
    }
    return;
  }

  // Game over: message on the left, Again/Menu buttons on the right
  if (app.phase == Phase::Won) {
    DrawText(TextFormat("Cleared in %s!", time_text(app)), 14, 19, 18, kWinGreen);
  } else {
    DrawText("HONK! Game over", 14, 19, 18, {255, 138, 128, 255});
  }
  const Rectangle again_button = {window_width - 150.0f, 12, 64, 32};
  if (button(again_button, "Again", 16)) {
    start_game(app, app.preset);
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
    } else {
      update_game(app);
    }
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
