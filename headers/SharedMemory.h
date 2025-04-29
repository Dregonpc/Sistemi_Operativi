#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include "sportelli.h"
#include "servizi.h"

typedef struct {
    Sportello sportelli[NUM_SPORTELLI];
} DailyConfig;

typedef struct {
    // Contatori globali
    int utenti_serviti_tot_sim; // numero di utenti serviti totali nella simulazione
    int servizi_erogati_tot_sim; // numero di servizi erogati totali nella simuazione
    int servizi_non_erogati_tot_sim; // numero di servizi non erogati totali nella simuazione
    int servizi_non_erogati_tot_day; // numero di servizi non erogati totali nella giornata (da usare per explode)
    int operatori_attivi_day; // numero di operatori attivi durante la giornata
    int operatori_attivi_sim; // numero di operatori attivi durante la simulazione
    int pause_effettuate_sim; // numero totale di pause effettuate durante la simulazione
    int durata_simulazione; // numero di giorni che dura la simulazione (viene incrementato ogni inizio giornata dal direttore)

    // Medie (gestite dal direttore)
    double utenti_serviti_avg; // numero di utenti serviti in media al giorno
    double servizi_erogati_avg; // numero di servizi erogati in media al giorno
    double servizi_non_erogati_avg; // numero di servizi non erogati in media al giorno
    double pause_effettuate_avg; // numero medio di pause effettuate al giorno

    // Tempi
    double tempo_attesa_utenti_sim; // tempo medio di attesa degli utenti nella simulazione
    double tempo_attesa_utenti_day; // tempo medio di attesa degli utenti nella giornata
    double tempo_erogazione_servizi_sim; // tempo medio di erogazione dei servizi nella simulazione
    double tempo_erogazione_servizi_day; // tempo medio di erogazione dei servizi nella giornata

    // Contatori divisi per servizi
    int utenti_serviti_tot_sim_services[NUM_SERVIZI];
    int servizi_erogati_tot_sim_services[NUM_SERVIZI];
    int servizi_non_erogati_tot_sim_services[NUM_SERVIZI];
    double utenti_serviti_avg_services[NUM_SERVIZI];
    double servizi_erogati_avg_services[NUM_SERVIZI];
    double servizi_non_erogati_avg_services[NUM_SERVIZI];
    double tempo_attesa_utenti_sim_services[NUM_SERVIZI];
    double tempo_attesa_utenti_day_services[NUM_SERVIZI];
    double tempo_erogazione_servizi_sim_services[NUM_SERVIZI];
    double tempo_erogazione_servizi_day_services[NUM_SERVIZI];

    // ANCORA DA FARE (come tutto il resto)
    // Rapporto tra operatori disponibili e sportelli esistenti, per ogni servizio e per ogni giornata
    int operatori_disponibili_services[NUM_SERVIZI]; // Facciamo in ogni operatore a fine giornata += 1 nel proprio index
    int sportelli_esistenti_services[NUM_SERVIZI];
    double rapporto_operatori_sportelli_services[NUM_SERVIZI];
} Stats;

#endif //SHAREDMEMORY_H   