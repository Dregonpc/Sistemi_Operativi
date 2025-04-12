#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>
#include "../headers/messaggi.h"
#include "../headers/servizi.h"
#include "../headers/SharedMemory.h"

volatile sig_atomic_t startDay = false;
volatile sig_atomic_t endDay = false;

// Signal handler per l'inizio giornata (SIGUSR1)
void handle_day_start(int signo) {
    printf("[Operatore =  %d] Ricevuto SIGUSR1: inizio del giorno.\n", getpid());
    startDay = true;
}

// Signal handler per il reset (SIGUSR2)
void handle_day_end(int signo) {
    printf("[Operatore = %d] Ricevuto SIGUSR2: fine del giorno. Reset in corso...\n", getpid());
    endDay = true;
    // Reset:
}

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

bool breakCondition() {
    // Da decidere una vera condizione (ad esempio che abbia servito almeno due clienti)
    return (rand() % 100) < 20; // 20% di possibilità di andare in pausa
}

int CalculateTimeExecution(int IndexServizio) {
    srand(time(NULL));
    int durata = servizi[IndexServizio].durata;
    int variazione = durata / 2;
    int delta = (rand() % (2 * variazione + 1)) - variazione;
    int durataCasuale = durata + delta;
    
    return durataCasuale;
}

void ReceiveTicketAndExecute(int msgIdOperator, int msgIdUser, int IndexServizio, char *operatoreID, int NOF_PAUSE, int* pause_effettuate) {
    while (1) {
        Messaggio msg;

        // if (msgrcv(msgIdOperator, &msg, sizeof(Messaggio) - sizeof(long), IndexServizio + 1, 0) < 0) { // Dobbiamo incrementarlo di 1 perchè il tipo del messaggio deve essere > 0, e quindi non potremmo passare il servizio 0.
        //     perror("msgrcv");
        //     //exit(EXIT_FAILURE);
        // }

        // ssize_t n;
        // do {
        //     n = msgrcv(msgIdOperator, &msg, sizeof(Messaggio) - sizeof(long), IndexServizio + 1, 0) < 0; // Dobbiamo incrementarlo di 1 perchè il tipo del messaggio deve essere > 0, e quindi non potremmo passare il servizio 0.
        // } while (n < 0 && errno == EINTR);

        sigset_t mask, oldmask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);  // Blocca SIGUSR1
        if (sigprocmask(SIG_BLOCK, &mask, &oldmask) == -1) {
            perror("sigprocmask");
            exit(EXIT_FAILURE);
        }

        // Ora chiamata bloccante a msgrcv
        ssize_t n = msgrcv(msgIdOperator, &msg, sizeof(Messaggio) - sizeof(long), IndexServizio + 1, 0);

        // Ripristina la maschera dei segnali
        if (sigprocmask(SIG_SETMASK, &oldmask, NULL) == -1) {
            perror("sigprocmask");
            exit(EXIT_FAILURE);
        }

        if (n < 0) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }

        msg.mtype--;

        printf("[%s] Servo il ticket %d per l'utente '%s' (servizio %ld).\n", operatoreID, msg.ticket_id, msg.text, msg.mtype);

        // Eseguo il servizio
        int tempo = CalculateTimeExecution(IndexServizio);
        sleep(2); // Da modificare con il valore calcolato * unità di misura scelta da noi

        // Manda risposta all'utente usando il suo PID come "destinatario"
        msg.mtype = msg.user_id;
        if (msgsnd(msgIdUser, &msg, sizeof(Messaggio) - sizeof(long), 0) < 0) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }

        printf("[%s] Ho finito di servire %ld. Ci ho impiegato %d secondi.\n", operatoreID, msg.mtype, tempo);

        if ((*pause_effettuate) < NOF_PAUSE && breakCondition()) {
            (*pause_effettuate)++;
            printf("[%s] Posso andare in pausa, termino la mia giornata.\n", operatoreID);
            break;
        }
    }
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *operatoreID = argv[0];
    int shmID = atoi(argv[1]);
    int semID = atoi(argv[2]);
    int msgIdOperator = atoi(argv[3]);
    int msgIdUser = atoi(argv[4]);
    int indexServizio = atoi(argv[5]);
    int NOF_PAUSE = atoi(argv[6]);

    int pause_effettuate = 0;
    bool alreadyNotifiedStart = false;

    printf("[%s] Avvio in corso. PID = %d\n", operatoreID, getpid());

    // Configuriamo i segnali
    struct sigaction sa_start, sa_reset; //, sa_term;

    // Installa il signal handler per SIGUSR1
    sa_start.sa_handler = handle_day_start;
    sigemptyset(&sa_start.sa_mask);
    sa_start.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa_start, NULL) < 0) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    // Installa il signal handler per SIGUSR2
    sa_reset.sa_handler = handle_day_end;
    sigemptyset(&sa_reset.sa_mask);
    sa_reset.sa_flags = 0;
    if (sigaction(SIGUSR2, &sa_reset, NULL) < 0) {
        perror("sigaction SIGUSR2");
        exit(EXIT_FAILURE);
    }

    // colleghiamoci alla memoria condivisa
    DailyConfig* config = SharedMemoryAttach(shmID, operatoreID);

    // proviamo ad occupare uno sportello
    while (!TakeUpPostOffice(config, semID, sops, indexServizio, operatoreID)) {
        printf("[%s] Nessuno sportello disponibile per il servizio %d, attendo...\n", operatoreID, indexServizio);
        if (!alreadyNotifiedStart) {
            notifyAndWait(semID, sops);
            alreadyNotifiedStart = true;
        }
        waitFreePostOffice(semID, sops, operatoreID);
    }

    if (!alreadyNotifiedStart) {
        printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", operatoreID);
        notifyAndWait(semID, sops);
        alreadyNotifiedStart = true;
    }
    
    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", operatoreID);

    // Mi metto a ricevere i ticket e ad eseguirli
    ReceiveTicketAndExecute(msgIdOperator, msgIdUser, indexServizio, operatoreID, NOF_PAUSE, &pause_effettuate);

    // rilascio lo sportello
    releasePostOffice(config, semID, sops, operatoreID);
    
    // Aggiorna le statistiche

    shmdt(config);

    return EXIT_SUCCESS;
}