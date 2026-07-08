// Clickable GUI front end for GeeseSpotter, built with raylib.
// Reuses the same Board logic as the terminal version.
#include "board.h"
#include "raylib.h"

#include <algorithm>
#include <cstddef>
#include <optional>

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
  int boom_x = -1; // cell that ended the game
  int boom_y = -1;
};

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

void draw_goose(float cx, float cy) {
  const Color body_brown = {121, 101, 79, 255};
  DrawEllipse(static_cast<int>(cx) + 3, static_cast<int>(cy) + 4, 9.0f, 6.0f, body_brown);
  DrawLineEx({cx - 2, cy + 3}, {cx - 6, cy - 7}, 4.0f, BLACK); // neck
  DrawCircle(static_cast<int>(cx) - 6, static_cast<int>(cy) - 8, 4.0f, BLACK); // head
  DrawTriangle({cx - 9, cy - 10}, {cx - 14, cy - 8}, {cx - 9, cy - 6}, ORANGE); // beak
  DrawCircle(static_cast<int>(cx) - 5, static_cast<int>(cy) - 5, 1.5f, WHITE); // chin patch
  DrawCircle(static_cast<int>(cx) - 7, static_cast<int>(cy) - 9, 1.0f, WHITE); // eye
}

void draw_cell(const App &app, std::size_t x, std::size_t y, int px, int py, bool hovered) {
  const Board &board = *app.board;
  const Rectangle rect = {static_cast<float>(px), static_cast<float>(py),
                          static_cast<float>(kCell), static_cast<float>(kCell)};
  const float cx = px + kCell / 2.0f;
  const float cy = py + kCell / 2.0f;

  if (board.is_hidden(x, y)) {
    DrawRectangleRec(rect, hovered && app.phase == Phase::Playing ? kHiddenHover : kHiddenTile);
    DrawRectangleLinesEx(rect, 1.0f, kGridLine);
    if (board.is_marked(x, y)) {
      draw_flag(cx, cy);
    }
    return;
  }

  const int value = board.value(x, y);
  const bool boom = static_cast<int>(x) == app.boom_x && static_cast<int>(y) == app.boom_y;
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
  app.boom_x = app.boom_y = -1;
  const int board_width = static_cast<int>(preset.width) * kCell;
  SetWindowSize(std::max(board_width, kMinWindowWidth),
                static_cast<int>(preset.height) * kCell + kHud);
}

void go_to_menu(App &app) {
  app.phase = Phase::Menu;
  app.board.reset();
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

  draw_text_centered("Left-click: reveal    Right-click: flag", kMenuWidth / 2.0f, 512, 18,
                     Fade(kHudBar, 0.65f));
}

void handle_board_click(App &app, std::size_t x, std::size_t y) {
  Board &board = *app.board;
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
    board.toggle_mark(x, y);
    return;
  }
  if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    return;
  }

  const RevealResult result = board.reveal(x, y);
  if (!app.timer_started && (result == RevealResult::Revealed || result == RevealResult::Goose)) {
    app.timer_started = true;
    app.start_time = GetTime();
  }
  if (result == RevealResult::Goose) {
    app.final_time = GetTime() - app.start_time;
    app.boom_x = static_cast<int>(x);
    app.boom_y = static_cast<int>(y);
    board.reveal_all_geese();
    app.phase = Phase::Lost;
  } else if (result == RevealResult::Revealed && board.is_won()) {
    app.final_time = app.timer_started ? GetTime() - app.start_time : 0.0;
    app.phase = Phase::Won;
  }
}

void update_game(App &app) {
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

  for (std::size_t y = 0; y < board.height(); ++y) {
    for (std::size_t x = 0; x < board.width(); ++x) {
      const bool hovered =
          static_cast<int>(x) == hover_x && static_cast<int>(y) == hover_y;
      draw_cell(app, x, y, offset_x + static_cast<int>(x) * kCell,
                offset_y + static_cast<int>(y) * kCell, hovered);
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
  SetConfigFlags(FLAG_VSYNC_HINT);
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
