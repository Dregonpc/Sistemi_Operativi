#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/shm.h>

typedef struct {
    sem_t sem_ready;
    sem_t sem_start;
} SharedData;



// int main(int argc, char *argv[]) {
//     char *erogatoreID = argv[0];
//     int shmID = atoi(argv[1]);
//     printf("[%s] Avvio in corso. PID = %d\n", erogatoreID, getpid());

//     // Colleghiamoci alla memoria condivisa
//     SharedData *shm_ptr = (SharedData *)shmat(shmID, NULL, 0);
//     if (shm_ptr == (void *) -1) {
//         printf("[%s] Collegamento alla memoria condivisa fallito.\n", erogatoreID);
//         exit(EXIT_FAILURE);
//     }

//     // Avviso il direttore che sono pronto
//     printf("[%s] Avviso il direttore che sono pronto e mi metto in attesa.\n", erogatoreID);
//     sem_post(&shm_ptr->sem_ready);

//     // Mi metto in attesa
//     sem_wait(&shm_ptr->sem_start);

//     // Posso iniziare a lavorare
//     printf("[%s] Inizio a lavorare.\n", erogatoreID);
//     sleep(2);

//     // Pulizia
//     shmdt(shm_ptr);

//     return EXIT_SUCCESS;
// }

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *erogatoreID = argv[0];
    int idSem = atoi(argv[1]);
    printf("[%s] Avvio in corso. PID = %d\n", erogatoreID, getpid());

    // collegamento con semaforo
    if (semget(idSem, 1, 0666) == -1) {
        // errore
        printf("[%s] Errore nel collegamento del semaforo.\n", erogatoreID);
        exit(EXIT_FAILURE);
    }

    printf("[%s] Collegamento al semaforo riuscito.\n", erogatoreID);

    // decremento il semaforo
    sops.sem_num = 0; // numero del semaforo (nell'insieme)
    sops.sem_op = -1; // operazione da eseguire
    sops.sem_flg = 0; // flag
    semop(idSem, &sops, 1);

    printf("[%s] Decremento del semaforo riuscito.\n", erogatoreID);
    

    // Avviso il direttore che sono pronto
    // printf("[%s] Avviso il direttore che sono pronto e mi metto in attesa.\n", erogatoreID);
    // sem_post(&shm_ptr->sem_ready);

    // // Mi metto in attesa
    // sem_wait(&shm_ptr->sem_start);

    // // Posso iniziare a lavorare
    // printf("[%s] Inizio a lavorare.\n", erogatoreID);
    // sleep(2);

    return EXIT_SUCCESS;
}