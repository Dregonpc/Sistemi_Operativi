#ifndef SERVIZI_H
#define SERVIZI_H

#define NUM_SERVICES 6

typedef struct {
    char name[50];
    int time; // in minuti
} Service;

// Elenco dei servizi disponibili
extern Service services[NUM_SERVICES];

#endif