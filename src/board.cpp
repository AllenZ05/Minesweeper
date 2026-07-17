#include "board.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <utility>

namespace {

constexpr int kNeighbourOffsets[8][2] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0},
                                         {1, 0},   {-1, 1}, {0, 1},  {1, 1}};

std::size_t validated_size(std::size_t width, std::size_t height, unsigned int num_geese) {
  if (width == 0 || width > Board::max_width || height == 0 || height > Board::max_height) {
    throw std::invalid_argument("Board dimensions out of range");
  }
  if (num_geese >= width * height) {
    throw std::invalid_argument("Number of geese must be less than the number of cells");
  }
  return width * height;
}

bool adjacent_or_same(std::size_t ax, std::size_t ay, std::size_t bx, std::size_t by) {
  return std::max(ax, bx) - std::min(ax, bx) <= 1 && std::max(ay, by) - std::min(ay, by) <= 1;
}

template <typename Fn>
void for_each_neighbour(std::size_t x, std::size_t y, std::size_t width, std::size_t height,
                        Fn &&fn) {
  for (const auto &offset : kNeighbourOffsets) {
    const long long nx = static_cast<long long>(x) + offset[0];
    const long long ny = static_cast<long long>(y) + offset[1];
    if (nx >= 0 && ny >= 0 && nx < static_cast<long long>(width) &&
        ny < static_cast<long long>(height)) {
      fn(static_cast<std::size_t>(nx), static_cast<std::size_t>(ny));
    }
  }
}

} // namespace

Board::Board(std::size_t width, std::size_t height, unsigned int num_geese)
    : width_(width), height_(height), num_geese_(num_geese),
      cells_(validated_size(width, height, num_geese)) {}

unsigned int Board::num_marked() const {
  return static_cast<unsigned int>(
      std::count_if(cells_.begin(), cells_.end(), [](const Cell &c) { return c.marked; }));
}

RevealResult Board::reveal(std::size_t x, std::size_t y) {
  Cell &cell = cells_[index(x, y)];
  if (cell.marked) {
    return RevealResult::Marked;
  }
  if (!cell.hidden) {
    return RevealResult::AlreadyRevealed;
  }

  if (!geese_placed_) {
    place_geese(x, y);
  }

  if (cell.value == goose_value) {
    cell.hidden = false;
    return RevealResult::Goose;
  }
  flood_reveal(x, y);
  return RevealResult::Revealed;
}

RevealResult Board::chord(std::size_t x, std::size_t y) {
  const Cell &cell = cells_[index(x, y)];
  if (cell.hidden || cell.value == 0 || cell.value == goose_value) {
    return RevealResult::AlreadyRevealed;
  }

  int marked_neighbours = 0;
  for_each_neighbour(x, y, width_, height_, [&](std::size_t nx, std::size_t ny) {
    if (cells_[index(nx, ny)].marked) {
      ++marked_neighbours;
    }
  });
  if (marked_neighbours != cell.value) {
    return RevealResult::AlreadyRevealed;
  }

  bool hit_goose = false;
  bool revealed_any = false;
  for_each_neighbour(x, y, width_, height_, [&](std::size_t nx, std::size_t ny) {
    Cell &neighbour = cells_[index(nx, ny)];
    if (!neighbour.hidden || neighbour.marked) {
      return;
    }
    if (neighbour.value == goose_value) {
      neighbour.hidden = false;
      hit_goose = true;
    } else {
      flood_reveal(nx, ny);
    }
    revealed_any = true;
  });
  if (hit_goose) {
    return RevealResult::Goose;
  }
  return revealed_any ? RevealResult::Revealed : RevealResult::AlreadyRevealed;
}

MarkResult Board::toggle_mark(std::size_t x, std::size_t y) {
  Cell &cell = cells_[index(x, y)];
  if (!cell.hidden) {
    return MarkResult::AlreadyRevealed;
  }
  cell.marked = !cell.marked;
  return MarkResult::Toggled;
}

bool Board::is_won() const {
  return std::all_of(cells_.begin(), cells_.end(),
                     [](const Cell &c) { return c.value == goose_value || !c.hidden; });
}

void Board::reveal_unmarked_geese() {
  for (Cell &cell : cells_) {
    if (cell.value == goose_value && !cell.marked) {
      cell.hidden = false;
    }
  }
}

void Board::mark_all_geese() {
  for (Cell &cell : cells_) {
    if (cell.value == goose_value) {
      cell.marked = true;
    }
  }
}

bool Board::is_hidden(std::size_t x, std::size_t y) const { return cells_[index(x, y)].hidden; }

bool Board::is_marked(std::size_t x, std::size_t y) const { return cells_[index(x, y)].marked; }

int Board::value(std::size_t x, std::size_t y) const { return cells_[index(x, y)].value; }

void Board::place_geese(std::size_t safe_x, std::size_t safe_y) {
  geese_placed_ = true;

  auto candidates_for = [&](bool exclude_neighbours) {
    std::vector<std::size_t> out;
    for (std::size_t y = 0; y < height_; ++y) {
      for (std::size_t x = 0; x < width_; ++x) {
        if (x == safe_x && y == safe_y) {
          continue;
        }
        if (exclude_neighbours && adjacent_or_same(x, y, safe_x, safe_y)) {
          continue;
        }
        out.push_back(index(x, y));
      }
    }
    return out;
  };

  // Keep the whole 3x3 around the first click clear when the board has room.
  std::vector<std::size_t> candidates = candidates_for(true);
  if (candidates.size() < num_geese_) {
    candidates = candidates_for(false);
  }

  static std::mt19937 rng{std::random_device{}()};
  std::shuffle(candidates.begin(), candidates.end(), rng);
  for (unsigned int i = 0; i < num_geese_; ++i) {
    cells_[candidates[i]].value = goose_value;
  }
  compute_neighbours();
}

void Board::compute_neighbours() {
  for (std::size_t y = 0; y < height_; ++y) {
    for (std::size_t x = 0; x < width_; ++x) {
      Cell &cell = cells_[index(x, y)];
      if (cell.value == goose_value) {
        continue;
      }
      int count = 0;
      for_each_neighbour(x, y, width_, height_, [&](std::size_t nx, std::size_t ny) {
        if (cells_[index(nx, ny)].value == goose_value) {
          ++count;
        }
      });
      cell.value = count;
    }
  }
}

// Iterative flood fill: reveals the cell and, from empty cells, expands to
// all neighbours. Marked cells are skipped (unmark to reveal them).
void Board::flood_reveal(std::size_t start_x, std::size_t start_y) {
  std::vector<std::pair<std::size_t, std::size_t>> stack{{start_x, start_y}};
  while (!stack.empty()) {
    const auto [x, y] = stack.back();
    stack.pop_back();
    Cell &cell = cells_[index(x, y)];
    if (!cell.hidden || cell.marked) {
      continue;
    }
    cell.hidden = false;
    if (cell.value == 0) {
      for_each_neighbour(x, y, width_, height_, [&](std::size_t nx, std::size_t ny) {
        stack.emplace_back(nx, ny);
      });
    }
  }
}
