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
    printf("\n");
    printf("+=============================================+\n");
    printf("|     [Direttore] STATISTICHE GIORNALIERE     |\n");
    printf("+=============================================+\n");

    printf(" Giorno simulazione: %d\n", stats->durata_simulazione);
    printf("-----------------------------------------------\n");
    printf(" • Utenti serviti:              %d\n", stats->utenti_serviti_tot_day);
    printf(" • Utenti NON serviti:          %d\n", stats->utenti_non_serviti_tot_day);
    printf(" • Servizi erogati:             %d\n", stats->servizi_erogati_tot_day);
    printf(" • Servizi non erogati:         %d\n", stats->servizi_non_erogati_tot_day);
    printf(" • Operatori attivi:            %d\n", stats->operatori_attivi_day);
    printf(" • Pause effettuate:            %d\n", stats->pause_effettuate_tot_day);
    printf(" • Tempo medio attesa utenti:   %.0f ns\n", stats->tempo_attesa_utenti_day);
    printf(" • Tempo medio erog. servizi:   %.0f ns\n", stats->tempo_erogazione_servizi_day);

    printf("\n Tabella per servizio:\n");
    printf("+----+--------------------+----------------------+-----------------------------+\n");
    printf("| ID | Tempo attesa (ns)  | Tempo erog. (ns)     | Rapporto op./sportelli      |\n");
    printf("+----+--------------------+----------------------+-----------------------------+\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        printf("| %2d | %18.0f | %20.0f | %27.4f |\n", i, stats->tempo_attesa_utenti_day_services[i], stats->tempo_erogazione_servizi_day_services[i], stats->rapporto_operatori_sportelli_services[i]);
    }
    printf("+----+--------------------+----------------------+-----------------------------+\n");

    // Reset
    for (int i = 0; i < NUM_SERVIZI; i++) {
        stats->operatori_disponibili_services[i] = 0;
        stats->sportelli_esistenti_services[i] = 0;
        stats->rapporto_operatori_sportelli_services[i] = 0.0;
    }
}

void PrintFinalStats(Stats* stats) {
    printf("\n");
    printf("+=============================================+\n");
    printf("|        [Direttore] STATISTICHE FINALI       |\n");
    printf("+=============================================+\n");

    printf(" Motivo termine simulazione: %s\n", stats->termine_simulazione);
    printf("-----------------------------------------------\n");
    printf(" • Utenti serviti totali:           %d\n", stats->utenti_serviti_tot_sim);
    printf(" • Utenti NON serviti totali:       %d\n", stats->utenti_non_serviti_tot_sim);
    printf(" • Servizi erogati totali:          %d\n", stats->servizi_erogati_tot_sim);
    printf(" • Servizi non erogati totali:      %d\n", stats->servizi_non_erogati_tot_sim);
    printf(" • Operatori attivi totali:         %d\n", stats->operatori_attivi_sim);
    printf(" • Pause effettuate totali:         %d\n", stats->pause_effettuate_tot_sim);
    printf(" • Durata simulazione:              %d giorni\n", stats->durata_simulazione);
    printf(" • Utenti serviti medi/giorno:      %.4lf\n", stats->utenti_serviti_avg);
    printf(" • Servizi erogati medi/giorno:     %.4lf\n", stats->servizi_erogati_avg);
    printf(" • Servizi NON erogati medi/giorno: %.4lf\n", stats->servizi_non_erogati_avg);
    printf(" • Pause medie/giorno:              %.4lf\n", stats->pause_effettuate_avg);
    printf(" • Tempo medio attesa utenti:       %.0f ns\n", stats->tempo_attesa_utenti_sim);
    printf(" • Tempo medio erog. servizi:       %.0f ns\n", stats->tempo_erogazione_servizi_sim);

    printf("\n Tabella totali per servizio:\n");
    printf("+----+---------------+----------------+--------------------+\n");
    printf("| ID | Utenti serv.  | Servizi erog.  | Servizi non erog.  |\n");
    printf("+----+---------------+----------------+--------------------+\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        printf("| %2d | %13d | %14d | %18d |\n", i, stats->utenti_serviti_tot_sim_services[i], stats->servizi_erogati_tot_sim_services[i], stats->servizi_non_erogati_tot_sim_services[i]);
    }
    printf("+----+---------------+----------------+--------------------+\n");

    printf("\n Tabella medie per servizio:\n");
    printf("+----+--------------------+----------------------+-----------------------------+\n");
    printf("| ID | Utenti serv. avg   | Servizi erog. avg    | Servizi non erog. avg       |\n");
    printf("+----+--------------------+----------------------+-----------------------------+\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        printf("| %2d | %18.4f | %20.4f | %27.4f |\n", i, stats->utenti_serviti_avg_services[i], stats->servizi_erogati_avg_services[i], stats->servizi_non_erogati_avg_services[i]);
    }
    printf("+----+--------------------+----------------------+-----------------------------+\n");

    printf("\n Tempi per servizio:\n");
    printf("+----+----------------+----------------+\n");
    printf("| ID | Tempo utenti   | Tempo servizi  |\n");
    printf("+----+----------------+----------------+\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        printf("| %2d | %14.0f | %14.0f |\n", i, stats->tempo_attesa_utenti_sim_services[i], stats->tempo_erogazione_servizi_sim_services[i]);
    }
    printf("+----+----------------+----------------+\n");
}
/*
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
*/
/*
void WriteDailyStatsCSV(const char *filename, Stats* stats) {
    FILE *f = fopen(filename, "a");
    if (!f) {
        perror("fopen");
        return;
    }

    // Dati aggregati del giorno
    fprintf(f, "tipo;giorno;chiave;valore\n");
    fprintf(f, "daily;%d;utenti_serviti_tot_day;%d\n", stats->durata_simulazione, stats->utenti_serviti_tot_day);
    fprintf(f, "daily;%d;utenti_non_serviti_tot_day;%d\n", stats->durata_simulazione, stats->utenti_non_serviti_tot_day);
    fprintf(f, "daily;%d;servizi_erogati_tot_day;%d\n", stats->durata_simulazione, stats->servizi_erogati_tot_day);
    fprintf(f, "daily;%d;servizi_non_erogati_tot_day;%d\n", stats->durata_simulazione, stats->servizi_non_erogati_tot_day);
    fprintf(f, "daily;%d;operatori_attivi_day;%d\n", stats->durata_simulazione, stats->operatori_attivi_day);
    fprintf(f, "daily;%d;pause_effettuate_tot_day;%d\n", stats->durata_simulazione, stats->pause_effettuate_tot_day);
    fprintf(f, "daily;%d;tempo_attesa_utenti_day_ns;%.0f\n", stats->durata_simulazione, stats->tempo_attesa_utenti_day);
    fprintf(f, "daily;%d;tempo_erogazione_servizi_day_ns;%.0f\n", stats->durata_simulazione, stats->tempo_erogazione_servizi_day);

    // Tabelle per servizi
    fprintf(f, "\ntipo;giorno;service_index;tempo_attesa_utenti_day_ns;tempo_erogazione_servizi_day_ns;rapporto_operatori_sportelli\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "daily;%d;%d;%.0f;%.0f;%.4f\n", stats->durata_simulazione, i, stats->tempo_attesa_utenti_day_services[i], stats->tempo_erogazione_servizi_day_services[i], stats->rapporto_operatori_sportelli_services[i]);
    }

    fprintf(f, "\n");
    fclose(f);
}

void WriteFinalStatsCSV(const char *filename, Stats* stats) {
    FILE *f = fopen(filename, "a");
    if (!f) {
        perror("fopen");
        return;
    }

    // Dati aggregati finali
    fprintf(f, "tipo;giorno;chiave;valore\n");
    fprintf(f, "final;-;motivo_termine_simulazione;%s\n", stats->termine_simulazione);
    fprintf(f, "final;-;utenti_serviti_tot_sim;%d\n", stats->utenti_serviti_tot_sim);
    fprintf(f, "final;-;utenti_non_serviti_tot_sim;%d\n", stats->utenti_non_serviti_tot_sim);
    fprintf(f, "final;-;servizi_erogati_tot_sim;%d\n", stats->servizi_erogati_tot_sim);
    fprintf(f, "final;-;servizi_non_erogati_tot_sim;%d\n", stats->servizi_non_erogati_tot_sim);
    fprintf(f, "final;-;operatori_attivi_sim;%d\n", stats->operatori_attivi_sim);
    fprintf(f, "final;-;pause_effettuate_tot_sim;%d\n", stats->pause_effettuate_tot_sim);
    fprintf(f, "final;-;durata_simulazione;%d\n", stats->durata_simulazione);
    fprintf(f, "final;-;utenti_serviti_avg;%.4f\n", stats->utenti_serviti_avg);
    fprintf(f, "final;-;servizi_erogati_avg;%.4f\n", stats->servizi_erogati_avg);
    fprintf(f, "final;-;servizi_non_erogati_avg;%.4f\n", stats->servizi_non_erogati_avg);
    fprintf(f, "final;-;pause_effettuate_avg;%.4f\n", stats->pause_effettuate_avg);
    fprintf(f, "final;-;tempo_attesa_utenti_sim_ns;%.0f\n", stats->tempo_attesa_utenti_sim);
    fprintf(f, "final;-;tempo_erogazione_servizi_sim_ns;%.0f\n", stats->tempo_erogazione_servizi_sim);

    // Tabelle per servizi
    fprintf(f, "\ntipo;giorno;service_index;utenti_serviti_tot;servizi_erogati_tot;servizi_non_erogati_tot\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "final;-;%d;%d;%d;%d\n", i, stats->utenti_serviti_tot_sim_services[i], stats->servizi_erogati_tot_sim_services[i], stats->servizi_non_erogati_tot_sim_services[i]);
    }

    fprintf(f, "\ntipo;giorno;service_index;utenti_serviti_avg;servizi_erogati_avg;servizi_non_erogati_avg\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "final;-;%d;%.4f;%.4f;%.4f\n", i, stats->utenti_serviti_avg_services[i], stats->servizi_erogati_avg_services[i], stats->servizi_non_erogati_avg_services[i]);
    }

    fprintf(f, "\ntipo;giorno;service_index;tempo_attesa_utenti_sim_ns;tempo_erogazione_servizi_sim_ns\n");
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "final;-;%d;%.0f;%.0f\n", i, stats->tempo_attesa_utenti_sim_services[i], stats->tempo_erogazione_servizi_sim_services[i]);
    }

    fprintf(f, "\n");
    fclose(f);
}
*/

void WriteDailyStatsCSV(const char *filename, Stats* stats) {
    FILE *f = fopen(filename, "a+");
    if (!f) {
        perror("fopen");
        return;
    }

    // Scrivo header solo se file vuoto
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) {
        fprintf(f, "tipo;giorno;chiave;valore;service_index;utenti_serviti;utenti_non_serviti;servizi_erogati;servizi_non_erogati;operatori_attivi;pause_effettuate;tempo_attesa;tempo_erog;rapporto\n");
    }

    int day = stats->durata_simulazione;

    // Dati aggregati giornalieri
    fprintf(f, "daily;%d;utenti_serviti_tot_day;%d;;;;;;;;;;;\n", day, stats->utenti_serviti_tot_day);
    fprintf(f, "daily;%d;utenti_non_serviti_tot_day;%d;;;;;;;;;;;\n", day, stats->utenti_non_serviti_tot_day);
    fprintf(f, "daily;%d;servizi_erogati_tot_day;%d;;;;;;;;;;;\n", day, stats->servizi_erogati_tot_day);
    fprintf(f, "daily;%d;servizi_non_erogati_tot_day;%d;;;;;;;;;;;\n", day, stats->servizi_non_erogati_tot_day);
    fprintf(f, "daily;%d;operatori_attivi_day;%d;;;;;;;;;;;\n", day, stats->operatori_attivi_day);
    fprintf(f, "daily;%d;pause_effettuate_tot_day;%d;;;;;;;;;;;\n", day, stats->pause_effettuate_tot_day);
    fprintf(f, "daily;%d;tempo_attesa_utenti_day_ns;%.0f;;;;;;;;;;;\n", day, stats->tempo_attesa_utenti_day);
    fprintf(f, "daily;%d;tempo_erogazione_servizi_day_ns;%.0f;;;;;;;;;;;\n", day, stats->tempo_erogazione_servizi_day);

    // Dati per servizi (giornalieri)
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "daily;%d;;;%d;;;;;;;;;%.0f;%.0f;%.4f\n", day, i, stats->tempo_attesa_utenti_day_services[i], stats->tempo_erogazione_servizi_day_services[i], stats->rapporto_operatori_sportelli_services[i]);
    }

    fclose(f);
}

void WriteFinalStatsCSV(const char *filename, Stats* stats) {
    FILE *f = fopen(filename, "a+");
    if (!f) {
        perror("fopen");
        return;
    }

    // Scrivo header solo se file vuoto
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) {
        fprintf(f, "tipo;giorno;chiave;valore;service_index;utenti_serviti;utenti_non_serviti;servizi_erogati;servizi_non_erogati;operatori_attivi;pause_effettuate;tempo_attesa;tempo_erog;rapporto\n");
    }

    // Dati aggregati finali
    fprintf(f, "final;-;motivo_termine_simulazione;%s;;;;;;;;;;;\n", stats->termine_simulazione);
    fprintf(f, "final;-;utenti_serviti_tot_sim;%d;;;;;;;;;;;\n", stats->utenti_serviti_tot_sim);
    fprintf(f, "final;-;utenti_non_serviti_tot_sim;%d;;;;;;;;;;;\n", stats->utenti_non_serviti_tot_sim);
    fprintf(f, "final;-;servizi_erogati_tot_sim;%d;;;;;;;;;;;\n", stats->servizi_erogati_tot_sim);
    fprintf(f, "final;-;servizi_non_erogati_tot_sim;%d;;;;;;;;;;;\n", stats->servizi_non_erogati_tot_sim);
    fprintf(f, "final;-;operatori_attivi_sim;%d;;;;;;;;;;;\n", stats->operatori_attivi_sim);
    fprintf(f, "final;-;pause_effettuate_tot_sim;%d;;;;;;;;;;;\n", stats->pause_effettuate_tot_sim);
    fprintf(f, "final;-;durata_simulazione;%d;;;;;;;;;;;\n", stats->durata_simulazione);
    fprintf(f, "final;-;utenti_serviti_avg;%.4f;;;;;;;;;;;\n", stats->utenti_serviti_avg);
    fprintf(f, "final;-;servizi_erogati_avg;%.4f;;;;;;;;;;;\n", stats->servizi_erogati_avg);
    fprintf(f, "final;-;servizi_non_erogati_avg;%.4f;;;;;;;;;;;\n", stats->servizi_non_erogati_avg);
    fprintf(f, "final;-;pause_effettuate_avg;%.4f;;;;;;;;;;;\n", stats->pause_effettuate_avg);
    fprintf(f, "final;-;tempo_attesa_utenti_sim_ns;%.0f;;;;;;;;;;;\n", stats->tempo_attesa_utenti_sim);
    fprintf(f, "final;-;tempo_erogazione_servizi_sim_ns;%.0f;;;;;;;;;;;\n", stats->tempo_erogazione_servizi_sim);

    // Dati per servizi (finali)
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "final;-;;;%d;%d;;;%d;%d;;%.0f;%.0f;\n", i, stats->utenti_serviti_tot_sim_services[i], stats->servizi_erogati_tot_sim_services[i], stats->servizi_non_erogati_tot_sim_services[i], stats->tempo_attesa_utenti_sim_services[i], stats->tempo_erogazione_servizi_sim_services[i]);
    }

    // Dati medi per servizi (finali)
    for (int i = 0; i < NUM_SERVIZI; i++) {
        fprintf(f, "final;-;;;%d;%.4f;;%.4f;%.4f;;;\n", i, stats->utenti_serviti_avg_services[i], stats->servizi_erogati_avg_services[i], stats->servizi_non_erogati_avg_services[i]);
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
