CC=clang
EXTRA_FLAGS=-D_DEFAULT_SOURCE
CFLAGS=-g -pedantic -Wall -Werror --std=c99 -O2 -lm $(EXTRA_FLAGS)

CODE_FILES=$(wildcard *.c *.h Makefile)
FILTER_FILES=Makefile %.h

all: fluids

clean:
	rm -f fluids *.o

test: fluids
	./fluids < inputs/logo.txt

fluids: $(CODE_FILES)
	$(CC) $(CFLAGS) $(filter-out $(FILTER_FILES), $^) -o $@

.PHONY: all clean test
