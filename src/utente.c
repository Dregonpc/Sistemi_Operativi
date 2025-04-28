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

// SIGUSR2 -> fine giornata
static void handle_day_end(int signo) {
    endDay = 1;
}

// SIGTERM -> fine simulazione
static void handle_simulation_end(int signo) {
    endSimulation = 1;
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

void WaitStartFromOperator(int semID, struct sembuf sops) {
    // aspetto che arrivi a 0 (ovvero gli operatori ci danno il via)
    sops.sem_num = 4;
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
    // srand(time(NULL) + getpid());
    // return rand() % NUM_SERVIZI;
    return 1; // Per testare il servizio 1
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

int main(int argc, char *argv[]) {

    struct sembuf sops;

    char *utenteID = argv[0];
    int shmID = atoi(argv[1]);
    int semID = atoi(argv[2]);
    int msgIdDispenser = atoi(argv[3]);
    int msgIdUser = atoi(argv[4]);
    int P_SERV_MIN = atoi(argv[5]);
    int P_SERV_MAX = atoi(argv[6]);
    int timeDay = atoi(argv[7]);
    int P_SERV = 0;
    int myPID = getpid();
    bool served = false;
    printf("[%s] Avvio in corso. PID = %d\n", utenteID, myPID);

    // Installa i signal handler
    struct sigaction sa_end = {0};
    sa_end.sa_handler = handle_day_end;
    sigemptyset(&sa_end.sa_mask);
    sa_end.sa_flags = 0;                // senza SA_RESTART
    sigaction(SIGUSR2, &sa_end, NULL);

    struct sigaction sa_term = {0};
    sa_term.sa_handler = handle_simulation_end;
    sigemptyset(&sa_term.sa_mask);
    sa_term.sa_flags = 0;
    sigaction(SIGTERM, &sa_term, NULL);

    // colleghiamoci alla memoria condivisa
    DailyConfig* config = SharedMemoryAttach(shmID, utenteID);

    printf("[%s] Sono pronto, avviso il direttore e aspetto il via.\n", utenteID);
    SlaveNotifyAndWait(semID, sops);

    // Posso iniziare a lavorare
    printf("[%s] Inizio a lavorare.\n", utenteID);

    srand(time(NULL) + getpid());

    while (1) {
        // reset flag
        endDay = 0;

        // Calcoliamo la probabilità per decidere se presentarsi all'ufficio postale oppure no
        P_SERV = RandomizeProbabilityUser(P_SERV_MIN, P_SERV_MAX);

        if (ChoosePresence(P_SERV, utenteID)) {
            // Aspettiamo che gli operatori "ci diano il via"
            WaitStartFromOperator(semID, sops);
    
            // scelgo il servizio
            int IndexServizioRichiesto = RandomizeService(); // PER I TEST: simuliamo di richiedere sempre il servizio 1, da sostituire con un random
    
            // verifico presenza
            if (CheckPresenceRequiredService(config, IndexServizioRichiesto, utenteID) && !endDay) {
                // Stabiliamo un orario in cui presentarci
                int timeToGo = CalculateTimeToGo(timeDay);
                printf("[%s] Ho deciso di presentarmi tra %d nanosecondi.\n", utenteID, timeToGo);
                struct timespec req;
                req.tv_sec  = 0;
                req.tv_nsec = timeToGo;
                nanosleep(&req, NULL);
                
                // invio richiesta e aspetto
                SendMessageToErogatore(msgIdDispenser, utenteID, IndexServizioRichiesto, myPID);
                served = ReceiveMessageFromOperator(msgIdUser, myPID, utenteID);
            }
        }

        // se servito o giornata finita, torno a casa
        // poi aspetto fine giornata per i prossimi giorni
        printf("[%s] %s, aspetto fine giornata.\n", utenteID, served ? "Servito" : "Non servito");
        while (!endDay) {
            pause();
        }

        // loop riparte per il giorno successivo
        printf("[%s] Fine giornata, ci vediamo domani!\n", utenteID);

        SlaveNotifyAndWait(semID, sops);

        if (endSimulation) {
            break;
        }
    }

    return EXIT_SUCCESS;
}
