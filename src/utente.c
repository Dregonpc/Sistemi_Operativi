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
            endDay = 1;
            break;
        default:
            break;
    }
}

int Randomizer(int value) {
    return rand() % value;
}

void SlaveNotifyAndWait(int semID, struct sembuf* sops) {

    if (endSimulation) return;

    // avviso il direttore che sono pronto
    ExecuteSemop(semID, sops, 0, -1);
    
    if (endSimulation) return;

    // aspetto che arrivi a 0 (ovvero il direttore mi da il via)
    ExecuteSemop(semID, sops, 1, 0);
}

bool ChoosePresence(int p_serv, char* utenteId) {
    // Probabilità del 70% che l'utente si presenti
    if (p_serv > 3) {
        #ifdef DEBUG
            printf("[%s] Ho deciso di presentarmi.\n", utenteId);
        #endif
        return true;
    }
    else {
        #ifdef DEBUG
            printf("[%s] Ho deciso di NON presentarmi.\n", utenteId);
        #endif
        return false;
    }
}

bool CheckPresenceRequiredService(DailyConfig* config, int IndexServizioRichiesto, char* utenteID) {
    int N = config->num_sportelli;
    for (int i = 0; i < N; i++) {
        if (!config->sportelli[i].disponibile && config->sportelli[i].indexServizioOfferto == IndexServizioRichiesto) { 
            // Se lo sportello non è disponibile significa che qualche operatore l'ha occupato
            #ifdef DEBUG
                printf("[%s] Ho trovato un operatore che può svolgere la mia richiesta (%d)\n", utenteID, IndexServizioRichiesto);
            #endif
            return true;
        }
    }
    
    #ifdef DEBUG
        printf("[%s] Non ho trovato un operatore che può svolgere la mia richiesta (%d)...\n", utenteID, IndexServizioRichiesto);
    #endif
    return false;
}

void SendMessageToErogatore(int msgID, char* utenteID, int IndexServizioRichiesto, int myPID) {
    Message msg;

    // Dobbiamo incrementarlo di 1 perchè il tipo del messaggio deve essere > 0, e quindi non potremmo passare il servizio 0.
    msg.mtype = IndexServizioRichiesto + 1;
    msg.ticket_id = -1;
    msg.user_id = myPID;
    snprintf(msg.text, MAX_TEXT, "%s", utenteID);

    if (endDay) return;
    
    if (msgsnd(msgID, &msg, sizeof(Message) - sizeof(long), 0) < 0) {
        #ifdef DEBUG
            printf("[%s] Errore durante l'invio del messaggio all'erogatore.\n", utenteID);
        #endif
        return;
    }

    #ifdef DEBUG
        printf("[%s] Ho richiesto un ticket per il servizio %d.\n", utenteID, IndexServizioRichiesto);
    #endif
}

bool ReceiveMessageFromOperator(int msgID, int myPID, char* utenteID, long* timeExecution) {
    Message msg;
    ssize_t n;
    int retry_count = 0;
    const int MAX_RETRIES = 50;

    #ifdef DEBUG
        printf("[%s] In attesa di risposta dall'operatore (PID: %d)...\n", utenteID, myPID);
    #endif

    do {
        n = msgrcv(msgID, &msg, sizeof(Message) - sizeof(long), myPID, IPC_NOWAIT);
        //n = msgrcv(msgID, &msg, sizeof(Message) - sizeof(long), myPID, 0);
        
        if (n >= 0) {
            // MESSAGGIO RICEVUTO CON SUCCESSO
            *timeExecution = msg.time_for_execution;
            #ifdef DEBUG
                printf("[%s] Ricevuta risposta dall'operatore (ticket %d, tempo: %ld ns)\n", utenteID, msg.ticket_id, *timeExecution);
            #endif
            return true;
        }
        
        if (errno == ENOMSG) {
            // Nessun messaggio disponibile, aspetta e riprova
            SleepNanoseconds(10000000); // 10ms
            retry_count++;
            
            if (retry_count >= MAX_RETRIES) {
                #ifdef DEBUG
                    printf("[%s] Timeout: nessuna risposta dall'operatore dopo 2 secondi\n", utenteID);
                #endif
                return false;
            }
            continue;
        }
        
        if (errno == EINTR && !endDay) {
            continue;
        }
        
        if (errno == EIDRM) {
            #ifdef DEBUG
                printf("[%s] La coda è stata cancellata\n", utenteID);
            #endif
            return false;
        }
        
        // Altri errori
        #ifdef DEBUG
            perror("msgrcv in ReceiveMessageFromOperator");
        #endif
        return false;
        
    } while (!endDay && retry_count < MAX_RETRIES);

    if (endDay) {
        #ifdef DEBUG
            printf("[%s] Fine giornata prima di essere servito\n", utenteID);
        #endif
    }
    
    return false;
}

void ResetCounters(int* utenti_serviti, int* utenti_non_serviti_day, int* servizi_non_erogati, bool* served, localStats stats[]) {
    *utenti_serviti = 0;
    *utenti_non_serviti_day = 0;
    *servizi_non_erogati = 0;
    *served = false;

    for(int i = 0; i < NUM_SERVICES; i++) {
        stats[i].utenti_serviti = 0;
        stats[i].utenti_non_serviti = 0;
        stats[i].servizi_non_erogati = 0;
        stats[i].time_total = 0;
    }
}

int main(int argc, char *argv[]) {

    struct sembuf sops = {0}; // Inizializza tutti i campi a 0

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
    int P_SERV = 0;
    int myPID = getpid();
    bool served = false;

    localStats localCounters[NUM_SERVICES];

    #ifdef DEBUG
        printf("[%s] Avvio in corso. PID = %d\n", utenteID, myPID);
    #endif

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
        #ifdef DEBUG
            printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", utenteID);
        #endif
        SlaveNotifyAndWait(semID, &sops);
    }

    // Posso iniziare a lavorare
    #ifdef DEBUG
        printf("[%s] Inizio a lavorare.\n", utenteID);
    #endif

    srand(time(NULL) + getpid());

    // Contatori locali
    int utenti_serviti = 0;
    int utenti_non_serviti_day = 0;
    int servizi_non_erogati = 0;

    while (!endSimulation) {
        int n_request_rand = -1;
        long time_total = 0.0;
        int* request = NULL;
        ResetCounters(&utenti_serviti, &utenti_non_serviti_day, &servizi_non_erogati, &served, localCounters);
        
        if (endSimulation) {
            break;
        }

        SlaveNotifyAndWait(semID, &sops);

        // reset local variables
        endDay = 0;

        // Calcoliamo la probabilità per decidere se presentarsi all'ufficio postale oppure no
        int userProbability = ((P_SERV_MAX - P_SERV_MIN + 1) + P_SERV_MIN);
        P_SERV = Randomizer(userProbability);

        if (ChoosePresence(P_SERV, utenteID)) {
            // scelgo quanti servizi richiedere
            n_request_rand = Randomizer(N_REQUEST) + 1;
            #ifdef DEBUG
                printf("[%s] Ho deciso di richiedere %d servizi\n", utenteID, n_request_rand);
            #endif

            request = malloc(n_request_rand * sizeof(int));
            if (!request) {
                #ifdef DEBUG
                    printf("[%s] Errore durante la malloc\n", utenteID);
                #endif
            }

            for (int i = 0; i < n_request_rand; i++) {
                // scelgo il servizio
                request[i] = Randomizer(NUM_SERVICES);
            }

            for (int i = 0; i < n_request_rand && !endDay; i++) {
                served = false;

                // verifico presenza
                if (CheckPresenceRequiredService(config, request[i], utenteID) && !endDay) {
                    // Stabiliamo un orario in cui presentarci
                    int timeToGo = Randomizer(timeDay);
                    #ifdef DEBUG
                        printf("[%s] Ho deciso di presentarmi tra %d nanosecondi.\n", utenteID, timeToGo);
                    #endif
                    SleepNanoseconds(timeToGo);

                    long timeExecution = 0.0;
                    
                    // Contiamo il tempo che ci vuole per essere serviti
                    struct timespec time_start, time_end;
                    clock_gettime(CLOCK_MONOTONIC, &time_start);

                    if (endDay) break;
                    
                    // invio richiesta e aspetto
                    SendMessageToErogatore(msgIdDispenser, utenteID, request[i], myPID);
                    served = ReceiveMessageFromOperator(msgIdUser, myPID, utenteID, &timeExecution);

                    clock_gettime(CLOCK_MONOTONIC, &time_end);
                    long sec = time_end.tv_sec - time_start.tv_sec;
                    long nsec = time_end.tv_nsec - time_start.tv_nsec;
                    if (nsec < 0) {
                        sec--;
                        nsec += (long)1000000000;
                    }
                    
                    // Se siamo qui e served è false, significa che abbiamo richiesto un servizio ma l'operatore non l'ha erogato
                    if (!served) {
                        servizi_non_erogati++;
                        localCounters[request[i]].servizi_non_erogati++;
                    } else {
                        localCounters[request[i]].time_total = nsec - timeExecution;
                        time_total += localCounters[request[i]].time_total;
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

        // se servito o giornata finita, torno a casa. Poi aspetto fine giornata per i prossimi giorni
        #ifdef DEBUG
            printf("[%s] %s, aspetto fine giornata.\n", utenteID, served ? "Servito" : "Non servito");
        #endif
        ExecuteSemop(semID, &sops, 5, -1);

        // Aggiorna le statistiche
        UpdateStaticStatsUsers(semID, stats, utenteID, &utenti_serviti, &utenti_non_serviti_day, &time_total, &servizi_non_erogati);
        for (int i = 0; i < NUM_SERVICES; i++) {
            UpdateDynamicStatsUsers(semID, stats, utenteID, i, &localCounters[i].utenti_serviti, &localCounters[i].utenti_non_serviti, &localCounters[i].time_total, &localCounters[i].servizi_non_erogati);
        }

        #ifdef DEBUG
            printf("[%s] Fine giornata, ci vediamo domani!\n", utenteID);
        #endif

        free(request);

        SlaveNotifyAndWait(semID, &sops);

        if (endSimulation) {
            break;
        }
    }

    SharedMmemoryDetach(config, utenteID);
    SharedMmemoryDetach(stats, utenteID);

    return EXIT_SUCCESS;
}
