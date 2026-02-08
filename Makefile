TARGET=mbrot

CC=gcc
CFLAGS=-Wall -Wextra -g -O2 -std=gnu99

SRCS=main.c

all: $(TARGET)

.PHONY: clean

$(TARGET): $(SRCS)
	$(CC) -o $@ $^ $(CFLAGS) -lncurses

clean:
	rm $(TARGET)
