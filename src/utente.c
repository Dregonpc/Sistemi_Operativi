#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>
#include "../headers/messaggi.h"
#include "../headers/SharedMemory.h"

static volatile sig_atomic_t endDay = 0;
static volatile sig_atomic_t endSimulation = 0;

// Handler per tutti i segnali
static void signalHandler(int signo) {
    switch (signo) {
        case SIGUSR2:
            endDay = 1;
            break;
        case SIGTERM:
            endSimulation = 1;
            break;
        default:
            break;
    }
}

// Collegamento alla memoria condivisa
DailyConfig* SharedMemoryAttach(int shmID, char* utenteId) {
    DailyConfig* config = (DailyConfig*)shmat(shmID, NULL, 0);
    if (config == (void *) -1) {
        printf("[%s] Collegamento alla memoria condivisa fallita.\n", utenteId);
        exit(EXIT_FAILURE);
    }

    return config;
}

// Collegamento alla memoria condivisa
Stats* SharedMemoryAttachStats(int shmID, char* utenteId) {
    Stats* stats = (Stats*)shmat(shmID, NULL, 0);
    if (stats == (void *) -1) {
        printf("[%s] Collegamento alla memoria condivisa fallita.\n", utenteId);
        exit(EXIT_FAILURE);
    }

    return stats;
}

void SlaveNotifyAndWait(int semID, struct sembuf sops) {
    // avviso il direttore che sono pronto
    sops.sem_num = 0;
    sops.sem_op = -1;
    semop(semID, &sops, 1);
    
    // aspetto che arrivi a 0 (ovvero il direttore mi da il via)
    sops.sem_num = 1;
    sops.sem_op = 0;
    semop(semID, &sops, 1);
}

int RandomizeProbabilityUser(int p_serv_min, int p_serv_max) {
    return (rand() % ((p_serv_max - p_serv_min + 1) + p_serv_min));
}

bool ChoosePresence(int p_serv, char* utenteId) {
    // Probabilità del 70% che l'utente si presenti
    if (p_serv > 3) {
        printf("[%s] Ho deciso di presentarmi.\n", utenteId);
        return true;
    }
    else {
        printf("[%s] Ho deciso di NON presentarmi.\n", utenteId);
        return false;
    }
}

int RandomizeService() {
    return rand() % NUM_SERVIZI;
    // return 1; // Per testare il servizio 1
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

int CalculateTimeToGo(int timeDay) {
    return (rand() % timeDay);
}

void SendMessageToErogatore(int msgID, char* utenteID, int IndexServizioRichiesto, int myPID) {
    Messaggio msg;

    // Dobbiamo incrementarlo di 1 perchè il tipo del messaggio deve essere > 0, e quindi non potremmo passare il servizio 0.
    msg.mtype = IndexServizioRichiesto + 1;
    msg.ticket_id = -1;
    msg.user_id = myPID;
    snprintf(msg.text, MAX_TEXT, "%s", utenteID);

    if (msgsnd(msgID, &msg, sizeof(Messaggio) - sizeof(long), 0) < 0) {
        printf("[%s] Errore durante l'invio del messaggio all'erogatore.\n", utenteID);
        //exit(EXIT_FAILURE);
    }

    printf("[%s] Ho richiesto un ticket per il servizio %d.\n", utenteID, IndexServizioRichiesto);
}

bool ReceiveMessageFromOperator(int msgID, int myPID, char* utenteID) {
    Messaggio msg;
    ssize_t n;

    do {
        n = msgrcv(msgID, &msg, sizeof(Messaggio) - sizeof(long), myPID, 0);
    } while(n < 0 && errno == EINTR && !endDay);

    if (endDay) {
        printf("[%s] Fine giornata prima di essere servito: rinuncio\n", utenteID);
        return false;
    }
    if (n < 0) {
        perror("msgrcv");
        //exit(EXIT_FAILURE);
    }

    printf("[%s] Ricevuto ticket %d, servito!\n", utenteID, msg.ticket_id);
    return true;
}

void ResetCounters(int* utenti_serviti, int* utenti_non_serviti_day) {
    *utenti_serviti = 0;
    *utenti_non_serviti_day = 0;
}

void PrintDailyStats(Stats* stats) {
    printf("[Utente] Statistiche giornaliere:\n");
    printf("Utenti serviti totali: %d\n", stats->utenti_serviti_tot_sim);
    // printf("Servizi erogati totali: %d\n", stats->servizi_erogati_tot_sim);
    // printf("Servizi non erogati totali: %d\n", stats->servizi_non_erogati_tot_sim);
    // printf("Operatori attivi totali: %d\n", stats->operatori_attivi_day);
    // printf("Pause effettuate totali: %d\n", stats->pause_effettuate_sim);
    // printf("Tempo medio di attesa degli utenti: %.2f nanosecondi\n", stats->tempo_attesa_utenti_day * 1000000000.0);
    // printf("Tempo medio di erogazione dei servizi: %.2f nanosecondi\n", stats->tempo_erogazione_servizi_day * 1000000000.0);
}

void UpdateStats(int semID, struct sembuf sops, Stats* stats, char *utenteId, int IndexServizioRichiesto,  int* utenti_serviti, int* utenti_non_serviti_day, float* time_total) {
    // acquisisco il lock
    sops.sem_num = 4;
    sops.sem_op = -1;
    if (semop(semID, &sops, 1) == -1) {
        printf("[%s] Errore durante l'acquisizione del lock per le statistiche.\n", utenteId);
    }

    printf("[%s] Aggiorno le statistiche...\n", utenteId);
    // Scrivo statistiche

    stats->utenti_serviti_tot_sim = stats->utenti_serviti_tot_sim + *utenti_serviti;
    stats->utenti_non_serviti_tot_day = stats->utenti_non_serviti_tot_day + *utenti_non_serviti_day;
    stats->utenti_non_serviti_tot_sim += *utenti_non_serviti_day;
    
    stats->utenti_serviti_tot_sim_services[IndexServizioRichiesto] += *utenti_serviti;
    stats->tempo_attesa_utenti_day_services[IndexServizioRichiesto] += *time_total;
    stats->tempo_attesa_utenti_sim += *time_total;
    stats->tempo_attesa_utenti_day += *time_total;

    PrintDailyStats(stats);
    
    // rilascio il lock
    sops.sem_num = 4;
    sops.sem_op = 1;
    if (semop(semID, &sops, 1) == -1) {
        printf("[%s] Errore durante il rilascio del lock per le statistiche.\n", utenteId);
    }
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *utenteID = argv[0];
    int shmID = atoi(argv[1]);
    int shmIddStats = atoi(argv[2]);
    int semID = atoi(argv[3]);
    int msgIdDispenser = atoi(argv[4]);
    int msgIdUser = atoi(argv[5]);
    int P_SERV_MIN = atoi(argv[6]);
    int P_SERV_MAX = atoi(argv[7]);
    int timeDay = atoi(argv[8]);
    int P_SERV = 0;
    int myPID = getpid();
    bool served = false;
    printf("[%s] Avvio in corso. PID = %d\n", utenteID, myPID);

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

    // colleghiamoci alla memoria condivisa
    DailyConfig* config = SharedMemoryAttach(shmID, utenteID);
    Stats* stats = SharedMemoryAttachStats(shmID, utenteID);

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", utenteID);
    SlaveNotifyAndWait(semID, sops);

    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", utenteID);

    srand(time(NULL) + getpid());

    // Contatori locali
    int utenti_serviti = 0;
    int utenti_non_serviti_day = 0;

    while (1) {
        int IndexServizioRichiesto = -1;
        float time_total = 0.0;
        ResetCounters(&utenti_serviti, &utenti_non_serviti_day);

        IndexServizioRichiesto = RandomizeService();
        
        
        SlaveNotifyAndWait(semID, sops);

        // reset flag
        endDay = 0;

        // Calcoliamo la probabilità per decidere se presentarsi all'ufficio postale oppure no
        P_SERV = RandomizeProbabilityUser(P_SERV_MIN, P_SERV_MAX);

        if (ChoosePresence(P_SERV, utenteID)) {    
            // scelgo il servizio
            //IndexServizioRichiesto = RandomizeService();
            
            // verifico presenza
            if (CheckPresenceRequiredService(config, IndexServizioRichiesto, utenteID) && !endDay) {
                // Stabiliamo un orario in cui presentarci
                int timeToGo = CalculateTimeToGo(timeDay);
                printf("[%s] Ho deciso di presentarmi tra %d nanosecondi.\n", utenteID, timeToGo);
                struct timespec req;
                req.tv_sec  = 0;
                req.tv_nsec = timeToGo;
                nanosleep(&req, NULL);
                
                // Contiamo il tempo che ci vuole per essere serviti
                clock_t time_start, time_end;
                time_start = clock();

                // invio richiesta e aspetto
                SendMessageToErogatore(msgIdDispenser, utenteID, IndexServizioRichiesto, myPID);
                served = ReceiveMessageFromOperator(msgIdUser, myPID, utenteID);

                time_end = clock();

                time_total = (float)((time_end - time_start) / CLOCKS_PER_SEC);
            }
        }

        if (served) {
            utenti_serviti++;
        }
        else {
            utenti_non_serviti_day++;
        }

        // se servito o giornata finita, torno a casa
        // poi aspetto fine giornata per i prossimi giorni
        printf("[%s] %s, aspetto fine giornata.\n", utenteID, served ? "Servito" : "Non servito");
        while (!endDay) {
            pause();
        }

        // Aggiorna le statistiche
        UpdateStats(semID, sops, stats, utenteID, IndexServizioRichiesto, &utenti_serviti, &utenti_non_serviti_day, &time_total);

        // loop riparte per il giorno successivo
        printf("[%s] Fine giornata, ci vediamo domani!\n", utenteID);

        SlaveNotifyAndWait(semID, sops);

        if (endSimulation) {
            break;
        }
    }

    shmdt(config);
    shmdt(stats);

    return EXIT_SUCCESS;
}
