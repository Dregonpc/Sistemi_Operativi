#ifndef MESSAGEQUEUELIB_H
#define MESSAGEQUEUELIB_H

#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "messaggi.h"

/**
 * @brief Crea una coda di messaggi
 * @param flag flag da assgenare alla coda di messaggi
 * @param processName nome del processo chiamante
 * @return L'ID assegnato alla coda di messaggi
 */
int messageQueueCreate(int flag, char* processName);

/**
 * @brief Pulisce una coda di messaggi
 * @param msgId ID della coda di messaggi da pulire
 */
void cleanMsgQueue(int msgId);

/**
 * @brief Rimuove una coda di messaggi
 * @param msgId ID della coda di messaggi da rimuovere
 */
void messageQueueRemove(int msgId);

#endif // MESSAGEQUEUELIB_H