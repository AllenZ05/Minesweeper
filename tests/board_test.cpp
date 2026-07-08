// Assert-based unit tests for the Board logic. Run with `make test`.
#include "board.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

namespace {

bool throws_invalid_argument(std::size_t w, std::size_t h, unsigned int geese) {
  try {
    Board board(w, h, geese);
  } catch (const std::invalid_argument &) {
    return true;
  }
  return false;
}

void test_construction_validation() {
  assert(throws_invalid_argument(0, 5, 0));
  assert(throws_invalid_argument(5, 0, 0));
  assert(throws_invalid_argument(Board::max_width + 1, 5, 0));
  assert(throws_invalid_argument(5, Board::max_height + 1, 0));
  assert(throws_invalid_argument(3, 3, 9)); // geese must be < cells
  Board ok(3, 3, 8);
  assert(ok.width() == 3 && ok.height() == 3 && ok.num_geese() == 8);
}

void test_first_reveal_is_always_safe() {
  // 10 geese in 12 cells: without first-click safety a random reveal
  // would hit a goose most of the time.
  for (int i = 0; i < 200; ++i) {
    Board board(4, 3, 10);
    assert(board.reveal(1, 1) != RevealResult::Goose);
  }
}

void test_full_board_of_geese_wins_on_first_reveal() {
  // With geese == cells - 1, placement is forced: everything except
  // the first-revealed cell. That cell must count all 8 neighbours.
  Board board(3, 3, 8);
  assert(!board.is_won());
  assert(board.reveal(1, 1) == RevealResult::Revealed);
  assert(board.value(1, 1) == 8);
  assert(board.is_won());
}

void test_neighbour_count_on_forced_placement() {
  Board board(2, 1, 1);
  assert(board.reveal(0, 0) == RevealResult::Revealed);
  assert(board.value(0, 0) == 1);
  assert(board.value(1, 0) == Board::goose_value);
  assert(board.is_won());
}

void test_flood_fill_reveals_empty_board() {
  Board board(5, 4, 0);
  assert(board.reveal(2, 2) == RevealResult::Revealed);
  for (std::size_t y = 0; y < board.height(); ++y) {
    for (std::size_t x = 0; x < board.width(); ++x) {
      assert(!board.is_hidden(x, y));
      assert(board.value(x, y) == 0);
    }
  }
  assert(board.is_won());
}

void test_flood_fill_skips_marked_cells() {
  Board board(5, 4, 0);
  assert(board.toggle_mark(0, 0) == MarkResult::Toggled);
  assert(board.reveal(2, 2) == RevealResult::Revealed);
  assert(board.is_hidden(0, 0));
  assert(!board.is_won()); // a non-goose cell is still hidden
  assert(board.toggle_mark(0, 0) == MarkResult::Toggled);
  assert(board.reveal(0, 0) == RevealResult::Revealed);
  assert(board.is_won());
}

void test_mark_and_reveal_rules() {
  Board board(3, 3, 1);
  assert(board.toggle_mark(0, 0) == MarkResult::Toggled);
  assert(board.is_marked(0, 0));
  assert(board.num_marked() == 1);
  assert(board.reveal(0, 0) == RevealResult::Marked); // marked cells can't be revealed
  assert(board.toggle_mark(0, 0) == MarkResult::Toggled);
  assert(!board.is_marked(0, 0));
  assert(board.num_marked() == 0);

  assert(board.reveal(1, 1) == RevealResult::Revealed);
  assert(board.reveal(1, 1) == RevealResult::AlreadyRevealed);
  assert(board.toggle_mark(1, 1) == MarkResult::AlreadyRevealed);
}

void test_losing_reveals_all_geese() {
  Board board(3, 3, 7);
  assert(board.reveal(1, 1) == RevealResult::Revealed);
  // 7 geese among the 8 remaining cells: find the goose and step on it.
  int geese_seen = 0;
  for (std::size_t y = 0; y < 3; ++y) {
    for (std::size_t x = 0; x < 3; ++x) {
      if (board.value(x, y) == Board::goose_value) {
        ++geese_seen;
        assert(board.reveal(x, y) == RevealResult::Goose);
        break;
      }
    }
    if (geese_seen > 0) {
      break;
    }
  }
  assert(geese_seen == 1);
  board.reveal_all_geese();
  int visible_geese = 0;
  for (std::size_t y = 0; y < 3; ++y) {
    for (std::size_t x = 0; x < 3; ++x) {
      if (board.value(x, y) == Board::goose_value) {
        assert(!board.is_hidden(x, y));
        ++visible_geese;
      }
    }
  }
  assert(visible_geese == 7);
}

void test_max_board_flood_fill() {
  // Largest allowed board with no geese: the old recursive flood fill
  // could overflow the stack here; the iterative one must not.
  Board board(Board::max_width, Board::max_height, 0);
  assert(board.reveal(0, 0) == RevealResult::Revealed);
  assert(board.is_won());
}

} // namespace

int main() {
  test_construction_validation();
  test_first_reveal_is_always_safe();
  test_full_board_of_geese_wins_on_first_reveal();
  test_neighbour_count_on_forced_placement();
  test_flood_fill_reveals_empty_board();
  test_flood_fill_skips_marked_cells();
  test_mark_and_reveal_rules();
  test_losing_reveals_all_geese();
  test_max_board_flood_fill();
  std::cout << "All board tests passed.\n";
  return 0;
}
