#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include "../headers/servizi.h"
#include "../headers/SharedMemory.h"

#define NUM_OF_WORKERS 3
#define NUM_OF_USERS 5
#define TOTAL_PROCESSES (1 + NUM_OF_WORKERS + NUM_OF_USERS)
#define TOTAL_PROCESSES_DIR (1 + TOTAL_PROCESSES)
#define NOF_PAUSE 0 //3 temporaneo, per non far andare in pausa l'operatore

#define NUM_OF_SEM 3

#define SIM_DURATION 3
#define MINUTES_FOR_DAY 480 // 480 minuti = 8 ore
#define SIMULATED_MINUTE 100000000 // 100 milioni di nanosecondi = 100ms

// Signal handler per l'inizio giornata (SIGUSR1)
void handle_day_start(int signo) {
    printf("[Direttore] Ricevuto SIGUSR1: non faccio niente perchè sono il direttore.\n");
}

// Creazione della memoria condivisa
int SharedMemoryCreate() {
    int shmID = shmget(IPC_PRIVATE, sizeof(DailyConfig), IPC_CREAT | 0666);
    if (shmID < 0) {
        printf("[Direttore] Creazione della memoria condivisa fallita.\n");
        exit(EXIT_FAILURE);
    }
    
    return shmID;
}

// Collegamento alla memoria condivisa
DailyConfig* SharedMemoryAttach(int shmID) {
    DailyConfig* config = (DailyConfig*)shmat(shmID, NULL, 0);
    if (config == (void *) -1) {
        printf("[Direttore] Collegamento alla memoria condivisa fallita.\n");
        exit(EXIT_FAILURE);
    }

    return config;
}

// Pulizia della memoria condivisa
void SharedMemoryClean(int shmID, DailyConfig* config) {
    shmdt(config);
    shmctl(shmID, IPC_RMID, NULL);
}

// Creazione dei semafori
int semCreate() {
    int semID = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);
    if (semID < 0) {
        printf("[Direttore] Creazione del semaforo fallita.\n");
        exit(EXIT_FAILURE);
    }
    
    return semID;
}

void semInizialize(int semID) {
    // semNum = 0 : semaforo per gestire la barriera di partenza dei processi
    // all'inizio, contiene il numero di tutti i processi, quando arriverà a zero la simulazione partirà
    if (semctl(semID, 0, SETVAL, TOTAL_PROCESSES_DIR) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo dedicato alla barriera.\n");
        exit(EXIT_FAILURE);
    }

    // semNum = 1 : semaforo per gestire se ci sono sportelli liberi
    // all'inizio, tutti gli sportelli sono liberi
    if (semctl(semID, 1, SETVAL, NUM_SPORTELLI) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo dedicato agli sportelli.\n");
        exit(EXIT_FAILURE);
    }

    // semNum = 2 : semaforo per coordinare l'accesso singolo agli operatori per provare ad occupare uno sportello, in modo che vada uno per volta
    // all'inizio, il semaforo vale 1, quindi il primo operatore può provare ad occupare uno sportello
    if (semctl(semID, 2, SETVAL, 1) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo per l'accesso coordinato agli sportelli.\n");
        exit(EXIT_FAILURE);
    }
}

int messageQueueCreate() {
    int msgID = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (msgID < 0) {
        perror("[Direttore] Errore durante la creazione della coda dei messaggi.\n");
        exit(EXIT_FAILURE);
    }

    return msgID;
}

int RandomizeService() {
    //return rand() % NUM_SERVIZI;
    return 1; // Per testare il servizio 1
}

void ConfigurePostOffices(DailyConfig* config) {
    for (int i = 0; i < NUM_SPORTELLI; i++) {
        config->sportelli[i].idSportello = i;
        config->sportelli[i].indexServizioOfferto = RandomizeService();
        config->sportelli[i].idOperatore = "";
        config->sportelli[i].disponibile = 1;

        printf("[Direttore] Sportello %d è stato creato con il servizio %d.\n", config->sportelli[i].idSportello, config->sportelli[i].indexServizioOfferto);
    }
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

void createAllSubProcess(int shmID, int semID, int msgIdDispenser, int msgIdOperator, int msgIdUser) {
    // Creiamo l'erogatore dei ticket
    char semID_str[15];
    sprintf(semID_str, "%d", semID);

    char msgIdDispenser_str[15];
    sprintf(msgIdDispenser_str, "%d", msgIdDispenser);

    char msgIdOperator_str[15];
    sprintf(msgIdOperator_str, "%d", msgIdOperator);

    char msgIdUser_str[15];
    sprintf(msgIdUser_str, "%d", msgIdUser);

    // Creiamo l'erogatore per i ticket
    char *erogatore_ticket_args[] = {"Erogatore_ticket", semID_str, msgIdDispenser_str, msgIdOperator_str, NULL};
    CreateProcess("./bin/erogatore_ticket", erogatore_ticket_args);

    int i;
    char id_buffer[50];  // Buffer per gli ID dinamici
    char shmID_str[15];
    sprintf(shmID_str, "%d", shmID);
    char random_service[10];

    char nof_pause_str[3];
    sprintf(nof_pause_str, "%d", NOF_PAUSE);

    // Creiamo tutti gli operatori
    for (i = 0; i < NUM_OF_WORKERS; i++) {
        sprintf(id_buffer, "Operator_%d", i);
        sprintf(random_service, "%d", RandomizeService());
        char *operatore_args[] = {id_buffer, shmID_str, semID_str, msgIdOperator_str, msgIdUser_str, random_service, nof_pause_str, NULL};
        CreateProcess("./bin/operatore", operatore_args);
    }

    // Creiamo tutti gli utenti
    for (i = 0; i < NUM_OF_USERS; i++) {
        sprintf(id_buffer, "User_%d", i);
        char *utente_args[] = {id_buffer, shmID_str, semID_str, msgIdDispenser_str, msgIdUser_str, NULL};
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

void messageQueueClean(int msgIdDispenser, int msgIdOperator, int msgIdUser) {
    msgctl(msgIdDispenser, IPC_RMID, NULL);
    msgctl(msgIdOperator, IPC_RMID, NULL);
    msgctl(msgIdUser, IPC_RMID, NULL);
}

void Clean(int msgIdDispenser, int msgIdOperator, int msgIdUser, int semID, int shmID, DailyConfig* config) {
    messageQueueClean(msgIdDispenser, msgIdOperator, msgIdUser);
    semCleanUp(semID);
    SharedMemoryClean(shmID, config);
} 

// Mandiamo ai figli il segnale per avvisarli che è iniziato il giorno lavorativo
void SendStartOfDaySignal() {
    kill(0, SIGUSR1);
}

// Mandiamo ai figli il segnale per avvisarli che è finito il giorno lavorativo
void SendEndOfDaySignal() {
    kill(0, SIGUSR2);
}

// Mandiamo ai figli il segnale per avvisarli che è terminata la simulazione
void SendEndOfSimulationSignal() {
    kill(0, SIGTERM);
}

int main(int argc, char *argv[]) {
    struct sembuf sops;

    // Inizializza il generatore di numeri casuali
    srand(time(NULL));

    printf("[Direttore] Avvio della simulazione. PID: %d\n", getpid());

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

    // creiamo la memoria condivisa e colleghiamoci
    int shmID = SharedMemoryCreate();
    DailyConfig* config = SharedMemoryAttach(shmID);

    // creiamo il semaforo e inizializziamolo
    int semID = semCreate();
    semInizialize(semID);

    // creiamo le due code per i messaggi per la comunicazione tra utente-erogatore e erogatore-operatore
    // Coda utente --> erogatore
    int msgIdDispenser = messageQueueCreate();

    // Coda erogatore --> operatore
    int msgIdOperator = messageQueueCreate();

    // Coda operatore --> utente
    int msgIdUser = messageQueueCreate();
    
    // configuriamo gli sportelli
    ConfigurePostOffices(config);

    // creiamo tutti i figli
    createAllSubProcess(shmID, semID, msgIdDispenser, msgIdOperator, msgIdUser);

    // aspettiamo che tutti i figli siano pronti e diamogli il via
    notifyAndWait(semID, sops);

    // Scorriamo i giorni e avvisiamo ogni volta i figli quando finisce un giorno
    for (int giorni = 1; giorni <= SIM_DURATION; giorni++) {
        printf("[Direttore] Inizio del giorno %d...\n", giorni);
        SendStartOfDaySignal();
        sleep(20);
        printf("[Direttore] Avviso i figli che è terminato il giorno %d...\n", giorni);
        //SendEndOfDaySignal();
    }

    // Fermiamo la simulazione
    printf("[Direttore] Simulazione finita, mando il segnale di terminazione a tutti i figli...\n");
    //SendEndOfSimulationSignal();

    // aspettiamo che tutti i processi finiscano la loro esecuzione
    waitFinishAllSubProcess();

    printf("[Direttore] Tutti i processi sono terminati, avvio la pulizia.\n");
    
    // Pulizia
    Clean(msgIdDispenser, msgIdOperator, msgIdUser, semID, shmID, config);

    printf("[Direttore] Fine della simulazione.\n");

    return EXIT_SUCCESS;
}
