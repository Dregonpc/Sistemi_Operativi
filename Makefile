CC = gcc
CFLAGS = -Wvla -Wextra -Werror -D_GNU_SOURCE

# Directories
SRC := src
BIN := bin
BLD := build
LIB := lib
HDR := headers

$(shell mkdir -p $(BIN) $(BLD))

default: all

all: $(BIN)/main $(BIN)/direttore $(BIN)/erogatore_ticket $(BIN)/operatore $(BIN)/utente $(BIN)/addUsers

# main
$(BIN)/main: $(BLD)/main.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/main.o: $(SRC)/main.c
	$(CC) $(CFLAGS) -c $< -o $@

# direttore
$(BIN)/direttore: $(BLD)/servizi.o $(BLD)/SemsLib.o $(BLD)/SharedMemory.o $(BLD)/MessageQueueLib.o $(BLD)/StatsLib.o $(BLD)/direttore.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/servizi.o: $(LIB)/servizi.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/SemsLib.o: $(LIB)/SemsLib.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/SharedMemory.o: $(LIB)/SharedMemory.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/MessageQueueLib.o: $(LIB)/MessageQueueLib.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/StatsLib.o: $(LIB)/StatsLib.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/direttore.o: $(SRC)/direttore.c
	$(CC) $(CFLAGS) -c $< -o $@

# erogatore_ticket
$(BIN)/erogatore_ticket: $(BLD)/SemsLib.o $(BLD)/SharedMemory.o $(BLD)/erogatore_ticket.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/erogatore_ticket.o: $(SRC)/erogatore_ticket.c
	$(CC) $(CFLAGS) -c $< -o $@

# operatore
$(BIN)/operatore: $(BLD)/servizi.o $(BLD)/SemsLib.o $(BLD)/SharedMemory.o $(BLD)/StatsLib.o $(BLD)/operatore.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/operatore.o: $(SRC)/operatore.c
	$(CC) $(CFLAGS) -c $< -o $@

# utente
$(BIN)/utente: $(BLD)/SemsLib.o $(BLD)/StatsLib.o $(BLD)/SharedMemory.o $(BLD)/utente.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/utente.o: $(SRC)/utente.c
	$(CC) $(CFLAGS) -c $< -o $@

# addUsers
$(BIN)/addUsers: $(BLD)/MessageQueueLib.o $(BLD)/addUsers.o
	$(CC) $(CFLAGS) $^ -o $@

$(BLD)/addUsers.o: $(SRC)/addUsers.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BLD) $(BIN)