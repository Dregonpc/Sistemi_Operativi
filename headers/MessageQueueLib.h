#ifndef MESSAGEQUEUELIB_H
#define MESSAGEQUEUELIB_H

#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "messaggi.h"

int messageQueueCreate(int flag, char* processName);

void cleanMsgQueue(int msgId);

void messageQueueRemove(int msgId);

#endif // MESSAGEQUEUELIB_H