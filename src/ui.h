#ifndef UI_H
#define UI_H

#include "game.h"
#include "raylib.h"

#include <string>
#include <vector>

// Presentation and input. All drawing uses logical coordinates, scaled together
// with hit targets when a window is resized or fitted to a smaller monitor.
class UI {
public:
  UI();
  ~UI();
  UI(const UI &) = delete;
  UI &operator=(const UI &) = delete;
  void update(Game &game);
  void draw(const Game &game);

private:
  enum class Kind { Button, Quiet, Preset, Field, Stepper, Board };
  struct Control {
    int id;
    Rectangle rect;
    std::string label;
    Kind kind = Kind::Button;
    bool primary = false;
    bool disabled = false;
  };
  Font regular_{};
  Font bold_{};
  std::vector<Control> controls_;
  Camera2D camera_{};
  Vector2 mouse_{};
  float width_ = 640;
  float height_ = 780;
  Rectangle board_rect_{};
  int hot_ = -1;
  int active_ = -1;
  int focus_ = 1;
  bool keyboard_focus_ = false;
  bool board_press_ = false;
  int cell_x_ = 0;
  int cell_y_ = 0;
  int hover_x_ = -1;
  int hover_y_ = -1;
  bool fitted_ = false;
  bool help_ = false;
  bool help_keys_ = false;
  bool reduced_motion_ = false;
  double next_repeat_ = 0;
  int repeats_ = 0;
  int custom_[3] = {16, 12, 40};
  int editing_ = -1;
  std::string edit_;
  bool select_all_ = false;
  std::vector<bool> hidden_;
  std::vector<bool> marked_;
  std::vector<double> revealed_at_;
  std::vector<double> flagged_at_;
  Phase previous_phase_ = Phase::Menu;
  double phase_at_ = 0;

  void layout(const Game &game);
  void fit_window(const Game &game);
  void build_controls(const Game &game);
  void activate(Game &game, int id);
  void start(Game &game, const Preset &preset, int index);
  void commit_edit();
  void clamp_custom();
  void track_cells(const Game &game);
  const Control *control(int id) const;
  void text(const std::string &label, float x, float y, float size, Color color, bool bold = false) const;
  float measure(const std::string &label, float size, bool bold = false) const;
  void centered(const std::string &label, float cx, float y, float size, Color color, bool bold = false) const;
  void draw_control(const Control &item) const;
  void draw_menu(const Game &game) const;
  void draw_custom() const;
  void draw_game(const Game &game) const;
  void draw_help() const;
};

#endif
