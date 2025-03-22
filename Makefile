CC = gcc
CFLAGS = -Wvla -Wextra -Werror -D_GNU_SOURCE

# Directories
SRC := src
BIN := bin
BLD := build

$(shell mkdir -p $(BIN) $(BLD))

default: all

all: $(BIN)/main

# main
$(BIN)/main: $(BLD)/main.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/main.o: $(SRC)/main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BLD) $(BIN)