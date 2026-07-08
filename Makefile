CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2

SRC := src/board.cpp src/gui_main.cpp
BIN := geesespotter

all: $(BIN)

$(BIN): $(SRC) src/board.h
	$(CXX) $(CXXFLAGS) $(SRC) -o $@ $(shell pkg-config --cflags --libs raylib)

test: build/board_test
	./build/board_test

build/board_test: tests/board_test.cpp src/board.cpp src/board.h
	mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc tests/board_test.cpp src/board.cpp -o $@

clean:
	rm -rf $(BIN) build

.PHONY: all test clean
