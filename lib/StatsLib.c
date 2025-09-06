#include "../headers/StatsLib.h"

// FUNCTION FOR DIRETTORE

void StatsInitialize(Stats* stats, int semID) {
    struct sembuf sops = {0}; 

    // acquisisco il lock
    if (CaptureLock(semID, &sops, 4) == -1) {
        printf("[Direttore] Errore durante l'acquisizione del lock per le statistiche.\n");
        return; // TO DO: DECIDERE SE RIPROVARE O LASCIAR STARE
    }

    // Inizializziamo le statistiche
    stats->utenti_serviti_tot_sim = 0;
    stats->utenti_serviti_tot_day = 0;
    stats->utenti_non_serviti_tot_sim = 0;
    stats->utenti_non_serviti_tot_day = 0;
    stats->servizi_erogati_tot_sim = 0;
    stats->servizi_erogati_tot_day = 0;
    stats->servizi_non_erogati_tot_sim = 0;
    stats->servizi_non_erogati_tot_day = 0;
    stats->operatori_attivi_day = 0;
    stats->operatori_attivi_sim = 0;
    stats->pause_effettuate_tot_sim = 0;
    stats->pause_effettuate_tot_day = 0;
    stats->durata_simulazione = 0;

    // Medie (gestite dal direttore)
    stats->utenti_serviti_avg = 0.0;
    stats->servizi_erogati_avg = 0.0;
    stats->servizi_non_erogati_avg = 0.0;
    stats->pause_effettuate_avg = 0.0;

    // Tempi
    stats->tempo_attesa_utenti_sim = 0.0;
    stats->tempo_attesa_utenti_day = 0.0;
    stats->tempo_erogazione_servizi_sim = 0.0;
    stats->tempo_erogazione_servizi_day = 0.0;

    // Contatori divisi per servizi
    for (int i = 0; i < NUM_SERVIZI; i++) {
        stats->utenti_serviti_day_sim_services[i] = 0;
        stats->utenti_serviti_tot_sim_services[i] = 0;
        stats->servizi_erogati_day_sim_services[i] = 0;
        stats->servizi_erogati_tot_sim_services[i] = 0;
        stats->servizi_non_erogati_tot_sim_services[i] = 0;
        stats->utenti_serviti_avg_services[i] = 0.0;
        stats->servizi_erogati_avg_services[i] = 0.0; 
        stats->servizi_non_erogati_avg_services[i] = 0.0; 
        stats->tempo_attesa_utenti_sim_services[i] = 0.0; 
        stats->tempo_attesa_utenti_day_services[i] = 0.0; 
        stats->tempo_erogazione_servizi_sim_services[i] = 0.0; 
        stats->tempo_erogazione_servizi_day_services[i] = 0.0;

        stats->operatori_disponibili_services[i] = 0;
        stats->sportelli_esistenti_services[i] = 0;
        stats->rapporto_operatori_sportelli_services[i] = 0.0;
    }

    stats->termine_simulazione = "SIM_DURATION";

    // rilascio il lock
    if (ReleaseLock(semID, &sops, 4) == -1) {
        printf("[Direttore] Errore durante il rilascio del lock per le statistiche.\n");
    }
}

void ResetStatsDaily(Stats* stats, int semID) {
    struct sembuf sops = {0};

    // acquisisco il lock
    if (CaptureLock(semID, &sops, 4) == -1) {
        printf("[Direttore] Errore durante l'acquisizione del lock per le statistiche.\n");
        return; // TO DO: DECIDERE SE RIPROVARE O LASCIAR STARE
    }

    stats->durata_simulazione++;

    // Resettiamo le statistiche per il nuovo giorno
    stats->utenti_serviti_tot_day = 0;
    stats->utenti_non_serviti_tot_day = 0;
    stats->servizi_erogati_tot_day = 0;
    stats->servizi_non_erogati_tot_day = 0;
    stats->pause_effettuate_tot_day = 0;
    stats->operatori_attivi_day = 0;
    
    stats->tempo_attesa_utenti_day = 0.0;
    stats->tempo_erogazione_servizi_day = 0.0;
    
    for (int i = 0; i < NUM_SERVIZI; i++) {
        stats->utenti_serviti_day_sim_services[i] = 0;
        stats->servizi_erogati_day_sim_services[i] = 0;
        stats->tempo_attesa_utenti_day_services[i] = 0.0;
        stats->tempo_erogazione_servizi_day_services[i] = 0.0;
    }

    // rilascio il lock
    if (ReleaseLock(semID, &sops, 4) == -1) {
        printf("[Direttore] Errore durante il rilascio del lock per le statistiche.\n");
    }
}

void CalculateDailyStats(Stats* stats, int semID) {
    struct sembuf sops = {0};

    // acquisisco il lock
    if (CaptureLock(semID, &sops, 4) == -1) {
        printf("[Direttore] Errore durante l'acquisizione del lock per le statistiche.\n");
        return; // TO DO: DECIDERE SE RIPROVARE O LASCIAR STARE
    }

    // Calcoliamo le statistiche giornalmente
    stats->tempo_attesa_utenti_day = (stats->utenti_serviti_tot_day != 0) ? ((double)stats->tempo_attesa_utenti_day / (double)stats->utenti_serviti_tot_day) : 0;
    stats->tempo_erogazione_servizi_day = (stats->servizi_erogati_tot_day != 0) ? ((double)stats->tempo_erogazione_servizi_day / (double)stats->servizi_erogati_tot_day) : 0;

    for (int i = 0; i < NUM_SERVIZI; i++) {
        stats->tempo_attesa_utenti_day_services[i] = (stats->utenti_serviti_day_sim_services[i] != 0) ? ((double)stats->tempo_attesa_utenti_day_services[i] / (double)stats->utenti_serviti_day_sim_services[i]) : 0;
        stats->tempo_erogazione_servizi_day_services[i] = (stats->servizi_erogati_day_sim_services[i] != 0) ? ((double)stats->tempo_erogazione_servizi_day_services[i] / (double)stats->servizi_erogati_day_sim_services[i]) : 0;
        stats->rapporto_operatori_sportelli_services[i] = (stats->sportelli_esistenti_services[i] != 0) ? ((double)stats->operatori_disponibili_services[i] / (double)stats->sportelli_esistenti_services[i]) : 0;
    }

    // rilascio il lock
    if (ReleaseLock(semID, &sops, 4) == -1) {
        printf("[Direttore] Errore durante il rilascio del lock per le statistiche.\n");
    }
}

void CalculateFinalStats(Stats* stats, int semID) {
    struct sembuf sops = {0};

    // acquisisco il lock
    if (CaptureLock(semID, &sops, 4) == -1) {
        printf("[Direttore] Errore durante l'acquisizione del lock per le statistiche.\n");
        return; // TO DO: DECIDERE SE RIPROVARE O LASCIAR STARE
    }

    // Calcoliamo le statistiche finali
    stats->utenti_serviti_avg = (double)stats->utenti_serviti_tot_sim / (double)stats->durata_simulazione;
    stats->servizi_erogati_avg = (double)stats->servizi_erogati_tot_sim / (double)stats->durata_simulazione;
    stats->servizi_non_erogati_avg = (double)stats->servizi_non_erogati_tot_sim / (double)stats->durata_simulazione;
    stats->pause_effettuate_avg = (double)stats->pause_effettuate_tot_sim / (double)stats->durata_simulazione;

    stats->tempo_attesa_utenti_sim = (double)stats->tempo_attesa_utenti_sim / (double)stats->utenti_serviti_tot_sim;
    stats->tempo_erogazione_servizi_sim = (double)stats->tempo_erogazione_servizi_sim / (double)stats->servizi_erogati_tot_sim;

    // Calcoliamo le medie suddivise per i servizi
    for (int i = 0; i < NUM_SERVIZI; i++) {
        stats->utenti_serviti_avg_services[i] = (double)stats->utenti_serviti_tot_sim_services[i] / (double)stats->durata_simulazione;
        stats->servizi_erogati_avg_services[i] = (double)stats->servizi_erogati_tot_sim_services[i] / (double)stats->durata_simulazione;
        stats->servizi_non_erogati_avg_services[i] = (double)stats->servizi_non_erogati_tot_sim_services[i] / (double)stats->durata_simulazione;
    }

    // rilascio il lock
    if (ReleaseLock(semID, &sops, 4) == -1) {
        printf("[Direttore] Errore durante il rilascio del lock per le statistiche.\n");
    }
}

void PrintDailyStats(Stats* stats) {
    printf("[Direttore] STATISTICHE GIORNO: %d\n", stats->durata_simulazione);
    printf("Utenti serviti giornalmente: %d\n", stats->utenti_serviti_tot_day);
    printf("Utenti NON serviti giornalmente: %d\n", stats->utenti_non_serviti_tot_day);
    printf("Servizi erogati giornalmente: %d\n", stats->servizi_erogati_tot_day);
    printf("Servizi non erogati giornalmente: %d\n", stats->servizi_non_erogati_tot_day);
    printf("Operatori attivi giornalmente: %d\n", stats->operatori_attivi_day);
    printf("Pause effettuate giornalmente: %d\n", stats->pause_effettuate_tot_day);
    printf("Tempo medio di attesa degli utenti giornalmente: %.0f nanosecondi\n", stats->tempo_attesa_utenti_day);
    printf("Tempo medio di erogazione dei servizi giornalmente: %.0f nanosecondi\n", stats->tempo_erogazione_servizi_day);

    printf("   | Tempo_attesa_day | tempo_erogazione_day | rapporto_operatori_sportelli |\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        printf("%1d: | %16.0f | %20.0f | %28.4f |\n", i, stats->tempo_attesa_utenti_day_services[i], stats->tempo_erogazione_servizi_day_services[i], stats->rapporto_operatori_sportelli_services[i]);
    }

    // Resettiamo qui questi valori perchè li valorizziamo prima di iniziare il nuovo giorno, quindi non possiamo metterli nella funzione reset
    for (int i = 0; i < NUM_SERVIZI; i++) {
        stats->operatori_disponibili_services[i] = 0;
        stats->sportelli_esistenti_services[i] = 0;
        stats->rapporto_operatori_sportelli_services[i] = 0.0;
    }
}

void PrintFinalStats(Stats* stats) {
    printf("[Direttore] STATISTICHE FINALI:\n");
    printf("Motivo termine simulazione: %s\n", stats->termine_simulazione);
    printf("Utenti serviti totali: %d\n", stats->utenti_serviti_tot_sim);
    printf("Utenti NON serviti totali: %d\n", stats->utenti_non_serviti_tot_sim);
    printf("Servizi erogati totali: %d\n", stats->servizi_erogati_tot_sim);
    printf("Servizi non erogati totali: %d\n", stats->servizi_non_erogati_tot_sim);
    printf("Operatori attivi totali: %d\n", stats->operatori_attivi_sim);
    printf("Pause effettuate totali: %d\n", stats->pause_effettuate_tot_sim);
    printf("Durata della simulazione: %d giorni\n", stats->durata_simulazione);
    printf("Numero di utenti serviti in media al giorno: %.4lf\n", stats->utenti_serviti_avg);
    printf("Numero di servizi erogati in media al giorno: %.4lf\n", stats->servizi_erogati_avg);
    printf("Numero di servizi NON erogati in media al giorno: %.4lf\n", stats->servizi_non_erogati_avg);
    printf("Numero medio di pause effettuate al giorno: %.4lf\n", stats->pause_effettuate_avg);
    printf("Tempo medio di attesa degli utenti: %.0f nanosecondi\n", stats->tempo_attesa_utenti_sim);
    printf("Tempo medio di erogazione dei servizi: %.0f nanosecondi\n", stats->tempo_erogazione_servizi_sim);

    printf("   | utenti serv. | serv. erogati | serv. non erogati |\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        printf("%1d: | %12.0d | %13.0d | %17.0d |\n", i, stats->utenti_serviti_tot_sim_services[i], stats->servizi_erogati_tot_sim_services[i], stats->servizi_non_erogati_tot_sim_services[i]);
    }

    printf("   | utenti serv. avg | serv. erogati avg | serv. non erogati avg |\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        printf("%1d: | %16.4f | %17.4f | %21.4f |\n", i, stats->utenti_serviti_avg_services[i], stats->servizi_erogati_avg_services[i], stats->servizi_non_erogati_avg_services[i]);
    }

    printf("   | tempo utenti | tempo servizi |\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        printf("%1d: | %12.0f | %13.0f |\n", i, stats->tempo_attesa_utenti_sim_services[i], stats->tempo_erogazione_servizi_sim_services[i]);
    }
}

void WriteDailyStatsCSV(const char *filename, Stats* stats) {
    FILE *f = fopen(filename, "a");
    if (!f) {
        perror("fopen");
        return;
    }

    // Intestazione generale giornaliera
    fprintf(f, "STATISTICHE_GIORNO;%d\n", stats->durata_simulazione);
    fprintf(f, "utenti_serviti_tot_day;%d\n", stats->utenti_serviti_tot_day);
    fprintf(f, "utenti_non_serviti_tot_day;%d\n", stats->utenti_non_serviti_tot_day);
    fprintf(f, "servizi_erogati_tot_day;%d\n", stats->servizi_erogati_tot_day);
    fprintf(f, "servizi_non_erogati_tot_day;%d\n", stats->servizi_non_erogati_tot_day);
    fprintf(f, "operatori_attivi_day;%d\n", stats->operatori_attivi_day);
    fprintf(f, "pause_effettuate_tot_day;%d\n", stats->pause_effettuate_tot_day);
    fprintf(f, "tempo_attesa_utenti_day_ns;%.0f\n", stats->tempo_attesa_utenti_day);
    fprintf(f, "tempo_erogazione_servizi_day_ns;%.0f\n", stats->tempo_erogazione_servizi_day);

    // Intestazione per array giornalieri
    fprintf(f, "service_index;tempo_attesa_utenti_day_ns;tempo_erogazione_servizi_day_ns;rapporto_operatori_sportelli\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "%d,%.0f,%.0f,%.0f\n", i, stats->tempo_attesa_utenti_day_services[i], stats->tempo_erogazione_servizi_day_services[i], stats->rapporto_operatori_sportelli_services[i]);
    }

    fclose(f);
}

void WriteFinalStatsCSV(const char *filename, Stats* stats) {
    FILE *f = fopen(filename, "a");
    if (!f) {
        perror("fopen");
        return;
    }

    // Intestazione generale
    fprintf(f, "STATISTICHE_FINALI\n");
    fprintf(f, "motivo_termine_simulazione;%s\n", stats->termine_simulazione);
    fprintf(f, "utenti_serviti_tot_sim;%d\n", stats->utenti_serviti_tot_sim);
    fprintf(f, "utenti_non_serviti_tot_sim;%d\n", stats->utenti_non_serviti_tot_sim);
    fprintf(f, "servizi_erogati_tot_sim;%d\n", stats->servizi_erogati_tot_sim);
    fprintf(f, "servizi_non_erogati_tot_sim;%d\n", stats->servizi_non_erogati_tot_sim);
    fprintf(f, "operatori_attivi_sim;%d\n", stats->operatori_attivi_sim);
    fprintf(f, "pause_effettuate_tot_sim;%d\n", stats->pause_effettuate_tot_sim);
    fprintf(f, "durata_simulazione;%d\n", stats->durata_simulazione);
    fprintf(f, "utenti_serviti_avg;%.4f\n", stats->utenti_serviti_avg);
    fprintf(f, "servizi_erogati_avg;%.4f\n", stats->servizi_erogati_avg);
    fprintf(f, "servizi_non_erogati_avg;%.4f\n", stats->servizi_non_erogati_avg);
    fprintf(f, "pause_effettuate_avg;%.4f\n", stats->pause_effettuate_avg);
    fprintf(f, "tempo_attesa_utenti_sim_ns;%.0f\n", stats->tempo_attesa_utenti_sim);
    fprintf(f, "tempo_erogazione_servizi_sim_ns;%.0f\n", stats->tempo_erogazione_servizi_sim);

    // Intestazione per array dei servizi totali
    fprintf(f, "service_index;utenti_serviti_tot;servizi_erogati_tot;servizi_non_erogati_tot\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "%d,%d,%d,%d\n", i, stats->utenti_serviti_tot_sim_services[i], stats->servizi_erogati_tot_sim_services[i], stats->servizi_non_erogati_tot_sim_services[i]);
    }

    // Intestazione per array dei servizi avg
    fprintf(f, "service_index;utenti_serviti_avg;servizi_erogati_avg;servizi_non_erogati_avg\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "%d,%.2f,%.2f,%.2f\n", i, stats->utenti_serviti_avg_services[i], stats->servizi_erogati_avg_services[i], stats->servizi_non_erogati_avg_services[i]);
    }

    // Intestazione per tempi
    fprintf(f, "service_index;tempo_attesa_utenti_sim_ns;tempo_erogazione_servizi_sim_ns\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "%d,%.0f,%.0f\n", i, stats->tempo_attesa_utenti_sim_services[i], stats->tempo_erogazione_servizi_sim_services[i]);
    }

    fclose(f);
}

// FUNCTION FOR OPERATORS

void UpdateStatsOperators(int semID, Stats* stats, char *operatoreId, int IndexServizio, int* servizi_erogati, int* operatori_attivi, int* counter_pause, double* tempo_erogazione) {
    struct sembuf sops = {0};
    
    // acquisisco il lock
    if (CaptureLock(semID, &sops, 4) == -1) {
        printf("[%s] Errore durante l'acquisizione del lock per le statistiche.\n", operatoreId);
        return; // TO DO: DECIDERE SE RIPROVARE O LASCIAR STARE
    }

    #ifdef DEBUG
        printf("[%s] Aggiorno le statistiche...\n", operatoreId);
    #endif

    // Scrivo statistiche
    stats->servizi_erogati_tot_sim += *servizi_erogati;
    stats->servizi_erogati_tot_day += *servizi_erogati;
    stats->operatori_attivi_day += *operatori_attivi;
    stats->operatori_attivi_sim += *operatori_attivi;
    stats->pause_effettuate_tot_sim += *counter_pause;
    stats->pause_effettuate_tot_day += *counter_pause;
    stats->tempo_erogazione_servizi_day += *tempo_erogazione;
    stats->tempo_erogazione_servizi_sim += *tempo_erogazione;
    stats->servizi_erogati_day_sim_services[IndexServizio] += *servizi_erogati;
    stats->servizi_erogati_tot_sim_services[IndexServizio] += *servizi_erogati;
    stats->tempo_erogazione_servizi_day_services[IndexServizio] += *tempo_erogazione;
    stats->tempo_erogazione_servizi_sim_services[IndexServizio] += *tempo_erogazione;

    stats->operatori_disponibili_services[IndexServizio] += 1;

    // rilascio il lock
    if (ReleaseLock(semID, &sops, 4) == -1) {
        printf("[%s] Errore durante il rilascio del lock per le statistiche.\n", operatoreId);
    }
}

// FUNCTION FOR USERS

void UpdateStaticStatsUsers(int semID, Stats* stats, char *utenteId, int* utenti_serviti, int* utenti_non_serviti_day, long* time_total, int* servizi_non_erogati) {
    struct sembuf sops = {0};
    
    // acquisisco il lock
    if (CaptureLock(semID, &sops, 4) == -1) {
        printf("[%s] Errore durante l'acquisizione del lock per le statistiche.\n", utenteId);
        return; // TO DO: DECIDERE SE RIPROVARE O LASCIAR STARE
    }

    #ifdef DEBUG
        printf("[%s] Aggiorno le statistiche...\n", utenteId);
    #endif
    
    // Scrivo statistiche
    stats->utenti_serviti_tot_day += *utenti_serviti;
    stats->utenti_serviti_tot_sim += *utenti_serviti;
    stats->utenti_non_serviti_tot_day += *utenti_non_serviti_day;
    stats->utenti_non_serviti_tot_sim += *utenti_non_serviti_day;
    stats->servizi_non_erogati_tot_sim += *servizi_non_erogati;
    stats->servizi_non_erogati_tot_day += *servizi_non_erogati;
    
    stats->tempo_attesa_utenti_sim += *time_total;
    stats->tempo_attesa_utenti_day += *time_total;
    
    // rilascio il lock
    if (ReleaseLock(semID, &sops, 4) == -1) {
        printf("[%s] Errore durante il rilascio del lock per le statistiche.\n", utenteId);
    }
}

void UpdateDynamicStatsUsers(int semID, Stats* stats, char *utenteId, int IndexServizioRichiesto,  int* utenti_serviti, int* utenti_non_serviti_day, long* time_total, int* servizi_non_erogati) {
    struct sembuf sops = {0};
    
    // acquisisco il lock
    if (CaptureLock(semID, &sops, 4) == -1) {
        printf("[%s] Errore durante l'acquisizione del lock per le statistiche.\n", utenteId);
        return; // TO DO: DECIDERE SE RIPROVARE O LASCIAR STARE
    }
    
    stats->utenti_serviti_day_sim_services[IndexServizioRichiesto] += *utenti_serviti;
    stats->utenti_serviti_tot_sim_services[IndexServizioRichiesto] += *utenti_serviti;
    stats->servizi_non_erogati_tot_sim_services[IndexServizioRichiesto] += *servizi_non_erogati;
    stats->tempo_attesa_utenti_day_services[IndexServizioRichiesto] += *time_total;
    stats->tempo_attesa_utenti_sim_services[IndexServizioRichiesto] += *time_total;
    
    // rilascio il lock
    if (ReleaseLock(semID, &sops, 4) == -1) {
        printf("[%s] Errore durante il rilascio del lock per le statistiche.\n", utenteId);
    }
}
