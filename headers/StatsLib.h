#ifndef STATSLIB_H
#define STATSLIB_H

#include "../headers/SharedMemory.h"
#include "../headers/SemsLib.h"
#include <stdio.h>

/**
 * @brief Inizializzazione delle statistiche a 0
 * @param stats puntatore alla struttura delle statistiche
 * @param semID identificativo del semaforo per il lock
 */
void StatsInitialize(Stats* stats, int semID);

/**
 * @brief Resetta le statistiche giornaliere
 * @param stats puntatore alla struttura delle statistiche
 * @param semID identificativo del semaforo per il lock
 */
void ResetStatsDaily(Stats* stats, int semID);

/**
 * @brief Calcola le statistiche giornaliere
 * @param stats puntatore alla struttura delle statistiche
 * @param semID identificativo del semaforo per il lock
 */
void CalculateDailyStats(Stats* stats, int semID);

/**
 * @brief Calcola le statistiche alla fine della simulazione
 * @param stats puntatore alla struttura delle statistiche
 * @param semID identificativo del semaforo per il lock
 */
void CalculateFinalStats(Stats* stats, int semID);

/**
 * @brief Stampa le statistiche giornaliere
 * @param stats puntatore alla struttura delle statistiche
 */
void PrintDailyStats(Stats* stats);

/**
 * @brief Stampa le statistiche finali
 * @param stats puntatore alla struttura delle statistiche
 */ 
void PrintFinalStats(Stats* stats);

/**
 * @brief Scrive le statistiche giornaliere in un file CSV
 * @param filename nome del file CSV
 * @param stats puntatore alla struttura delle statistiche
 */
void WriteDailyStatsCSV(const char *filename, Stats* stats);


/**
 * @brief Scrive le statistiche finali in un file CSV
 * @param filename nome del file CSV
 * @param stats puntatore alla struttura delle statistiche
 */
void WriteFinalStatsCSV(const char *filename, Stats* stats);

//? Creare una struttura per passare un solo puntatore per le stats?
/**
 * @brief Aggiorna le statistiche da parte gli operatori
 * @param semID identificativo del semaforo per il lock
 * @param sops operazione da eseguire sul semaforo
 * @param stats puntatore alla struttura delle statistiche
 * @param operatoreId identificativo dell'operatore
 * @param IndexServizio indice del servizio dell' operatore
 * @param servizi_erogati puntatore al numero di servizi erogati
 * @param operatori_attivi puntatore al numero di operatori attivi
 * @param counter_pause puntatore al numero di pause effettuate
 * @param tempo_erogazione puntatore al tempo di erogazione
 */
void UpdateStatsOperators(int semID, struct sembuf sops, Stats* stats, char *operatoreId, int IndexServizio, int* servizi_erogati, int* operatori_attivi, int* counter_pause, double* tempo_erogazione);

//? Creare una struttura per passare un solo puntatore per le stats?
/**
 * @brief Aggiorna le statistiche da parte degli utenti
 * @param semID identificativo del semaforo per il lock
 * @param sops operazione da eseguire sul semaforo
 * @param stats puntatore alla struttura delle statistiche
 * @param utenteId identificativo dell'utente
 * @param IndexServizioRichiesto indice del servizio richiesto dall'utente
 * @param utenti_serviti puntatore al numero di utenti serviti
 * @param utenti_non_serviti_day puntatore al numero di utenti non serviti nella giornata
 * @param time_total puntatore al tempo totale di attesa
 * @param servizi_non_erogati puntatore al numero di servizi non erogati
 */
void UpdateStaticStatsUsers(int semID, struct sembuf sops, Stats* stats, char *utenteId, int* utenti_serviti, int* utenti_non_serviti_day, long* time_total, int* servizi_non_erogati);

void UpdateDynamicStatsUsers(int semID, struct sembuf sops, Stats* stats, char *utenteId, int IndexServizioRichiesto,  int* utenti_serviti, int* utenti_non_serviti_day, long* time_total, int* servizi_non_erogati);

#endif // STATSLIB_H