#include "SharedMemory.h"


int SharedMemoryCreate() {
    int shmID = shmget(IPC_PRIVATE, sizeof(DailyConfig), IPC_CREAT | 0666);
    if (shmID < 0) {
        printf("[Direttore] Creazione della memoria condivisa fallita.\n");
    }
    
    return shmID;
}

DailyConfig* SharedMemoryAttach(int shmID, char* processName) {
    DailyConfig* config = (DailyConfig*)shmat(shmID, NULL, 0);
    if (config == (void *) -1) {
        printf("[%s] Collegamento alla memoria condivisa fallita.\n", processName);
    }

    return config;
}

Stats* SharedMemoryAttachStats(int shmID, char* processName) {
    Stats* stats = (Stats*)shmat(shmID, NULL, 0);
    if (stats == (void *) -1) {
        printf("[%s] Collegamento alla memoria condivisa fallita.\n", processName);
    }

    return stats;
}

void SharedMemoryCleanConfig(int shmID, DailyConfig* config) {
    shmdt(config);
    shmctl(shmID, IPC_RMID, NULL);
}

void SharedMemoryCleanStats(int shmID, Stats* stats) {
    shmdt(stats);
    shmctl(shmID, IPC_RMID, NULL);
}