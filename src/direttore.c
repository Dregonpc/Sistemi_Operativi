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

void CreateUsers(int shmID, int shmIdStats, int semID, int msgIdDispenser, int msgIdUser, int counter, int oldUser, int IsNormalUser) {
    // Creiamo le stringhe da passare ai figli
    char id_buffer[50], shmID_str[15], shmIdStats_str[15], semID_str[15], msgIdDispenser_str[15], msgIdUser_str[15], n_request_str[10], p_serv_min_str[10], p_serv_max_str[10], timeDay[20], IsNormalUser_str[2];
    sprintf(semID_str, "%d", semID);
    sprintf(msgIdDispenser_str, "%d", msgIdDispenser);
    sprintf(msgIdUser_str, "%d", msgIdUser);
    sprintf(shmID_str, "%d", shmID);
    sprintf(shmIdStats_str, "%d", shmIdStats);
    sprintf(n_request_str, "%d", N_REQUEST);
    sprintf(p_serv_min_str, "%d", P_SERV_MIN);
    sprintf(p_serv_max_str, "%d", P_SERV_MAX);
    sprintf(timeDay, "%d", CalculateTimeDayUser());
    sprintf(IsNormalUser_str, "%d", IsNormalUser);

    // Creiamo tutti gli utenti
    for (int i = 0; i < counter; i++) {
        int j = i + oldUser;
        sprintf(id_buffer, "User_%d", j);
        char *utente_args[] = {id_buffer, shmID_str, shmIdStats_str, semID_str, msgIdDispenser_str, msgIdUser_str, n_request_str, p_serv_min_str, p_serv_max_str, timeDay, IsNormalUser_str, NULL};
        CreateProcess("./bin/utente", utente_args);
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
    char *erogatore_ticket_args[] = {"Erogatore_ticket", semID_str, msgIdDispenser_str, msgIdOperator_str, shmID_str, NULL};
    CreateProcess("./bin/erogatore_ticket", erogatore_ticket_args);

    // Creiamo tutti gli operatori
    for (int i = 0; i < NUM_OF_WORKERS; i++) {
        sprintf(id_buffer, "Operator_%d", i);
        sprintf(random_service, "%d", RandomizeService());
        char *operatore_args[] = {id_buffer, shmID_str, shmIdStats_str, semID_str, msgIdOperator_str, msgIdUser_str, random_service, nof_pause_str, simulated_minute_str, NULL};
        CreateProcess("./bin/operatore", operatore_args);
    }

    // Creiamo tutti gli utenti
    CreateUsers(shmID, shmIdStats, semID, msgIdDispenser, msgIdUser, NUM_OF_USERS, 0, 1);
}

void CheckNewUsers(int msgIdNewUsers, int shmID, int shmIdStats, int semID, int msgIdDispenser, int msgIdUser) {
    NewUserMsg msg;
    int counter_new_users = 0;
    ssize_t n;

    while ((n = msgrcv(msgIdNewUsers, &msg, sizeof(msg.new_users), 0, IPC_NOWAIT)) >= 0) {
        counter_new_users += msg.new_users;
    }

    if (counter_new_users > 0) {
        // Sistemo i contatori
        int oldUser = NUM_OF_USERS;
        NUM_OF_USERS += counter_new_users;
        TOTAL_PROCESSES = 1 + NUM_OF_WORKERS + NUM_OF_USERS;
        TOTAL_PROCESSES_DIR = 1 + TOTAL_PROCESSES;

        struct sembuf sops = {0};

        // Se dobbiamo creare nuovi utenti, dobbiamo sincronizzarli con tutti gli altri processi
        // Per farlo, aggiungiamo al semaforo dei figli il nuovo valore, cosi lo andranno a decrementare e si metteranno in wait
        ExecuteSemop(semID, &sops, 0, counter_new_users);

        // Crea nuovi utenti
        // Facciamo skippare il primo SlaveNotifyAndWait in modo da fargli iniziare subito la giornata e metterli in pari con tutti gli altri
        CreateUsers(shmID, shmIdStats, semID, msgIdDispenser, msgIdUser, counter_new_users, oldUser, 0);

        printf("[Direttore] Ho creato %d nuovi utenti.\n", counter_new_users);

        // Mettiamo in wait il direttore in modo da aspettare tutti i nuovi utenti
        ExecuteSemop(semID, &sops, 0, 0);
        // Continuiamo con il flusso normale
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

void MasterNotifyAndWait(int semID, struct sembuf* sops, DailyConfig* config, Stats* stats, bool endDay, bool* endSim, bool printStats, char* csvPath, char* direttoreID, int msgIdDispenser, int msgIdOperator, int msgIdUser, int msgIdNewUsers, int shmID, int shmIdStats, bool CheckUsers) {
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

        // ✅ CORREZIONE: Inizializza sem 5 PRIMA che i processi possano accedervi
        if (semctl(semID, 5, SETVAL, TOTAL_PROCESSES * 2) < 0) {
            printf("[%s] Errore durante la semctl del semaforo di pausa.\n", direttoreID);
            exit(EXIT_FAILURE);
        }

        // Puliamo le code di messaggi per il nuovo giorno
        // cleanMsgQueue(msgIdDispenser);
        // cleanMsgQueue(msgIdOperator);
        // cleanMsgQueue(msgIdUser);
        // messageQueueRemove(msgIdDispenser);
        // messageQueueRemove(msgIdOperator);
        // messageQueueRemove(msgIdUser);
        // ✅ STEP 1: CREA le NUOVE code PRIMA di eliminare le vecchie
        key_t idDis = ftok("src/main.c", 5001);
        key_t idOpe = ftok("src/main.c", 5002);
        key_t idUse = ftok("src/main.c", 5003);

        int newMsgIdDispenser = messageQueueCreate(idDis, 0666, direttoreID);
        int newMsgIdOperator = messageQueueCreate(idOpe, 0666, direttoreID);
        int newMsgIdUser = messageQueueCreate(idUse, 0666, direttoreID);

        // ✅ STEP 2: Aggiorna la config CON I NUOVI ID
        config->idDispenser = newMsgIdDispenser;
        config->idOperator = newMsgIdOperator;
        config->idUsers = newMsgIdUser;

        // ✅ STEP 3: SOLO ORA elimina le code vecchie (se esistevano)
        if (msgIdDispenser > 0) messageQueueRemove(msgIdDispenser);
        if (msgIdOperator > 0) messageQueueRemove(msgIdOperator);
        if (msgIdUser > 0) messageQueueRemove(msgIdUser);

        // ✅ STEP 4: Aggiorna le variabili locali
        msgIdDispenser = newMsgIdDispenser;
        msgIdOperator = newMsgIdOperator;
        msgIdUser = newMsgIdUser;
    }

    if (CheckUsers) {
        // Cotrolliamo se dobbiamo inserire nuovi utenti
        CheckNewUsers(msgIdNewUsers, shmID, shmIdStats, semID, msgIdDispenser, msgIdUser);
    }

    if (endDay && !(*endSim)) {
        // configuriamo gli sportelli per l'inizio giornata
        ConfigurePostOffices(config, stats);
    }

    if (!(*endSim)) {
        // avvisiamo i figli che possono partire
        ExecuteSemop(semID, sops, 1, -1);
    }
    
    struct timespec req;
    req.tv_sec  = 0;
    req.tv_nsec = 200000000;
    nanosleep(&req, NULL);
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

void Clean(int msgIdDispenser, int msgIdOperator, int msgIdUser, int msgIdNewUser, int semID, int semMessageQueueID, int shmID, DailyConfig* config, int shmIdStat, Stats* stats) {
    // messageQueueRemove(msgIdDispenser);
    // messageQueueRemove(msgIdOperator);
    // messageQueueRemove(msgIdUser);
    messageQueueRemove(msgIdNewUser);

    semCleanUp(semID);
    semCleanUp(semMessageQueueID);
    SharedMemoryCleanConfig(shmID, config);
    SharedMemoryCleanStats(shmIdStat, stats);
}

int main(int argc, char *argv[]) {
    struct sembuf sops = {0}; // Inizializza tutti i campi a 0

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
    int semID = semCreate(0666, NUM_OF_SEM + 1, direttoreID);
    semInizialize(semID, TOTAL_PROCESSES, 1, NUM_SPORTELLI, 1, 1, direttoreID);

    // SEMAFORO PER IL PAUSE. DA SISTEMARE
    if (semctl(semID, 5, SETVAL, 0) < 0) {
        printf("[%s] Errore durante la semctl del semaforo nuovo.\n", direttoreID);
        exit(EXIT_FAILURE);
    }

    int semMessageQueueID = semCreate(0666, NUM_SERVIZI, direttoreID);
    semMessageInitialize(semMessageQueueID, NUM_SERVIZI, direttoreID);
    
    // inizializziamo le statistiche
    StatsInitialize(stats, semID);

    // creiamo le tre code per i messaggi per la comunicazione tra utente-erogatore e erogatore-operatore
    // Coda utente --> erogatore
    //int msgIdDispenser = messageQueueCreate(IPC_PRIVATE, 0666, direttoreID);
    int msgIdDispenser = 0;

    // Coda erogatore --> operatore
    //int msgIdOperator = messageQueueCreate(IPC_PRIVATE, 0666, direttoreID);
    int msgIdOperator = 0;

    // Coda operatore --> utente
    //int msgIdUser = messageQueueCreate(IPC_PRIVATE, 0666, direttoreID);
    int msgIdUser = 0;

    // Coda per inserire nuovi utenti durante la simulazione
    int msgIdNewUsers = messageQueueCreate(PUBLIC_KEY, 0666, direttoreID);

    // creiamo tutti i figli
    createAllSubProcess(shmID, shmIdStats, semID, msgIdDispenser, msgIdOperator, msgIdUser);

    bool endSim = false;
    
    // aspettiamo che tutti i figli siano pronti e diamogli il via
    MasterNotifyAndWait(semID, &sops, config, stats, true, &endSim, false, csvPath, direttoreID, msgIdDispenser, msgIdOperator, msgIdUser, msgIdNewUsers, shmID, shmIdStats, false);

    // Scorriamo i giorni e avvisiamo ogni volta i figli quando finisce un giorno
    for (int giorni = 1; giorni <= SIM_DURATION && !endSim; giorni++) {
        printf("[Direttore] Inizio del giorno %d...\n", giorni);

        // La simulazione continua, resettiamo le statistiche daily
        ResetStatsDaily(stats, semID);

        // Diamo il tempo ai figli per configurarsi per il nuovo giorno
        MasterNotifyAndWait(semID, &sops, config, stats, false, &endSim, false, csvPath, direttoreID, msgIdDispenser, msgIdOperator, msgIdUser, msgIdNewUsers, shmID, shmIdStats, true);

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

        // SEMAFORO PER IL PAUSE. DA SISTEMARE
        if (semctl(semID, 5, SETVAL, TOTAL_PROCESSES * 2) < 0) {
            printf("[%s] Errore durante la semctl del semaforo nuovo.\n", direttoreID);
            exit(EXIT_FAILURE);
        }
        // PROVA
        // messageQueueRemove(config->idDispenser);
        // messageQueueRemove(config->idOperator);
        // messageQueueRemove(config->idUsers);
        
        // Facciamo ripartire i figli per il nuovo giorno
        endSim = (giorni == SIM_DURATION);
        MasterNotifyAndWait(semID, &sops, config, stats, true, &endSim, true, csvPath, direttoreID, msgIdDispenser, msgIdOperator, msgIdUser, msgIdNewUsers, shmID, shmIdStats, false);
    }

    // Fermiamo la simulazione
    printf("[Direttore] Simulazione finita, mando il segnale di terminazione a tutti i figli...\n");
    // ✅ PRIMO: Invia SIGTERM a tutti i processi
    kill(0, SIGTERM);

    // ✅ SECONDO: Aspetta un momento per permettere ai processi di ricevere il segnale
    struct timespec wait_term = {2, 0}; // 1 secondo
    nanosleep(&wait_term, NULL);

    kill(0, SIGTERM);
    
    nanosleep(&wait_term, NULL);

    // ✅ QUARTO: Solo DOPO che tutti sono terminati, pulisci le risorse
    printf("[Direttore] Tutti i processi sono terminati, avvio la pulizia.\n");
    messageQueueRemove(config->idDispenser);
    messageQueueRemove(config->idOperator);
    messageQueueRemove(config->idUsers);

    // ✅ TERZO: Aspetta che tutti i processi terminino
    waitFinishAllSubProcess();


    printf("[Direttore] Tutti i processi sono terminati, avvio la pulizia.\n");

    CalculateFinalStats(stats, semID);
    PrintFinalStats(stats);
    WriteFinalStatsCSV(csvPath, stats);
    
    // Pulizia
    Clean(msgIdDispenser, msgIdOperator, msgIdUser, msgIdNewUsers, semID, semMessageQueueID, shmID, config, shmIdStats, stats);

    printf("[Direttore] Fine della simulazione.\n");

    return EXIT_SUCCESS;
}
