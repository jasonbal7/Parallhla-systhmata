CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c17
LDLIBS = -pthread

TARGETS = 1 2 3 4

all: $(TARGETS)

1: exercise1/1.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

2: exercise2/2.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

3: exercise3/3.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

4: exercise4/4.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

.PHONY: all clean

clean:
	rm -f $(TARGETS)
