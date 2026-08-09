CXX ?= c++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror
CPPFLAGS ?= -Iinclude

BUILD_DIR := build
LIB_SOURCE := src/order_book.cpp

.PHONY: all test demo benchmark clean

all: $(BUILD_DIR)/lob_replay

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/lob_replay: $(LIB_SOURCE) src/replay.cpp include/lob/order_book.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIB_SOURCE) src/replay.cpp -o $@

$(BUILD_DIR)/order_book_tests: $(LIB_SOURCE) tests/order_book_test.cpp include/lob/order_book.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIB_SOURCE) tests/order_book_test.cpp -o $@

$(BUILD_DIR)/lob_benchmark: $(LIB_SOURCE) src/benchmark.cpp include/lob/order_book.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIB_SOURCE) src/benchmark.cpp -o $@

test: $(BUILD_DIR)/order_book_tests
	./$(BUILD_DIR)/order_book_tests

demo: $(BUILD_DIR)/lob_replay
	./$(BUILD_DIR)/lob_replay data/sample_events.csv

benchmark: $(BUILD_DIR)/lob_benchmark
	./$(BUILD_DIR)/lob_benchmark

clean:
	rm -f $(BUILD_DIR)/lob_replay $(BUILD_DIR)/order_book_tests $(BUILD_DIR)/lob_benchmark
