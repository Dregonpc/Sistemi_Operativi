#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include "sportelli.h"
#include "servizi.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    Sportello sportelli[NUM_SPORTELLI];
} DailyConfig;

typedef struct {
    // Contatori globali
    int utenti_serviti_tot_sim; // numero di utenti serviti totali nella simulazione
    int utenti_serviti_tot_day; // numero di utenti serviti giornalmente nella simulazione
    int utenti_non_serviti_tot_sim; // numero di utenti non serviti totali nella simulazione
    int utenti_non_serviti_tot_day; // numero di utenti serviti totali nella giornata (da usare per explode)
    int servizi_erogati_tot_sim; // numero di servizi erogati totali nella simuazione
    int servizi_erogati_tot_day; // numero di servizi erogati giornalmente nella simulazione
    int servizi_non_erogati_tot_sim; // numero di servizi non erogati totali nella simuazione (comprende utenti non serviti anche se avevano richiesto il servizio)
    int servizi_non_erogati_tot_day; // numero di servizi non erogati totali nella giornata (comprende utenti non serviti anche se avevano richiesto il servizio)
    int operatori_attivi_day; // numero di operatori attivi durante la giornata //? TO FIX
    int operatori_attivi_sim; // numero di operatori attivi durante la simulazione //? TO FIX
    int pause_effettuate_tot_sim; // numero totale di pause effettuate durante la simulazione
    int pause_effettuate_tot_day; // numero totale di pause effettuate giornalmente durante la simulazione
    int durata_simulazione; // numero di giorni che dura la simulazione (viene incrementato ogni inizio giornata dal direttore)

    // Medie (gestite dal direttore)
    double utenti_serviti_avg; // numero di utenti serviti in media al giorno
    double servizi_erogati_avg; // numero di servizi erogati in media al giorno
    double servizi_non_erogati_avg; // numero di servizi non erogati in media al giorno
    double pause_effettuate_avg; // numero medio di pause effettuate al giorno

    // Tempi
    double tempo_attesa_utenti_sim; // tempo medio di attesa degli utenti nella simulazione attese / utenti_serviti_tot_sim
    double tempo_attesa_utenti_day; // tempo medio di attesa degli utenti nella giornata
    double tempo_erogazione_servizi_sim; // tempo medio di erogazione dei servizi nella simulazione
    double tempo_erogazione_servizi_day; // tempo medio di erogazione dei servizi nella giornata

    // Contatori divisi per servizi
    int utenti_serviti_day_sim_services[NUM_SERVIZI]; 
    int utenti_serviti_tot_sim_services[NUM_SERVIZI]; 
    int servizi_erogati_day_sim_services[NUM_SERVIZI]; 
    int servizi_erogati_tot_sim_services[NUM_SERVIZI]; 
    int servizi_non_erogati_tot_sim_services[NUM_SERVIZI]; 
    double utenti_serviti_avg_services[NUM_SERVIZI]; 
    double servizi_erogati_avg_services[NUM_SERVIZI]; 
    double servizi_non_erogati_avg_services[NUM_SERVIZI]; 
    double tempo_attesa_utenti_sim_services[NUM_SERVIZI]; 
    double tempo_attesa_utenti_day_services[NUM_SERVIZI];
    double tempo_erogazione_servizi_sim_services[NUM_SERVIZI]; 
    double tempo_erogazione_servizi_day_services[NUM_SERVIZI];

    // Rapporto tra operatori disponibili e sportelli esistenti, per ogni servizio e per ogni giornata
    int operatori_disponibili_services[NUM_SERVIZI];
    int sportelli_esistenti_services[NUM_SERVIZI];
    double rapporto_operatori_sportelli_services[NUM_SERVIZI]; // operatori servizio / sportelli servizio
} Stats;

/**
 * @brief Crea la memoria condivisa per la configurazione giornaliera.
 * @return L'ID della memoria condivisa.
 */
int SharedMemoryCreate();

/**
 * @brief Collega il processo alla memoria condivisa.
 * @param shmID L'ID della memoria condivisa.
 * @param processName Il nome del processo che si sta collegando.
 * @return Un puntatore alla struttura DailyConfig.
 */
DailyConfig* SharedMemoryAttach(int shmID, char* processName);

/**
 * @brief Collega il processo alla memoria condivisa per le statistiche.
 * @param shmID L'ID della memoria condivisa.
 * @return Un puntatore alla struttura Stats.
 */
Stats* SharedMemoryAttachStats(int shmID, char* processName);

/**
 * @brief Collega il processo alla memoria condivisa in modo generico.
 * @param shmID L'ID della memoria condivisa.
 * @param processName Il nome del processo che si sta collegando.
 * @return Un puntatore void alla memoria condivisa.
 */
void* SharedMemoryAttachGeneral(int shmID, char* processName);

/**
 * 
 * @brief Scollega la memoria condivisa dal processo.
 * @param config Il puntatore alla struttura della memoria condivisa.
 * @param processName Il nome del processo che si sta scollegando.
 */
void SharedMmemoryDeTouch(void* config, char* processName);

/**
 * @brief Pulisce la memoria condivisa e la cancella.
 * @param shmID L'ID della memoria condivisa.
 * @param config Il puntatore alla struttura DailyConfig.
 */
void SharedMemoryCleanConfig(int shmID, DailyConfig* config);

/**
 * @brief Pulisce la memoria condivisa e la cancella.
 * @param shmID L'ID della memoria condivisa.
 * @param config Il puntatore alla struttura Stats.
 */
void SharedMemoryCleanStats(int shmID, Stats* stats);

#endif //SHAREDMEMORY_H