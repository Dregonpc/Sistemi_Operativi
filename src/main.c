#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define MAX_LINE 100

void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len-1] == '\n') str[len-1] = '\0';
}

void trim(char *s) {
    // rimuove newline e spazi finali
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == ' ' || s[len-1] == '\r'))
        s[--len] = '\0';
    // rimuove spazi iniziali
    char *start = s;
    while (*start == ' ') start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
}

int NUM_OF_WORKERS = 0;
int NUM_OF_USERS = 0;
int NUM_SPORTELLI = 0;
int NOF_PAUSE = 0;
int N_REQUEST = 0;
int P_SERV_MIN = 0;
int P_SERV_MAX = 0;
int SIM_DURATION = 0;
int EXPLODE_THRESHOLD = 0;

void read_config(char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("apertura config");
        return;
    }
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (line[0]=='\0' || line[0]=='#') 
            continue;  // salto righe vuote o commenti
        char *eq = strchr(line, '=');
        if (!eq) 
            continue;
        *eq = '\0';
        char *key   = line;
        char *value = eq + 1;
        trim(key);
        trim(value);

        if (strcmp(key, "NUM_OF_WORKERS") == 0) {
            NUM_OF_WORKERS = atoi(value);
        } else if (strcmp(key, "NUM_OF_USERS") == 0) {
            NUM_OF_USERS = atoi(value);
        } else if (strcmp(key, "NUM_SPORTELLI") == 0) {
            NUM_SPORTELLI = atoi(value);
        } else if (strcmp(key, "NOF_PAUSE") == 0) {
            NOF_PAUSE = atoi(value);
        } else if (strcmp(key, "N_REQUEST") == 0) {
            N_REQUEST = atoi(value);
        } else if (strcmp(key, "P_SERV_MIN") == 0) {
            P_SERV_MIN = atoi(value);
        } else if (strcmp(key, "P_SERV_MAX") == 0) {
            P_SERV_MAX = atoi(value);
        } else if (strcmp(key, "SIM_DURATION") == 0) {
            SIM_DURATION = atoi(value);
        } else if (strcmp(key, "EXPLODE_THRESHOLD") == 0) {
            EXPLODE_THRESHOLD = atoi(value);
        } else {
            printf("Chiave non riconosciuta: %s\n", key);
        }
    }
    fclose(f);
}


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

    // cast int --> string per passare i parametri al direttore
    char str_num_of_workers[15];
    sprintf(str_num_of_workers, "%d", NUM_OF_WORKERS);

    char str_num_of_users[15];
    sprintf(str_num_of_users, "%d", NUM_OF_USERS);

    char str_num_sportelli[15];
    sprintf(str_num_sportelli, "%d", NUM_SPORTELLI);

    char str_nof_pause[15];
    sprintf(str_nof_pause, "%d", NOF_PAUSE);

    char str_n_request[15];
    sprintf(str_n_request, "%d", N_REQUEST);

    char str_p_serv_min[15];
    sprintf(str_p_serv_min, "%d", P_SERV_MIN);

    char str_p_serv_max[15];
    sprintf(str_p_serv_max, "%d", P_SERV_MAX);

    char str_sim_duration[15];
    sprintf(str_sim_duration, "%d", SIM_DURATION);

    char str_explode_threshold[15];
    sprintf(str_explode_threshold, "%d", EXPLODE_THRESHOLD);

    printf("[Main] Numero di operatori: %d\n", NUM_OF_WORKERS);
    printf("[Main] Numero di utenti: %d\n", NUM_OF_USERS);
    printf("[Main] Numero degli sportelli: %d\n", NUM_SPORTELLI);
    printf("[Main] Numero di pause: %d\n", NOF_PAUSE);
    printf("[Main] Numero di richieste massime per utente: %d\n", N_REQUEST);
    printf("[Main] Tempo di servizio minimo: %d\n", P_SERV_MIN);
    printf("[Main] Tempo di servizio massimo: %d\n", P_SERV_MAX);
    printf("[Main] Durata della simulazione: %d\n", SIM_DURATION);
    printf("[Main] Soglia di esplosione: %d\n", EXPLODE_THRESHOLD);
        
    // Passa i parametri al direttore
    char* direttore_args[] = {"Direttore", str_num_of_workers, str_num_of_users, str_num_sportelli, str_nof_pause, str_n_request, str_p_serv_min, str_p_serv_max, str_sim_duration, str_explode_threshold, NULL};

    execve("./bin/direttore", direttore_args, NULL);

    printf("[Main] Errore nella execve\n");
    exit(EXIT_FAILURE);

    return EXIT_SUCCESS;
}
