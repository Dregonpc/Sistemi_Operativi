#include "../headers/GlobalVars.h"

int NUM_OF_WORKERS = 0;
int NUM_OF_USERS = 0;
int TOTAL_PROCESSES = 0;
int TOTAL_PROCESSES_DIR = 0;
int NOF_PAUSE = 0;
int P_SERV_MIN = 0;
int P_SERV_MAX = 0;
int SIM_DURATION = 0;
int EXPLODE_THRESHOLD = 0;
int MINUTES_FOR_DAY = 480;
int SIMULATED_MINUTE = 4000000;

void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len-1] == '\n') str[len-1] = '\0';
}

void trim(char *s) {
    // rimuove newline e spazi finali
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == ' ' || s[len-1] == '\r')) {
        s[--len] = '\0';
    }

    // rimuove spazi iniziali
    char *start = s;
    while (*start == ' ') {
        start++;
    }

    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }
}

void read_config(char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("apertura config");
        return;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (line[0]=='\0' || line[0]=='#') {
            continue;  // salto righe vuote o commenti
        }

        char *eq = strchr(line, '=');
        if (!eq) {
            continue;
        }

        *eq = '\0';
        char *key   = line;
        char *value = eq + 1;
        trim(key);
        trim(value);

        if (strcmp(key, "NUM_OF_WORKERS") == 0) {
            NUM_OF_WORKERS = atoi(value);
        } else if (strcmp(key, "NUM_OF_USERS") == 0) {
            NUM_OF_USERS = atoi(value);
        } else if (strcmp(key, "NOF_PAUSE") == 0) {
            NOF_PAUSE = atoi(value);
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
