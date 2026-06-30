CC ?= gcc
CPPFLAGS ?=
CFLAGS ?= -std=c99 -Wall -Wextra -Wpedantic -Werror -Iinclude
LDFLAGS ?=
LDLIBS ?=

TARGET := tests/test_all.exe
SOURCES := src/massert.c tests/test_all.c

.PHONY: all test clean

all: $(TARGET)

test: $(TARGET)
	./$(TARGET)

$(TARGET): $(SOURCES) include/massert.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $@

clean:
	$(RM) $(TARGET)
