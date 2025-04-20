#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "../headers/messaggi.h"

// flag globali gestite dai signal handler
static volatile sig_atomic_t endDay = 0;
static volatile sig_atomic_t startDay = 0;

// Signal handler per SIGUSR2 -> fine giornata
static void handle_day_end(int signo) {
    endDay = 1;
}

// Signal handler per SIGUSR1 -> inizio nuovo giorno
static void handle_day_start(int signo) {
    startDay = 1;
}

void SlaveNotifyAndWait(int semID, struct sembuf sops) {
    // avviso il direttore che sono pronto
    sops.sem_num = 0;
    sops.sem_op = -1;
    semop(semID, &sops, 1);
    
    // aspetto che arrivi a 0 (ovvero il direttore mi da il via)
    sops.sem_num = 1;
    sops.sem_op = 0;
    semop(semID, &sops, 1);
}

void ReceiveAndSendMessage(int msgIdDispenser, int msgIdOperator, char *erogatoreID, int semID, struct sembuf sops) {
    int ticket_number = 1;

    while (!endDay) {
        Messaggio msg;

        // if (msgrcv(msgIdDispenser, &msg, sizeof(Messaggio) - sizeof(long), 0, 0) < 0) {
        //     perror("msgrcv");
        //     // exit(EXIT_FAILURE);
        // }

        ssize_t n;

        // ricevo finché non ottengo un messaggio valido o endDay
        do {
            n = msgrcv(msgIdDispenser, &msg, sizeof(Messaggio) - sizeof(long), 0, 0);
        } while (n < 0 && errno == EINTR && !endDay);

        if (endDay) {
            printf("[%s] Fine giornata rilevata, interrompo ricezione.\n", erogatoreID);
            break;
        }
        if (n < 0) {
            perror("msgrcv");
            //exit(EXIT_FAILURE);
        }

        msg.ticket_id = ticket_number++;

        long realService = msg.mtype - 1;
        printf("[%s] Ticket %d assegnato all'utente '%s' per il servizio %ld.\n", erogatoreID, msg.ticket_id, msg.text, realService);

        if (msgsnd(msgIdOperator, &msg, sizeof(Messaggio) - sizeof(long), 0) < 0) {
            perror("msgsnd");
            //exit(EXIT_FAILURE);
        }

        printf("[%s] Ticket %d inviato all'operatore.\n", erogatoreID, msg.ticket_id);
    }
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *erogatoreID = argv[0];
    int semID = atoi(argv[1]);
    int msgIdDispenser = atoi(argv[2]);
    int msgIdOperator = atoi(argv[3]);
    printf("[%s] Avvio in corso. PID = %d\n", erogatoreID, getpid());

    // Installa i signal handler
    struct sigaction sa_end = {0}, sa_start = {0};
    sa_end.sa_handler = handle_day_end;
    sigemptyset(&sa_end.sa_mask);
    sa_end.sa_flags = 0;                // senza SA_RESTART
    sigaction(SIGUSR2, &sa_end, NULL);

    sa_start.sa_handler = handle_day_start;
    sigemptyset(&sa_start.sa_mask);
    sa_start.sa_flags = 0;//SA_RESTART;       // opzionale
    sigaction(SIGUSR1, &sa_start, NULL);

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", erogatoreID);
    SlaveNotifyAndWait(semID, sops);
    
    while (1) {
        // Posso iniziare a lavorare
        endDay = 0;
        startDay = 0;

        printf("[%s] Inizio giornata.\n", erogatoreID);
        // while (!startDay) {
        //     pause();
        // }
        //SlaveNotifyAndWait(semID, sops);

        // Mi metto in ricezione
        ReceiveAndSendMessage(msgIdDispenser, msgIdOperator, erogatoreID, semID, sops);

        // Scrivo statistiche

        // Siamo usciti dal while quindi la giornata è finita
        printf("[%s] Giorno terminato, attendo SIGUSR1 per il giorno successivo...\n", erogatoreID);
        // aspetto SIGUSR1
        // while (!startDay) {
        //     pause();
        // }
        SlaveNotifyAndWait(semID, sops);

        //printf("[%s] Giorno terminato.\n", erogatoreID);
        // riparte il loop per il prossimo giorno
    }

    //sleep(2);

    return EXIT_SUCCESS;
}