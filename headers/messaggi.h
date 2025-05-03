#ifndef MESSAGGI_H
#define MESSAGGI_H

#define MAX_TEXT 100

typedef struct {
    long mtype; // Corrisponde all'indice del servizio richiesto
    int ticket_id;
    int user_id; // PID dell'utente che ha richiesto il messaggio
    char text[MAX_TEXT];
    long time_for_execution; // Tempo che l'operatore impiega per eseguire il servizio richiesto dall'utente
} Messaggio;

#endif // MESSAGGI_H