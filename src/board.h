#ifndef BOARD_H
#define BOARD_H

#include <cstddef>
#include <vector>

enum class RevealResult { Revealed, Goose, AlreadyRevealed, Marked };
enum class MarkResult { Toggled, AlreadyRevealed };

// Pure game logic: cell state, goose placement, reveal/mark rules.
// Rendering and input live in game.cpp.
class Board {
public:
  static constexpr int goose_value = 9;
  static constexpr std::size_t max_width = 60;
  static constexpr std::size_t max_height = 20;

  // Throws std::invalid_argument if dimensions are out of range or
  // num_geese >= width * height (at least one cell must be safe).
  Board(std::size_t width, std::size_t height, unsigned int num_geese);

  std::size_t width() const { return width_; }
  std::size_t height() const { return height_; }
  unsigned int num_geese() const { return num_geese_; }
  unsigned int num_marked() const;

  // Geese are placed lazily on the first reveal, so the first revealed
  // cell is never a goose (nor its neighbours, when the board has room).
  RevealResult reveal(std::size_t x, std::size_t y);
  MarkResult toggle_mark(std::size_t x, std::size_t y);

  bool is_won() const;
  void reveal_all_geese();

  // Coordinates must be on the board.
  bool is_hidden(std::size_t x, std::size_t y) const;
  bool is_marked(std::size_t x, std::size_t y) const;
  int value(std::size_t x, std::size_t y) const;

private:
  struct Cell {
    int value = 0; // 0-8 = adjacent geese, 9 = goose
    bool hidden = true;
    bool marked = false;
  };

  std::size_t index(std::size_t x, std::size_t y) const { return y * width_ + x; }
  void place_geese(std::size_t safe_x, std::size_t safe_y);
  void compute_neighbours();
  void flood_reveal(std::size_t x, std::size_t y);

  std::size_t width_;
  std::size_t height_;
  unsigned int num_geese_;
  bool geese_placed_ = false;
  std::vector<Cell> cells_;
};

#endif
