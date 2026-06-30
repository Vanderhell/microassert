CC      ?= gcc
CPPFLAGS ?=
CFLAGS  ?= -std=c99 -Wall -Wextra -Wpedantic -Werror
LDFLAGS ?=
LDLIBS  ?=
PROJECT_CPPFLAGS := -I../include
SRC     := ../src/massert.c
TESTS   := test_all.c

.PHONY: all clean

all: test_all
	@./test_all

test_all: $(TESTS) $(SRC) ../include/massert.h
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) $(SRC) $(TESTS) $(LDFLAGS) $(LDLIBS) -o $@

clean:
	$(RM) test_all
