#include "SharedMemory.h"


int SharedMemoryCreate() {
    int shmID = shmget(IPC_PRIVATE, sizeof(DailyConfig), IPC_CREAT | 0666);
    if (shmID < 0) {
        printf("[Direttore] Creazione della memoria condivisa fallita.\n");
    }
    
    return shmID;
}

void* SharedMemoryAttachGeneral(int shmID, char* processName) {
    void* config = (void*)shmat(shmID, NULL, 0);
    if (config == (void *) -1) {
        printf("[%s] Collegamento alla memoria condivisa fallita.\n", processName);
    }

    return config;
}

void SharedMmemoryDeTouch(void* config, char* processName) {
    if (shmdt(config) == -1) {
        printf("[%s] Errore durante la disconnessione dalla memoria condivisa.\n", processName);
    }
}

void SharedMemoryCleanConfig(int shmID, DailyConfig* config) {
    shmdt(config);
    shmctl(shmID, IPC_RMID, NULL);
}

void SharedMemoryCleanStats(int shmID, Stats* stats) {
    shmdt(stats);
    shmctl(shmID, IPC_RMID, NULL);
}