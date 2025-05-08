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

void SlaveNotifyAndWait(int semID, struct sembuf sops) {
    // avviso il direttore che sono pronto
    ExecuteSemop(semID, sops, 0, -1);
    
    // aspetto che arrivi a 0 (ovvero il direttore mi da il via)
    ExecuteSemop(semID, sops, 1, 0);
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
    }

    printf("[%s] Ho richiesto un ticket per il servizio %d.\n", utenteID, IndexServizioRichiesto);
}

bool ReceiveMessageFromOperator(int msgID, int myPID, char* utenteID, long* timeExecution) {
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
    }

    *timeExecution = msg.time_for_execution;
    printf("[%s] Ricevuto ticket %d, servito!\n", utenteID, msg.ticket_id);
    return true;
}

void ResetCounters(int* utenti_serviti, int* utenti_non_serviti_day, int* servizi_non_erogati, bool* served) {
    *utenti_serviti = 0;
    *utenti_non_serviti_day = 0;
    *servizi_non_erogati = 0;
    *served = false;
}

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *utenteID = argv[0];
    int shmID = atoi(argv[1]);
    int shmIdStats = atoi(argv[2]);
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

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", utenteID);
    SlaveNotifyAndWait(semID, sops);

    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", utenteID);

    srand(time(NULL) + getpid());

    // Contatori locali
    int utenti_serviti = 0;
    int utenti_non_serviti_day = 0;
    int servizi_non_erogati = 0;

    while (1) {
        int IndexServizioRichiesto = -1;
        long time_total = 0.0;
        ResetCounters(&utenti_serviti, &utenti_non_serviti_day, &servizi_non_erogati, &served);
        
        SlaveNotifyAndWait(semID, sops);

        // reset flag
        endDay = 0;

        // Calcoliamo la probabilità per decidere se presentarsi all'ufficio postale oppure no
        P_SERV = RandomizeProbabilityUser(P_SERV_MIN, P_SERV_MAX);

        if (ChoosePresence(P_SERV, utenteID)) {    
            // scelgo il servizio
            IndexServizioRichiesto = RandomizeService();
            
            // verifico presenza
            if (CheckPresenceRequiredService(config, IndexServizioRichiesto, utenteID) && !endDay) {
                // Stabiliamo un orario in cui presentarci
                int timeToGo = CalculateTimeToGo(timeDay);
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
                SendMessageToErogatore(msgIdDispenser, utenteID, IndexServizioRichiesto, myPID);
                served = ReceiveMessageFromOperator(msgIdUser, myPID, utenteID, &timeExecution);

                clock_gettime(CLOCK_MONOTONIC, &time_end);
                long sec = time_end.tv_sec - time_start.tv_sec;
                long nsec = time_end.tv_nsec - time_start.tv_nsec;
                if (nsec < 0) {
                    sec--;
                    nsec += (long)1000000000;
                }

                // Sottraiammo al tempo totale il tempo che ci ha messo l'operatore per servirmi per trovare il tempo di attesa in coda
                time_total = nsec - timeExecution;
                
                // Se siamo qui e served è false, significa che abbiamo richiesto un servizio ma l'operatore non l'ha erogato
                if (!served) {
                    servizi_non_erogati++;
                }
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
        UpdateStatsUsers(semID, sops, stats, utenteID, IndexServizioRichiesto, &utenti_serviti, &utenti_non_serviti_day, &time_total, &servizi_non_erogati);

        // loop riparte per il giorno successivo
        printf("[%s] Fine giornata, ci vediamo domani!\n", utenteID);

        SlaveNotifyAndWait(semID, sops);

        if (endSimulation) {
            break;
        }
    }

    SharedMmemoryDetach(config, utenteID);
    SharedMmemoryDetach(stats, utenteID);

    return EXIT_SUCCESS;
}
