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
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>
#include "../headers/servizi.h"
#include "../headers/SharedMemory.h"

/*  Global Var  */
int NUM_OF_WORKERS;
int NUM_OF_USERS;
int TOTAL_PROCESSES;
int TOTAL_PROCESSES_DIR;

int NOF_PAUSE;  // Numero di pause che un operatore può fare in tutta la simulazione

// Probabilità per l'utente
int P_SERV_MIN;
int P_SERV_MAX;

int SIM_DURATION; // Durata della simulazione in giorni
int EXPLODE_THRESHOLD;  // max numero di utenti a fine giornata che non sono stati serviti --> se supera la soglia termina la simulazione

#define NUM_OF_SEM 3

#define MINUTES_FOR_DAY 480 // 480 minuti = 8 ore
//#define SIMULATED_MINUTE 100000000 // 100 milioni di nanosecondi = 100ms
//#define SIMULATED_MINUTE 50000000 // 50 milioni di nanosecondi = 50ms
#define SIMULATED_MINUTE 4000000 // 4 milioni di nanosecondi = 4ms
// 4ms * 480 = 1,92 secondi

// Handler per tutti i segnali
static void signalHandler(int signo) {
    // Do nothing
}

void readConfig(char *numOfWorkers, char *numOfUsers, char *nofPause, char *pServMin, char *pServMax, char *simDuration, char *explodeThreshold) {
    NUM_OF_WORKERS = atoi(numOfWorkers);
    NUM_OF_USERS = atoi(numOfUsers);
    NOF_PAUSE = atoi(nofPause);
    P_SERV_MIN = atoi(pServMin);
    P_SERV_MAX = atoi(pServMax);
    SIM_DURATION = atoi(simDuration);
    EXPLODE_THRESHOLD = atoi(explodeThreshold);

    TOTAL_PROCESSES = 1 + NUM_OF_WORKERS + NUM_OF_USERS;
    TOTAL_PROCESSES_DIR = 1 + TOTAL_PROCESSES;

}

// Creazione della memoria condivisa
int SharedMemoryCreate(size_t sizeStruct) {
    int shmID = shmget(IPC_PRIVATE, sizeStruct, IPC_CREAT | 0666);
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

// Collegamento alla memoria condivisa
Stats* SharedMemoryAttachStats(int shmID) {
    Stats* stats = (Stats*)shmat(shmID, NULL, 0);
    if (stats == (void *) -1) {
        printf("[Direttore] Collegamento alla memoria condivisa fallita.\n");
        exit(EXIT_FAILURE);
    }

    return stats;
}

// Pulizia della memoria condivisa
void SharedMemoryClean(int shmID, DailyConfig* config, int shmIdStat, Stats* stats) {
    shmdt(config);
    shmctl(shmID, IPC_RMID, NULL);

    shmdt(stats);
    shmctl(shmIdStat, IPC_RMID, NULL);
}

// Creazione dei semafori
int semCreate() {
    int semID = semget(IPC_PRIVATE, 6, IPC_CREAT | 0666);
    if (semID < 0) {
        printf("[Direttore] Creazione del semaforo fallita.\n");
        exit(EXIT_FAILURE);
    }
    
    return semID;
}

void semInizialize(int semID) {
    // semNum = 0 : semaforo per gestire la barriera di partenza dei processi
    // all'inizio, contiene il numero di tutti i processi, quando arriverà a zero la simulazione partirà
    if (semctl(semID, 0, SETVAL, TOTAL_PROCESSES) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo dedicato alla barriera.\n");
        exit(EXIT_FAILURE);
    }

    // semNum = 1 : semaforo per gestire lo start (ovvero i figli possono partire)
    // all'inizio, vale 1, tutti i figli aspettano che diventi 0
    if (semctl(semID, 1, SETVAL, 1) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo dedicato allo start.\n");
        exit(EXIT_FAILURE);
    }

    // semNum = 2 : semaforo per gestire se ci sono sportelli liberi
    // all'inizio, tutti gli sportelli sono liberi
    if (semctl(semID, 2, SETVAL, NUM_SPORTELLI) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo dedicato agli sportelli.\n");
        exit(EXIT_FAILURE);
    }

    // semNum = 3 : semaforo per coordinare l'accesso singolo agli operatori per provare ad occupare uno sportello, in modo che vada uno per volta
    // all'inizio, il semaforo vale 1, quindi il primo operatore può provare ad occupare uno sportello
    if (semctl(semID, 3, SETVAL, 1) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo per l'accesso coordinato agli sportelli.\n");
        exit(EXIT_FAILURE);
    }

    // semNum = 4 : semaforo per avvisare gli utenti che possono chiedere un servizio perchè tutti gli operatori hanno provato a occupare uno sportello
    // all'inizio, il semaforo vale come il numero di operatori, che andranno a decrementarlo in modo che quando arriva a 0 gli utenti possano iniziare a chiedere i servizi (si resetta ogni giornata)
    //if (semctl(semID, 4, SETVAL, NUM_OF_WORKERS) < 0) {
    if (semctl(semID, 4, SETVAL, 0) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo per la sincronizzazione tra operatori e utenti.\n");
        exit(EXIT_FAILURE);
    }

    // semNum = 5 : semaforo per gestire il lock per le statistiche
    // vale sempre 1, quando qualcuno acquisice il lock diventa 0 e nessuno ci può più accedere finchè non torna a valere 1
    if (semctl(semID, 5, SETVAL, 1) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo per il lock delle statistiche.\n");
        exit(EXIT_FAILURE);
    }
}

void SemBarrierRestart(int semID, struct sembuf sops) {
    if (semctl(semID, 0, SETVAL, TOTAL_PROCESSES) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo dedicato alla barriera.\n");
        exit(EXIT_FAILURE);
    }

    if (semctl(semID, 1, SETVAL, 1) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo dedicato allo start.\n");
        exit(EXIT_FAILURE);
    }
}

void SemOperatorsUsersRestart(int semID, struct sembuf sops) {
    if (semctl(semID, 2, SETVAL, NUM_SPORTELLI) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo dedicato agli sportelli.\n");
        exit(EXIT_FAILURE);
    }

    if (semctl(semID, 3, SETVAL, 1) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo per l'accesso coordinato agli sportelli.\n");
        exit(EXIT_FAILURE);
    }
    
    if (semctl(semID, 4, SETVAL, NUM_OF_WORKERS) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo per la sincronizzazione tra operatori e utenti.\n");
        exit(EXIT_FAILURE);
    }

    if (semctl(semID, 5, SETVAL, 1) < 0) {
        perror("[Direttore] Errore durante la semctl del semaforo per il lock delle statistiche.\n");
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

int CalculateTimeDayUser() {
    return SIMULATED_MINUTE * MINUTES_FOR_DAY;
}

void ConfigurePostOffices(DailyConfig* config, Stats* stats) {
    for (int i = 0; i < NUM_SPORTELLI; i++) {
        config->sportelli[i].idSportello = i;
        config->sportelli[i].indexServizioOfferto = RandomizeService();
        config->sportelli[i].idOperatore = "";
        config->sportelli[i].disponibile = 1;

        // Inserisco dati dello sportello nella stats
        stats->sportelli_esistenti_services[config->sportelli[i].indexServizioOfferto]++;


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

void createAllSubProcess(int shmID, int shmIdStats, int semID, int msgIdDispenser, int msgIdOperator, int msgIdUser) {
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
    char shmIdStats_str[15];
    sprintf(shmIdStats_str, "%d", shmIdStats);
    char random_service[10];

    char p_serv_min_str[10];
    sprintf(p_serv_min_str, "%d", P_SERV_MIN);
    char p_serv_max_str[10];
    sprintf(p_serv_max_str, "%d", P_SERV_MAX);
    char timeDay[20];
    sprintf(timeDay, "%d", CalculateTimeDayUser());
    char simulated_minute_str[20];
    sprintf(simulated_minute_str, "%d", SIMULATED_MINUTE);

    char nof_pause_str[3];
    sprintf(nof_pause_str, "%d", NOF_PAUSE);

    // Creiamo tutti gli operatori
    for (i = 0; i < NUM_OF_WORKERS; i++) {
        sprintf(id_buffer, "Operator_%d", i);
        sprintf(random_service, "%d", RandomizeService());
        char *operatore_args[] = {id_buffer, shmID_str, shmIdStats_str, semID_str, msgIdOperator_str, msgIdUser_str, random_service, nof_pause_str, simulated_minute_str, NULL};
        CreateProcess("./bin/operatore", operatore_args);
    }

    // Creiamo tutti gli utenti
    for (i = 0; i < NUM_OF_USERS; i++) {
        sprintf(id_buffer, "User_%d", i);
        char *utente_args[] = {id_buffer, shmID_str, shmIdStats_str, semID_str, msgIdDispenser_str, msgIdUser_str, p_serv_min_str, p_serv_max_str, timeDay, NULL};
        CreateProcess("./bin/utente", utente_args);
    }
}

void PrintFinalStats(Stats* stats) {
    printf("[Direttore] Statistiche finali:\n");
    printf("Utenti serviti totali: %d\n", stats->utenti_serviti_tot_sim);
    printf("Servizi erogati totali: %d\n", stats->servizi_erogati_tot_sim);
    printf("Servizi non erogati totali: %d\n", stats->servizi_non_erogati_tot_sim);
    printf("Operatori attivi totali: %d\n", stats->operatori_attivi_sim);
    printf("Pause effettuate totali: %d\n", stats->pause_effettuate_sim);
    printf("Durata della simulazione: %d giorni\n", stats->durata_simulazione);
    printf("Tempo medio di attesa degli utenti: %.2f nanosecondi\n", stats->tempo_attesa_utenti_sim * 1000000000.0);
    printf("Tempo medio di erogazione dei servizi: %.2f nanosecondi\n", stats->tempo_erogazione_servizi_sim * 1000000000.0);
}

void PrintDailyStats(Stats* stats) {
    printf("[Direttore] Statistiche giornaliere:\n");
    printf("Utenti serviti totali: %d\n", stats->utenti_serviti_tot_sim);
    printf("Servizi erogati totali: %d\n", stats->servizi_erogati_tot_sim);
    printf("Servizi non erogati totali: %d\n", stats->servizi_non_erogati_tot_sim);
    printf("Operatori attivi totali: %d\n", stats->operatori_attivi_day);
    printf("Pause effettuate totali: %d\n", stats->pause_effettuate_sim);
    printf("Tempo medio di attesa degli utenti: %.2f nanosecondi\n", stats->tempo_attesa_utenti_day * 1000000000.0);
    printf("Tempo medio di erogazione dei servizi: %.2f nanosecondi\n", stats->tempo_erogazione_servizi_day * 1000000000.0);
}

void StatsInitialize(Stats* stats, int semID) {
    struct sembuf sops;

    // acquisisco il lock
    sops.sem_num = 5;
    sops.sem_op = -1;
    if (semop(semID, &sops, 1) == -1) {
        printf("[Direttore] Errore durante l'acquisizione del lock per le statistiche.\n");
    }

    // Inizializziamo le statistiche
    stats->utenti_serviti_tot_sim = 0;
    stats->utenti_non_serviti_tot_sim = 0;
    stats->utenti_non_serviti_tot_day = 0;
    stats->servizi_erogati_tot_sim = 0;
    stats->servizi_non_erogati_tot_sim = 0;
    stats->servizi_non_erogati_tot_day = 0;
    stats->operatori_attivi_day = 0;
    stats->operatori_attivi_sim = 0;
    stats->pause_effettuate_sim = 0;
    stats->durata_simulazione = 0;

    // Medie (gestite dal direttore)
    stats->utenti_serviti_avg = 0.0;
    stats->servizi_erogati_avg = 0.0;
    stats->servizi_non_erogati_avg = 0.0;
    stats->pause_effettuate_avg = 0.0;

    // Tempi
    stats->tempo_attesa_utenti_sim = 0.0;
    stats->tempo_attesa_utenti_day = 0.0;
    stats->tempo_erogazione_servizi_sim = 0.0;
    stats->tempo_erogazione_servizi_day = 0.0;

    // Contatori divisi per servizi
    for (int i = 0; i < NUM_SERVIZI; i++) {
        stats->utenti_serviti_tot_sim_services[i] = 0;
        stats->servizi_erogati_tot_sim_services[i] = 0;
        stats->servizi_non_erogati_tot_sim_services[i] = 0;
        stats->utenti_serviti_avg_services[i] = 0.0;
        stats->servizi_erogati_avg_services[i] = 0.0; 
        stats->servizi_non_erogati_avg_services[i] = 0.0; 
        stats->tempo_attesa_utenti_sim_services[i] = 0.0; 
        stats->tempo_attesa_utenti_day_services[i] = 0.0; 
        stats->tempo_erogazione_servizi_sim_services[i] = 0.0; 
        stats->tempo_erogazione_servizi_day_services[i] = 0.0;

        stats->operatori_disponibili_services[i] = 0;
        stats->sportelli_esistenti_services[i] = 0;
        stats->rapporto_operatori_sportelli_services[i] = 0.0;
    }

    // rilascio il lock
    sops.sem_num = 5;
    sops.sem_op = 1;
    if (semop(semID, &sops, 1) == -1) {
        printf("[Direttore] Errore durante il rilascio del lock per le statistiche.\n");
    }

}

void MasterNotifyAndWait(int semID, struct sembuf sops, bool endSim, DailyConfig* config, Stats* stats) {
    // aspettiamo che arrivi a 0 (ovvero tutti i figli sono pronti e hanno decrementato il semaforo)
    sops.sem_num = 0;
    sops.sem_op = 0;
    semop(semID, &sops, 1);
    
    // Possibile lettura delle statistiche qui
    // TODO: check explode threshold

    PrintFinalStats(stats);
    
    // Resettiamo il semaforo tra operatori e utenti
    SemOperatorsUsersRestart(semID, sops);
    
    if (!endSim) {
        // configuriamo gli sportelli
        ConfigurePostOffices(config, stats);

        // avvisiamo i figli che possono partire
        sops.sem_num = 1;
        sops.sem_op = -1;
        semop(semID, &sops, 1);
    }

    // Resettiamo il semaforo della barriera
    SemBarrierRestart(semID, sops);
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

void messageQueueClean(int msgId) {
    msgctl(msgId, IPC_RMID, NULL);
}

void Clean(int msgIdDispenser, int msgIdOperator, int msgIdUser, int semID, int shmID, DailyConfig* config, int shmIdStat, Stats* stats) {
    messageQueueClean(msgIdDispenser);
    messageQueueClean(msgIdOperator);
    messageQueueClean(msgIdUser);

    semCleanUp(semID);
    SharedMemoryClean(shmID, config, shmIdStat, stats);
}

void ResetStatsDaily(Stats* stats, int semID) {
    struct sembuf sops;

    // acquisisco il lock
    sops.sem_num = 5;
    sops.sem_op = -1;
    if (semop(semID, &sops, 1) == -1) {
        printf("[Direttore] Errore durante l'acquisizione del lock per le statistiche.\n");
    }

    // Resettiamo le statistiche
    stats->servizi_non_erogati_tot_day = 0;
    stats->operatori_attivi_day = 0;
    
    stats->tempo_attesa_utenti_day = 0.0;
    stats->tempo_erogazione_servizi_day = 0.0;
    
    for (int i = 0; i < NUM_SERVIZI; i++) {
        stats->tempo_attesa_utenti_day_services[i] = 0.0;
        stats->tempo_erogazione_servizi_day_services[i] = 0.0;
        stats->sportelli_esistenti_services[i] = 0;
    }

    // rilascio il lock
    sops.sem_num = 5;
    sops.sem_op = 1;
    if (semop(semID, &sops, 1) == -1) {
        printf("[Direttore] Errore durante il rilascio del lock per le statistiche.\n");
    }
}

void CalculateFinalStats(Stats* stats, int semID) {
    struct sembuf sops;

    // acquisisco il lock
    sops.sem_num = 5;
    sops.sem_op = -1;
    if (semop(semID, &sops, 1) == -1) {
        printf("[Direttore] Errore durante l'acquisizione del lock per le statistiche.\n");
    }


    // Calcoliamo le statistiche finali
    stats->utenti_serviti_avg = (double)stats->utenti_serviti_tot_sim / (double)stats->durata_simulazione;
    stats->servizi_erogati_avg = (double)stats->servizi_erogati_tot_sim / (double)stats->durata_simulazione;
    stats->servizi_non_erogati_avg = (double)stats->servizi_non_erogati_tot_sim / (double)stats->durata_simulazione;
    stats->pause_effettuate_avg = (double)stats->pause_effettuate_sim / (double)stats->durata_simulazione;

    stats->tempo_attesa_utenti_sim = (double)stats->tempo_attesa_utenti_sim / (double)stats->utenti_serviti_tot_sim;
    stats->tempo_erogazione_servizi_sim = (double)stats->tempo_erogazione_servizi_sim / (double)stats->servizi_erogati_tot_sim;

    // rilascio il lock
    sops.sem_num = 5;
    sops.sem_op = 1;
    if (semop(semID, &sops, 1) == -1) {
        printf("[Direttore] Errore durante il rilascio del lock per le statistiche.\n");
    }
}



int main(int argc, char *argv[]) {
    struct sembuf sops;

    readConfig(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);

    // Inizializza il generatore di numeri casuali
    srand(time(NULL) + getpid());

    printf("[Direttore] Avvio della simulazione. PID: %d\n", getpid());

    // Installa i signal handler
    struct sigaction sa_end = {0};
    sa_end.sa_handler = signalHandler;
    sigemptyset(&sa_end.sa_mask);
    sa_end.sa_flags = 0;                // senza SA_RESTART
    sigaction(SIGUSR2, &sa_end, NULL);

    struct sigaction sa_term = {0};
    sa_term.sa_handler = signalHandler;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);

    // creiamo la memoria condivisa e colleghiamoci
    int shmID = SharedMemoryCreate(sizeof(DailyConfig));
    DailyConfig* config = SharedMemoryAttach(shmID);

    int shmIdStats = SharedMemoryCreate(sizeof(Stats));
    Stats* stats = SharedMemoryAttachStats(shmIdStats);

    
    // creiamo il semaforo e inizializziamolo
    int semID = semCreate();
    semInizialize(semID);
    
    // inizializziamo le statistiche
    StatsInitialize(stats, semID);

    // creiamo le due code per i messaggi per la comunicazione tra utente-erogatore e erogatore-operatore
    // Coda utente --> erogatore
    int msgIdDispenser = messageQueueCreate();

    // Coda erogatore --> operatore
    int msgIdOperator = messageQueueCreate();

    // Coda operatore --> utente
    int msgIdUser = messageQueueCreate();

    // creiamo tutti i figli
    createAllSubProcess(shmID, shmIdStats, semID, msgIdDispenser, msgIdOperator, msgIdUser);

    // aspettiamo che tutti i figli siano pronti e diamogli il via
    MasterNotifyAndWait(semID, sops, false, config, stats);

    bool endSim = false;

    // Scorriamo i giorni e avvisiamo ogni volta i figli quando finisce un giorno
    for (int giorni = 1; giorni <= SIM_DURATION; giorni++) {
        printf("[Direttore] Inizio del giorno %d...\n", giorni);

        // simulo il passare dei minuti
        printf("[Direttore] Giorno %d in corso (480 minuti)...\n", giorni);
        struct timespec req, rem;
        req.tv_sec  = 0;
        req.tv_nsec = SIMULATED_MINUTE;
        for (int minuti = 0; minuti < MINUTES_FOR_DAY; minuti++) {
            rem = req;
            // la syscall nanosleep può essere interrotta da un segnale (EINTR)
            while (nanosleep(&rem, &rem) == -1 && errno == EINTR) {
                // rimane in rem il tempo residuo da dormire
            }
        }

        printf("[Direttore] Avviso i figli che è terminato il giorno %d...\n", giorni);
        kill(0, SIGUSR2); // Fine giornata

        
        // Facciamo ripartire i figli per il nuovo giorno
        endSim = (giorni == SIM_DURATION);
        MasterNotifyAndWait(semID, sops, endSim, config, stats);
    }

    // Fermiamo la simulazione
    printf("[Direttore] Simulazione finita, mando il segnale di terminazione a tutti i figli...\n");
    kill(0, SIGTERM); // Fine simulazione

    // aspettiamo che tutti i processi finiscano la loro esecuzione
    waitFinishAllSubProcess();

    printf("[Direttore] Tutti i processi sono terminati, avvio la pulizia.\n");

    CalculateFinalStats(stats, semID);
    PrintFinalStats(stats);
    
    // Pulizia
    Clean(msgIdDispenser, msgIdOperator, msgIdUser, semID, shmID, config, shmIdStats, stats);

    printf("[Direttore] Fine della simulazione.\n");

    return EXIT_SUCCESS;
}
