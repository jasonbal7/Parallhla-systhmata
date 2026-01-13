CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c17 -fopenmp -D_POSIX_C_SOURCE=200809L

BUILD_DIR = build

DIR1 = exercise1
DIR2 = exercise2
DIR3 = exercise3

TARGET1 = $(BUILD_DIR)/exercise1
TARGET2 = $(BUILD_DIR)/exercise2
TARGET3 = $(BUILD_DIR)/mergesort

all: $(BUILD_DIR) $(TARGET1) $(TARGET2) $(TARGET3)

$(BUILD_DIR):
	mkdir -p $@

$(TARGET1): $(DIR1)/1.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(TARGET2): $(DIR2)/2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(TARGET3): $(DIR3)/mergesort.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean