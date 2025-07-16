#define _POSIX_C_SOURCE 199309L // per clock_gettime
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <time.h>
#include <errno.h>
#include "../headers/SemsLib.h"
#include "../headers/messaggi.h"
#include "../headers/SharedMemory.h"
#include "../headers/StatsLib.h"

typedef struct {
    int utenti_serviti;
    int utenti_non_serviti;
    int servizi_non_erogati;
    long time_total;
} localStats;

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

int Randomizer(int value) {
    return rand() % value;
}

void SlaveNotifyAndWait(int semID, struct sembuf sops) {
    // avviso il direttore che sono pronto
    ExecuteSemop(semID, sops, 0, -1);
    
    // aspetto che arrivi a 0 (ovvero il direttore mi da il via)
    ExecuteSemop(semID, sops, 1, 0);
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

bool CheckPresenceRequiredService(DailyConfig* config, int IndexServizioRichiesto, char* utenteID) {
    int N = config->num_sportelli;
    for (int i = 0; i < N; i++) {
        if (!config->sportelli[i].disponibile && config->sportelli[i].indexServizioOfferto == IndexServizioRichiesto) { // Se lo sportello non è disponibile significa che qualche operatore l'ha occupato, si potrebbe sostituire con config->sportelli[i].idOperatore != ""
            printf("[%s] Ho trovato un operatore che può svolgere la mia richiesta (%d)\n", utenteID, IndexServizioRichiesto);
            return true;
        }
    }
    
    printf("[%s] Non ho trovato un operatore che può svolgere la mia richiesta (%d)...\n", utenteID, IndexServizioRichiesto);
    return false;
}

void SendMessageToErogatore(int msgID, char* utenteID, int IndexServizioRichiesto, int myPID, int myIndex, int semIdDispenser) {
    Messaggio msg;
    struct sembuf sops;

    // Dobbiamo incrementarlo di 1 perchè il tipo del messaggio deve essere > 0, e quindi non potremmo passare il servizio 0.
    msg.mtype = IndexServizioRichiesto + 1;
    msg.ticket_id = -1;
    msg.user_id = myPID;
    msg.user_index = myIndex;
    snprintf(msg.text, MAX_TEXT, "%s", utenteID);

    if (msgsnd(msgID, &msg, sizeof(Messaggio) - sizeof(long), 0) < 0) {
        printf("[%s] Errore durante l'invio del messaggio all'erogatore.\n", utenteID);
    }

    // Sveglio l'erogatore
    ExecuteSemop(semIdDispenser, sops, 0, 1);

    printf("[%s] Ho richiesto un ticket per il servizio %d.\n", utenteID, IndexServizioRichiesto);
}

bool ReceiveMessageFromOperator(int msgID, int myPID, char* utenteID, long* timeExecution, int semIdUsers, int myIndex, int msgIdEOD) {
    Messaggio msg;
    ssize_t n;
    struct sembuf sops = { .sem_num = 0, .sem_op = 0, .sem_flg = 0 };

    // Controllo se è arrivato endDay
    if (msgrcv(msgIdEOD, &msg, sizeof(Messaggio) - sizeof(long), EOD_mtype, IPC_NOWAIT) >= 0) {
        endDay = true;
        printf("[%s] Fine giornata prima di essere servito: rinuncio\n", utenteID);
        return false;
    }
    
    // Se non è arrivato, mi metto in attesa che qualcuno mi svegli
    CaptureLock(semIdUsers, sops, myIndex);
    
    // Qualcuno mi ha svegliato, controllo se è endDay
    if (msgrcv(msgIdEOD, &msg, sizeof(Messaggio) - sizeof(long), EOD_mtype, IPC_NOWAIT) >= 0) {
        endDay = true;
        printf("[%s] Fine giornata prima di essere servito: rinuncio\n", utenteID);
        return false;
    }

    // Significa che ho un ticket da servire
    n = msgrcv(msgID, &msg, sizeof(Messaggio) - sizeof(long), myPID, IPC_NOWAIT);

    // do {
    //     n = msgrcv(msgID, &msg, sizeof(Messaggio) - sizeof(long), myPID, 0);
    // } while(n < 0 && errno == EINTR && !endDay);

    // if (endDay) {
    //     printf("[%s] Fine giornata prima di essere servito: rinuncio\n", utenteID);
    //     return false;
    // }
    if (n < 0) {
        if (errno == ENOMSG) {
            if (endDay) return false;
        }
        printf("[%s] ", utenteID);
        perror("msgrcv");
        return false;
    }

    *timeExecution = msg.time_for_execution;
    printf("[%s] Ricevuto ticket %d, servito!\n", utenteID, msg.ticket_id);
    return true;
}

void ResetCounters(int* utenti_serviti, int* utenti_non_serviti_day, int* servizi_non_erogati, bool* served, localStats stats[]) {
    *utenti_serviti = 0;
    *utenti_non_serviti_day = 0;
    *servizi_non_erogati = 0;
    *served = false;

    for(int i = 0; i < NUM_SERVIZI; i++) {
        stats[i].utenti_serviti = 0;
        stats[i].utenti_non_serviti = 0;
        stats[i].servizi_non_erogati = 0;
        stats[i].time_total = 0;
    }
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *utenteID = argv[0];
    int shmID = atoi(argv[1]);
    int shmIdStats = atoi(argv[2]);
    int semID = atoi(argv[3]);
    int msgIdDispenser = atoi(argv[4]);
    int msgIdUser = atoi(argv[5]);
    int N_REQUEST = atoi(argv[6]);
    int P_SERV_MIN = atoi(argv[7]);
    int P_SERV_MAX = atoi(argv[8]);
    int timeDay = atoi(argv[9]);
    int IsNormalUser = atoi(argv[10]);
    int semIdUsers = atoi(argv[11]);
    int myIndex = atoi(argv[12]);
    int semIdDispenser = atoi(argv[13]);
    int msgIdEOD = atoi(argv[14]);
    int P_SERV = 0;
    int myPID = getpid();
    bool served = false;

    localStats localCounters[NUM_SERVIZI];

    printf("[%s] Avvio in corso. PID = %d\n", utenteID, myPID);

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

    // colleghiamoci alla memoria condivisa
    DailyConfig* config = (DailyConfig*)SharedMemoryAttachGeneral(shmID, utenteID);
    Stats* stats = (Stats*)SharedMemoryAttachGeneral(shmIdStats, utenteID);

    if (IsNormalUser) {
        printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", utenteID);
        SlaveNotifyAndWait(semID, sops);
    }

    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", utenteID);

    srand(time(NULL) + getpid());

    // Contatori locali
    int utenti_serviti = 0;
    int utenti_non_serviti_day = 0;
    int servizi_non_erogati = 0;

    while (1) {
        int n_request_rand = -1;
        long time_total = 0.0;
        int* request = NULL;
        ResetCounters(&utenti_serviti, &utenti_non_serviti_day, &servizi_non_erogati, &served, localCounters);
        
        SlaveNotifyAndWait(semID, sops);

        // reset flag
        endDay = 0;

        // Calcoliamo la probabilità per decidere se presentarsi all'ufficio postale oppure no
        int userProbability = ((P_SERV_MAX - P_SERV_MIN + 1) + P_SERV_MIN);
        P_SERV = Randomizer(userProbability);

        if (ChoosePresence(P_SERV, utenteID)) {
            // scelgo quanti servizi richiedere
            n_request_rand = Randomizer(N_REQUEST);
            printf("[%s] Ho deciso di richiedere %d servizi\n", utenteID, n_request_rand);

            request = malloc(n_request_rand * sizeof(int));
            if (!request) {
                printf("[%s] Errore durante la malloc\n", utenteID);
            }

            for (int i = 0; i < n_request_rand; i++) {
                // scelgo il servizio
                request[i] = Randomizer(NUM_SERVIZI);
            }

            for (int i = 0; i < n_request_rand && !endDay; i++) {
                served = false;

                // verifico presenza
                if (CheckPresenceRequiredService(config, request[i], utenteID) && !endDay) {
                    // Stabiliamo un orario in cui presentarci
                    int timeToGo = Randomizer(timeDay);
                    printf("[%s] Ho deciso di presentarmi tra %d nanosecondi.\n", utenteID, timeToGo);
                    struct timespec req;
                    req.tv_sec  = 0;
                    req.tv_nsec = timeToGo;
                    nanosleep(&req, NULL);

                    long timeExecution = 0.0;
                    
                    // Contiamo il tempo che ci vuole per essere serviti
                    struct timespec time_start, time_end;
                    clock_gettime(CLOCK_MONOTONIC, &time_start);

                    // invio richiesta e aspetto
                    SendMessageToErogatore(msgIdDispenser, utenteID, request[i], myPID, myIndex, semIdDispenser);
                    served = ReceiveMessageFromOperator(msgIdUser, myPID, utenteID, &timeExecution, semIdUsers, myIndex, msgIdEOD);

                    clock_gettime(CLOCK_MONOTONIC, &time_end);
                    long sec = time_end.tv_sec - time_start.tv_sec;
                    long nsec = time_end.tv_nsec - time_start.tv_nsec;
                    if (nsec < 0) {
                        sec--;
                        nsec += (long)1000000000;
                    }

                    // Sottraiammo al tempo totale il tempo che ci ha messo l'operatore per servirmi per trovare il tempo di attesa in coda
                    localCounters[request[i]].time_total = nsec - timeExecution;
                    time_total += localCounters[request[i]].time_total;
                    
                    // Se siamo qui e served è false, significa che abbiamo richiesto un servizio ma l'operatore non l'ha erogato
                    if (!served) {
                        servizi_non_erogati++;
                        localCounters[request[i]].servizi_non_erogati++;
                    }
                }

                if (served) {
                    utenti_serviti++;
                    localCounters[request[i]].utenti_serviti++;
                }
                else {
                    utenti_non_serviti_day++;
                    localCounters[request[i]].utenti_non_serviti++;
                }
            }
        }

        // se servito o giornata finita, torno a casa
        // poi aspetto fine giornata per i prossimi giorni
        printf("[%s] %s, aspetto fine giornata.\n", utenteID, served ? "Servito" : "Non servito");
        // while (!endDay) {
        //     pause();
        // }

        // Aggiorna le statistiche
        UpdateStaticStatsUsers(semID, stats, utenteID, &utenti_serviti, &utenti_non_serviti_day, &time_total, &servizi_non_erogati);
        for (int i = 0; i < NUM_SERVIZI; i++) {
            UpdateDynamicStatsUsers(semID, stats, utenteID, i, &localCounters[i].utenti_serviti, &localCounters[i].utenti_non_serviti, &localCounters[i].time_total, &localCounters[i].servizi_non_erogati);
        }

        // loop riparte per il giorno successivo
        printf("[%s] Fine giornata, ci vediamo domani!\n", utenteID);

        free(request);

        SlaveNotifyAndWait(semID, sops);

        if (endSimulation) {
            break;
        }
    }

    SharedMmemoryDetach(config, utenteID);
    SharedMmemoryDetach(stats, utenteID);

    return EXIT_SUCCESS;
}
