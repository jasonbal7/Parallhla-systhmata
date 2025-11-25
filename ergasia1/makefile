CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c17
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
LDLIBS = -pthread

BUILD_DIR = build
EXECUTABLES = 1 2 3 4 5.1 5.2 5.3

TARGETS = $(addprefix $(BUILD_DIR)/, $(EXECUTABLES))

all: $(BUILD_DIR) $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


# --------------- Exercise 1 ----------------
$(BUILD_DIR)/1: exercise1/1.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
	chmod +x $@


# --------------- Exercise 2 ----------------
$(BUILD_DIR)/2: exercise2/2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
	chmod +x $@


# --------------- Exercise 3 ----------------
$(BUILD_DIR)/3: exercise3/3.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
	chmod +x $@


# --------------- Exercise 4 ----------------
EX4_SRC = exercise4/4.c exercise4/globals.c exercise4/transactions.c exercise4/workers.c
EX4_OBJ = $(addprefix $(BUILD_DIR)/, 4.o globals.o transactions.o workers.o)

$(BUILD_DIR)/4: $(EX4_OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(EX4_OBJ) -o $@ $(LDLIBS)
	chmod +x $@

$(BUILD_DIR)/%.o: exercise4/%.c exercise4/*.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
	chmod +x $@


# --------------- Exercise 5 ----------------
$(BUILD_DIR)/5.1: exercise5/5.1.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
	chmod +x $@

$(BUILD_DIR)/5.2: exercise5/5.2.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
	chmod +x $@

$(BUILD_DIR)/5.3: exercise5/5.3.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
	chmod +x $@



clean:
	rm -rf $(BUILD_DIR)

	
.PHONY: all clean