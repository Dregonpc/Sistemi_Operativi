#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <semaphore.h>

#define NUM_OF_WORKERS 3
#define NUM_OF_USERS 5
#define TOTAL_PROCESSES (1 + NUM_OF_WORKERS + NUM_OF_USERS)

typedef struct {
    sem_t sem_ready;
    sem_t sem_start;
} SharedData;

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

// Creazione di un semaforo
int semCreate(int key) {
    int semID = semget(IPC_PRIVATE, 2, IPC_CREAT | key);
    if (semID < 0) {
        printf("[Direttore] Creazione del semaforo fallita.\n");
        exit(EXIT_FAILURE);
    }

    return semID;
}

// Colleghiamo la memoria condivisa ??? procedure standard -> Direttore trasformarlo in variabile per messaggi di errore dinamici?
SharedData* shmAttac(int shmID) {
    SharedData *shm_ptr = (SharedData *)shmat(shmID, NULL, 0);
    if (shm_ptr == (void *) -1) {
        printf("[Direttore] Collegamento alla memoria condivisa fallito.\n");
        exit(EXIT_FAILURE);
    }

    return shm_ptr;
}

// Creiamo la memoria condivisa
int shmCreate(int token) {
    int shmID = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | token);
    if (shmID < 0) {
        printf("[Direttore] Creazione della memoria condivisa fallita.\n");
        exit(EXIT_FAILURE);
    }

    return shmID;
}

// Pulizia i semafori
void semCleanUp(int semid) {
    // Rimuovi il set di semafori
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        exit(EXIT_FAILURE);
    }
}

// Pulizia della memoria condivisa
void shmCleanUp(SharedData *shm_ptr, int shmID) {
    shmdt(shm_ptr);
    shmctl(shmID, IPC_RMID, NULL);
}

// Inizializziamo i semafori nella memoria condivisa
void semInitialize(SharedData *shm_ptr) {
    if (sem_init(&shm_ptr->sem_ready, 1, 0) == -1) {
        printf("[Direttore] Inizializzazione del semaforo (sem_ready) fallita.\n");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&shm_ptr->sem_start, 1, 0) == -1) {
        printf("[Direttore] Inizializzazione del semaforo (sem_start) fallita.\n");
        exit(EXIT_FAILURE);
    }
}

void createAllSubProcess(char *shmID_str) {
    // Creiamo l'erogatore dei ticket
    char *erogatore_ticket_args[] = {"Erogatore_ticket", shmID_str, NULL};
    CreateProcess("./bin/erogatore_ticket", erogatore_ticket_args);

    int i;
    char id_buffer[50];  // Buffer per gli ID dinamici

    // Creiamo tutti gli operatori
    for (i = 0; i < NUM_OF_WORKERS; i++) {
        sprintf(id_buffer, "Operator_%d", i);
        char *operatore_args[] = {id_buffer, shmID_str, NULL};
        CreateProcess("./bin/operatore", operatore_args);
    }

    // Creiamo tutti gli utenti
    for (i = 0; i < NUM_OF_USERS; i++) {
        sprintf(id_buffer, "User_%d", i);
        char *utente_args[] = {id_buffer, shmID_str, NULL};
        CreateProcess("./bin/utente", utente_args);
    }
}

// Aspettiamo che i processi avvisino che sono pronti
void waitReadyAllSubProcess(SharedData *shm_ptr) {
    int i;

    printf("[Direttore] Aspetto che tutti i processi siano pronti...\n");
    for (i = 0; i < TOTAL_PROCESSES; i++) {
        sem_wait(&shm_ptr->sem_ready);
    }

    // Sblocchiamo tutti i processi
    printf("[Direttore] Tutti i processi sono pronti, sblocchiamoli.\n");
    for (i = 0; i < TOTAL_PROCESSES; i++) {
        sem_post(&shm_ptr->sem_start);
    }
}

// Aspettiamo tutti i processi finiscano di lavorare
void waitFinishAllSubProcess() {
    int i;
    for (i = 0; i < TOTAL_PROCESSES; i++) {
        wait(NULL);
    }
}

void createAllSubProcessSEM(int semID) {
    // Creiamo l'erogatore dei ticket
    char semID_str[15];
    sprintf(semID_str, "%d", semID);

    char *erogatore_ticket_args[] = {"Erogatore_ticket", semID_str, NULL};
    CreateProcess("./bin/erogatore_ticket", erogatore_ticket_args);

    // int i;
    // char id_buffer[50];  // Buffer per gli ID dinamici

    // // Creiamo tutti gli operatori
    // for (i = 0; i < NUM_OF_WORKERS; i++) {
    //     sprintf(id_buffer, "Operator_%d", i);
    //     char *operatore_args[] = {id_buffer, semID, NULL};
    //     CreateProcess("./bin/operatore", operatore_args);
    // }

    // // Creiamo tutti gli utenti
    // for (i = 0; i < NUM_OF_USERS; i++) {
    //     sprintf(id_buffer, "User_%d", i);
    //     char *utente_args[] = {id_buffer, semID, NULL};
    //     CreateProcess("./bin/utente", utente_args);
    // }
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    printf("[Direttore] Avvio della simulazione. PID: %d\n", getpid());

    // int shmID = shmCreate(0666);
    // SharedData *shm_ptr = shmAttac(shmID);

    // semInitialize(shm_ptr);
    
    // Convertiamo id della memoria in stringa
    // char shmID_str[15];
    // sprintf(shmID_str, "%d", shmID);

    // createAllSubProcess(shmID_str);

    // waitReadyAllSubProcess(shm_ptr);

    //---- con semaforo ----

    int semID = semCreate(0666);

    semctl(semID, 0, SETVAL, TOTAL_PROCESSES);

    int valSem = semctl(semID, 0, GETVAL);

    printf("[Direttore] Valore del semaforo: %d\n", valSem);

    createAllSubProcessSEM(semID);

    //waitFinishAllSubProcess();

    printf("[Direttore] Tutti i processi sono terminati, avvio la pulizia.\n");
    
    // Pulizia
    semCleanUp(semID);
    printf("Set di semafori con ID %d rimosso con successo.\n", semID);

    // shmCleanUp(shm_ptr, shmID);

    printf("[Direttore] Fine della simulazione.\n");

    return EXIT_SUCCESS;
}
