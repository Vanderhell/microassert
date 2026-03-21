CC      ?= gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -Werror -I../include
SRC     := ../src/massert.c
TESTS   := test_all.c

.PHONY: all clean

all: test_all
	@./test_all

test_all: $(TESTS) $(SRC) ../include/massert.h
	$(CC) $(CFLAGS) $(SRC) $(TESTS) -o $@

clean:
	rm -f test_all
