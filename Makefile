CC = gcc
CFLAGS = -Wvla -Wextra -Werror -D_GNU_SOURCE

# Directories
SRC := src
BIN := bin
BLD := build

$(shell mkdir -p $(BIN) $(BLD))

default: all

all: $(BIN)/main $(BIN)/direttore $(BIN)/erogatore_ticket $(BIN)/operatore $(BIN)/utente

# main
$(BIN)/main: $(BLD)/main.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/main.o: $(SRC)/main.c
	$(CC) $(CFLAGS) -c $< -o $@

# direttore
$(BIN)/direttore: $(BLD)/direttore.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/direttore.o: $(SRC)/direttore.c
	$(CC) $(CFLAGS) -c $< -o $@

# erogatore_ticket
$(BIN)/erogatore_ticket: $(BLD)/erogatore_ticket.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/erogatore_ticket.o: $(SRC)/erogatore_ticket.c
	$(CC) $(CFLAGS) -c $< -o $@

# operatore
$(BIN)/operatore: $(BLD)/operatore.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/operatore.o: $(SRC)/operatore.c
	$(CC) $(CFLAGS) -c $< -o $@

# utente
$(BIN)/utente: $(BLD)/utente.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/utente.o: $(SRC)/utente.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BLD) $(BIN)