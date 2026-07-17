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
  board.reveal_unmarked_geese();
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

void test_chord_reveals_neighbours_when_flags_match() {
  // 3x3, 1 goose, reveal the centre: placement is forced into the outer
  // ring, so the centre always shows 1.
  Board board(3, 3, 1);
  assert(board.reveal(1, 1) == RevealResult::Revealed);
  assert(board.value(1, 1) == 1);
  assert(board.chord(1, 1) == RevealResult::AlreadyRevealed); // no flags yet
  for (std::size_t y = 0; y < 3; ++y) {
    for (std::size_t x = 0; x < 3; ++x) {
      if (board.value(x, y) == Board::goose_value) {
        assert(board.toggle_mark(x, y) == MarkResult::Toggled);
      }
    }
  }
  assert(board.chord(1, 1) == RevealResult::Revealed); // reveals the 7 safe cells
  assert(board.is_won());
}

void test_chord_on_wrong_flag_hits_goose() {
  Board board(3, 3, 1);
  assert(board.reveal(1, 1) == RevealResult::Revealed);
  // Flag a safe hidden cell instead of the goose, then chord the centre.
  bool flagged = false;
  for (std::size_t y = 0; y < 3 && !flagged; ++y) {
    for (std::size_t x = 0; x < 3 && !flagged; ++x) {
      if (board.is_hidden(x, y) && board.value(x, y) != Board::goose_value) {
        assert(board.toggle_mark(x, y) == MarkResult::Toggled);
        flagged = true;
      }
    }
  }
  assert(flagged);
  assert(board.chord(1, 1) == RevealResult::Goose);
  for (std::size_t y = 0; y < 3; ++y) {
    for (std::size_t x = 0; x < 3; ++x) {
      if (board.value(x, y) == Board::goose_value) {
        assert(!board.is_hidden(x, y)); // the chord stepped on it
      }
    }
  }
}

void test_chord_does_not_apply() {
  Board board(3, 3, 1);
  assert(board.chord(0, 0) == RevealResult::AlreadyRevealed); // hidden cell
  assert(board.reveal(1, 1) == RevealResult::Revealed);
  assert(board.chord(1, 1) == RevealResult::AlreadyRevealed); // flag count mismatch

  Board empty(5, 4, 0);
  assert(empty.reveal(2, 2) == RevealResult::Revealed);
  assert(empty.chord(2, 2) == RevealResult::AlreadyRevealed); // zero cells can't chord
}

void test_win_marks_geese_and_loss_keeps_flags() {
  // mark_all_geese flags the goose left unmarked on a win.
  Board board(2, 1, 1);
  assert(board.reveal(0, 0) == RevealResult::Revealed);
  assert(board.is_won());
  board.mark_all_geese();
  assert(board.is_marked(1, 0));

  // reveal_unmarked_geese leaves a flagged goose hidden.
  Board lost(3, 3, 7);
  assert(lost.reveal(1, 1) == RevealResult::Revealed);
  std::size_t flag_x = 0;
  std::size_t flag_y = 0;
  bool flagged = false;
  for (std::size_t y = 0; y < 3 && !flagged; ++y) {
    for (std::size_t x = 0; x < 3 && !flagged; ++x) {
      if (lost.value(x, y) == Board::goose_value) {
        assert(lost.toggle_mark(x, y) == MarkResult::Toggled);
        flag_x = x;
        flag_y = y;
        flagged = true;
      }
    }
  }
  assert(flagged);
  lost.reveal_unmarked_geese();
  assert(lost.is_hidden(flag_x, flag_y)); // flag stays
  int visible_geese = 0;
  for (std::size_t y = 0; y < 3; ++y) {
    for (std::size_t x = 0; x < 3; ++x) {
      if (lost.value(x, y) == Board::goose_value && !lost.is_hidden(x, y)) {
        ++visible_geese;
      }
    }
  }
  assert(visible_geese == 6);
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
  test_chord_reveals_neighbours_when_flags_match();
  test_chord_on_wrong_flag_hits_goose();
  test_chord_does_not_apply();
  test_win_marks_geese_and_loss_keeps_flags();
  test_max_board_flood_fill();
  std::cout << "All board tests passed.\n";
  return 0;
}
