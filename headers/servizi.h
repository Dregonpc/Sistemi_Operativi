#ifndef SERVIZI_H
#define SERVIZI_H

#define NUM_SERVIZI 6

typedef struct {
    char nome[50];
    int durata; // in minuti
} Servizio;

// Elenco dei servizi disponibili
extern Servizio servizi[NUM_SERVIZI];

#endif