#include "ui.h"
#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

namespace {
constexpr float kCell = 40;
constexpr float kHelpWidth = 560;
constexpr float kHelpHeight = 620;
constexpr Color kPaper{246, 245, 240, 255};
constexpr Color kInk{35, 43, 39, 255};
constexpr Color kMuted{104, 111, 104, 255};
constexpr Color kLine{222, 223, 213, 255};
constexpr Color kGold{244, 196, 64, 255};
constexpr Color kGoldSoft{253, 244, 208, 255};
constexpr Color kGoldDark{143, 100, 15, 255};
constexpr Color kTile{214, 219, 207, 255};
constexpr Color kTileHover{227, 231, 219, 255};
constexpr Color kOpen{240, 241, 232, 255};
constexpr Color kRed{180, 58, 44, 255};
constexpr Color kGreen{39, 111, 76, 255};
constexpr Color kNumbers[] = {BLANK, {41, 99, 163, 255}, kGreen, kRed,
    {75, 64, 143, 255}, {129, 57, 39, 255}, {15, 114, 122, 255},
    {128, 55, 123, 255}, {76, 85, 80, 255}};

enum Id { Custom = 5, Help, Motion, Menu, Pause, Restart, Replay, Close,
          BoardFocus, Start, Back, Resume, Save, HelpRules, HelpKeys, SoundToggle };

Rectangle inset(Rectangle r, float amount) {
  return {r.x + amount, r.y + amount, r.width - 2 * amount, r.height - 2 * amount};
}
void rounded(Rectangle r, float radius, Color color) {
  DrawRectangleRounded(r, std::min(1.0f, 2 * radius / std::min(r.width, r.height)), 12, color);
}
void outline(Rectangle r, float radius, Color color, float thickness = 1) {
  DrawRectangleRoundedLinesEx(r, std::min(1.0f, 2 * radius / std::min(r.width, r.height)),
                             12, thickness, color);
}
void card(Rectangle r, float radius = 16) {
  rounded({r.x, r.y + 3, r.width, r.height}, radius, {32, 41, 34, 9});
  rounded(r, radius, WHITE);
  outline(r, radius, kLine);
}
Color blend(Color a, Color b, float t) {
  t = std::clamp(t, 0.0f, 1.0f);
  return {static_cast<unsigned char>(a.r + (b.r - a.r) * t),
          static_cast<unsigned char>(a.g + (b.g - a.g) * t),
          static_cast<unsigned char>(a.b + (b.b - a.b) * t), 255};
}
bool key(int code) { return IsKeyPressed(code) || IsKeyPressedRepeat(code); }
std::string clock_text(double seconds) {
  const int value = static_cast<int>(std::clamp(seconds, 0.0, 1e9));
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%02d:%02d", value / 60, value % 60);
  return buffer;
}
void goose(float x, float y, float size) {
  // Draw in a shared coordinate system so the mascot and tiny board geese
  // retain the same silhouette without rounding their positions to pixels.
  rlPushMatrix();
  rlTranslatef(x, y, 0);
  rlScalef(size / 48, size / 48, 1);
  const Color feathers{137, 122, 95, 255};
  const Color wing{110, 98, 77, 255};
  const Color cheek{248, 246, 234, 255};

  // Dark legs and short, flat feet sit behind the body.
  DrawLineEx({1, 16}, {-1, 22}, 2, kInk);
  DrawLineEx({-1, 22}, {-5, 22}, 2, kInk);
  DrawLineEx({12, 16}, {10, 22}, 2, kInk);
  DrawLineEx({10, 22}, {6, 22}, 2, kInk);

  // A tapered S-curve joins the head to the breast instead of two hard bars.
  const Vector2 a{-6, 8}, b{-17, 0}, c{-9, -7}, d{-12, -16};
  Vector2 previous = a;
  for (int i = 0; i <= 24; ++i) {
    const float t = i / 24.0f;
    const float u = 1 - t;
    const Vector2 next{u*u*u*a.x + 3*u*u*t*b.x + 3*u*t*t*c.x + t*t*t*d.x,
                       u*u*u*a.y + 3*u*u*t*b.y + 3*u*t*t*c.y + t*t*t*d.y};
    const float radius = 4.6f - 1.3f * t;
    if (i > 0) DrawLineEx(previous, next, radius * 2, kInk);
    DrawCircleV(next, radius, kInk);
    previous = next;
  }
  DrawTriangle({17, 4}, {19, 12}, {26, 3}, wing);
  DrawEllipse(6, 9, 17, 10, feathers);
  DrawEllipse(9, 7, 11.5f, 6.4f, wing);
  DrawLineEx({4, 10}, {16, 10}, 0.8f, {160, 145, 116, 255});
  DrawLineEx({7, 12}, {16, 12}, 0.8f, {160, 145, 116, 255});

  // Canada geese have dark bills. The pale marking follows the lower cheek
  // and jaw; a dark inset keeps it a crescent, distinct from the small eye.
  DrawEllipse(-21, -17, 5.6f, 1.8f, kInk);
  DrawEllipse(-13, -17, 6.7f, 5.6f, kInk);
  DrawEllipse(-11, -15, 4.1f, 2.9f, cheek);
  DrawEllipse(-13, -18, 5.8f, 3.8f, kInk);
  DrawCircleV({-15.5f, -18.2f}, 1.05f, cheek);
  DrawCircleV({-15.6f, -18.2f}, 0.5f, kInk);
  rlPopMatrix();
}
void flag(float x, float y, float scale = 1) {
  DrawLineEx({x - 4 * scale, y - 11 * scale}, {x - 4 * scale, y + 10 * scale}, 2.3f * scale, kInk);
  DrawLineEx({x - 9 * scale, y + 10 * scale}, {x + 4 * scale, y + 10 * scale}, 2.3f * scale, kInk);
  DrawTriangle({x - 3 * scale, y - 11 * scale}, {x - 3 * scale, y + scale},
               {x + 10 * scale, y - 5 * scale}, kRed);
}
void chevron(float x, float y, Color color) {
  DrawLineEx({x - 3, y - 5}, {x + 2, y}, 1.7f, color);
  DrawLineEx({x + 2, y}, {x - 3, y + 5}, 1.7f, color);
}
Font load_font(const char *name) {
  std::filesystem::path root = GetApplicationDirectory();
  auto path = root / "assets" / "fonts" / name;
  if (!std::filesystem::exists(path)) path = std::filesystem::path("assets/fonts") / name;
  if (!std::filesystem::exists(path)) return GetFontDefault();
  Font font = LoadFontEx(path.string().c_str(), 96, nullptr, 0);
  SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
  return font;
}
} // namespace

UI::UI() : regular_(load_font("Lato-Regular.ttf")), bold_(load_font("Lato-Bold.ttf")) {}
UI::~UI() {
  if (regular_.texture.id != GetFontDefault().texture.id) UnloadFont(regular_);
  if (bold_.texture.id != GetFontDefault().texture.id) UnloadFont(bold_);
}
void UI::text(const std::string &label, float x, float y, float size, Color color, bool bold) const {
  DrawTextEx(bold ? bold_ : regular_, label.c_str(), {x, y}, size, 0, color);
}
float UI::measure(const std::string &label, float size, bool bold) const {
  return MeasureTextEx(bold ? bold_ : regular_, label.c_str(), size, 0).x;
}
void UI::centered(const std::string &label, float cx, float y, float size, Color color, bool bold) const {
  text(label, cx - measure(label, size, bold) / 2, y, size, color, bold);
}

void UI::layout(const Game &game) {
  width_ = 640;
  height_ = 780;
  if (game.board) {
    width_ = std::max(640.0f, game.board->width() * kCell + 64);
    height_ = std::max(680.0f, game.board->height() * kCell + 388);
    board_rect_ = {(width_ - game.board->width() * kCell) / 2, 196,
                   game.board->width() * kCell, game.board->height() * kCell};
  }
  const float zoom = std::min(GetScreenWidth() / width_, GetScreenHeight() / height_);
  camera_ = {{(GetScreenWidth() - width_ * zoom) / 2, (GetScreenHeight() - height_ * zoom) / 2},
             {0, 0}, 0, zoom};
  mouse_ = GetScreenToWorld2D(GetMousePosition(), camera_);
}

void UI::fit_window(const Game &game) {
  layout(game);
  const int monitor = GetCurrentMonitor();
  // Reserve desktop space for the menu bar, title bar and dock. All content and
  // hit targets share the same scale, including the widest custom board.
  const float available_w = std::max(320, GetMonitorWidth(monitor) - 80);
  const float available_h = std::max(320, GetMonitorHeight(monitor) - 140);
  const float scale = std::min({1.0f, available_w / width_, available_h / height_});
  const int w = static_cast<int>(width_ * scale);
  const int h = static_cast<int>(height_ * scale);
  SetWindowMinSize(std::min(w, 480), std::min(h, 420));
  SetWindowSize(w, h);
  const Vector2 origin = GetMonitorPosition(monitor);
  const Vector2 old = GetWindowPosition();
  const int x = static_cast<int>(std::clamp(old.x, origin.x + 20,
      origin.x + GetMonitorWidth(monitor) - w - 20));
  const int y = static_cast<int>(std::clamp(old.y, origin.y + 50,
      origin.y + GetMonitorHeight(monitor) - h - 70));
  SetWindowPosition(x, y);
  layout(game);
}

const UI::Control *UI::control(int id) const {
  for (const auto &item : controls_) if (item.id == id) return &item;
  return nullptr;
}

void UI::build_controls(const Game &game) {
  controls_.clear();
  if (help_) {
    const float x = (width_ - kHelpWidth) / 2;
    const float y = (height_ - kHelpHeight) / 2;
    controls_.push_back({HelpRules, {x + 28, y + 98, 248, 36}, "Mouse & rules", Kind::Button, !help_keys_});
    controls_.push_back({HelpKeys, {x + 284, y + 98, 248, 36}, "Keyboard", Kind::Button, help_keys_});
    controls_.push_back({Close, {x + 28, y + 548, 504, 44}, "Close guide", Kind::Button, true});
    return;
  }
  if (game.phase == Phase::Menu) {
    for (int i = 0; i < static_cast<int>(kPresets.size()); ++i)
      controls_.push_back({i, {40, 246.0f + i * 73, 560, 63}, kPresets[i].name, Kind::Preset});
    controls_.push_back({Custom, {40, 625, 560, 52}, "Custom board", Kind::Button});
  } else if (game.phase == Phase::Custom) {
    controls_.push_back({Back, {508, 38, 92, 38}, "Back", Kind::Quiet});
    for (int row = 0; row < 3; ++row) {
      const float y = 252.0f + row * 92;
      const int maximum = row == 0 ? 40 : row == 1 ? 20 : custom_[0] * custom_[1] - 9;
      const int minimum = row == 2 ? 1 : 4;
      controls_.push_back({30 + row * 3, {366, y, 40, 40}, "-", Kind::Stepper, false, custom_[row] <= minimum});
      controls_.push_back({31 + row * 3, {412, y, 94, 40}, std::to_string(custom_[row]), Kind::Field});
      controls_.push_back({32 + row * 3, {512, y, 40, 40}, "+", Kind::Stepper, false, custom_[row] >= maximum});
    }
    controls_.push_back({Start, {40, 625, 560, 52}, "Start exploring", Kind::Button, true});
  } else {
    if (game.phase == Phase::Playing && !game.paused)
      controls_.push_back({BoardFocus, board_rect_, "", Kind::Board});
    if (game.phase == Phase::Playing)
      controls_.push_back({Pause, {width_ - 282, 32, 78, 40}, game.paused ? "Resume" : "Pause", Kind::Quiet});
    controls_.push_back({Restart, {width_ - 194, 32, 78, 40}, "Restart", Kind::Quiet});
    controls_.push_back({Menu, {width_ - 106, 32, 74, 40}, "Menu", Kind::Quiet});
    if (game.paused)
      controls_.push_back({Resume, {width_ / 2 - 64, board_rect_.y + board_rect_.height / 2 + 26, 128, 40}, "Resume", Kind::Button, true});
    if (game.phase == Phase::Won || game.phase == Phase::Lost)
      controls_.push_back({Replay, {width_ - 192, height_ - 119, 136, 44}, "Play again", Kind::Button, true});
    if (!game.records_saved && game.phase == Phase::Won)
      controls_.push_back({Save, {32, height_ - 44, 210, 30}, "Record not saved. Retry", Kind::Quiet});
  }
  const char *sound_label = !audio_.available() ? "Sound unavailable" : audio_.enabled() ? "Sound on" : "Sound off";
  if (game.phase == Phase::Menu || game.phase == Phase::Custom) {
    controls_.push_back({SoundToggle, {180, 710, 148, 36}, sound_label, Kind::Quiet, false, !audio_.available()});
    controls_.push_back({Motion, {40, 710, 126, 36}, reduced_motion_ ? "Motion off" : "Motion on", Kind::Quiet});
    controls_.push_back({Help, {460, 710, 140, 36}, "How to play", Kind::Quiet});
  } else {
    controls_.push_back({SoundToggle, {width_ - 270, height_ - 44, 130, 30}, sound_label, Kind::Quiet, false, !audio_.available()});
    controls_.push_back({Help, {width_ - 132, height_ - 44, 100, 30}, "Controls", Kind::Quiet});
  }
}

void UI::clamp_custom() {
  custom_[0] = std::clamp(custom_[0], 4, 40);
  custom_[1] = std::clamp(custom_[1], 4, 20);
  custom_[2] = std::clamp(custom_[2], 1, custom_[0] * custom_[1] - 9);
}
void UI::commit_edit() {
  if (editing_ >= 0 && !edit_.empty()) custom_[editing_] = std::stoi(edit_);
  editing_ = -1;
  edit_.clear();
  clamp_custom();
}
void UI::start(Game &game, const Preset &preset, int index) {
  audio_.stop();
  game.start(preset, index);
  cell_x_ = static_cast<int>(preset.width) / 2;
  cell_y_ = static_cast<int>(preset.height) / 2;
  focus_ = BoardFocus;
  active_ = -1;
  board_press_ = false;
  hidden_.clear();
  marked_.clear();
  fit_window(game);
}
void UI::activate(Game &game, int id) {
  const double now = GetTime();
  if (id >= 0 && id < 5) { start(game, kPresets[id], id); return; }
  switch (id) {
  case Custom: game.phase = Phase::Custom; focus_ = 31; break;
  case Back: case Menu:
    audio_.stop();
    commit_edit(); game.menu(); focus_ = 1; board_press_ = false; fit_window(game); break;
  case Start: {
    commit_edit();
    const Preset custom{"Custom", static_cast<std::size_t>(custom_[0]),
        static_cast<std::size_t>(custom_[1]), static_cast<unsigned int>(custom_[2])};
    start(game, custom, -1); break;
  }
  case Pause: case Resume:
    audio_.stop();
    game.pause(!game.paused, now); board_press_ = false;
    focus_ = game.paused ? Resume : BoardFocus; break;
  case Restart: case Replay: start(game, game.preset, game.preset_index); break;
  case Motion: reduced_motion_ = !reduced_motion_; break;
  case SoundToggle: audio_.toggle(); break;
  case Help:
    audio_.stop();
    help_ = true; help_keys_ = false; game.pause(true, now); board_press_ = false; focus_ = Close; break;
  case HelpRules: help_keys_ = false; break;
  case HelpKeys: help_keys_ = true; break;
  case Close: help_ = false; focus_ = game.board ? (game.paused ? Resume : BoardFocus) : Help; break;
  case Save: game.save_records(); break;
  default:
    if (id >= 30 && id <= 38) {
      const int row = (id - 30) / 3;
      const int part = (id - 30) % 3;
      if (part == 1) {
        editing_ = row; edit_ = std::to_string(custom_[row]); select_all_ = true;
      } else {
        custom_[row] += part == 0 ? -1 : 1;
        clamp_custom();
      }
    }
    break;
  }
}

void UI::track_cells(const Game &game) {
  if (!game.board) return;
  const auto count = game.board->width() * game.board->height();
  if (hidden_.size() != count) {
    hidden_.assign(count, true); marked_.assign(count, false);
    revealed_at_.assign(count, -10); flagged_at_.assign(count, -10);
  }
  for (std::size_t y = 0; y < game.board->height(); ++y) {
    for (std::size_t x = 0; x < game.board->width(); ++x) {
      const auto i = y * game.board->width() + x;
      const bool hidden = game.board->is_hidden(x, y);
      const bool marked = game.board->is_marked(x, y);
      if (hidden_[i] && !hidden) revealed_at_[i] = GetTime();
      if (!marked_[i] && marked) flagged_at_[i] = GetTime();
      hidden_[i] = hidden; marked_[i] = marked;
    }
  }
}

void UI::update(Game &game) {
  if (!fitted_) { fit_window(game); fitted_ = true; }
  layout(game);
  if (game.phase == Phase::Playing && !game.paused && !IsWindowFocused()) {
    audio_.stop();
    game.pause(true, GetTime()); board_press_ = false; active_ = -1; focus_ = Resume;
  }
  build_controls(game);
  hot_ = -1;
  for (const auto &item : controls_)
    if (!item.disabled && CheckCollisionPointRec(mouse_, item.rect)) hot_ = item.id;
  hover_x_ = hover_y_ = -1;
  if (game.board && !help_ && CheckCollisionPointRec(mouse_, board_rect_) &&
      mouse_.x < board_rect_.x + board_rect_.width &&
      mouse_.y < board_rect_.y + board_rect_.height) {
    hover_x_ = static_cast<int>((mouse_.x - board_rect_.x) / kCell);
    hover_y_ = static_cast<int>((mouse_.y - board_rect_.y) / kCell);
  }
  if (hot_ == BoardFocus && hover_x_ < 0) hot_ = -1;

  bool consumed = false;
  if (editing_ >= 0) {
    for (int ch = GetCharPressed(); ch != 0; ch = GetCharPressed()) {
      if (ch >= '0' && ch <= '9') {
        if (select_all_) edit_.clear();
        select_all_ = false;
        if (edit_.size() < 3) edit_ += static_cast<char>(ch);
      }
    }
    if (key(KEY_BACKSPACE)) {
      if (select_all_) edit_.clear();
      else if (!edit_.empty()) edit_.pop_back();
      select_all_ = false;
    }
    if (IsKeyPressed(KEY_ESCAPE)) { editing_ = -1; edit_.clear(); consumed = true; }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) { commit_edit(); consumed = true; }
    if (IsKeyPressed(KEY_TAB) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hot_ != 31 + editing_ * 3))
      commit_edit();
  }

  if (!consumed && IsKeyPressed(KEY_ESCAPE)) {
    activate(game, help_ ? Close : game.phase == Phase::Custom ? Back : game.board ? Menu : -1);
    consumed = true;
  }
  if (!consumed && editing_ < 0 && IsKeyPressed(KEY_M)) activate(game, Motion);
  if (!consumed && editing_ < 0 && IsKeyPressed(KEY_S)) activate(game, SoundToggle);
  if (!consumed && editing_ < 0 && !help_) {
    if (IsKeyPressed(KEY_H) || IsKeyPressed(KEY_F1)) { activate(game, Help); consumed = true; }
    else if (game.board && IsKeyPressed(KEY_R)) { activate(game, Restart); consumed = true; }
    else if (game.phase == Phase::Playing && IsKeyPressed(KEY_P)) { activate(game, Pause); consumed = true; }
  }
  build_controls(game);
  if (!consumed && editing_ < 0) {
    const bool menu_arrows = !game.board && (key(KEY_UP) || key(KEY_DOWN));
    if (IsKeyPressed(KEY_TAB) || menu_arrows) {
      std::vector<int> ids;
      for (const auto &item : controls_) if (!item.disabled) ids.push_back(item.id);
      if (!ids.empty()) {
        auto found = std::find(ids.begin(), ids.end(), focus_);
        int index = found == ids.end() ? -1 : static_cast<int>(found - ids.begin());
        const int dir = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) || key(KEY_UP)) ? -1 : 1;
        index = (index + dir + static_cast<int>(ids.size())) % static_cast<int>(ids.size());
        focus_ = ids[index]; keyboard_focus_ = true;
      }
      consumed = true;
    }
    if (!consumed && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) &&
        focus_ != BoardFocus) {
      if (const auto *item = control(focus_); item && !item->disabled) activate(game, focus_);
      keyboard_focus_ = true; consumed = true;
    }
  }

  if (!consumed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    active_ = hot_;
    keyboard_focus_ = false;
    if (hot_ >= 0) focus_ = hot_;
    board_press_ = hot_ == BoardFocus;
    if (const auto *item = control(active_); item && item->kind == Kind::Stepper) {
      activate(game, active_); next_repeat_ = GetTime() + 0.4; repeats_ = 0;
    }
  }
  if (!consumed && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && active_ == hot_) {
    if (const auto *item = control(active_); item && item->kind == Kind::Stepper && !item->disabled && GetTime() >= next_repeat_) {
      activate(game, active_); next_repeat_ = GetTime() + (++repeats_ > 15 ? 0.035 : 0.09);
    }
  }
  if (!consumed && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
    const auto *item = control(active_);
    if (item && !item->disabled && active_ == hot_ && item->kind != Kind::Stepper && item->kind != Kind::Board) {
      activate(game, active_); consumed = true;
    }
    active_ = -1;
  }

  if (!consumed && game.phase == Phase::Playing && !game.paused && !help_) {
    if (hover_x_ >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
      audio_.play(game.flag(hover_x_, hover_y_)); focus_ = BoardFocus; keyboard_focus_ = false;
    }
    if (hover_x_ >= 0 && board_press_ && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      audio_.play(game.reveal(hover_x_, hover_y_, GetTime()));
      cell_x_ = hover_x_; cell_y_ = hover_y_;
    }
    if (focus_ == BoardFocus) {
      const bool moved = key(KEY_LEFT) || key(KEY_RIGHT) || key(KEY_UP) || key(KEY_DOWN);
      if (moved) {
        cell_x_ = std::clamp(cell_x_ + (key(KEY_RIGHT) ? 1 : 0) - (key(KEY_LEFT) ? 1 : 0), 0, static_cast<int>(game.board->width()) - 1);
        cell_y_ = std::clamp(cell_y_ + (key(KEY_DOWN) ? 1 : 0) - (key(KEY_UP) ? 1 : 0), 0, static_cast<int>(game.board->height()) - 1);
        keyboard_focus_ = true;
      }
      if (IsKeyPressed(KEY_F)) { audio_.play(game.flag(cell_x_, cell_y_)); keyboard_focus_ = true; }
      if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
        audio_.play(game.reveal(cell_x_, cell_y_, GetTime())); keyboard_focus_ = true;
      }
    }
  }
  if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) board_press_ = false;
  if (game.phase != previous_phase_) {
    phase_at_ = GetTime();
    if (game.phase == Phase::Won || game.phase == Phase::Lost) { focus_ = Replay; board_press_ = false; }
    previous_phase_ = game.phase;
  }
  track_cells(game);
  layout(game);
  build_controls(game);
  SetMouseCursor(hot_ >= 0 && hot_ != BoardFocus ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);
}

void UI::draw_control(const Control &item) const {
  if (item.kind == Kind::Board || item.kind == Kind::Preset) return;
  const bool hovered = hot_ == item.id && !item.disabled;
  const bool pressed = hovered && active_ == item.id && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  Rectangle r = item.rect;
  if (pressed) r.y += 1;
  const bool editing = item.kind == Kind::Field && editing_ == (item.id - 31) / 3;
  Color background = item.primary ? kGold : WHITE;
  if (item.kind == Kind::Quiet) background = hovered ? kLine : BLANK;
  else if (item.kind == Kind::Stepper) background = hovered ? kGoldSoft : kPaper;
  else if (hovered) background = item.primary ? Color{249, 206, 89, 255} : kGoldSoft;
  if (pressed) background = item.primary ? Color{228, 177, 39, 255} : kLine;
  if (editing) background = kGoldSoft;
  rounded(r, item.kind == Kind::Field ? 8 : 10, background);
  if (item.kind != Kind::Quiet) outline(r, item.kind == Kind::Field ? 8 : 10, editing ? kGoldDark : item.primary ? kGold : kLine);
  if (keyboard_focus_ && focus_ == item.id && !item.disabled)
    outline(inset(r, -3), 12, kGoldDark, 2);
  const std::string label = editing ? (edit_.empty() ? " " : edit_) : item.label;
  const float size = item.kind == Kind::Field ? 22 : item.kind == Kind::Stepper ? 23 : item.kind == Kind::Quiet ? 15 : 17;
  const float x = r.x + (r.width - measure(label, size, item.kind != Kind::Quiet)) / 2;
  if (editing && select_all_) rounded({x - 3, r.y + 8, measure(label, size, true) + 6, 25}, 3, {235, 209, 124, 255});
  text(label, x, r.y + (r.height - size) / 2 - 1, size, item.disabled ? Color{145, 150, 141, 255} : kInk, item.kind != Kind::Quiet);
  if (editing && !select_all_ && static_cast<int>(GetTime() * 2) % 2 == 0)
    DrawLineEx({x + measure(label, size, true) + 2, r.y + 9}, {x + measure(label, size, true) + 2, r.y + 30}, 1.3f, kInk);
}

void UI::draw_menu(const Game &game) const {
  rounded({40, 36, 94, 26}, 13, kGold);
  centered("UW EDITION", 87, 42, 12, kInk, true);
  text("Minesweeper", 40, 89, 49, kInk, true);
  text("A little strategy. A lot of geese.", 42, 150, 19, kMuted);
  rounded({478, 86, 122, 102}, 26, kGoldSoft);
  DrawCircle(577, 106, 5, kGold);
  DrawCircle(494, 169, 3, kGold);
  goose(539, 135, 82);
  text("CHOOSE YOUR CHALLENGE", 42, 216, 12, kMuted, true);
  text("PERSONAL BEST", 495, 216, 12, kMuted, true);
  for (int i = 0; i < static_cast<int>(kPresets.size()); ++i) {
    const auto *item = control(i);
    // The help modal replaces the live controls, but the menu remains visible.
    const Rectangle r = {40, 246.0f + i * 73, 560, 63};
    const bool hovered = item && hot_ == i;
    const bool focused = item && keyboard_focus_ && focus_ == i;
    const bool pressed = hovered && active_ == i && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    rounded({r.x, r.y + 2, r.width, r.height}, 12, {35, 43, 39, 7});
    rounded(r, 12, pressed ? Color{245, 231, 182, 255} : hovered || focused ? kGoldSoft : WHITE);
    outline(r, 12, hovered || focused ? kGoldDark : kLine, focused ? 2 : 1);
    rounded({57, r.y + 13, 40, 37}, 9, hovered || focused ? Color{243, 214, 130, 255} : kPaper);
    for (int bar = 0; bar < 5; ++bar) {
      const float h = 5.0f + bar * 3;
      rounded({65.0f + bar * 5, r.y + 38 - h, 3, h}, 1, bar <= i ? kGoldDark : kLine);
    }
    text(kPresets[i].name, 112, r.y + 11, 19, kInk, true);
    const std::string detail = std::to_string(kPresets[i].width) + " x " + std::to_string(kPresets[i].height) + "   /   " + std::to_string(kPresets[i].geese) + " geese";
    text(detail, 112, r.y + 36, 14, kMuted);
    const std::string best = game.best_times[i] > 0 ? clock_text(game.best_times[i]) : "--:--";
    text(best, 554 - measure(best, 18, true), r.y + 23, 18, game.best_times[i] > 0 ? kInk : kMuted, true);
    chevron(575, r.y + 32, kMuted);
  }
}

void UI::draw_custom() const {
  rounded({40, 36, 94, 26}, 13, kGold);
  centered("YOUR RULES", 87, 42, 12, kInk, true);
  text("Make it your own.", 40, 101, 39, kInk, true);
  text("Find your perfect balance of calm and chaos.", 42, 153, 18, kMuted);
  const char *labels[] = {"Columns", "Rows", "Geese"};
  const std::string details[] = {"4 to 40 tiles across", "4 to 20 tiles down",
      "1 to " + std::to_string(custom_[0] * custom_[1] - 9) + " hidden geese"};
  for (int i = 0; i < 3; ++i) {
    const float y = 232.0f + i * 92;
    card({40, y, 560, 80}, 12);
    text(labels[i], 60, y + 17, 20, kInk, true);
    text(details[i], 60, y + 46, 14, kMuted);
  }
  rounded({40, 536, 560, 58}, 12, kGoldSoft);
  const int density = static_cast<int>(std::round(100.0 * custom_[2] / (custom_[0] * custom_[1])));
  text(std::to_string(density) + "% geese", 60, 553, 20, kInk, true);
  text("Your opening and its neighbours stay safe.", 191, 557, 15, kGoldDark);
}

void UI::draw_game(const Game &game) const {
  const Board &board = *game.board;
  goose(48, 48, 28);
  text("Minesweeper", 70, 29, 24, kInk, true);
  const std::string detail = std::string(game.preset.name) + "  /  " + std::to_string(board.width()) + " x " + std::to_string(board.height());
  text(detail, 72, 59, 13, kMuted);

  const Rectangle stats{32, 98, width_ - 64, 70};
  rounded(stats, 14, kInk);
  const float column = stats.width / 3;
  const long long remaining = static_cast<long long>(board.num_geese()) - board.num_marked();
  const std::string values[] = {std::to_string(remaining), clock_text(game.elapsed(GetTime())),
      game.preset_index >= 0 && game.best_times[game.preset_index] > 0 ? clock_text(game.best_times[game.preset_index]) : "--:--"};
  const char *labels[] = {"GEESE LEFT", "TIME", "PERSONAL BEST"};
  for (int i = 0; i < 3; ++i) {
    const float x = 52 + i * column;
    text(labels[i], x, 111, 11, {186, 198, 185, 255}, true);
    text(values[i], x, 131, 25, i == 0 ? kGold : WHITE, true);
    if (i != 0) DrawLineEx({x - 20, 117}, {x - 20, 150}, 1, {66, 77, 68, 255});
  }

  const Rectangle surround = inset(board_rect_, -9);
  card(surround, 13);
  if (game.paused) {
    rounded(board_rect_, 8, kPaper);
    const float cy = board_rect_.y + board_rect_.height / 2;
    goose(width_ / 2, cy - 53, 37);
    centered("Take your time.", width_ / 2, cy - 21, 23, kInk, true);
    centered("Your game is paused.", width_ / 2, cy + 6, 14, kMuted);
  } else {
    const bool press_active = board_press_ && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && hover_x_ >= 0 && game.phase == Phase::Playing;
    const bool chord = press_active && !board.is_hidden(hover_x_, hover_y_);
    for (std::size_t y = 0; y < board.height(); ++y) {
      for (std::size_t x = 0; x < board.width(); ++x) {
        const auto i = y * board.width() + x;
        const Rectangle r{board_rect_.x + x * kCell + 1, board_rect_.y + y * kCell + 1, kCell - 2, kCell - 2};
        const float cx = r.x + r.width / 2;
        const float cy = r.y + r.height / 2;
        const bool hovered = static_cast<int>(x) == hover_x_ && static_cast<int>(y) == hover_y_ && !help_;
        const bool marked = board.is_marked(x, y);
        const bool hidden = board.is_hidden(x, y);
        const bool preview = press_active && hidden && !marked &&
            (chord ? std::abs(static_cast<int>(x) - hover_x_) <= 1 && std::abs(static_cast<int>(y) - hover_y_) <= 1 : hovered);
        const bool boom = std::find(game.boom_cells.begin(), game.boom_cells.end(), std::pair{x, y}) != game.boom_cells.end();
        if (hidden && !preview) {
          rounded({r.x, r.y + 1, r.width, r.height - 1}, 5, {182, 190, 174, 255});
          rounded({r.x, r.y, r.width, r.height - 2}, 5, hovered && game.phase == Phase::Playing ? kTileHover : kTile);
          DrawLineEx({r.x + 6, r.y + 1}, {r.x + r.width - 6, r.y + 1}, 1, {234, 237, 228, 255});
        } else {
          Color color = boom ? Color{251, 210, 196, 255} : kOpen;
          if (!reduced_motion_ && i < revealed_at_.size() && !boom) {
            const float t = static_cast<float>((GetTime() - revealed_at_[i]) / 0.24);
            color = blend(kGoldSoft, color, t);
          }
          rounded(r, 4, color);
        }
        if (marked) {
          const float t = reduced_motion_ || i >= flagged_at_.size() ? 1 : std::clamp(static_cast<float>((GetTime() - flagged_at_[i]) / 0.16), 0.0f, 1.0f);
          flag(cx, cy, 0.78f + 0.22f * (1 - (1 - t) * (1 - t)));
          if (game.phase == Phase::Lost && board.value(x, y) != Board::goose_value) {
            DrawLineEx({cx - 12, cy - 12}, {cx + 12, cy + 12}, 2, kInk);
            DrawLineEx({cx - 12, cy + 12}, {cx + 12, cy - 12}, 2, kInk);
          }
        } else if (!hidden) {
          const int value = board.value(x, y);
          if (value == Board::goose_value) goose(cx, cy - 1, 29);
          else if (value > 0) centered(std::to_string(value), cx, cy - 14, 25, kNumbers[value], true);
        }
        if (keyboard_focus_ && focus_ == BoardFocus && static_cast<int>(x) == cell_x_ && static_cast<int>(y) == cell_y_ && game.phase == Phase::Playing)
          outline(inset(r, 1), 4, kGoldDark, 2.5f);
        else if (hovered && game.phase == Phase::Playing) outline(inset(r, 1), 4, Fade(kGoldDark, 0.6f), 1.5f);
      }
    }
  }

  const float result_y = height_ - 152;
  if (game.phase == Phase::Won || game.phase == Phase::Lost) {
    const bool won = game.phase == Phase::Won;
    const float t = reduced_motion_ ? 1 : std::clamp(static_cast<float>((GetTime() - phase_at_) / 0.18), 0.0f, 1.0f);
    rounded({32, result_y, width_ - 64, 104}, 14, Fade(won ? kGoldSoft : Color{242, 228, 219, 255}, t));
    text(won ? "Campus cleared." : "The geese win this round.", 52, result_y + 18, 25, Fade(kInk, t), true);
    std::string detail_text;
    if (won) detail_text = game.new_best ? "A new personal best. Nicely done!" : "Every safe tile found. Nicely done!";
    else detail_text = "One more try? Your next opening is safe.";
    text(detail_text, 52, result_y + 54, 15, Fade(won ? kGoldDark : kRed, t));
  } else {
    const std::string heading = game.paused ? "A breather between moves." : !game.timer_started ? "Make your first move." : "Leave no safe tile behind.";
    centered(heading, width_ / 2, result_y + 14, 21, kInk, true);
    centered(!game.timer_started ? "Your opening and its neighbours are safe." : "Reveal the clear ground. Flag the geese.", width_ / 2, result_y + 43, 15, kMuted);
    const int safe_total = static_cast<int>(board.width() * board.height()) - board.num_geese();
    int revealed = 0;
    for (std::size_t y = 0; y < board.height(); ++y)
      for (std::size_t x = 0; x < board.width(); ++x)
        if (!board.is_hidden(x, y) && board.value(x, y) != Board::goose_value) ++revealed;
    const float bar_width = std::min(220.0f, width_ - 140);
    rounded({(width_ - bar_width) / 2, result_y + 76, bar_width, 4}, 2, kLine);
    if (revealed > 0) rounded({(width_ - bar_width) / 2, result_y + 76, bar_width * revealed / safe_total, 4}, 2, kGoldDark);
  }
  if (game.records_saved || game.phase != Phase::Won)
    text(keyboard_focus_ ? "Arrows move   /   Space reveals   /   F flags" : "Click to reveal   /   Right-click to flag", 32, height_ - 35, 13, kMuted);
}

void UI::draw_help() const {
  DrawRectangleRec({0, 0, width_, height_}, Fade(kInk, 0.38f));
  const float x = (width_ - kHelpWidth) / 2;
  const float y = (height_ - kHelpHeight) / 2;
  card({x, y, kHelpWidth, kHelpHeight}, 20);
  text("How to play", x + 28, y + 26, 29, kInk, true);
  text("Reveal every tile without a goose to win.", x + 28, y + 65, 17, kMuted);

  if (!help_keys_) {
    text("Left-click: reveal a tile", x + 28, y + 158, 20, kInk, true);
    text("Open a covered tile. Clicking a goose ends the game.", x + 28, y + 187, 16, kMuted);

    rounded({x + 28, y + 226, 40, 40}, 6, kOpen);
    centered("2", x + 48, y + 231, 28, kNumbers[2], true);
    text("A 2 means two geese are in the surrounding tiles,", x + 82, y + 224, 16, kInk);
    text("including the diagonal ones.", x + 82, y + 247, 16, kMuted);

    text("Right-click: place a flag", x + 28, y + 296, 20, kInk, true);
    text("Mark a tile you think hides a goose. Right-click again", x + 28, y + 326, 16, kMuted);
    text("to remove the flag. Flags alone do not win the game.", x + 28, y + 349, 16, kMuted);

    rounded({x + 28, y + 391, 504, 112}, 12, kPaper);
    text("Shortcut: reveal neighbours", x + 44, y + 407, 18, kInk, true);
    text("OPTIONAL", x + 457, y + 411, 11, kMuted, true);
    text("Works on any number (1-8). When its nearby flags match", x + 44, y + 435, 15, kMuted);
    text("its number, left-click it to reveal the unflagged neighbours.", x + 44, y + 456, 15, kMuted);
    text("Wrong flags can still uncover a goose.", x + 44, y + 477, 15, kRed);
    text("Your first reveal and the tiles around it are always safe.", x + 28, y + 516, 14, kMuted);
  } else {
    text("ACTION", x + 28, y + 161, 12, kMuted, true);
    text("KEY", x + 362, y + 161, 12, kMuted, true);
    const char *actions[] = {"Move between tiles", "Reveal a tile / clear neighbours",
        "Place or remove a flag", "Move between controls", "Activate a button",
        "Pause / resume", "Restart the game", "Return to the menu", "Turn animations on / off", "Turn sound on / off"};
    const char *keys[] = {"Arrow keys", "Space / Enter", "F", "Tab / Shift + Tab",
        "Space / Enter", "P", "R", "Esc", "M", "S"};
    for (int i = 0; i < 10; ++i) {
      const float row_y = y + 184 + i * 30;
      text(actions[i], x + 28, row_y + 5, 16, kInk);
      rounded({x + 350, row_y, 182, 27}, 6, kPaper);
      text(keys[i], x + 362, row_y + 6, 14, kInk, true);
    }
    text("Use Tab to focus the board before moving between tiles.", x + 28, y + 498, 14, kMuted);
    const std::string sound_status = !audio_.available() ? "Sound unavailable" : audio_.enabled() ? "Sound on" : "Sound off";
    text(sound_status + "   /   " + (reduced_motion_ ? "Animations off" : "Animations on"), x + 28, y + 520, 14, kMuted);
  }
}
void UI::draw(const Game &game) {
  ClearBackground(kPaper);
  // Preserve BeginDrawing's Retina screen scale. BeginMode2D resets it in
  // raylib 6, which otherwise draws the interface at half size on a 2x display.
  rlPushMatrix();
  rlTranslatef(camera_.offset.x, camera_.offset.y, 0);
  rlScalef(camera_.zoom, camera_.zoom, 1);
  if (game.phase == Phase::Menu) draw_menu(game);
  else if (game.phase == Phase::Custom) draw_custom();
  else draw_game(game);
  if (help_) draw_help();
  for (const auto &item : controls_) draw_control(item);
  rlPopMatrix();
}
