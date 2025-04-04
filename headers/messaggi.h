#ifndef MESSAGGI_H
#define MESSAGGI_H

#define MAX_TEXT 100

typedef struct {
    long mtype; // Corrisponde all'indice del servizio richiesto
    int ticket_id;
    //char* user_id; 
    int user_id; // PID dell'utente che ha richiesto il messaggio
    char text[MAX_TEXT];
} Messaggio;

#endif // MESSAGGI_H