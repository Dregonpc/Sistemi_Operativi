#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "../headers/servizi.h"
#include "../headers/SharedMemory.h"

// Collegamento alla memoria condivisa
DailyConfig* SharedMemoryAttach(int shmID, char* operatoreId) {
    DailyConfig* config = (DailyConfig*)shmat(shmID, NULL, 0);
    if (config == (void *) -1) {
        printf("[%s] Collegamento alla memoria condivisa fallita.\n", operatoreId);
        exit(EXIT_FAILURE);
    }

    return config;
}

bool TakeUpPostOffice(DailyConfig* config, int indexServizioOperatore, char* operatoreId) {
    bool check = false;
    //semaforo
    for (int i = 0; i < NUM_SPORTELLI; i++) {
        if (config->sportelli[i].disponibile && config->sportelli[i].indexServizioOfferto == indexServizioOperatore) {
            config->sportelli[i].idOperatore = operatoreId;
            config->sportelli[i].disponibile = 0;
            printf("[%s] Sono stato assegnato allo sportello %d per il servizio %d.\n", operatoreId, config->sportelli[i].idSportello, indexServizioOperatore);
            check = true;
            break;
        }
    }
    //semaforo
    return check;
}

void notifyAndWait(int semID, struct sembuf sops) {
    // decremento il semaforo = sono nato e sono pronto
    sops.sem_num = 0;
    sops.sem_op = -1;
    semop(semID, &sops, 1);

    // aspetto il direttore
    sops.sem_num = 0;
    sops.sem_op = 0;
    semop(semID, &sops, 1);
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *operatoreID = argv[0];
    int shmID = atoi(argv[1]);
    int semID = atoi(argv[2]);
    int indexServizio = atoi(argv[3]);
    Servizio specializzazione = servizi[indexServizio];
    printf("[%s] Avvio in corso. PID = %d\n", operatoreID, getpid());

    // colleghiamoci alla memoria condivisa
    DailyConfig* config = SharedMemoryAttach(shmID, operatoreID);

    // proviamo ad occupare uno sportello
    TakeUpPostOffice(config, indexServizio, operatoreID);

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", operatoreID);
    notifyAndWait(semID, sops);

    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", operatoreID);
    sleep(2);

    return EXIT_SUCCESS;
}