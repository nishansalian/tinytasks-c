CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

.PHONY: all clean

all: todo

todo: todo.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f todo
