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

volatile sig_atomic_t startDay = false;
volatile sig_atomic_t endDay = false;

// Signal handler per l'inizio giornata (SIGUSR1)
void handle_day_start(int signo) {
    printf("[PID %d] Ricevuto SIGUSR1: inizio del giorno.\n", getpid());
    startDay = true;
}

// Signal handler per il reset (SIGUSR2)
void handle_day_end(int signo) {
    printf("[PID %d] Ricevuto SIGUSR2: fine del giorno. Reset in corso...\n", getpid());
    endDay = true;
    // Reset:
}

// Signal handler per terminazione (SIGTERM)
void handle_termination(int signo) {
    printf("[PID %d] Ricevuto SIGTERM: terminazione finale.\n", getpid());
    exit(EXIT_SUCCESS);
}

void CheckStatusDay(char *erogatoreID) {
    if (endDay) {
        printf("[%s] Giorno terminato, attento il giorno nuovo\n", erogatoreID);
        endDay = false;
        startDay = false;

        while(!startDay) {
            pause();
        }

        printf("[%s] Giorno iniziato, mi metto a lavorare\n", erogatoreID);
    }
}

void notifyAndWait(int semID, struct sembuf sops) {
    // decremento il semaforo = sono nato e sono pronto
    sops.sem_num = 0; // numero del semaforo (nell'insieme)
    sops.sem_op = -1; // operazione da eseguire
    semop(semID, &sops, 1);

    // aspetto il direttore
    sops.sem_num = 0;
    sops.sem_op = 0;
    semop(semID, &sops, 1);
}

void ReceiveAndSendMessage(int msgIdDispenser, int msgIdOperator, char *erogatoreID) {
    int ticket_number = 1;

    while (1) {
        CheckStatusDay(erogatoreID);

        Messaggio msg;

        // if (msgrcv(msgIdDispenser, &msg, sizeof(Messaggio) - sizeof(long), 0, 0) < 0) {
        //     perror("msgrcv");
        //     // exit(EXIT_FAILURE);
        // }

        // ssize_t n;
        // do {
        //     n = msgrcv(msgIdDispenser, &msg, sizeof(Messaggio) - sizeof(long), 0, 0);
        // } while (n < 0 && errno == EINTR);

        // if (n < 0) {
        //     perror("msgrcv");
        //     exit(EXIT_FAILURE);
        // }

        sigset_t mask, oldmask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);  // Blocca SIGUSR1
        if (sigprocmask(SIG_BLOCK, &mask, &oldmask) == -1) {
            perror("sigprocmask");
            exit(EXIT_FAILURE);
        }

        // Ora chiamata bloccante a msgrcv
        ssize_t n = msgrcv(msgIdDispenser, &msg, sizeof(Messaggio) - sizeof(long), 0, 0);

        // Ripristina la maschera dei segnali
        if (sigprocmask(SIG_SETMASK, &oldmask, NULL) == -1) {
            perror("sigprocmask");
            exit(EXIT_FAILURE);
        }

        if (n < 0) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }

        msg.ticket_id = ticket_number++;

        long realService = msg.mtype - 1;
        printf("[%s] Ticket %d assegnato all'utente '%s' per il servizio %ld.\n", erogatoreID, msg.ticket_id, msg.text, realService);

        if (msgsnd(msgIdOperator, &msg, sizeof(Messaggio) - sizeof(long), 0) < 0) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
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
    
    // Configuriamo i segnali
    struct sigaction sa_start;//, sa_reset, sa_term;

    // Installa il signal handler per SIGUSR1
    sa_start.sa_handler = handle_day_start;
    sigemptyset(&sa_start.sa_mask);
    sa_start.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sa_start, NULL) < 0) {
        perror("sigaction SIGUSR1");
        exit(EXIT_FAILURE);
    }

    // Installa il signal handler per SIGUSR2
    // sa_reset.sa_handler = handle_day_end;
    // sigemptyset(&sa_reset.sa_mask);
    // sa_reset.sa_flags = 0;
    // if (sigaction(SIGUSR2, &sa_reset, NULL) < 0) {
    //     perror("sigaction SIGUSR2");
    //     exit(EXIT_FAILURE);
    // }

    // // Installa il signal handler per SIGTERM
    // sa_term.sa_handler = handle_termination;
    // sigemptyset(&sa_term.sa_mask);
    // sa_term.sa_flags = 0;
    // if (sigaction(SIGTERM, &sa_term, NULL) < 0) {
    //     perror("sigaction SIGTERM");
    //     exit(EXIT_FAILURE);
    // }

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", erogatoreID);
    notifyAndWait(semID, sops);
    
    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", erogatoreID);

    // Mi metto in ricezione
    ReceiveAndSendMessage(msgIdDispenser, msgIdOperator, erogatoreID);

    sleep(2);

    return EXIT_SUCCESS;
}