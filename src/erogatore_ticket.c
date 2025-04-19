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

    while (1) {
        Messaggio msg;

        if (msgrcv(msgIdDispenser, &msg, sizeof(Messaggio) - sizeof(long), 0, 0) < 0) {
            perror("msgrcv");
            // exit(EXIT_FAILURE);
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

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", erogatoreID);
    SlaveNotifyAndWait(semID, sops);
    
    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", erogatoreID);

    // Mi metto in ricezione
    ReceiveAndSendMessage(msgIdDispenser, msgIdOperator, erogatoreID, semID, sops);

    // Siamo usciti dal while quindi la giornata è finita

    sleep(2);

    return EXIT_SUCCESS;
}