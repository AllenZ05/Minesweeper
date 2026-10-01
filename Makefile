CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2

SRC := src/board.cpp src/game.cpp src/ui.cpp src/gui_main.cpp
HEADERS := src/board.h src/game.h src/ui.h
BIN := mineseeker

all: $(BIN)

$(BIN): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(shell pkg-config --cflags --libs raylib)

test: build/board_test build/game_test
	./build/board_test
	./build/game_test

build/board_test: tests/board_test.cpp src/board.cpp src/board.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc tests/board_test.cpp src/board.cpp -o $@

build/game_test: tests/game_test.cpp src/game.cpp src/board.cpp src/game.h src/board.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc tests/game_test.cpp src/game.cpp src/board.cpp -o $@

build/ui_test: tests/ui_test.cpp src/ui.cpp src/game.cpp src/board.cpp $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc tests/ui_test.cpp src/ui.cpp src/game.cpp src/board.cpp -o $@ $(shell pkg-config --cflags --libs raylib)

test-ui: build/ui_test
	./build/ui_test

clean:
	rm -rf $(BIN) build

.PHONY: all test test-ui clean
