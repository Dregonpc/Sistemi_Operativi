#ifndef SPORTELLI_H
#define SPORTELLI_H

#include "servizi.h"

#define NUM_SPORTELLI 3

typedef struct {
    int idSportello;
    int indexServizioOfferto;
    char* idOperatore;
    int disponibile;
} Sportello;

#endif //SPORTELLI_H