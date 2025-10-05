#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
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
            endDay = 1;
            break;
        default:
            break;
    }
}

bool CheckDailyService(DailyConfig* config, int indexServizioOperatore, char* operatoreId) {
    int N = config->num_sportelli;
    for (int i = 0; i < N; i++) {
        if (config->sportelli[i].indexServizioOfferto == indexServizioOperatore) {
            #ifdef DEBUG
                printf("[%s] Ho controllato gli sportelli è c'è il servizio che offro io (%d).\n", operatoreId, indexServizioOperatore);
            #endif
            return true;
        }
    }

    #ifdef DEBUG
        printf("[%s] Ho controllato gli sportelli è NON c'è il servizio che offro io (%d)... Attendo la fine giornata.\n", operatoreId, indexServizioOperatore);
    #endif
    return false;
}

void SlaveNotifyAndWait(int semID, struct sembuf* sops) {

    if (endSimulation) return;

    // avviso il direttore che sono pronto
    ExecuteSemop(semID, sops, 0, -1);

    if (endSimulation) return;
    
    // aspetto che arrivi a 0 (ovvero il direttore mi da il via)
    ExecuteSemop(semID, sops, 1, 0);
}

bool TakeUpPostOffice(DailyConfig* config, int semID, struct sembuf* sops, int indexServizioOperatore, char* operatoreId, int* operatori_attivi, bool* firstTry) {
    bool taken = false;
    // CHECK SIM
    if (endDay || endSimulation) {
        return false;
    }

    //PROVA DI OCCUPAZIONE
    // num sportelli liberi
    int checkSportelli = SemGetVal(semID, 2);
    // se ci sono sportelli provo se no non occupo
    if (checkSportelli != 0) {
        if (!endDay) {
        // acquisisco il lock per l'accesso coordinato agli sportelli
            if (CaptureLock(semID, sops, 3) == -1) {
                #ifdef DEBUG
                    printf("[%s] Errore durante l'acquisizione del lock per gli sportelli.\n", operatoreId);
                #endif
            }

            int N = config->num_sportelli;
        
            for (int i = 0; i < N; i++) {
                if (config->sportelli[i].disponibile && config->sportelli[i].indexServizioOfferto == indexServizioOperatore) {
                    config->sportelli[i].idOperatore = operatoreId;
                    config->sportelli[i].disponibile = 0;

                    taken = true;
                    (*operatori_attivi)++;

                    //segnalamo che uno sportello è stato occupato
                    if (ExecuteSemop(semID, sops, 2, -1) == -1) {
                        #ifndef DEBUG
                            printf("[%s] Errore nella segnalazione dell'occupazione di uno sportello", operatoreId);
                        #endif
                    }
                    
                    
                    #ifdef DEBUG
                        printf("[%s] Sono stato assegnato allo sportello %d per il servizio %d.\n", operatoreId, config->sportelli[i].idSportello, indexServizioOperatore);
                    #endif
                    checkSportelli = true;
                    break;
                }
            }

            // rilascio il lock
            if (ReleaseLock(semID, sops, 3) == -1) {
                #ifdef DEBUG
                    printf("[%s] Errore durante il rilascio del lock per gli sportelli.\n", operatoreId);
                #endif
            }
        }
    }

    // FIST TRY UPDATE
    if (*firstTry) {
        *firstTry = false;
    }
    
    return taken;
}

void releasePostOffice(DailyConfig* config, int semID, struct sembuf* sops, char* operatoreId) {
    // acquisisco il lock per l'accesso coordinato agli sportelli
    if (CaptureLock(semID, sops, 3) == -1) {
        #ifdef DEBUG
            printf("[%s] Errore durante l'acquisizione del lock per gli sportelli.\n", operatoreId);
        #endif
    }

    int N = config->num_sportelli;

    for (int i = 0; i < N; i++) {
        if (config->sportelli[i].idOperatore == operatoreId) {
            config->sportelli[i].idOperatore = "";
            config->sportelli[i].disponibile = 1;

            // Avvisiamo i colleghi che ho rilasciato lo sportello
            if (ExecuteSemop(semID, sops, 2, 1) == -1) {
                #ifdef DEBUG
                    printf("[%s] Errore durante il rilascio di uno sportello.\n", operatoreId);
                #endif
            }

            #ifdef DEBUG
                printf("[%s] Ho rilasciato lo sportello e mandato la notifica ai miei colleghi.\n", operatoreId);
            #endif
            break;
        }
    }

    // rilascio il lock
    if (ReleaseLock(semID, sops, 3) == -1) {
        #ifdef DEBUG
            printf("[%s] Errore durante il rilascio del lock per gli sportelli.\n", operatoreId);
        #endif
    }
}

bool breakCondition(int counter_servizi_erogati) {
    // 30% di possibilità di andare in pausa e deve aver servito almeno due clienti
    return ((rand() % 100) < 30) && (counter_servizi_erogati > 1);
}

int CalculateTimeExecution(int IndexServizio, int simulated_minute) {
    int durata = servizi[IndexServizio].durata;
    int variazione = durata / 2;
    int delta = (rand() % (2 * variazione + 1)) - variazione;
    int durataCasuale = durata + delta;
    
    return durataCasuale * simulated_minute;
}

void ReceiveTicketAndExecute(int msgIdOperator, int msgIdUser, int IndexServizio, char *operatoreID, int NOF_PAUSE, int* pause_effettuate, int simulated_minute, int* servizi_erogati, int* counter_pause, double* tempo_erogazione) {
    while (!endDay) {
        Messaggio msg;
        ssize_t n;

        // ricevo finché non ottengo un messaggio valido o endDay
        do {
            n = msgrcv(msgIdOperator, &msg, sizeof(Messaggio) - sizeof(long), IndexServizio + 1, 0);
        } while (n < 0 && errno == EINTR && !endDay);

        if (endDay) {
            #ifdef DEBUG
                printf("[%s] Fine giornata rilevata, interrompo ricezione.\n", operatoreID);
            #endif
            break;
        }
        else if (errno == EIDRM) {
            #ifdef DEBUG
                printf("[%s] La coda è stata cancellata, interrompo ricezione.\n", operatoreID);
            #endif
            break;
        }
        if (n < 0) {
            #ifdef DEBUG
                perror("msgrcv");
            #endif
            break;
        }

        msg.mtype--;

        #ifdef DEBUG
            printf("[%s] Servo il ticket %d per l'utente '%s' (servizio %ld).\n", operatoreID, msg.ticket_id, msg.text, msg.mtype);
        #endif

        // Eseguo il servizio
        int executionTime = CalculateTimeExecution(IndexServizio, simulated_minute);
        SleepNanoseconds(executionTime);
        *tempo_erogazione = executionTime;

        // Manda risposta all'utente usando il suo PID come "destinatario"
        msg.mtype = msg.user_id;
        msg.time_for_execution = executionTime;
        if (msgsnd(msgIdUser, &msg, sizeof(Messaggio) - sizeof(long), 0) < 0) {
            #ifdef DEBUG
                perror("msgsnd");
            #endif
        }

        // Aumentiamo i contatori
        (*servizi_erogati)++;

        #ifdef DEBUG
            printf("[%s] Ho finito di servire %ld. Ci ho impiegato %d nanosecondi.\n", operatoreID, msg.mtype, executionTime);
        #endif

        if ((*pause_effettuate) < NOF_PAUSE && breakCondition(*servizi_erogati)) {
            (*pause_effettuate)++;
            (*counter_pause)++;
            #ifdef DEBUG
                printf("[%s] Posso andare in pausa, termino la mia giornata.\n", operatoreID);
            #endif
            break;
        }
    }
}

void ResetCounters(int* servizi_erogati, int* operatori_attivi, int* counter_pause, double* tempo_erogazione) {
    *servizi_erogati = 0;
    *operatori_attivi = 0;
    *counter_pause = 0;
    *tempo_erogazione = 0;
}

int main(int argc, char *argv[]) {

    struct sembuf sops = {0}; // Inizializza tutti i campi a 0

    char *operatoreID = argv[0];
    int shmID = atoi(argv[1]);
    int shmIdStats = atoi(argv[2]);
    int semID = atoi(argv[3]);
    int msgIdOperator = atoi(argv[4]);
    int msgIdUser = atoi(argv[5]);
    int indexServizio = atoi(argv[6]);
    int NOF_PAUSE = atoi(argv[7]);
    int SIMULATED_MINUTE = atoi(argv[8]);
    int pause_effettuate = 0;

    #ifdef DEBUG
        printf("[%s] Avvio in corso. PID = %d\n", operatoreID, getpid());
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
    DailyConfig* config = (DailyConfig*)SharedMemoryAttachGeneral(shmID, operatoreID);
    Stats* stats = (Stats*)SharedMemoryAttachGeneral(shmIdStats, operatoreID);

    // barrier iniziale
    #ifdef DEBUG
        printf("[%s] Ready, aspetto il via.\n", operatoreID);
    #endif
    SlaveNotifyAndWait(semID, &sops);

    srand(time(NULL) + getpid());
    bool CheckService = false;

    // Contatori locali
    int servizi_erogati = 0;
    int operatori_attivi = 0;
    int counter_pause = 0;
    double tempo_erogazione = 0;

    while (!endSimulation) {
        ResetCounters(&servizi_erogati, &operatori_attivi, &counter_pause, &tempo_erogazione);

        // Posso iniziare a lavorare
        #ifdef DEBUG
            printf("[%s] Inizio giornata.\n", operatoreID);
        #endif
        endDay = 0;

        if (endSimulation) {
            break;
        }

        // Controllo che il servizio di cui mi occupo è presente negli sportelli
        CheckService = CheckDailyService(config, indexServizio, operatoreID);

        bool firstTryTakeUp = true;

        if (endSimulation) {
            break;
        }

        if (CheckService) {
            // Provo ad occupare uno sportello
            bool active = TakeUpPostOffice(config, semID, &sops, indexServizio, operatoreID, &operatori_attivi, &firstTryTakeUp);

            if (endSimulation) {
                break;
            }

            //tutti avvisano il direttore di essere setuppati
            SlaveNotifyAndWait(semID, &sops);

            // se sono in uno sportello, se no si mettono in attesa che si liberi uno sportello
            if (active) {
                // lavorano
                if (!endDay) {
                    // LAVORO finché non finisce il giorno o vado in pausa
                    #ifdef DEBUG
                        printf("[%s] Inizio turno (servizio %d)\n", operatoreID, indexServizio);
                    #endif
            
                    // Mi metto a ricevere i ticket e ad eseguirli
                    ReceiveTicketAndExecute(msgIdOperator, msgIdUser, indexServizio, operatoreID, NOF_PAUSE, &pause_effettuate, SIMULATED_MINUTE, &servizi_erogati, &counter_pause, &tempo_erogazione);
            
                    // rilascio lo sportello
                    releasePostOffice(config, semID, &sops, operatoreID);
                }
            }
            else{
                // attesa di uno sportello
                while (!active && !endDay && !endSimulation) {
                    // Attendo la liberazione di uno sportello
                    ExecuteSemop(semID, &sops, 2, -1);
                    if (endDay || endSimulation) break;
                    active = TakeUpPostOffice(config, semID, &sops, indexServizio, operatoreID, &operatori_attivi, &firstTryTakeUp);

                    if (active) {
                        if (!endDay) {
                            // LAVORO finché non finisce il giorno o vado in pausa
                            #ifdef DEBUG
                                printf("[%s] Inizio turno (servizio %d)\n", operatoreID, indexServizio);
                            #endif
                    
                            // Mi metto a ricevere i ticket e ad eseguirli
                            ReceiveTicketAndExecute(msgIdOperator, msgIdUser, indexServizio, operatoreID, NOF_PAUSE, &pause_effettuate, SIMULATED_MINUTE, &servizi_erogati, &counter_pause, &tempo_erogazione);
                    
                            // rilascio lo sportello
                            releasePostOffice(config, semID, &sops, operatoreID);
                        }
                    } else {
                        if (endDay || endSimulation) break;
                        continue;
                    }
                    
                }
            }
        }
        else {
            if (endSimulation) {
                break;
            }
            
            // Se non c'è il servizio che offro io, avviso lo stesso il direttore
            SlaveNotifyAndWait(semID, &sops);
        }

        if (endSimulation) {
            break;
        }

        // Aspetto fine giornata se non già arrivata (per gli operatori che vanno in pausa)
        if (!endDay || !CheckService) {
            #ifdef DEBUG
                printf("[%s] Attendo fine giornata...\n", operatoreID);
            #endif
            ExecuteSemop(semID, &sops, 5, -1);
        }

        // Aggiorna le statistiche
        UpdateStatsOperators(semID, stats, operatoreID, indexServizio, &servizi_erogati, &operatori_attivi, &counter_pause, &tempo_erogazione);
        
        // loop riparte per il giorno successivo
        #ifdef DEBUG
            printf("[%s] Fine giornata elaborata.\n", operatoreID);
        #endif

        SlaveNotifyAndWait(semID, &sops);

        if (endSimulation) {
            break;
        }
    }

    SharedMmemoryDetach(config, operatoreID);
    SharedMmemoryDetach(stats, operatoreID);

    return EXIT_SUCCESS;
}
