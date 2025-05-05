#ifndef STATSLIB_H
#define STATSLIB_H

#include "../headers/SharedMemory.h"
#include <stdio.h>
#include <sys/sem.h>

void StatsInitialize(Stats* stats, int semID);

void ResetStatsDaily(Stats* stats, int semID);

void CalculateDailyStats(Stats* stats, int semID);

void PrintDailyStats(Stats* stats);

void PrintFinalStats(Stats* stats);

// Scrive le statistiche giornaliere in un file CSV
void WriteDailyStatsCSV(const char *filename, Stats* stats);

// Scrive le statistiche finali in un file CSV
void WriteFinalStatsCSV(const char *filename, Stats* stats);

void UpdateStatsOperators(int semID, struct sembuf sops, Stats* stats, char *operatoreId, int IndexServizio, int* servizi_erogati, int* operatori_attivi, int* counter_pause, double* tempo_erogazione);

void UpdateStatsUsers(int semID, struct sembuf sops, Stats* stats, char *utenteId, int IndexServizioRichiesto,  int* utenti_serviti, int* utenti_non_serviti_day, long* time_total, int* servizi_non_erogati);

#endif // STATSLIB_H