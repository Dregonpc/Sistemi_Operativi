#include <stdio.h>
#include <stdlib.h>
#include "../headers/MessageQueueLib.h"

#define MAX_NEW_USERS 20

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("[addUsers] Errore: numero di argomenti non valido!\nUsare: %s <n_new_users>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = atoi(argv[1]);

    if (n <= 0 || n > MAX_NEW_USERS) {
        printf("[addUsers] Errore: il numero inserito deve essere compreso tra 1 e %d.\n", MAX_NEW_USERS);
        return EXIT_FAILURE;
    }

    int msgId = messageQueueCreate(PUBLIC_KEY, 0666, "addUsers");

    NewUserMsg msg;
    msg.mtype = 1;
    msg.new_users = n;

    if (msgsnd(msgId, &msg, sizeof(msg.new_users), 0) < 0) {
        printf("[addUsers] Errore durante l'invio del messaggio.\n");
        return EXIT_FAILURE;
    }

    printf("[addUsers] Richiesti %d nuovi utenti.\n", n);

    return EXIT_SUCCESS;
}