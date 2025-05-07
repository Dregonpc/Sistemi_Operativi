#ifndef MESSAGEQUEUELIB_H
#define MESSAGEQUEUELIB_H

#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>

int messageQueueCreate(int flag, char* processName);

void messageQueueClean(int msgId);

#endif // MESSAGEQUEUELIB_H