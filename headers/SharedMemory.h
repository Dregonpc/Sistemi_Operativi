#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include "sportelli.h"
#include "servizi.h"

typedef struct {
    Sportello sportelli[NUM_SPORTELLI];
} DailyConfig;

typedef struct {
    // Contatori globali
    int utenti_serviti_tot_sim; // numero di utenti serviti totali nella simulazione //? DONE DONE
    int utenti_serviti_tot_day; // numero di utenti serviti giornalmente nella simulazione //? DONE DONE
    int utenti_non_serviti_tot_sim; // numero di utenti non serviti totali nella simulazione //? DONE DONE
    int utenti_non_serviti_tot_day; // numero di utenti serviti totali nella giornata (da usare per explode) //? DONE DONE
    int servizi_erogati_tot_sim; // numero di servizi erogati totali nella simuazione //? DONE DONE
    int servizi_erogati_tot_day; // numero di servizi erogati giornalmente nella simulazione //? DONE DONE
    int servizi_non_erogati_tot_sim; // numero di servizi non erogati totali nella simuazione (comprende utenti non serviti + sportelli non occupati) //? DONE DONE
    int servizi_non_erogati_tot_day; // numero di servizi non erogati totali nella giornata (comprende utenti non serviti + sportelli non occupati) //? DONE DONE
    int operatori_attivi_day; // numero di operatori attivi durante la giornata //? DONE DONE
    int operatori_attivi_sim; // numero di operatori attivi durante la simulazione //? DONE DONE
    int pause_effettuate_tot_sim; // numero totale di pause effettuate durante la simulazione //? DONE DONE
    int pause_effettuate_tot_day; // numero totale di pause effettuate giornalmente durante la simulazione //? DONE DONE
    int durata_simulazione; // numero di giorni che dura la simulazione (viene incrementato ogni inizio giornata dal direttore) //? DONE DONE

    // Medie (gestite dal direttore)
    double utenti_serviti_avg; // numero di utenti serviti in media al giorno //? DONE DONE
    double servizi_erogati_avg; // numero di servizi erogati in media al giorno //? DONE DONE
    double servizi_non_erogati_avg; // numero di servizi non erogati in media al giorno //? DONE DONE
    double pause_effettuate_avg; // numero medio di pause effettuate al giorno //? DONE DONE

    // Tempi
    double tempo_attesa_utenti_sim; // tempo medio di attesa degli utenti nella simulazione attese / utenti_serviti_tot_sim //? DONE DONE
    double tempo_attesa_utenti_day; // tempo medio di attesa degli utenti nella giornata //? DONE DONE
    double tempo_erogazione_servizi_sim; // tempo medio di erogazione dei servizi nella simulazione //? DONE DONE
    double tempo_erogazione_servizi_day; // tempo medio di erogazione dei servizi nella giornata //? DONE DONE

    // Contatori divisi per servizi
    int utenti_serviti_tot_sim_services[NUM_SERVIZI]; //? DONE
    int servizi_erogati_tot_sim_services[NUM_SERVIZI]; //? DONE
    int servizi_non_erogati_tot_sim_services[NUM_SERVIZI]; //? DONE
    double utenti_serviti_avg_services[NUM_SERVIZI]; //? DONE
    double servizi_erogati_avg_services[NUM_SERVIZI]; //? DONE
    double servizi_non_erogati_avg_services[NUM_SERVIZI]; //? DONE
    double tempo_attesa_utenti_sim_services[NUM_SERVIZI]; //? DONE
    double tempo_attesa_utenti_day_services[NUM_SERVIZI]; //? DONE DONE
    double tempo_erogazione_servizi_sim_services[NUM_SERVIZI]; //? DONE
    double tempo_erogazione_servizi_day_services[NUM_SERVIZI]; //? DONE DONE

    // Rapporto tra operatori disponibili e sportelli esistenti, per ogni servizio e per ogni giornata
    int operatori_disponibili_services[NUM_SERVIZI]; // Facciamo in ogni operatore a fine giornata += 1 nel proprio index //? DONE
    int sportelli_esistenti_services[NUM_SERVIZI]; //? DONE
    double rapporto_operatori_sportelli_services[NUM_SERVIZI]; // operatori servizio / sportelli servizio //? DONE
} Stats;

#endif //SHAREDMEMORY_H