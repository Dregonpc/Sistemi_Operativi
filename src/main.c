#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("[Main] Leggo file di configurazione e passo i dati al direttore.\n");

    char* direttore_args[] = {"Direttore", NULL};
    execve("./bin/direttore", direttore_args, NULL);

    printf("[Main] Errore nella execve\n");
    exit(EXIT_FAILURE);

    return EXIT_SUCCESS;
}
