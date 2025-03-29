#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <semaphore.h>
#include "../headers/servizi.h"

#define NUM_OF_WORKERS 3
#define NUM_OF_USERS 5
#define TOTAL_PROCESSES (1 + NUM_OF_WORKERS + NUM_OF_USERS)
#define TOTAL_PROCESSES_DIR (1 + TOTAL_PROCESSES)

// Creazione di un semaforo
int semCreate() {
    int semID = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    if (semID < 0) {
        printf("[Direttore] Creazione del semaforo fallita.\n");
        exit(EXIT_FAILURE);
    }
    
    return semID;
}

int RandomizeService() {
    return rand() % NUM_SERVIZI;
}

void CreateProcess(const char *path, char *const argv[]) {
    pid_t pid = fork();

    if (pid < 0) {
        printf("[Direttore] Errore nella fork\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        execve(path, argv, NULL);
        // Se siamo qui, c'è stato un errore
        printf("[Direttore] Errore nella execve\n");
        exit(EXIT_FAILURE);
    }
}

void createAllSubProcess(int semID) {
    // Creiamo l'erogatore dei ticket
    char semID_str[15];
    sprintf(semID_str, "%d", semID);

    char *erogatore_ticket_args[] = {"Erogatore_ticket", semID_str, NULL};
    CreateProcess("./bin/erogatore_ticket", erogatore_ticket_args);

    int i;
    char id_buffer[50];  // Buffer per gli ID dinamici

    // Creiamo tutti gli operatori
    for (i = 0; i < NUM_OF_WORKERS; i++) {
        sprintf(id_buffer, "Operator_%d", i);
        char *operatore_args[] = {id_buffer, semID_str, RandomizeService(), NULL};
        CreateProcess("./bin/operatore", operatore_args);
    }

    // Creiamo tutti gli utenti
    for (i = 0; i < NUM_OF_USERS; i++) {
        sprintf(id_buffer, "User_%d", i);
        char *utente_args[] = {id_buffer, semID_str, NULL};
        CreateProcess("./bin/utente", utente_args);
    }
}

void notifyAndWait(int semID, struct sembuf sops) {
    // decremento il semaforo di 1 per farlo arrivare a 0
    sops.sem_num = 0;
    sops.sem_op = -1;
    semop(semID, &sops, 1);

    // aspettiamo che arrivi a 0
    sops.sem_num = 0;
    sops.sem_op = 0;
    semop(semID, &sops, 1);
}

// Aspettiamo tutti i processi finiscano di lavorare
void waitFinishAllSubProcess() {
    int i;
    for (i = 0; i < TOTAL_PROCESSES; i++) {
        wait(NULL);
    }
}

// Pulizia dei semafori
void semCleanUp(int semid) {
    // Rimuovi il set di semafori
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    struct sembuf sops;

    printf("[Direttore] Avvio della simulazione. PID: %d\n", getpid());

    // creiamo il semaforo e inizializziamolo
    int semID = semCreate();
    semctl(semID, 0, SETVAL, TOTAL_PROCESSES_DIR);

    // creiamo tutti i figli
    createAllSubProcess(semID);

    // aspettiamo che tutti i figli siano pronti e diamogli il via
    notifyAndWait(semID, sops);

    // aspettiamo che tutti i processi finiscano la loro esecuzione
    waitFinishAllSubProcess();

    printf("[Direttore] Tutti i processi sono terminati, avvio la pulizia.\n");
    
    // Pulizia del semaforo
    semCleanUp(semID);

    printf("[Direttore] Fine della simulazione.\n");

    return EXIT_SUCCESS;
}
