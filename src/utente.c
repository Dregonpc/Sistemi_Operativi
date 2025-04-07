#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <stdbool.h>
#include "../headers/messaggi.h"
#include "../headers/SharedMemory.h"

// Collegamento alla memoria condivisa
DailyConfig* SharedMemoryAttach(int shmID, char* utenteId) {
    DailyConfig* config = (DailyConfig*)shmat(shmID, NULL, 0);
    if (config == (void *) -1) {
        printf("[%s] Collegamento alla memoria condivisa fallita.\n", utenteId);
        exit(EXIT_FAILURE);
    }

    return config;
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

int RandomizeService() {
    //return rand() % NUM_SERVIZI;
    return 1; // Per testare il servizio 1
}

bool CheckPresenceRequiredService(DailyConfig* config, int IndexServizioRichiesto, char* utenteID) {
    for (int i = 0; i < NUM_SPORTELLI; i++) {
        if (!config->sportelli[i].disponibile && config->sportelli[i].indexServizioOfferto == IndexServizioRichiesto) { // Se lo sportello non è disponibile significa che qualche operatore l'ha occupato, si potrebbe sostituire con config->sportelli[i].idOperatore != ""
            printf("[%s] Ho trovato un operatore che può svolgere la mia richiesta (%d)\n", utenteID, IndexServizioRichiesto);
            return true;
        }
    }
    
    printf("[%s] Non ho trovato un operatore che può svolgere la mia richiesta (%d)...\n", utenteID, IndexServizioRichiesto);
    return false;
}

void SendMessageToErogatore(int msgID, char* utenteID, int IndexServizioRichiesto, int myPID) {
    Messaggio msg;
    
    msg.mtype = IndexServizioRichiesto + 1; // Dobbiamo incrementarlo di 1 perchè il tipo del messaggio deve essere > 0, e quindi non potremmo passare il servizio 0.
    msg.ticket_id = -1;
    msg.user_id = myPID;
    snprintf(msg.text, MAX_TEXT, "%s", utenteID);

    if (msgsnd(msgID, &msg, sizeof(Messaggio) - sizeof(long), 0) < 0) {
        printf("[%s] Errore durante l'invio del messaggio all'erogatore.\n", utenteID);
        exit(EXIT_FAILURE);
    }

    printf("[%s] Ho richiesto un ticket per il servizio %d.\n", utenteID, IndexServizioRichiesto);
}

void ReceiveMessageFromOperator(int msgID, int myPID) {
    Messaggio msg;

    if (msgrcv(msgID, &msg, sizeof(Messaggio) - sizeof(long), myPID, 0) < 0) {
        perror("msgrcv");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *utenteID = argv[0];
    int shmID = atoi(argv[1]);
    int semID = atoi(argv[2]);
    int msgIdDispenser = atoi(argv[3]);
    int msgIdUser = atoi(argv[4]);
    int myPID = getpid();
    printf("[%s] Avvio in corso. PID = %d\n", utenteID, myPID);

    // colleghiamoci alla memoria condivisa
    DailyConfig* config = SharedMemoryAttach(shmID, utenteID);

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", utenteID);
    notifyAndWait(semID, sops);

    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", utenteID);

    int IndexServizioRichiesto = RandomizeService(); // PER I TEST: simuliamo di richiedere sempre il servizio 1, da sostituire con un random
    
    sleep(5);

    // Richiedo un ticket
    if (CheckPresenceRequiredService(config, IndexServizioRichiesto, utenteID)) {
        SendMessageToErogatore(msgIdDispenser, utenteID, IndexServizioRichiesto, myPID);
        printf("[%s] In attesa di ricevere il messaggio dall'operatore...\n", utenteID);
        ReceiveMessageFromOperator(msgIdUser, myPID);
        printf("[%s] Sono stato servito.\n", utenteID);
    }

    sleep(2);

    return EXIT_SUCCESS;
}