#ifndef GLOBALVARS_H
#define GLOBALVARS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define MAX_LINE 100

extern int NUM_OF_WORKERS;
extern int NUM_OF_USERS;
extern int TOTAL_PROCESSES;
extern int TOTAL_PROCESSES_DIR;

extern int NOF_PAUSE;  // Numero di pause che un operatore può fare in tutta la simulazione

// Probabilità per l'utente
extern int P_SERV_MIN;
extern int P_SERV_MAX;

extern int SIM_DURATION; // Durata della simulazione in giorni
extern int EXPLODE_THRESHOLD;  // max numero di utenti a fine giornata che non sono stati serviti --> se supera la soglia termina la simulazione

extern int MINUTES_FOR_DAY; // 480 minuti = 8 ore
extern int SIMULATED_MINUTE; // 4 milioni di nanosecondi = 4ms
// 4ms * 480 = 1,92 secondi

void read_config(char *filename);

#endif // GLOBALVARS_H