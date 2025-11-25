CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c17
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
LDLIBS = -pthread

BUILD_DIR := build

SRC_E1 := $(wildcard exercise1/*.c)
SRC_E2 := $(wildcard exercise2/*.c)
SRC_E3 := $(wildcard exercise3/*.c)
SRC_E5 := $(wildcard exercise5/*.c)

BIN_E1 := $(patsubst exercise1/%.c,$(BUILD_DIR)/%.out,$(SRC_E1))
BIN_E2 := $(patsubst exercise2/%.c,$(BUILD_DIR)/%.out,$(SRC_E2))
BIN_E3 := $(patsubst exercise3/%.c,$(BUILD_DIR)/%.out,$(SRC_E3))
BIN_E5 := $(patsubst exercise5/%.c,$(BUILD_DIR)/%.out,$(SRC_E5))
BIN_E4 := $(BUILD_DIR)/4.out

BINARIES := $(BIN_E1) $(BIN_E2) $(BIN_E3) $(BIN_E5) $(BIN_E4)

all: $(BUILD_DIR) $(BINARIES)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.out: exercise1/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
	chmod +x $@

$(BUILD_DIR)/%.out: exercise2/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
	chmod +x $@

$(BUILD_DIR)/%.out: exercise3/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
	chmod +x $@

$(BUILD_DIR)/%.out: exercise5/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
	chmod +x $@

EX4_SRC := exercise4/4.c exercise4/globals.c exercise4/transactions.c exercise4/workers.c

$(BUILD_DIR)/4.out: $(EX4_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EX4_SRC) -o $@ $(LDLIBS)
	chmod +x $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean