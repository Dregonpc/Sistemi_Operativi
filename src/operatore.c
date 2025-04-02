#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include "../headers/messaggi.h"
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

bool TakeUpPostOffice(DailyConfig* config, int semID, struct sembuf sops, int indexServizioOperatore, char* operatoreId) {
    bool check = false;

    // acquisisco il lock per l'accesso coordinato agli sportelli
    sops.sem_num = 2;
    sops.sem_op = -1;
    if (semop(semID, &sops, 1) == -1) {
        printf("[%s] Errore durante l'acquisizione del lock per gli sportelli.\n", operatoreId);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_SPORTELLI; i++) {
        if (config->sportelli[i].disponibile && config->sportelli[i].indexServizioOfferto == indexServizioOperatore) {
            config->sportelli[i].idOperatore = operatoreId;
            config->sportelli[i].disponibile = 0;
            printf("[%s] Sono stato assegnato allo sportello %d per il servizio %d.\n", operatoreId, config->sportelli[i].idSportello, indexServizioOperatore);
            check = true;

            // invio il segnale che uno sportello è stato occupato
            sops.sem_num = 1;
            sops.sem_op = -1;
            if (semop(semID, &sops, 1) == -1) {
                printf("[%s] Errore durante l'invio del segnale che uno sportello è stato occupato.\n", operatoreId);
                exit(EXIT_FAILURE);
            }

            break;
        }
    }

    // rilascio il lock
    sops.sem_num = 2;
    sops.sem_op = 1;
    if (semop(semID, &sops, 1) == -1) {
        printf("[%s] Errore durante il rilascio del lock per gli sportelli.\n", operatoreId);
        exit(EXIT_FAILURE);
    }

    return check;
}

void waitFreePostOffice(int semID, struct sembuf sops, char *operatoreId) {
    // attendo il segnale che uno sportello si sia liberato
    sops.sem_num = 1;
    sops.sem_op = -1;
    if (semop(semID, &sops, 1) == -1) { // con questo comando il processo si mette in wait
        printf("[%s] Errore durante l'attesa di uno sportello.\n", operatoreId);
        exit(EXIT_FAILURE);
    }

    printf("[%s] Ho ricevuto il segnale che uno sportello si è liberato. Riprovo ad occuparlo...\n", operatoreId);
}

void releasePostOffice(DailyConfig* config, int semID, struct sembuf sops, char* operatoreId) {
    for (int i = 0; i < NUM_SPORTELLI; i++) {
        if (config->sportelli[i].idOperatore = operatoreId) {
            config->sportelli[i].idOperatore = "";
            config->sportelli[i].disponibile = 1;
            break;
        }
    }

    sops.sem_num = 1;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    if (semop(semID, &sops, 1) == -1) {
        printf("[%s] Errore durante il rilascio di uno sportello.\n", operatoreId);
        exit(EXIT_FAILURE);
    }

    printf("[%s] Ho rilasciato lo sportello e mandato la notifica ai miei colleghi.\n", operatoreId);
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

void ReceiveTicketAndExecute(int msgIdOperator, int IndexServizio, char *operatoreID) {
    while (1) {
        Messaggio msg;

        if (msgrcv(msgIdOperator, &msg, sizeof(Messaggio) - sizeof(long), IndexServizio - 1, 0) < 0) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }

        printf("[%s] Servo il ticket %d per l'utente %s (servizio %ld).\n", operatoreID, msg.ticket_id, msg.user_id, msg.mtype);

        // Eseguo il servizio
        sleep(2); // Simulazione, dovremmo mettere il tempo dedicato al servizio richiesto
    }
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *operatoreID = argv[0];
    int shmID = atoi(argv[1]);
    int semID = atoi(argv[2]);
    int msgIdOperator = atoi(argv[3]);
    int indexServizio = atoi(argv[4]);
    Servizio specializzazione = servizi[indexServizio];
    printf("[%s] Avvio in corso. PID = %d\n", operatoreID, getpid());

    // colleghiamoci alla memoria condivisa
    DailyConfig* config = SharedMemoryAttach(shmID, operatoreID);

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", operatoreID);
    notifyAndWait(semID, sops);
    
    // proviamo ad occupare uno sportello
    while (!TakeUpPostOffice(config, semID, sops, indexServizio, operatoreID)) {
        printf("[%s] Nessuno sportello disponibile per il servizio %d, attendo...\n", operatoreID, indexServizio);
        waitFreePostOffice(semID, sops, operatoreID);
    }
    
    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", operatoreID);

    // Mi metto a ricevere i ticket e ad eseguirli
    ReceiveTicketAndExecute(msgIdOperator, indexServizio, operatoreID);

    // rilascio lo sportello
    releasePostOffice(config, semID, sops, operatoreID);

    shmdt(config);

    return EXIT_SUCCESS;
}