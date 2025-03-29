#define NUM_SERVIZI 6

typedef struct {
    char nome[50];
    int durata; // in minuti
} Servizio;

// Elenco dei servizi disponibili
Servizio servizi[NUM_SERVIZI] = {
    {"Invio e ritiro pacchi", 10},
    {"Invio e ritiro lettere e raccomandate", 8},
    {"Prelievi e versamenti Bancoposta", 6},
    {"Pagamento bollettini postali", 8},
    {"Acquisto prodotti finanziari", 20},
    {"Acquisto orologi e braccialetti", 20}
};