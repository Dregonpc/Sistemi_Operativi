#include <stdio.h>
#include <stdlib.h>
#include "../headers/GlobalVars.h"

int main(int argc, char *argv[]) {
    if (argc != 4)
    {
        printf("[Main] Errore: numero di argomenti non valido.\n");
        printf("[Main] inserire i path dei file : <config_general.conf> <config_timeout.conf> <config_explode.conf>\n");
        exit(EXIT_FAILURE);
    }

    printf("[Main] Leggo file di configurazione e passo i dati al direttore.\n");

    // Leggi i file di configurazione
    read_config(argv[1]);
    read_config(argv[2]);
    read_config(argv[3]);

    TOTAL_PROCESSES = 1 + NUM_OF_WORKERS + NUM_OF_USERS;
    TOTAL_PROCESSES_DIR = 1 + TOTAL_PROCESSES;

    printf("[Main] Numero di operatori: %d\n", NUM_OF_WORKERS);
    printf("[Main] Numero di utenti: %d\n", NUM_OF_USERS);
    printf("[Main] Numero di pause: %d\n", NOF_PAUSE);
    printf("[Main] Tempo di servizio minimo: %d\n", P_SERV_MIN);
    printf("[Main] Tempo di servizio massimo: %d\n", P_SERV_MAX);
    printf("[Main] Durata della simulazione: %d\n", SIM_DURATION);
    printf("[Main] Soglia di esplosione: %d\n", EXPLODE_THRESHOLD);
        
    // Passa i parametri al direttore
    char* direttore_args[] = {"Direttore", NULL};

    execve("./bin/direttore", direttore_args, NULL);

    printf("[Main] Errore nella execve\n");
    exit(EXIT_FAILURE);

    return EXIT_SUCCESS;
}
