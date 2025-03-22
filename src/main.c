#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_OF_WORKERS 3
#define NUM_OF_USERS 5

void CreateProcess(const char *id) {
    pid_t pid = fork();

    if (pid < 0) {
        printf("[%s] Errore nella fork\n", id);
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        printf("[%s] Processo creato! PID: %d\n", id, getpid());
        sleep(2); // Simuliamo un'operazione
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {
    // ATTENZIONE! Tutto questo codice va spostato nel file del direttore
    // Nel main dovrebbe rimanere la lettura della configurazione e l'esecuzione del direttore con execve

    printf("[Direttore] Avvio della simulazione. PID: %d\n", getpid());

    int total_processes = 0;

    // Creiamo l'erogatore dei ticket
    CreateProcess("Erogatore_ticket");
    total_processes++;

    int i;
    char id_buffer[50];  // Buffer per gli ID dinamici

    // Creiamo tutti gli operatori
    for (i = 0; i < NUM_OF_WORKERS; i++) {
        sprintf(id_buffer, "Operator_%d", i);
        CreateProcess(id_buffer);
        total_processes++;
    }

    // Creiamo tutti gli utenti
    for (i = 0; i < NUM_OF_USERS; i++) {
        sprintf(id_buffer, "User_%d", i);
        CreateProcess(id_buffer);
        total_processes++;
    }

    // Aspettiamo tutti i processi creati
    for (i = 0; i < total_processes; i++) {
        wait(NULL);
    }

    printf("[Direttore] Tutti i processi sono terminati, la simulazione è finita.\n");
    return EXIT_SUCCESS;
}
