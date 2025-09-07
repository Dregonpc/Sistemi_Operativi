#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include "../headers/messaggi.h"
#include "../headers/SemsLib.h"

// flag globali gestite dai signal handler
static volatile sig_atomic_t endDay = 0;
static volatile sig_atomic_t endSimulation = 0;

// Handler per tutti i segnali
static void signalHandler(int signo) {
    switch (signo) {
        case SIGUSR2:
            endDay = 1;
            break;
        case SIGTERM:
            endSimulation = 1;
            endDay = 1;
            break;
        default:
            break;
    }
}

void SlaveNotifyAndWait(int semID, struct sembuf* sops) {

    if (endSimulation) return;

    // avviso il direttore che sono pronto
    ExecuteSemop(semID, sops, 0, -1);

    if (endSimulation) return;
    
    // aspetto che arrivi a 0 (ovvero il direttore mi da il via)
    ExecuteSemop(semID, sops, 1, 0);
}

void ReceiveAndSendMessage(int msgIdDispenser, int msgIdOperator, char *erogatoreID, int semID, struct sembuf sops) {
    int ticket_number = 1;

    while (!endDay) {
        Messaggio msg;
        ssize_t n;

        // ricevo finché non ottengo un messaggio valido o endDay
        do {
            n = msgrcv(msgIdDispenser, &msg, sizeof(Messaggio) - sizeof(long), 0, 0);
        } while (n < 0 && errno == EINTR && !endDay);

        if (endDay) {
            #ifdef DEBUG
                printf("[%s] Fine giornata rilevata, interrompo ricezione.\n", erogatoreID);
            #endif
            break;
        }
        else if (errno == EIDRM) {
            #ifdef DEBUG
                printf("[%s] La coda è stata cancellata, interrompo ricezione.\n", erogatoreID);
            #endif
            break;
        }
        if (n < 0) {
            #ifdef DEBUG
                perror("msgrcv");
            #endif
            break;
        }

        msg.ticket_id = ticket_number++;

        long realService = msg.mtype - 1;
        #ifdef DEBUG
            printf("[%s] Ticket %d assegnato all'utente '%s' per il servizio %ld.\n", erogatoreID, msg.ticket_id, msg.text, realService);
        #endif

        if (msgsnd(msgIdOperator, &msg, sizeof(Messaggio) - sizeof(long), 0) < 0) {
            #ifdef DEBUG
                perror("msgsnd");
            #endif
            break;
        }

        #ifdef DEBUG
            printf("[%s] Ticket %d inviato all'operatore.\n", erogatoreID, msg.ticket_id);
        #endif
    }
}

int main(int argc, char *argv[]) {

    struct sembuf sops = {0}; // Inizializza tutti i campi a 0

    char *erogatoreID = argv[0];
    int semID = atoi(argv[1]);
    int msgIdDispenser = atoi(argv[2]);
    int msgIdOperator = atoi(argv[3]);

    #ifdef DEBUG
        printf("[%s] Avvio in corso. PID = %d\n", erogatoreID, getpid());
    #endif

    // Installa i signal handler
    struct sigaction sa_end = {0};
    sa_end.sa_handler = signalHandler;
    sigemptyset(&sa_end.sa_mask);
    sa_end.sa_flags = 0;
    sigaction(SIGUSR2, &sa_end, NULL);

    struct sigaction sa_term = {0};
    sa_term.sa_handler = signalHandler;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);

    #ifdef DEBUG
        printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", erogatoreID);
    #endif
    SlaveNotifyAndWait(semID, &sops);
    
    while (!endSimulation) {
        SlaveNotifyAndWait(semID, &sops);
        
        if (endSimulation) {
            break;
        }
        
        // Posso iniziare a lavorare
        #ifdef DEBUG
            printf("[%s] Inizio giornata.\n", erogatoreID);
        #endif
        endDay = 0;

        // Mi metto in ricezione
        ReceiveAndSendMessage(msgIdDispenser, msgIdOperator, erogatoreID, semID, sops);

        // Siamo usciti dal while quindi la giornata è finita
        #ifdef DEBUG
            printf("[%s] Giorno terminato.\n", erogatoreID);
        #endif
        SlaveNotifyAndWait(semID, &sops);

        if (endSimulation) {
            break;
        }
        
        // riparte il loop per il prossimo giorno
    }

    return EXIT_SUCCESS;
}
