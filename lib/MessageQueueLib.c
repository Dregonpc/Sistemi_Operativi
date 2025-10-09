#include "../headers/MessageQueueLib.h"

int messageQueueCreate(int key, int flag, char* processName) {
    int msgID = msgget(key, IPC_CREAT | flag);
    if (msgID < 0) {
        printf("[%s] Errore durante la creazione della coda dei messaggi.\n", processName);
        exit(EXIT_FAILURE);
    }

    return msgID;
}

void cleanMsgQueue(int msgId) {
    Message msg;
    ssize_t n;

    // Leggi finché ci sono messaggi; IPC_NOWAIT fa tornare subito ENOMSG se è vuota
    while (1) {
        n = msgrcv(msgId, &msg, sizeof(Message) - sizeof(long), 0, IPC_NOWAIT);
        if (n >= 0) {
            // messaggio buttato via
            continue;
        }
        if (errno == ENOMSG) {
            // coda vuota
            break;
        }
        // altro errore
        perror("CleanMsgQueue: msgrcv");
        break;
    }
}

void messageQueueRemove(int msgId) {
    if (msgctl(msgId, IPC_RMID, NULL) < 0) {
        printf("Errore durante la rimozione della coda con id: %d, errno: %d\n", msgId, errno);
    }
}
