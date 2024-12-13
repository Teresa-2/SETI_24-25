
/* 
 * incApache_threads.c: implementazione dei thread per il web server del corso di SET
 *
 * Programma sviluppato a supporto del laboratorio di
 * Sistemi di Elaborazione e Trasmissione del corso di laurea
 * in Informatica classe L-31 presso l'Universita` degli Studi di
 * Genova, per l'anno accademico 2024/2025.
 *
 * Copyright (C) 2012-2014 by Giovanni Chiola <chiolag@acm.org>
 * Copyright (C) 2015-2016 by Giovanni Lagorio <giovanni.lagorio@unige.it>
 * Copyright (C) 2016-2024 by Giovanni Chiola <chiolag@acm.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "incApache.h"
pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER; //DA RIVEDERE
pthread_mutex_t mime_mutex = PTHREAD_MUTEX_INITIALIZER; //DA RIVEDERE
#ifdef INCaPACHE_8_1
    int client_sockets[MAX_CONNECTIONS]; /* for each connection, its socket FD */
    //NOTA: array che associa ad ogni connessione il relativo file descriptor
    int no_response_threads[MAX_CONNECTIONS]; /* for each connection, how many response threads */
    //NOTA: array che associa ad ogni connessione il numero di thread di risposta ad essa associati
    pthread_t thread_ids[MAX_THREADS]; //NOTA: array di thread di tipo PTHREAD cioè array che contiene tutti gli id dei 16 thread. Ogni indice indica un thread e il valore contenuto in quell'indice è l'id del thread. Esempio: thread_ids[0] = 99, indica che il thread 0 ha valore "99" come id
    int connection_no[MAX_THREADS]; /* connection_no[i] >= 0 means that i-th thread belongs to connection connection_no[i] */
    //NOTA: array che contiene per ogni thread l'indice della connessione a cui esso è associato. I primi 4 thread sono thread di connessione riservati ai client (in particolare, thread 0 per client 0, thread 1 per client 1, e così via). Gli altri thread sono thread di risposta ai client. Inizialmente tutti i thread di risposta saranno liberi (FREE_SLOT) perché nessuno è stato ancora associato ad un client. 
    pthread_t *to_join[MAX_THREADS]; /* for each thread, the pointer to the previous (response) thread, if any */
    /*NOTA: array in cui l'indice indica il numero di thread considerato. La struttura è complessa e il contenuto delle celle varia a seconda del tipo di thread considerato:
    celle 0-3 (che corrispondono ai thread di connessione): nella relativa cella è contenuto l'indirizzo dell'ultimo thread associato alla connessione a cui fanno riferimento. esempio: se il client 1 ha associata una coda di thread e l'ultimo è il thread 7, allora to_join[1] = indirizzo dell'identificativo del thread n. 7
    celle 4-15 (che corrispondono ai thread di risposta): nella relativa cella è contenuto l'indirizzo del thread che viene subito prima di lui nella coda (e che di conseguenza deve attendere prima di essere elaborato). esempio: se to_join[10]=&thread_id[5] significa che il thread 10 ha davanti a sè in coda il thread 5.   
    */
    int no_free_threads = MAX_THREADS - 2 * MAX_CONNECTIONS; /* each connection has one thread listening and one reserved for replies */
    struct response_params thread_params[MAX_THREADS - MAX_CONNECTIONS]; /* params for the response threads (the first MAX_CONNECTIONS threads are waiting/parsing requests) */
    //NOTA: array che contiene per ciascun thread i metadati ad esso associati (esempio: thread_params[9]=metadati associati al thread 9)
    pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER; /* protects the access to thread-related data structures */
    static int reserve_unused_thread() { //NOTA: metodo che trova il primo thread di risposta non utilizzato e lo riserva (evitando che venga utilizzato da altri)
    int idx;
    for (idx = MAX_CONNECTIONS; idx < MAX_THREADS; ++idx)
        if (connection_no[idx] == FREE_SLOT) {
            connection_no[idx] = RESERVED_SLOT;
            return idx;
        }
    assert(0);
    return -1;
    }
    

int find_unused_thread_idx(int conn_no)
    {
    int idx = -1;
    pthread_mutex_lock(&threads_mutex); //NOTA: per evitare race conditions
    /* reserved thread already used, try to find another (unused) one */
    if (no_response_threads[conn_no] > 0) { 
        if (no_free_threads > 0) { //NOTA: se ci sono thread di risposta ancora liberi
            --no_free_threads; //NOTA: toglie un thread di risposta da quelli disponibili, per riservarlo
            ++no_response_threads[conn_no]; //NOTA: indica che è stato riservato un nuovo thread alla connessione passata come argomento (conn_no) incrementando il numero di thread di risposta associati alla connessione conn_no
            idx = reserve_unused_thread(); //NOTA: salva l'indice del thread che è stato riservato
            pthread_mutex_unlock(&threads_mutex); //NOTA: per evitare race conditions
            return idx; //NOTA: ritorna l'indice del thread che è stato riservato
        }
        pthread_mutex_unlock(&threads_mutex); //NOTA: se non ci sono thread di risposta liberi:
        join_all_threads(conn_no); //NOTA: manda in esecuzione tutti i thread relativi alla connessione passata come argomento (conn_no) fino a che non li ha esauriti tutti
        pthread_mutex_lock(&threads_mutex); //NOTA: per evitare race conditions
    }
    assert(no_response_threads[conn_no] == 0); //NOTA: quando ha finito di eseguire la join_all_threads (cioè quando non ci sono più thread di risposta associati al client conn_no)
    no_response_threads[conn_no] = 1; //NOTA: indica che è stato riservato un nuovo thread alla connessione passata come argomento (conn_no) incrementando il numero di thread di risposta associati alla connessione conn_no
    idx = reserve_unused_thread(); //NOTA: salva l'indice del thread che vuole riservare pe ril client conn_no
    pthread_mutex_unlock(&threads_mutex);
    return idx; //NOTA: ritorna l'indice del thread che è stato riservato
    }


void join_all_threads(int conn_no) //NOTA: manda in esecuzione tutti i thread relativi alla connessione passata come argomento (conn_no) fino a che non li ha esauriti tutti
    {
    size_t i;
    /*** compute the index i of the thread to join,
     *** call pthread_join() on thread_ids[i], and update shared variables
     *** no_free_threads, no_response_threads[conn_no], and
     *** connection_no[i] ***/
/*** TO BE DONE 8.1 START ***/
    pthread_mutex_lock(&threads_mutex);
    for (i = MAX_CONNECTIONS; i < MAX_THREADS; ++i) { //NOTA: per ogni thread di risposta...
        if (connection_no[i] == conn_no) { //NOTA: ...associato al client conn_no (passato per argomento)
            if (pthread_join(thread_ids[i], NULL)) //NOTA: aspetta la terminazione dell'eventuale thread che lo precede nella coda
                fail("error in join_all_thread"); //aggiunta pazza
            connection_no[i] = FREE_SLOT;
            --no_response_threads[conn_no];
            ++no_free_threads;
        }
    }
    
    pthread_mutex_unlock(&threads_mutex);
/*** TO BE DONE 8.1 END ***/
    }
    void join_prev_thread(int thrd_no)
    {
    size_t i;
    int conn_no;
    
    //debug("start of join_prev_thread(%d)\n", thrd_no);
    
    /*** check whether there is a previous thread to join before
     *** proceeding with the current thread.
     *** In that case compute the index i of the thread to join,
     *** wait for its termination, and update the shared variables
     *** no_free_threads, no_response_threads[conn_no], and connection_no[i],
     *** avoiding race conditions ***/
/*** TO BE DONE 8.1 START ***/
    
    if (to_join[thrd_no] != NULL) { //NOTA: se il thread ha un thread precedente da attendere, allora procedo con la terminazione del thread che lo anticipa nella coda
        pthread_mutex_lock(&threads_mutex); //NOTA: per evitare race condition (1 di 2)
        i = to_join[thrd_no] - thread_ids; //NOTA: determinazione dell'indice del thread precedente a cui accodarsi, che è presente all'interno dell'array thread_ids
        conn_no = connection_no[thrd_no]; //NOTA: salvo il numero di connessione del thread che vo
        pthread_mutex_unlock(&threads_mutex);
        
        if (pthread_join(thread_ids[i], NULL) != 0) {
            
            //con test threads.sh entra qui dentro dopo un po' di chiamate

            //NOTA: verifico se il join sta provando a joinare se stesso
            if (pthread_self() == thread_ids[i]) {
                //fail("Thread is trying to join itself\n");
                pthread_mutex_unlock(&threads_mutex);
                return;
            }

            fail("error in phtread join");//NOTA: attendo la terminazione del thread antecedente a quello passato come parametro, che è in posizione i (come calcolato nel for sovrastante) }
        }
        pthread_mutex_lock(&threads_mutex);
        ++no_free_threads; //NOTA: incremento il numero di thread liberi, perché un thread (di risposta) è stato risolto
        --no_response_threads[conn_no]; //NOTA: decremento il numero di thread di risposta per la connessione conn_no, perché un thread è stato risolto
        connection_no[i] = FREE_SLOT; //NOTA: libero la posizione i nella coda dei thread
        pthread_mutex_unlock(&threads_mutex);
    }
    
    //pthread_mutex_unlock(&threads_mutex); //NOTA: per evitare race condition (2 di 2)
	
/*** TO BE DONE 8.1 END ***/
    }
    void *response_thread(void *vp) //NOTA: viene invocato nella creazione di un thread e fa iniziare l'esecuzione del thread di risposta
    {
    size_t thread_no = ((int *) vp) - connection_no;
    int connection_idx = *((int *) vp);
    debug(" ... response_thread() thread_no=%lu, conn_no=%d\n", (unsigned long) thread_no, connection_idx);
    const size_t i = thread_no - MAX_CONNECTIONS;
    send_response(client_sockets[connection_idx],
              thread_params[i].code,
              thread_params[i].cookie,
              thread_params[i].is_http1_0,
              (int)thread_no,
              thread_params[i].filename,
              thread_params[i].p_stat); //NOTA: invoca il metodo send_response() passando i parametri necessari per la risposta presenti nel thread di risposta e inviando la risposta al client
    debug(" ... response_thread() freeing filename and stat\n");
    free(thread_params[i].filename);
    debug(" ... filename free\n"); //aggiunto
    free(thread_params[i].p_stat);
    debug(" ... p_stat free\n"); //aggiunto
    return NULL;
    }
#else /* #ifndef INCaPACHE_8_1 */
    pthread_t thread_ids[MAX_CONNECTIONS];
    int connection_no[MAX_CONNECTIONS];
#endif /* ifdef INCaPACHE_8_1 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void *client_connection_thread(void *vp) //NOTA: crea pthread (del connection_no) il cui indice è specificato nell'argomento del metodo
{
    int client_fd;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;
#ifdef INCaPACHE_8_1
    pthread_mutex_lock(&threads_mutex);
    int connection_no = *((int *) vp);
    /*** properly initialize the thread queue to_join ***/
/*** TO BE DONE 8.1 START ***/
    for (int i=0; i<MAX_THREADS; i++) 
    {
        to_join[i]=NULL; //NOTA: forse i=MAX_CONNECTIONS
    }
/*** TO BE DONE 8.1 END ***/
    pthread_mutex_unlock(&threads_mutex);
#endif
    for (; ;) {
        addr_size = sizeof(client_addr);
        pthread_mutex_lock(&accept_mutex);
        if ((client_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &addr_size)) == -1)
            fail_errno("Cannot accept client connection");
        pthread_mutex_unlock(&accept_mutex);
#ifdef INCaPACHE_8_1
        client_sockets[connection_no] = client_fd;
#endif
        char str[INET_ADDRSTRLEN];
        struct sockaddr_in *ipv4 = (struct sockaddr_in *) &client_addr;
        printf("Accepted connection from %s\n", inet_ntop(AF_INET, &(ipv4->sin_addr), str, INET_ADDRSTRLEN));
        manage_http_requests(client_fd
#ifdef INCaPACHE_8_1
                , connection_no
#endif
        );
    }
}
#pragma clang diagnostic pop

char *get_mime_type(char *filename)
{
    char *mime_t = NULL;
    const size_t len_filename = strlen(filename);
    ssize_t nchars_read = 0;
    size_t sz = 0;
    if (len_filename > 4 && strcmp(filename + len_filename - 4, ".css") == 0)
        return my_strdup("text/css;");
    debug("      ... get_mime_type(%s): was not .css\n", filename);
    /*** What is missing here to avoid race conditions ? ***/
/*** TO BE DONE 8.0 START ***/
pthread_mutex_lock(&mime_mutex);
/*** TO BE DONE 8.0 END ***/
    fprintf(mime_request_stream, "%s\n", filename);
    fflush(mime_request_stream);
    debug("      ... get_mime_type(%s): printed filename on mime_request_stream\n", filename);
    if ((nchars_read = getline(&mime_t, &sz, mime_reply_stream)) < 1)
        fail("Could not get answer from file");
    /*** What is missing here to avoid race conditions ? ***/
/*** TO BE DONE 8.0 START ***/
pthread_mutex_unlock(&mime_mutex);
/*** TO BE DONE 8.0 END ***/
    if (mime_t[--nchars_read] == '\n')
        mime_t[nchars_read] = '\0';
    debug("      ... get_mime_type(%s): got answer %s\n", filename, mime_t);
    return mime_t;
}

#ifdef INCaPACHE_8_1
void send_resp_thread(int out_socket, int response_code, int cookie,
              int is_http1_0, int connection_idx, int new_thread_idx,
              char *filename, struct stat *stat_p)
{
    struct response_params *params =  thread_params + (new_thread_idx - MAX_CONNECTIONS);
    debug(" ... send_resp_thread(): idx=%lu\n", (unsigned long)(params - thread_params));
    params->code = response_code;
    params->cookie = cookie;
    params->is_http1_0 = is_http1_0;
    params->filename = filename ? my_strdup(filename) : NULL;
    params->p_stat = stat_p;
    pthread_mutex_lock(&threads_mutex);
    connection_no[new_thread_idx] = connection_idx;
    debug(" ... send_resp_thread(): parameters set, conn_no=%d\n", connection_idx);
    /*** enqueue the current thread in the "to_join" data structure ***/
/*** TO BE DONE 8.1 START ***/
//RIGUARDA ARRAY
    to_join[new_thread_idx]=to_join[connection_idx]; 
    to_join[connection_idx]=&thread_ids[new_thread_idx];
/*** TO BE DONE 8.1 END ***/
    if (pthread_create(thread_ids + new_thread_idx, NULL, response_thread, connection_no + new_thread_idx)) //NOTA: crea un nuovo thread (di risposta) nel processo chiamante; response_thread è la chiamata al metodo response_thread() sovrastante che si occupa di iniziare l'esecuzione del thread
        fail_errno("Could not create response thread");
    pthread_mutex_unlock(&threads_mutex);
    debug(" ... send_resp_thread(): new thread created\n");
}
#endif
