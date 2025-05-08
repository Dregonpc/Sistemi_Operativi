#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>
#include "../headers/SemsLib.h"
#include "../headers/SharedMemory.h"
#include "../headers/MessageQueueLib.h"
#include "../headers/StatsLib.h"

/*  Global Var  */
int NUM_OF_WORKERS;
int NUM_OF_USERS;
int NUM_SPORTELLI;
int TOTAL_PROCESSES;
int TOTAL_PROCESSES_DIR;
int NOF_PAUSE;  // Numero di pause che un operatore può fare in tutta la simulazione
int N_REQUEST;
int P_SERV_MIN; // Probabilità per l'utente
int P_SERV_MAX; // Probabilità per l'utente
int SIM_DURATION; // Durata della simulazione in giorni
int EXPLODE_THRESHOLD;  // max numero di utenti a fine giornata che non sono stati serviti --> se supera la soglia termina la simulazione

#define MINUTES_FOR_DAY 480 // 480 minuti = 8 ore
#define SIMULATED_MINUTE 4000000 // 4 milioni di nanosecondi = 4ms
// 4ms * 480 = 1,92 secondi

// Handler per tutti i segnali
static void signalHandler(int signo) {
    // Do nothing
}

void readConfig(char *numOfWorkers, char *numOfUsers, char *numSportelli, char *nofPause, char *nRequest, char *pServMin, char *pServMax, char *simDuration, char *explodeThreshold) {
    NUM_OF_WORKERS = atoi(numOfWorkers);
    NUM_OF_USERS = atoi(numOfUsers);
    NUM_SPORTELLI = atoi(numSportelli);
    NOF_PAUSE = atoi(nofPause);
    N_REQUEST = atoi(nRequest);
    P_SERV_MIN = atoi(pServMin);
    P_SERV_MAX = atoi(pServMax);
    SIM_DURATION = atoi(simDuration);
    EXPLODE_THRESHOLD = atoi(explodeThreshold);
    TOTAL_PROCESSES = 1 + NUM_OF_WORKERS + NUM_OF_USERS;
    TOTAL_PROCESSES_DIR = 1 + TOTAL_PROCESSES;
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
    // Creiamo le stringhe da passare ai figli
    char semID_str[15], msgIdDispenser_str[15], msgIdOperator_str[15], msgIdUser_str[15], id_buffer[50], shmID_str[15], shmIdStats_str[15], random_service[10], n_request_str[10], p_serv_min_str[10], p_serv_max_str[10], timeDay[20], simulated_minute_str[20], nof_pause_str[3];
    sprintf(semID_str, "%d", semID);
    sprintf(msgIdDispenser_str, "%d", msgIdDispenser);
    sprintf(msgIdOperator_str, "%d", msgIdOperator);
    sprintf(msgIdUser_str, "%d", msgIdUser);
    sprintf(shmID_str, "%d", shmID);
    sprintf(shmIdStats_str, "%d", shmIdStats);
    sprintf(n_request_str, "%d", N_REQUEST);
    sprintf(p_serv_min_str, "%d", P_SERV_MIN);
    sprintf(p_serv_max_str, "%d", P_SERV_MAX);
    sprintf(timeDay, "%d", CalculateTimeDayUser());
    sprintf(simulated_minute_str, "%d", SIMULATED_MINUTE);
    sprintf(nof_pause_str, "%d", NOF_PAUSE);

    // Creiamo l'erogatore per i ticket
    char *erogatore_ticket_args[] = {"Erogatore_ticket", semID_str, msgIdDispenser_str, msgIdOperator_str, NULL};
    CreateProcess("./bin/erogatore_ticket", erogatore_ticket_args);

    // Creiamo tutti gli operatori
    for (int i = 0; i < NUM_OF_WORKERS; i++) {
        sprintf(id_buffer, "Operator_%d", i);
        sprintf(random_service, "%d", RandomizeService());
        char *operatore_args[] = {id_buffer, shmID_str, shmIdStats_str, semID_str, msgIdOperator_str, msgIdUser_str, random_service, nof_pause_str, simulated_minute_str, NULL};
        CreateProcess("./bin/operatore", operatore_args);
    }

    // Creiamo tutti gli utenti
    for (int i = 0; i < NUM_OF_USERS; i++) {
        sprintf(id_buffer, "User_%d", i);
        char *utente_args[] = {id_buffer, shmID_str, shmIdStats_str, semID_str, msgIdDispenser_str, msgIdUser_str, n_request_str, p_serv_min_str, p_serv_max_str, timeDay, NULL};
        CreateProcess("./bin/utente", utente_args);
    }
}

bool CheckThreshold(Stats* stats) {
    if (stats->servizi_non_erogati_tot_day > EXPLODE_THRESHOLD) {
        stats->termine_simulazione = "EXPLODE_THRESHOLD";
        printf("[Direttore] Il numero di utenti in attesa a fine giornata è maggiore della soglia prevista, termino la simulazione.\n");
        return true;
    }

    return false;
}

void MasterNotifyAndWait(int semID, struct sembuf sops, DailyConfig* config, Stats* stats, bool endDay, bool* endSim, bool printStats, char* csvPath, char* direttoreID, int msgIdDispenser, int msgIdOperator, int msgIdUser) {
    // aspettiamo che arrivi a 0 (ovvero tutti i figli sono pronti e hanno decrementato il semaforo)
    ExecuteSemop(semID, sops, 0, 0);
    
    if (printStats) {
        CalculateDailyStats(stats, semID);
        PrintDailyStats(stats);
        WriteDailyStatsCSV(csvPath, stats);
    }

    if (endDay) {
        // Check explode threshold
        if (CheckThreshold(stats)) {
            *endSim = true;
            return;
        }

        // Resettiamo il semaforo tra operatori e utenti
        SemRestart(semID, sops, NUM_SPORTELLI, 1, 1, direttoreID);

        // Puliamo le code di messaggi per il nuovo giorno
        cleanMsgQueue(msgIdDispenser);
        cleanMsgQueue(msgIdOperator);
        cleanMsgQueue(msgIdUser);
    }
    
    if (endDay && !(*endSim)) {
        // configuriamo gli sportelli
        ConfigurePostOffices(config, stats);
    }

    if (!(*endSim)) {
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

void Clean(int msgIdDispenser, int msgIdOperator, int msgIdUser, int semID, int shmID, DailyConfig* config, int shmIdStat, Stats* stats) {
    messageQueueRemove(msgIdDispenser);
    messageQueueRemove(msgIdOperator);
    messageQueueRemove(msgIdUser);

    semCleanUp(semID);
    SharedMemoryCleanConfig(shmID, config);
    SharedMemoryCleanStats(shmIdStat, stats);
}

int main(int argc, char *argv[]) {
    struct sembuf sops;

    readConfig(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8], argv[9]);

    char* direttoreID = "Direttore";
    char* csvPath = "Stats.csv";

    // Inizializza il generatore di numeri casuali
    srand(time(NULL) + getpid());

    printf("[Direttore] Avvio della simulazione. PID: %d\n", getpid());

    // Installa i signal handler
    struct sigaction sa_end = {0};
    sa_end.sa_handler = signalHandler;
    sigemptyset(&sa_end.sa_mask);
    sa_end.sa_flags = 0;
    sigaction(SIGUSR2, &sa_end, NULL);

    struct sigaction sa_term = {0};
    sa_term.sa_handler = signalHandler;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);

    // creiamo la memoria condivisa e colleghiamoci
    size_t size = sizeof(DailyConfig) + NUM_SPORTELLI * sizeof(Sportello);
    int shmID = SharedMemoryCreate(size, 0666, direttoreID);
    DailyConfig* config = (DailyConfig*)SharedMemoryAttachGeneral(shmID, direttoreID);
    config->num_sportelli = NUM_SPORTELLI;

    int shmIdStats = SharedMemoryCreate(sizeof(Stats), 0666, direttoreID);
    Stats* stats = (Stats*)SharedMemoryAttachGeneral(shmIdStats, direttoreID);
    
    // creiamo il semaforo e inizializziamolo
    int semID = semCreate(0666, direttoreID);
    semInizialize(semID, TOTAL_PROCESSES, 1, NUM_SPORTELLI, 1, 1, direttoreID);
    
    // inizializziamo le statistiche
    StatsInitialize(stats, semID);

    // creiamo le due code per i messaggi per la comunicazione tra utente-erogatore e erogatore-operatore
    // Coda utente --> erogatore
    int msgIdDispenser = messageQueueCreate(0666, direttoreID);

    // Coda erogatore --> operatore
    int msgIdOperator = messageQueueCreate(0666, direttoreID);

    // Coda operatore --> utente
    int msgIdUser = messageQueueCreate(0666, direttoreID);

    // creiamo tutti i figli
    createAllSubProcess(shmID, shmIdStats, semID, msgIdDispenser, msgIdOperator, msgIdUser);

    bool endSim = false;
    
    // aspettiamo che tutti i figli siano pronti e diamogli il via
    MasterNotifyAndWait(semID, sops, config, stats, true, &endSim, false, csvPath, direttoreID, msgIdDispenser, msgIdOperator, msgIdUser);

    // Scorriamo i giorni e avvisiamo ogni volta i figli quando finisce un giorno
    for (int giorni = 1; giorni <= SIM_DURATION && !endSim; giorni++) {
        printf("[Direttore] Inizio del giorno %d...\n", giorni);

        // La simulazione continua, resettiamo le statistiche daily
        ResetStatsDaily(stats, semID);

        // Diamo il tempo ai figli per configurarsi per il nuovo giorno
        MasterNotifyAndWait(semID, sops, config, stats, false, &endSim, false, csvPath, direttoreID, msgIdDispenser, msgIdOperator, msgIdUser);

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
        MasterNotifyAndWait(semID, sops, config, stats, true, &endSim, true, csvPath, direttoreID, msgIdDispenser, msgIdOperator, msgIdUser);
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
