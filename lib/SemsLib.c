#include "../headers/SemsLib.h"

int semCreate(int flag, int numOfSem, char* processName) {
    int semID = semget(IPC_PRIVATE, numOfSem, IPC_CREAT | flag);
    if (semID < 0) {
        printf("[%s] Creazione del semaforo fallita.\n", processName);
        exit(EXIT_FAILURE);
    }
    
    return semID;
}

void semInizialize(int semID, int quantity0, int quantity1, int quantity2, int quantity3, int quantity4, char* processName) {
    // semNum = 0 : semaforo per gestire la barriera di partenza dei processi
    // all'inizio, contiene il numero di tutti i processi, quando arriverà a zero la simulazione partirà
    if (semctl(semID, 0, SETVAL, quantity0) < 0) {
        printf("[%s] Errore durante la semctl del semaforo dedicato alla barriera.\n", processName);
        exit(EXIT_FAILURE);
    }

    // semNum = 1 : semaforo per gestire lo start (ovvero i figli possono partire)
    // all'inizio, vale 1, tutti i figli aspettano che diventi 0
    if (semctl(semID, 1, SETVAL, quantity1) < 0) {
        printf("[%s] Errore durante la semctl del semaforo dedicato allo start.\n", processName);
        exit(EXIT_FAILURE);
    }

    // semNum = 2 : semaforo per gestire se ci sono sportelli liberi
    // all'inizio, tutti gli sportelli sono liberi
    if (semctl(semID, 2, SETVAL, quantity2) < 0) {
        printf("[%s] Errore durante la semctl del semaforo dedicato agli sportelli.\n", processName);
        exit(EXIT_FAILURE);
    }

    // semNum = 3 : semaforo per coordinare l'accesso singolo agli operatori per provare ad occupare uno sportello, in modo che vada uno per volta
    // all'inizio, il semaforo vale 1, quindi il primo operatore può provare ad occupare uno sportello
    if (semctl(semID, 3, SETVAL, quantity3) < 0) {
        printf("[%s] Errore durante la semctl del semaforo per l'accesso coordinato agli sportelli.\n", processName);
        exit(EXIT_FAILURE);
    }

    // semNum = 4 : semaforo per gestire il lock per le statistiche
    // vale sempre 1, quando qualcuno acquisice il lock diventa 0 e nessuno ci può più accedere finchè non torna a valere 1
    if (semctl(semID, 4, SETVAL, quantity4) < 0) {
        printf("[%s] Errore durante la semctl del semaforo per il lock delle statistiche.\n", processName);
        exit(EXIT_FAILURE);
    }   
}

void semMessageInitialize(int semID, int NUM_OF_SERVICE, char* processName){
    for (int i = 0; i < NUM_OF_SERVICE; i++) {
        if (semctl(semID, i, SETVAL, 0) < 0) {
            printf("[%s] Errore durante la semctl del semaforo dedicato alla coda di messaggi del servizio: %d.\n", processName, i);
            exit(EXIT_FAILURE);
        }
    }
}

void SemBarrierRestart(int semID, struct sembuf* sops, int quantity0, int quantity1, char* processName) {
    if (semctl(semID, 0, SETVAL, quantity0) < 0) {
        printf("[%s] Errore durante la semctl del semaforo dedicato alla barriera.\n", processName);
        exit(EXIT_FAILURE);
    }

    if (semctl(semID, 1, SETVAL, quantity1) < 0) {
        printf("[%s] Errore durante la semctl del semaforo dedicato allo start.\n", processName);
        exit(EXIT_FAILURE);
    }
}

void SemRestart(int semID, struct sembuf* sops, int quantity2, int quantity3, int quantity4, char* processName) {
    if (semctl(semID, 2, SETVAL, quantity2) < 0) {
        printf("[%s] Errore durante la semctl del semaforo dedicato agli sportelli.\n", processName);
        exit(EXIT_FAILURE);
    }

    if (semctl(semID, 3, SETVAL, quantity3) < 0) {
        printf("[%s] Errore durante la semctl del semaforo per l'accesso coordinato agli sportelli.\n", processName);
        exit(EXIT_FAILURE);
    }

    if (semctl(semID, 4, SETVAL, quantity4) < 0) {
        printf("[%s] Errore durante la semctl del semaforo per il lock delle statistiche.\n", processName);
        exit(EXIT_FAILURE);
    }
}

void semCleanUp(int semID) {
    if (semctl(semID, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        exit(EXIT_FAILURE);
    }
}

int SemGetVal(int semID, int semNum) {
    return semctl(semID, semNum, GETVAL);
}

int ExecuteSemop(int semID, struct sembuf* sops, int semNum, int semOp) {
    sops->sem_num = semNum;
    sops->sem_op = semOp;
    sops->sem_flg = 0;  // FONDAMENTALE???
    return semop(semID, sops, 1);
}

int CaptureLock(int semID, struct sembuf* sops, int semNum) {
    return ExecuteSemop(semID, sops, semNum, -1);
}

int ReleaseLock(int semID, struct sembuf* sops, int semNum) {
    return ExecuteSemop(semID, sops, semNum, 1);
}
