#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include "../headers/messaggi.h"

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

void SendMessageToErogatore(int msgID, char* utenteID) {
    Messaggio msg;
    int IndexServizioRichiesto = 1; // PER I TEST: simuliamo di richiedere sempre il servizio 1, da sostituire con un random
    msg.mtype = IndexServizioRichiesto + 1; // Dobbiamo incrementarlo di 1 perchè il tipo del messaggio deve essere > 0, e quindi non potremmo passare il servizio 0. Nel receiver dell'operatore andiamo a decrementarlo di 1
    msg.ticket_id = -1;
    msg.user_id = utenteID;
    snprintf(msg.text, MAX_TEXT, "%s richiede servizio %d", utenteID, IndexServizioRichiesto);

    if (msgsnd(msgID, &msg, sizeof(Messaggio) - sizeof(long), 0) < 0) {
        printf("[%s] Errore durante l'invio del messaggio all'erogatore.\n", utenteID);
        exit(EXIT_FAILURE);
    }

    printf("[%s] Ho richiesto un ticket per il servizio %d.\n", utenteID, IndexServizioRichiesto);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *utenteID = argv[0];
    int semID = atoi(argv[1]);
    int msgIdUser = atoi(argv[2]);
    printf("[%s] Avvio in corso. PID = %d\n", utenteID, getpid());

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", utenteID);
    notifyAndWait(semID, sops);

    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", utenteID);

    // Richiedo un ticket
    SendMessageToErogatore(msgIdUser, utenteID);

    sleep(2);

    return EXIT_SUCCESS;
}