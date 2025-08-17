#ifndef SEMSLIB_H
#define SEMSLIB_H

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/sem.h>

#define NUM_OF_SEM 5

/**
 * @brief Crea una collezione di semaforo
 * @param flag Flag da assegnare al set di semafori
 * @param numOfSem Numero di semafori da creare nella collezione
 * @param processName Nome del processo che crea i semafori
 * @return ID del semaforo creato
 */
int semCreate(int flag, int numOfSem, char* processName);

/**
 * @brief Inizializza i semafori
 * @param semID ID del semaforo
 * @param quantity0 Valore iniziale del semaforo 0
 * @param quantity1 Valore iniziale del semaforo 1
 * @param quantity2 Valore iniziale del semaforo 2
 * @param quantity3 Valore iniziale del semaforo 3
 * @param quantity4 Valore iniziale del semaforo 4
 * @param processName Nome del processo che inizializza i semafori
 */
void semInizialize(int semID, int quantity0, int quantity1, int quantity2, int quantity3, int quantity4, char* processName);

/**
 * @brief Inizializza i semafori per la gestione delle code di messaggi
 * @param semID ID del semaforo
 * @param numOfService Numero dei servizi => numero di semafori nella collezione
 * @param processName Nome del processo che inizializza i semafori
 */
void semMessageInitialize(int semID, int numOfServices, char* processName);

/**
 * @brief Inizializza i semafori per la barriera
 * @param semID ID del semaforo
 * @param sops Operazione da eseguire sui semafori
 * @param quantity0 Valore iniziale del semaforo 0
 * @param quantity1 Valore iniziale del semaforo 1
 * @param processName Nome del processo che inizializza i semafori
 */
void SemBarrierRestart(int semID, struct sembuf sops, int quantity0, int quantity1, char* processName);

/**
 * @brief Inizializza i semafori per il restart
 * @param semID ID del semaforo
 * @param sops Operazione da eseguire sui semafori
 * @param quantity2 Valore iniziale del semaforo 2
 * @param quantity3 Valore iniziale del semaforo 3
 * @param quantity4 Valore iniziale del semaforo 4
 * @param processName Nome del processo che inizializza i semafori
 */
void SemRestart(int semID, struct sembuf sops, int quantity2, int quantity3, int quantity4, char* processName);

/**
 * @brief Rimuove il set di semafori
 * @param semID ID del semaforo
 */
void semCleanUp(int semID);

/**
 * @brief Ottiene il valore di un semaforo
 * @param semID ID del semaforo
 * @param semNum Numero del semaforo
 */
int SemGetVal(int semID, int semNum);

/**
 * @brief Esegue un'operazione su un semaforo
 * @param semID ID del semaforo
 * @param sops Struct per l'operazione da eseguire
 * @param semNum Numero del semaforo
 * @param semOp Operazione da eseguire
 */
int ExecuteSemop(int semID, struct sembuf sops, int semNum, int semOp);

/**
 * @brief Acquisisce un lock su un semaforo
 * @param semID ID del semaforo
 * @param sops Struct per l'operazione da eseguire
 * @param semNum Numero del semaforo
 */
int CaptureLock(int semID, struct sembuf sops, int semNum);

/**
 * @brief Rilascia un lock su un semaforo
 * @param semID ID del semaforo
 * @param sops Struct per l'operazione da eseguire
 * @param semNum Numero del semaforo
 */
int ReleaseLock(int semID, struct sembuf sops, int semNum);

#endif // SEMSLIB_H