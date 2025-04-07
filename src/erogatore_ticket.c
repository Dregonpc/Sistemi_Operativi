#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "../headers/messaggi.h"

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
        Messaggio msg;

        if (msgrcv(msgIdDispenser, &msg, sizeof(Messaggio) - sizeof(long), 0, 0) < 0) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }

        msg.ticket_id = ticket_number++;

        printf("[%s] Ticket %d assegnato all'utente '%s' per il servizio %ld.\n", erogatoreID, msg.ticket_id, msg.text, msg.mtype);

        if (msgsnd(msgIdOperator, &msg, sizeof(Messaggio) - sizeof(long), 0) < 0) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }

        printf("[%s] Ticket %d inviato all'operatore.\n", erogatoreID, msg.ticket_id);
    }

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *erogatoreID = argv[0];
    int semID = atoi(argv[1]);
    int msgIdDispenser = atoi(argv[2]);
    int msgIdOperator = atoi(argv[3]);
    printf("[%s] Avvio in corso. PID = %d\n", erogatoreID, getpid());

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", erogatoreID);
    notifyAndWait(semID, sops);
    
    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", erogatoreID);

    // Mi metto in ricezione
    ReceiveAndSendMessage(msgIdDispenser, msgIdOperator, erogatoreID);

    sleep(2);

    return EXIT_SUCCESS;
}