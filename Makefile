CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -D_POSIX_C_SOURCE=200809L
BUILD_DIR := build
COMMON_OBJECTS := $(BUILD_DIR)/osi_layers.o $(BUILD_DIR)/socket_io.o

.PHONY: all run test clean

all: $(BUILD_DIR)/osi-client $(BUILD_DIR)/osi-server

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/osi_layers.o: src/osi_layers.c src/osi_layers.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/socket_io.o: src/socket_io.c src/socket_io.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/osi-client: src/client.c $(COMMON_OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/osi-server: src/server.c $(COMMON_OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

run: all
	python3 python/visualizer.py

test: all
	python3 -m unittest discover -s tests -v

clean:
	rm -rf $(BUILD_DIR) __pycache__ python/__pycache__ tests/__pycache__