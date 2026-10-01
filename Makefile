CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2

SRC := src/board.cpp src/game.cpp src/audio.cpp src/ui.cpp src/gui_main.cpp
HEADERS := src/board.h src/game.h src/audio.h src/ui.h
BIN := minesweeper

all: $(BIN)

$(BIN): $(SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(shell pkg-config --cflags --libs raylib)

test: build/board_test build/game_test build/audio_test
	./build/board_test
	./build/game_test
	./build/audio_test

build/board_test: tests/board_test.cpp src/board.cpp src/board.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc tests/board_test.cpp src/board.cpp -o $@

build/game_test: tests/game_test.cpp src/game.cpp src/board.cpp src/game.h src/board.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc tests/game_test.cpp src/game.cpp src/board.cpp -o $@

build/audio_test: tests/audio_test.cpp tests/fake_audio.h src/audio.cpp src/audio.h src/game.h src/board.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc $(shell pkg-config --cflags raylib) tests/audio_test.cpp src/audio.cpp -o $@

build/ui_test: tests/fake_audio.h tests/ui_test.cpp src/ui.cpp src/audio.cpp src/game.cpp src/board.cpp $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc tests/ui_test.cpp src/ui.cpp src/audio.cpp src/game.cpp src/board.cpp -o $@ $(shell pkg-config --cflags --libs raylib)

test-ui: build/ui_test
	./build/ui_test

clean:
	rm -rf $(BIN) build

.PHONY: all test test-ui clean
