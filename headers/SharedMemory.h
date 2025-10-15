#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include "sportelli.h"
#include "servizi.h"
#include <sys/types.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int num_sportelli;
    Sportello sportelli[];
} DailyConfig;

typedef struct {
    // Contatori globali
    int utenti_serviti_tot_sim; // Numero di utenti serviti totali nella simulazione
    int utenti_serviti_tot_day; // Numero di utenti serviti giornalmente nella simulazione
    int utenti_non_serviti_tot_sim; // Numero di utenti non serviti totali nella simulazione
    int utenti_non_serviti_tot_day; // Numero di utenti serviti totali nella giornata (da usare per explode)
    int servizi_erogati_tot_sim; // Numero di servizi erogati totali nella simuazione
    int servizi_erogati_tot_day; // Numero di servizi erogati giornalmente nella simulazione
    int servizi_non_erogati_tot_sim; // Numero di servizi non erogati totali nella simuazione (comprende utenti non serviti anche se avevano richiesto il servizio)
    int servizi_non_erogati_tot_day; // Numero di servizi non erogati totali nella giornata (comprende utenti non serviti anche se avevano richiesto il servizio)
    int operatori_attivi_day; // Numero di operatori attivi durante la giornata 
    int operatori_attivi_sim; // Numero di operatori attivi durante la simulazione
    int pause_effettuate_tot_sim; // Numero totale di pause effettuate durante la simulazione
    int pause_effettuate_tot_day; // Numero totale di pause effettuate giornalmente durante la simulazione
    int durata_simulazione; // Numero di giorni che dura la simulazione (viene incrementato ogni inizio giornata dal direttore)

    // Medie (gestite dal direttore)
    double utenti_serviti_avg; // Numero di utenti serviti in media al giorno
    double servizi_erogati_avg; // Numero di servizi erogati in media al giorno
    double servizi_non_erogati_avg; // Numero di servizi non erogati in media al giorno
    double pause_effettuate_avg; // Numero medio di pause effettuate al giorno

    // Tempi
    double tempo_attesa_utenti_sim; // Tempo medio di attesa degli utenti nella simulazione attese / utenti_serviti_tot_sim
    double tempo_attesa_utenti_day; // Tempo medio di attesa degli utenti nella giornata
    double tempo_erogazione_servizi_sim; // Tempo medio di erogazione dei servizi nella simulazione
    double tempo_erogazione_servizi_day; // Tempo medio di erogazione dei servizi nella giornata

    // Contatori divisi per servizi
    int utenti_serviti_day_sim_services[NUM_SERVICES]; 
    int utenti_serviti_tot_sim_services[NUM_SERVICES]; 
    int servizi_erogati_day_sim_services[NUM_SERVICES]; 
    int servizi_erogati_tot_sim_services[NUM_SERVICES]; 
    int servizi_non_erogati_tot_sim_services[NUM_SERVICES]; 
    double utenti_serviti_avg_services[NUM_SERVICES]; 
    double servizi_erogati_avg_services[NUM_SERVICES]; 
    double servizi_non_erogati_avg_services[NUM_SERVICES]; 
    double tempo_attesa_utenti_sim_services[NUM_SERVICES]; 
    double tempo_attesa_utenti_day_services[NUM_SERVICES];
    double tempo_erogazione_servizi_sim_services[NUM_SERVICES]; 
    double tempo_erogazione_servizi_day_services[NUM_SERVICES];

    // Rapporto tra operatori disponibili e sportelli esistenti, per ogni servizio e per ogni giornata
    int operatori_disponibili_services[NUM_SERVICES];
    int sportelli_esistenti_services[NUM_SERVICES];
    double rapporto_operatori_sportelli_services[NUM_SERVICES]; // Operatori servizio / Sportelli servizio

    char* termine_simulazione;
} Stats;

/**
 * @brief Crea la memoria condivisa per la configurazione giornaliera.
 * @param size Dimensione della memroia da creare.
 * @param flag Flag da assegnare alla memoria.
 * @param processName Il nome del processo che si sta collegando.
 * @return L'ID della memoria condivisa.
 */
int SharedMemoryCreate(size_t size, int flag, char* processName);

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
void SharedMmemoryDetach(void* config, char* processName);

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