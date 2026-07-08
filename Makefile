CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2

SRC := src/board.cpp src/game.cpp src/main.cpp
HDR := src/board.h src/game.h
BIN := geesespotter

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

test: build/board_test
	./build/board_test

build/board_test: tests/board_test.cpp src/board.cpp src/board.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc tests/board_test.cpp src/board.cpp -o $@

clean:
	rm -rf $(BIN) build

.PHONY: all test clean
