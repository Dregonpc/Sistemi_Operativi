#ifndef SEMSLIB_H
#define SEMSLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/sem.h>

int semCreate(int quantity, char* processName);

void semInizialize(int semID, int quantity0, int quantity1, int quantity2, int quantity3, int quantity4, char* processName);

void SemBarrierRestart(int semID, struct sembuf sops, int quantity0, int quantity1, char* processName);

void SemRestart(int semID, struct sembuf sops, int quantity2, int quantity3, int quantity4, char* processName);

// Pulizia dei semafori
void semCleanUp(int semID);

int SemGetVal(int semID, int semNum);

int ExecuteSemop(int semID, struct sembuf sops, int semNum, int semOp);

int CaptureLock(int semID, struct sembuf sops, int semNum);

int ReleaseLock(int semID, struct sembuf sops, int semNum);

#endif // SEMSLIB_H