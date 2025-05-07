#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>
#include "../lib/SemsLib.h"
#include "../headers/SharedMemory.h"
#include "../lib/StatsLib.h"

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

#define NUM_OF_SEM 5

#define MINUTES_FOR_DAY 480 // 480 minuti = 8 ore
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

int messageQueueCreate() {
    int msgID = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (msgID < 0) {
        perror("[Direttore] Errore durante la creazione della coda dei messaggi.\n");
        exit(EXIT_FAILURE);
    }

    return msgID;
}

int RandomizeService() {
    return rand() % NUM_SERVIZI;
    // return 1; // Per testare il servizio 1
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
        stats->sportelli_esistenti_services[config->sportelli[i].indexServizioOfferto] += 1;

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

void MasterNotifyAndWait(int semID, struct sembuf sops, DailyConfig* config, Stats* stats, bool endDay, bool endSim, bool printStats, char* csvPath, char* direttoreID) {
    // aspettiamo che arrivi a 0 (ovvero tutti i figli sono pronti e hanno decrementato il semaforo)
    ExecuteSemop(semID, sops, 0, 0);
    
    // Possibile lettura delle statistiche qui
    // TODO: check explode threshold

    if (printStats) {
        CalculateDailyStats(stats, semID);
        PrintDailyStats(stats);
        WriteDailyStatsCSV(csvPath, stats);
    }

    if (endDay) {
        // Resettiamo il semaforo tra operatori e utenti
        SemRestart(semID, sops, NUM_SPORTELLI, 1, 1, direttoreID);
    }
    
    if (endDay && !endSim) {
        // configuriamo gli sportelli
        ConfigurePostOffices(config, stats);
    }

    if (!endSim) {
        // avvisiamo i figli che possono partire
        ExecuteSemop(semID, sops, 1, -1);
    }
    
    // Resettiamo il semaforo della barriera
    SemBarrierRestart(semID, sops, TOTAL_PROCESSES, 1, direttoreID);
}

// Aspettiamo tutti i processi finiscano di lavorare
void waitFinishAllSubProcess() {
    int i;
    for (i = 0; i < TOTAL_PROCESSES; i++) {
        wait(NULL);
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
    SharedMemoryCleanConfig(shmID, config);
    SharedMemoryCleanStats(shmIdStat, stats);
}

int main(int argc, char *argv[]) {
    struct sembuf sops;

    readConfig(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);

    char* direttoreID = "Direttore";
    char* csvPath = "Stats.csv";

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
    DailyConfig* config = (DailyConfig*)SharedMemoryAttachGeneral(shmID, "Direttore");

    int shmIdStats = SharedMemoryCreate(sizeof(Stats));
    Stats* stats = (Stats*)SharedMemoryAttachGeneral(shmIdStats, "Direttore");
    
    // creiamo il semaforo e inizializziamolo
    int semID = semCreate(NUM_OF_SEM, direttoreID);
    semInizialize(semID, TOTAL_PROCESSES, 1, NUM_SPORTELLI, 1, 1, direttoreID);
    
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
    MasterNotifyAndWait(semID, sops, config, stats, true, false, false, csvPath, direttoreID);

    bool endSim = false;

    // Scorriamo i giorni e avvisiamo ogni volta i figli quando finisce un giorno
    for (int giorni = 1; giorni <= SIM_DURATION; giorni++) {
        printf("[Direttore] Inizio del giorno %d...\n", giorni);

        // La simulazione continua, resettiamo le statistiche daily
        ResetStatsDaily(stats, semID);

        // Diamo il tempo ai figli per configurarsi per il nuovo giorno
        MasterNotifyAndWait(semID, sops, config, stats, false, false, false, csvPath, direttoreID);

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
        MasterNotifyAndWait(semID, sops, config, stats, true, endSim, true, csvPath, direttoreID);
    }

    // Fermiamo la simulazione
    printf("[Direttore] Simulazione finita, mando il segnale di terminazione a tutti i figli...\n");
    kill(0, SIGTERM); // Fine simulazione

    // aspettiamo che tutti i processi finiscano la loro esecuzione
    waitFinishAllSubProcess();

    printf("[Direttore] Tutti i processi sono terminati, avvio la pulizia.\n");

    CalculateFinalStats(stats, semID);
    PrintFinalStats(stats);
    WriteFinalStatsCSV(csvPath, stats);
    
    // Pulizia
    Clean(msgIdDispenser, msgIdOperator, msgIdUser, semID, shmID, config, shmIdStats, stats);

    printf("[Direttore] Fine della simulazione.\n");

    return EXIT_SUCCESS;
}
