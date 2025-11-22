CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c17
LDLIBS = -pthread

TARGETS = 1 2 3 4 5

all: $(TARGETS)

1: 1.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

2: 2.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

3: 3.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

4: 4.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

5: 5.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

.PHONY: all clean

clean:
	rm -f $(TARGETS)
