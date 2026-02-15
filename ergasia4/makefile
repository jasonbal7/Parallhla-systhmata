CC = gcc

CFLAGS = -O2 -mavx2 -Wall -Wextra -D_POSIX_C_SOURCE=200809L

BUILD_DIR = build

DIR1 = exercise1

TARGET = $(BUILD_DIR)/1

all: $(BUILD_DIR) $(TARGET)


$(BUILD_DIR):
	mkdir -p $@


$(TARGET): $(DIR1)/1.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean