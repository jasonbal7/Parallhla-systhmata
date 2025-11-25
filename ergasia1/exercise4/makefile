CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c17
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
LDLIBS = -pthread

TARGET = 4
SRC = 4.c globals.c transactions.c workers.c
OBJ = $(SRC:.c=.o)
HEADERS = globals.h transactions.h workers.h

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDLIBS)

%.o: %.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: all clean

clean:
	rm -f $(TARGET) $(OBJ)
