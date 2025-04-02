#ifndef SPORTELLI_H
#define SPORTELLI_H
#include "servizi.h"

typedef struct {
    int idSportello;
    int indexServizioOfferto;
    char* idOperatore;
    int disponibile;
} Sportello;

#endif //SPORTELLI_H