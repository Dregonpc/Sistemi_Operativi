#ifndef MESSAGGI_H
#define MESSAGGI_H

#define PUBLIC_KEY 12345
#define MAX_TEXT 100
#define EOD_mtype 30

typedef struct {
    long mtype; // Corrisponde all'indice del servizio richiesto
    int ticket_id;
    int user_id; // PID dell'utente che ha richiesto il messaggio
    int user_index; // Indice del semaforo per ogni utente
    char text[MAX_TEXT];
    long time_for_execution; // Tempo che l'operatore impiega per eseguire il servizio richiesto dall'utente
} Messaggio;

typedef struct {
    long mtype; // deve essere > 0
    int new_users;
} NewUserMsg;

#endif // MESSAGGI_H