CC = mpicc

CFLAGS = -O3 -Wall -Wextra -D_POSIX_C_SOURCE=200809L

BUILD_DIR = build

DIR1 = exercise1
DIR2 = exercise2

TARGET1 = $(BUILD_DIR)/1
TARGET2 = $(BUILD_DIR)/2

all: $(BUILD_DIR) $(TARGET1) $(TARGET2)

$(BUILD_DIR):
	mkdir -p $@

$(TARGET1): $(DIR1)/1.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(TARGET2): $(DIR2)/2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean