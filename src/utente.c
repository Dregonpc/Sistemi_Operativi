#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/sem.h>

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

    char *utenteID = argv[0];
    int semID = atoi(argv[1]);
    int msgIdUser = atoi(argv[2]);
    printf("[%s] Avvio in corso. PID = %d\n", utenteID, getpid());

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", utenteID);
    notifyAndWait(semID, sops);

    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", utenteID);
    sleep(2);

    return EXIT_SUCCESS;
}