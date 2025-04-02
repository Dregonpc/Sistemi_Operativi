#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include "sportelli.h"

typedef struct {
    Sportello sportelli[NUM_SPORTELLI];
} DailyConfig;

#endif //SHAREDMEMORY_H   