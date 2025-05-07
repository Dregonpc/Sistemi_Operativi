#include "../headers/MessageQueueLib.h"

int messageQueueCreate(int flag, char* processName) {
    int msgID = msgget(IPC_PRIVATE, IPC_CREAT | flag);
    if (msgID < 0) {
        printf("[%s] Errore durante la creazione della coda dei messaggi.\n", processName);
        exit(EXIT_FAILURE);
    }

    return msgID;
}

void messageQueueClean(int msgId) {
    msgctl(msgId, IPC_RMID, NULL);
}
