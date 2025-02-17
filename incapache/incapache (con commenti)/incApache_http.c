/*
 * incApache_http.c: implementazione del protocollo HTTP per il web server
 *                   del corso di SETI
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

/***
#define DEBUG
***/

#define COOKIE_EXPIRE "; Expires=Wed, 31 Dec 2025 23:59:59 GMT"

#include "incApache.h"

#define OptimizeTCP

#define RESPONSE_CODE_OK		200
#define RESPONSE_CODE_MOVED_PERMANENTLY	301
#define RESPONSE_CODE_NOT_MODIFIED	304
#define RESPONSE_CODE_BAD_REQUEST	400
#define RESPONSE_CODE_NOT_FOUND		404
#define RESPONSE_CODE_INTERNAL_ERROR	500
#define RESPONSE_CODE_NOT_IMPLEMENTED	501

static int CurUID = 0;
static int UserTracker[MAX_COOKIES];
pthread_mutex_t cookie_mutex = PTHREAD_MUTEX_INITIALIZER;

int get_new_UID(void)
{
    int retval;

    /*** Compute retval by incrementing CurUID mod MAX_COOKIES
     *** and reset UserTracker[retval] to 0.
     *** Be careful in order to avoid race conditions ***/
/*** TO BE DONE 8.0 START ***/

	pthread_mutex_lock(&cookie_mutex); //NOTA: per evitare race condition
	retval= (CurUID++)%MAX_COOKIES; //NOTA: calcolo il nuovo UID incrementando CurUID e facendo il modulo con MAX_COOKIES, in modo da ottenere un valore compreso tra 1 e MAX_COOKIES-1. Non può essere 0 perchè 0 = root
	CurUID=retval; //NOTA: aggiorno il valore di CurUID con l'ultimo UID calcolato
	UserTracker[retval] = 0; //NOTA: imposto il valore di UserTracker[retval] a 0, cioè assegno all'utente con UID=retval (cioè quello che si è appena presentato al server) un # accessi = 0
	pthread_mutex_unlock(&cookie_mutex); //NOTA: per evitare race condition
	debug("...	Computing retval and resetting UserTracker[retval] to 0 \n");

/*** TO BE DONE 8.0 END ***/

    return retval;
}


int keep_track_of_UID(int myUID)
{
    int newcount;
    if ( (myUID < 0) || (myUID >= MAX_COOKIES) ) //NOTA: se l'UID passato come argomento al metodo è out of range restituisco -1
        return -1;

    /*** Increment UserTracker[myUID] and return the computed value.
     *** Be careful in order to avoid race conditions ***/
/*** TO BE DONE 8.0 START ***/

	pthread_mutex_lock(&cookie_mutex); //NOTA: per evitare race condition
	newcount = ++UserTracker[myUID]; //NOTA: incremento il contatore del numero di accessi dell'utente con UID=myUID e salvo il nuovo valore in newcount
	pthread_mutex_unlock(&cookie_mutex); //NOTA: per evitare race condition
	debug("...	Incrementing UserTracker[myUID] and returning the computed value \n");

/*** TO BE DONE 8.0 END ***/

    return newcount;
}


void send_response(int client_fd, int response_code, int cookie,
#ifdef INCaPACHE_8_1
		   int is_http1_0, int thread_no,
#endif
		   char *filename, struct stat *stat_p)
{
	time_t now_t = time(NULL);
	struct tm now_tm;
	time_t file_modification_time;
	struct tm file_modification_tm;
	char time_as_string[MAX_TIME_STR];
	char http_header[MAX_HEADER_SIZE] = "HTTP/1.";
	size_t header_size;
	int fd = -1;
	off_t file_size = 0;
	char *mime_type = NULL;
	const char * const HTML_mime = "text/html";
	struct stat stat_buffer;
	size_t header_sent;
	debug("  ... start send_response(response_code=%d, filename=%s)\n", response_code, filename);

	/*** Compute date of servicing current HTTP Request using a variant of gmtime() ***/
/*** TO BE DONE 8.0 START ***/

	if(!gmtime_r (&now_t, &now_tm)) fail_errno("Error in computing date of servicing current HTTP Request"); //NOTA: gmtime_r converte una data fornita in forma di calendario in formato UTC e la salva in una struct. A differenza della funzione gmtime che restituisce un puntatore a una struct statica (che può essere sovrascritta da altre chiamate a funzioni di tempo, per esempio), gmtime_r restituisce un puntatore a una struct passata come argomento dall'utente garantendo che tale valore non venga sovrascritto da altre chiamate
	//NOTA: il tempo salvato in now_t viene convertito in UTC e salvato nella struct now_tm
	debug("...	Computing date of servicing current HTTP Request using a variant of gmtime() \n");

/*** TO BE DONE 8.0 END ***/

	strftime(time_as_string, MAX_TIME_STR, "%a, %d %b %Y %T GMT", &now_tm); //NOTA: la funzione strftime converte il tempo salvato nella struct now_tm (che è in forma destrutturata e nel formato specificato dal terzo argomento del metodo, cioè l'UTC) e lo salva in time_as_string (che è un array di char)
	//NOTA: il formato %a, %d %b %Y %T GMT è il formato standard per la data in UTC: %a è il giorno della settimana abbreviato, %d è il giorno del mese, %b è il mese abbreviato, %Y è l'anno, %T è l'ora, i minuti e i secondi e GMT è il fuso orario. NON ESISTE un metodo più veloce per specificare UTC.
	 
#ifdef INCaPACHE_8_1
	strcat(http_header, is_http1_0 ? "0 " : "1 "); 	//NOTA: aggiungo alla stringa http_header il valore di is_http1_0 (che è un intero che viene passato come argomento alla send_response) convertito in stringa. Se is_http1_0 è 0, concateno "0" alla stringa http_header, altrimenti concateno "1". Questo valore indica se la richiesta è stata fatta con HTTP/1.0 o HTTP/1.1, quindi indica la versione del protocollo HTTP utilizzata.
#else /* #ifdef INCaPACHE_8_1 */
	strcat(http_header, "0 "); //NOTA: se non è definita la costante INCaPACHE_8_1, concateno alla stringa http_header il valore "0" perché siamo sicuramente in presenza di una richiesta HTTP/1.0
#endif /* #ifdef INCaPACHE_8_1 */
	switch (response_code) { //NOTA: in base al valore intero di response_code passato come argomento al metodo send_response, concateno alla stringa http_header il codice di risposta HTTP corrispondente. RESPONSE CODE possibili possono essere: 200, 404, 400, 501, ...
	case RESPONSE_CODE_OK: //NOTA: se il codice di risposta è 200, concateno alla stringa http_header il codice di risposta "200 OK"
		if (filename != NULL) 
		{ //NOTA: se il nome del file che il client chiede di visualizzare dal server non è nullo, allora il client ha fatto una richiesta corretta di visualizzazione di un file e il server procede a rispondere con il file richiesto
			if (stat_p == NULL) { //NOTA: se la struct stat_p passata come argomento al metodo è nulla, allora il server non ha informazioni sul file richiesto e deve aprirlo per ottenere informazioni aggiuntive (cioè i metadati)
				stat_p = &stat_buffer; //NOTA: la struct stat_p viene inizializzata con la struct stat_buffer, che essendo stata creata e mai utilizzata nel metodo send_response contiene i dati nella forma di default. così facendo evito comportamenti imprevisti dati da possibili contenuti ambigui di stat_p
				if (stat(filename, stat_p)) { //NOTA: recupero tutti i metadati dal file "filename" e li salvo nella struct stat_p. Se la funzione stat ritorna un valore diverso da 0, allora c'è stato un errore nel recupero dei metadati del file e viene segnalato errore
				    debug("stat failed \n");
                                    response_code = RESPONSE_CODE_INTERNAL_ERROR; //NOTA: se la funzione stat fallisce, allora il codice di risposta HTTP è 500 (Internal Error)
                                    goto int_err; //NOTA: salto alla label int_err
				}
			} else { //NOTA: se la struct stat_p passata come argomento al metodo NON è nulla, allora il server ha già i metadati sul file richiesto
				fd = open(filename, O_RDONLY); //NOTA: apro il file richiesto e lo rendo disponibile successivamente in sola lettura, inoltre salvo il file descriptor in fd. il fd restituito dalla open sarà di default il valore intero più piccolo disponibile tra quelli liberi (per non sprecare memoria), sempre a partire da 2. Il fd è un valore intero che identifica il file aperto e ha valore maggiore di 2 (0 = stdin, 1 = stdout, 2 = stderr). Se la funzione open fallisce, restituisce un valore negativo (-1). O_RDONLY è una costante che indica che il file deve essere aperto in sola lettura (quindi solo read per le open)
				if (fd<0) { //NOTA: se la open ha fallito, fd = -1 quindi il file richiesto non è stato aperto correttamente
				    debug("send_response: cannot open file for reading (has the file vanished?) \n");
                                    response_code = RESPONSE_CODE_INTERNAL_ERROR; //NOTA: se la open fallisce, il codice di risposta HTTP è 500 (Internal Error)
                                    goto int_err; //NOTA: salto alla label int_err
                                  }
				debug("    ... send_response(%d, %s): opened file %d\n", response_code, filename, fd);
			}
			mime_type = get_mime_type(filename); //NOTA: recupero il mime type del file richiesto e lo salvo in mime_type. get_mime_type è un metodo che restituisce il mime type di un file in base all'estensione del file. Se il file non ha un'estensione riconosciuta, restituisce "text/plain" come mime type. 

			debug("    ... send_response(%d, %s): got mime type %s\n", response_code, filename, mime_type);

			/*** compute file_size and file_modification_time ***/
/*** TO BE DONE 8.0 START ***/
			file_size = stat_p->st_size; //NOTA: recupero la grandezza del file richiesto dai metadati (salvati in stat_p) e la salvo in file_size
			if (file_size <= 0 ) {
				debug("...	send_response: file size cannot be zero or negative\n");
				goto int_err;
			}
			file_modification_time = stat_p->st_mtime; //NOTA: recupero il tempo di ultima modifica del file richiesto dai metadati (salvati in stat_p) e lo salvo in file_modification_time
			// NOTA: non mettiamo il controllo sul tempo (che in particolare indica il numero di secondi trascorsi dall'epoch) perché può essere sia negativo che positivo, con variazioni che dipendono dal SO

			debug("...	Computing file_size and file_modification_time \n");

/*** TO BE DONE 8.0 END ***/

			debug("      ... send_response(%3d,%s) : file opened, size=%lu, mime=%s\n", response_code, filename, (unsigned long)file_size, mime_type);
		        strcat(http_header, "200 OK");
		} else //NOTA: se il nome del file richiesto era nullo (cioè filename == NULL) allora il client ha fatto una richiesta di un file vuoto e il server risponde con un codice di risposta 200 OK ma senza alcun file (in quanto vuoto)
		    strcat(http_header, "200 OK");
		break;
	case RESPONSE_CODE_MOVED_PERMANENTLY:
		strcat(http_header, "301 Moved Permanently");
		strcat(http_header, "Location: ");
		strcat(http_header, filename);
		break;
	case RESPONSE_CODE_NOT_MODIFIED:
		strcat(http_header, "304 Not Modified");
		break;
	case RESPONSE_CODE_BAD_REQUEST:
		strcat(http_header, "400 Bad Request");
		break;
	case RESPONSE_CODE_NOT_FOUND:
		strcat(http_header, "404 Not Found");
		if ((fd = open(HTML_404, O_RDONLY)) >= 0) { //NOTA: se il file HTML_404 esiste e può essere aperto in sola lettura, allora il server risponde con il file HTML_404

			/*** compute file_size, mime_type, and file_modification_time of HTML_404 ***/
/*** TO BE DONE 8.0 START ***/
			stat_p = &stat_buffer; //NOTA: la struct stat_p viene inizializzata con la struct stat_buffer, che essendo stata creata e mai utilizzata nel metodo send_response contiene i dati nella forma di default. così facendo evito comportamenti imprevisti dati da possibili contenuti ambigui di stat_p.
					if (stat(HTML_404, stat_p)) { //NOTA: recupero tutti i metadati dal file "HTML_404" e li salvo nella struct stat_p. Se la funzione stat ritorna un valore diverso da 0, allora c'è stato un errore nel recupero dei metadati del file e viene segnalato errore
						debug("stat failed (HTML_404) \n");
										response_code = RESPONSE_CODE_INTERNAL_ERROR; //NOTA: se la funzione stat fallisce, allora il codice di risposta HTTP è 500 (Internal Error)
										goto int_err; //NOTA: salto alla label int_err
					}
					mime_type = strdup(HTML_mime); //NOTA: il mime type del file HTML_404 è "text/html", una variabile definita nel metodo. Il metodo strdup duplica la stringa passata come argomento e restituisce un puntatore alla nuova stringa. In questo caso, restituisce un puntatore alla stringa "text/html" che viene salvato in mime_type (che è una variabile locale al metodo che punta ad una stringa allocata dinamicamente)
					debug("    ... send_response(%d, %s): got MIME type %s\n", response_code, HTML_404, mime_type);
					
					file_size = stat_p -> st_size; //NOTA: recupero la grandezza del file HTML_404 dai metadati (salvati in stat_p) e la salvo in file_size
					if (file_size <= 0 ) { //NOTA: se la grandezza del file è negativa, c'è stato un errore nella lettura dei metadati oppure i metadati sono corrotti
						debug("send_response: file size cannot be zero or negative\n");
						goto int_err;
					}
					file_modification_time = stat_p -> st_mtime; //NOTA: recupero il tempo di ultima modifica del file HTML_404 dai metadati (salvati in stat_p) e lo salvo in file_modification_time, come prima non posso fare controlli sui valori ottenuti
					debug("      ... send_response(%3d,%s) : file opened, size=%lu, MIME=%s\n", response_code, HTML_404, (unsigned long)file_size, mime_type);

/*** TO BE DONE 8.0 END ***/

		}
		break;
	case RESPONSE_CODE_INTERNAL_ERROR:
            int_err: //NOTA: label int_err a cui si salta in caso di errori interni
		strcat(http_header, "500 Internal Error");
		break;
	default:

/*** TO BE OPTIONALLY CHANGED START ***/

		strcat(http_header, "501 Method Not Implemented\r\nAllow: HEAD,GET");

/*** TO BE OPTIONALLY CHANGED END ***/

		if ((fd = open(HTML_501, O_RDONLY)) >= 0) {

			/*** compute file_size, mime_type, and file_modification_time of HTML_501 ***/
/*** TO BE DONE 8.0 START ***/

			stat_p = &stat_buffer; //NOTA: la struct stat_p viene inizializzata con la struct stat_buffer, che essendo stata creata e mai utilizzata nel metodo send_response contiene i dati nella forma di default. così facendo evito comportamenti imprevisti dati da possibili contenuti ambigui di stat_p.
			
			if (stat(HTML_501, stat_p)) { //NOTA: recupero tutti i metadati dal file "HTML_404" e li salvo nella struct stat_p. Se la funzione stat ritorna un valore diverso da 0, allora c'è stato un errore nel recupero dei metadati del file e viene segnalato errore
				debug("stat failed (HTML_501) \n");
				response_code = RESPONSE_CODE_INTERNAL_ERROR; //NOTA: se la funzione stat fallisce, allora il codice di risposta HTTP è 500 (Internal Error)
				goto int_err; //NOTA: salto alla label int_err
			}
			
			mime_type = strdup(HTML_mime); //NOTA: il mime type del file HTML_404 è "text/html", una variabile definita nel metodo. Il metodo strdup duplica la stringa passata come argomento e restituisce un puntatore alla nuova stringa. In questo caso, restituisce un puntatore alla stringa "text/html" che viene salvato in mime_type (che è una variabile locale al metodo che punta ad una stringa allocata dinamicamente)
			debug("    ... send_response(%d, %s): got MIME type %s\n", response_code, HTML_501, mime_type);
					
			file_size = stat_p -> st_size; //NOTA: recupero la grandezza del file HTML_404 dai metadati (salvati in stat_p) e la salvo in file_size
			if (file_size <= 0 ) { //NOTA: se la grandezza del file è negativa, c'è stato un errore nella lettura dei metadati oppure i metadati sono corrotti
				debug("send_response: file size cannot be zero or negative\n");
				goto int_err;
			}			
			file_modification_time = stat_p -> st_mtime; //NOTA: recupero il tempo di ultima modifica del file HTML_404 dai metadati (salvati in stat_p) e lo salvo in file_modification_time, come prima non posso fare controlli sui valori ottenuti
			debug("      ... send_response(%3d,%s) : file opened, size=%lu, MIME=%s\n", response_code, HTML_501, (unsigned long)file_size, mime_type);

/*** TO BE DONE 8.0 END ***/
		}
		break;
	}
	strcat(http_header, "\r\nDate: "); //NOTA: aggiungo alla stringa http_header la data di servizio della richiesta HTTP
	strcat(http_header, time_as_string); //NOTA: la data di servizio della richiesta HTTP è stata calcolata precedentemente e salvata in time_as_string (cfr l. 112), quindi la concateno alla stringa http_header
        if ( cookie >= 0 ) { //NOTA: se il cookie è maggiore o uguale a 0, allora il client ha un cookie valido e il server lo salva nella risposta; altrimenti il cookie non è ancora stato assegnato al client (il client sta facendo la sua prima richiesta al server)
        
	/*** set permanent cookie in order to identify this client ***/

		/*** TO BE DONE 8.0 START ***/

			sprintf(http_header + strlen(http_header), "\r\nSet-Cookie: UserID=%i%s", cookie, COOKIE_EXPIRE); //NOTA: stampo la stringa http_header a cui aggiungo la lunghezza dell'header, il cookie del client (che è un intero passato come argomento al metodo send_response) e la scadenza del cookie. (cookie = UserID)
			debug("...	 Setting permanent cookie in order to identify this client \n");

		/*** TO BE DONE 8.0 END ***/

        }
#ifdef INCaPACHE_8_1
	strcat(http_header, "\r\nServer: incApache 8.1 for SETI.\r\n"); //NOTA: aggiungo alla stringa http_header il nome del server (incApache 8.1 for SETI) e un ritorno a capo
	if (response_code >= 500 || is_http1_0) //NOTA: se il codice di risposta è maggiore o uguale a 500 (cioè se è un errore del server) oppure la richiesta è stata fatta con HTTP/1.0, allora la connessione viene chiusa dopo la risposta
		strcat(http_header, "Connection: close\r\n");
#else //NOTA: se non è definita la costante INCaPACHE_8_1, allora il server risponde con la versione 8.0
	strcat(http_header, "\r\nServer: incApache 8.0 for SETI.\r\n");
	strcat(http_header, "Connection: close\r\n");
#endif
	if (file_size > 0 && mime_type != NULL) { //NOTA: quando il file non è vuoto e presenta un formato MIME esistente 
		sprintf(http_header + strlen(http_header), "Content-Length: %lu \r\nContent-Type: %s\r\nLast-Modified: ", (unsigned long)file_size, mime_type); //NOTA: stampa la lunghezza e la tipologia del contenuto del file richiesto dal server. La data di ultima modifica verrà invece stampata nella riga di codice successiva

		/*** compute time_as_string, corresponding to file_modification_time, in GMT standard format;
		     see gmtime and strftime ***/
/*** TO BE DONE 8.0 START ***/

	if(!gmtime_r(&file_modification_time, &file_modification_tm)){ //NOTA: converto il tempo di ultima modifica del file richiesto in formato UTC (concorde alla struct file_modification_tm) e lo salvo nella struct file_modification_tm (valore di ritorno: null in caso di errore; se si verifica errore,!null = true, entra nell'if)
		fail("Could not convert file modification time in UTC standard format in send response");
	}

	if(strftime(time_as_string, MAX_TIME_STR,"%a, %d %b %Y %T GMT",&file_modification_tm)==0){ //NOTA: converto il tempo di ultima modifica del file richiesto da formato UTC (broken-down time) in formato standard GMT (passato come argomento alla srtftime) e lo salvo nell'array di char time_as_string di dimensione MAX_TIME_STR  

	//NOTA MANUALE: Return Value: The strftime() function returns the number of bytes placed in the array s, not including the terminating null byte, provided the string, including the terminating null byte, fits. Otherwise, it returns 0, and the contents of the array is undefined. (This behavior applies since at least libc 4.4.4; very old versions of libc, such as libc 4.4.1, would return max if the array was too small.). Note that the return value 0 does not necessarily indicate an error; for example, in many locales %p yields an empty string.
		fail("Could not convert file modification time in UTC standard format to GMT standard format in send response");
	}

	debug("      ... send_response: file modification time converted in GMT standard format %s\n", time_as_string);

/*** TO BE DONE 8.0 END ***/

		strcat(http_header, time_as_string); //NOTA: la data di ultima modifica del file richiesto è stata convertita in formato standard GMT e salvata in time_as_string, quindi la concateno alla stringa http_header che termina con "Last-Modified: "
		strcat(http_header, "\r\n");
	}
	strcat(http_header, "\r\n");
	debug("      ... send_response(%d, %s) : header prepared\n", response_code, filename); //NOTA: stampo a video che l'header è stato preparato, fornisco il codice di risposta del server e il nome del file richiesto dal client
	printf("Sending the following response:\n%s\n",http_header); //NOTA: stampo a video l'header preparato, ad esempio potrei stampare Sending the following response: \n HTTP/1.0 200 OK
	header_size = strlen(http_header); //NOTA: calcolo la lunghezza dell'header
#ifdef INCaPACHE_8_1
	join_prev_thread(thread_no); //NOTA: controlla se c'è e, in caso affermativo, aspetta la terminazione del thread precedente in coda a quello che gli è stato passato come argomento. 
#endif
#ifdef OptimizeTCP
	if ((header_sent=send_all(client_fd, http_header, header_size, (fd >= 0 ? MSG_MORE : 0))) < header_size) //NOTA: se il numero di byte scritti è minore del numero di byte che dovevi scrivere allora controlla se c'è stato errore o sono stati persi dei byte 
#else
	if ((header_sent=send_all(client_fd, http_header, header_size, 0)) < header_size )
#endif
	{
		if (header_sent==-1)
			fail_errno("incApache: could not send HTTP header");
		debug("header partially sent; header_size=%lu, header_sent=%lu\n", (unsigned long) header_size, (unsigned long) header_sent);
		close(fd);
		fd = -1;
	}
	debug("      ... send_response(%d, %s): header sent\n", response_code, filename);
	if (fd >= 0) {

		/*** send fd file on client_fd, then close fd; see syscall sendfile  ***/
/*** TO BE DONE 8.0 START ***/
	//ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);

	if(sendfile(client_fd, fd, NULL, file_size) == -1) { fail("could not send data from fd file to client_fd"); } //NOTA: copia il file descriptor contenuto in fd nel file descriptor contenuto in client_fd. Se la copia fallisce, viene segnalato errore. Se la copia avviene nel kernel, sendfile risulta  più efficiente della combinazione di read(2) e write(2), le quali richiederebbero invece di trasferire i dati da/a uno spazio utente
	if (close(fd) == -1) {fail_errno("close of file descriptor fd has failed");} //NOTA: chiudo il file descriptor contenuto in fd. Se la chiusura fallisce, viene segnalato errore

	debug("...	Sending fd file on client_fd \n");
	
/*** TO BE DONE 8.0 END ***/

	}
#ifdef OptimizeTCP
	if (fd >= 0) {
		int optval = 1;
		if (setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int)))
			fail_errno("Cannot set socket options");
	}
#endif
	debug("  ... end send_response(%d, %s)\n", response_code, filename);
	free(mime_type);
}


void manage_http_requests(int client_fd
#ifdef INCaPACHE_8_1
		, int connection_no
#endif
)
{
#define METHOD_NONE		 0
#define METHOD_HEAD		 1
#define METHOD_GET	 	 2
#define METHOD_POST		 4
#define METHOD_NOT_CHANGED	 8 //NOTA: il file non è stato modificato
#define METHOD_CONDITIONAL	16
#define MethodIsConditional(m) ((m)&METHOD_CONDITIONAL)
	FILE *client_stream = fdopen(client_fd, "r"); //NOTA: apro il file descriptor client_fd in lettura e lo salvo in client_stream. Il file descriptor client_fd è il file descriptor associato alla connessione con il client
	char *http_request_line = NULL;
	char *strtokr_save;
	size_t n = 0;
	int http_method;
	struct tm since_tm;
	struct stat *stat_p;
        int UIDcookie = -1;
#ifdef INCaPACHE_8_1
	int is_http1_0 = 0;
	int thread_idx;
#endif
	if (!client_stream) //NOTA: se la creazione del file stream client_stream fallisce, viene segnalato errore
		fail_errno("cannot open client stream");
	while (getline(&http_request_line, &n, client_stream) >= 0) { //NOTA: legge una riga alla volta dal client e la salva in http_request_line. Se la lettura fallisce, viene segnalato errore. La lettura avviene fino all'esaurimento dello stream del client e ogni riga è separata da un ritorno a capo dalla successiva. La lunghezza della riga letta è salvata in &n. getline restituisce -1 se la lettura fallisce, altrimenti restituisce la lunghezza della riga letta
		char *method_str, *filename, *protocol;
		char *http_option_line = NULL;
		char *option_name, *option_val;
		printf("\n-----------------------------------------------\n");
		printf("Received the following request:\n");
		printf("%s", http_request_line);
#ifdef INCaPACHE_8_1
		thread_idx = find_unused_thread_idx(connection_no); //NOTA: trova un thread libero da associare al client responsabile della richiesta. Se non ci sono thread liberi, restituisce -1
#endif

		/*** parse first line defining the 3 strings method_str,
		 *** filename, and protocol ***/
/*** TO BE DONE 8.0 START ***/

	//NOTA: strtok_r è una funzione che permette di suddividere una stringa in sottostringhe in base a un delimitatore. In questo caso, la stringa da suddividere è http_request_line, il delimitatore è uno spazio e il puntatore strtokr_save serve per mantenere lo stato della stringa da suddividere. strtok_r restituisce un puntatore alla prima sottostringa trovata, NULL se non trova nulla. strtok_r è thread-safe, a differenza di strtok che non lo è.

	method_str = strtok_r(http_request_line, " ", &strtokr_save);
	filename = strtok_r(NULL, " ", &strtokr_save);
	protocol = strtok_r(NULL, "\r\n", &strtokr_save); // NOTA: Changed delimiter to "\r\n" instead of " \r\n"

	debug("...	Parsing the first line of the request (method_str, filename, protocol) \n");

/*** TO BE DONE 8.0 END ***/

		debug("   ... method_str=%s, filename=%s (0=%c), protocol=%s (len=%d)\n",
		      method_str, filename, filename ? filename[0] : '?', protocol, (int)(protocol ? strlen(protocol) : 0));
		if (method_str == NULL || filename == NULL || protocol == NULL ||
		    filename[0] != '/' || strncmp(protocol, "HTTP/1.", 7) != 0 ||
		    strlen(protocol) != 8) {
			debug("       ... Bad Request!\n");
			SEND_RESPONSE(client_fd, RESPONSE_CODE_BAD_REQUEST, UIDcookie,
#ifdef INCaPACHE_8_1
				      1, connection_no, thread_idx,
#endif
				      NULL, NULL);
			free(http_request_line); //NOTA: libero la memoria allocata per la stringa http_request_line per procedere alla prossima richiesta da parte di un client
			break;
		}
#ifdef INCaPACHE_8_1
		is_http1_0 = !strcmp(protocol, "HTTP/1.0"); //NOTA: se il protocollo è HTTP/1.0, allora is_http1_0 = 1, altrimenti is_http1_0 = 0
#endif
		memset(&since_tm, 0, sizeof(since_tm)); //NOTA: imposta a 0 tutti i byte della struct since_tm
		http_method = METHOD_NONE; //NOTA: inizializzo la variabile http_method a nessun metodo
		//NOTA: assegnazione del metodo HTTP in base alla stringa del metodo passata dal client
		if (strcmp(method_str, "GET") == 0)
			http_method = METHOD_GET;
		else if (strcmp(method_str, "HEAD") == 0)
			http_method = METHOD_HEAD;
		else if (strcmp(method_str, "POST") == 0)
			http_method = METHOD_POST;
		debug("     ... http_method=%d\n", http_method);
		for (http_option_line = NULL, n = 0;
		     getline(&http_option_line, &n, client_stream) >= 0 && strcmp(http_option_line, "\r\n") != 0;
		     free(http_option_line), http_option_line = NULL, n = 0) {
			debug("http_option_line: %s", http_option_line);
			option_name = strtok_r(http_option_line, ": ", &strtokr_save);
			if ( option_name != NULL ) {
			    if ( strcmp(option_name, "Cookie") == 0 ) {
                                /*** parse the cookie in order to get the UserID and count the number of requests coming from this client ***/
/*** TO BE DONE 8.0 START ***/

					//NB: questo metodo permette di trovare il PRIMO cookie presente nella richiesta HTTP (conforme alla struttura già definita Cookie: UserID=) e se ci sono più cookie, questi vengono ignorati. Inoltre accetta solo valori numerici (e non altri valori possibili) da inserire nel cookie del client. Se il cookie non è presente, viene assegnato un nuovo UID al client (in altra parte del codice?)

					option_val = strtok_r(NULL, ";", &strtokr_save); //NOTA: recupero il valore dell'opzione Cookie e lo salvo in option_val
					//option_val = strtok_r(NULL, "", &strtokr_save); //NOTA: recupero il valore dell'opzione Cookie e lo salvo in option_val
					char *uid_pos = strstr(option_val, "UserID="); //NOTA: cerco la stringa "UserID=" all'interno del valore dell'opzione Cookie
					if (uid_pos) sscanf(uid_pos, "UserID=%d", &UIDcookie); //NOTA: se la stringa "UserID=" è presente nel valore dell'opzione Cookie, allora recupero il valore numerico successivo a "UserID=" (cioè il valore che il client passa come cookie) e lo salvo nella variabile UIDcookie

					debug("...	Cookie parsing + counting the number of requests coming from this client \n");

/*** TO BE DONE 8.0 END ***/

                            }
			    if ( http_method == METHOD_GET ) {

				/*** parse option line, recognize "If-Modified-Since" option,
				 *** and possibly add METHOD_CONDITIONAL flag to http_method
                                 *** and store date in since_tm
                                 ***/
/*** TO BE DONE 8.0 START ***/

				if(strcmp(option_name, "If-Modified-Since") == 0) { //NOTA: se la richiesta del client contiene la stringa "If-Modified-Since"
					http_method |= METHOD_CONDITIONAL; //NOTA: aggiungo il flag METHOD_CONDITIONAL al metodo HTTP
					char* datestr = strtok_r(NULL,";",&strtokr_save); //NOTA: recupero la data di ultima modifica del file richiesta dal client
					if(!strptime(datestr," %a, %d %b %Y %H:%M:%S %Z", &since_tm)) //NOTA: converto la stringa contenente la data di ultima modifica del file richiesta dal client in una struct tm e la salvo in since_tm
						fail("Could not store date in since_tm"); 
					debug("recognised If-Modified-Since option\n");
				}

/*** TO BE DONE 8.0 END ***/

			    }
                        }
		}
		free(http_option_line); //NOTA: libero la memoria allocata per la stringa http_option_line per procedere alla prossima richiesta da parte di un client (non ha valori di ritorno)
                if ( UIDcookie >= 0 ) { /*** increment visit count for this user ***/
                    int current_visit_count = keep_track_of_UID(UIDcookie);

                    if ( current_visit_count < 0 ) /*** wrong Cookie value ***/
                        UIDcookie = get_new_UID();
                    else {
			printf("\n client provided UID Cookie %d for the %d time\n", UIDcookie, current_visit_count);
                        UIDcookie = -1;
                    }
                } else /*** user did not provide any Cookie ***/
                    UIDcookie = get_new_UID();

/*** TO BE OPTIONALLY CHANGED START ***/
		if (http_method == METHOD_NONE || http_method == METHOD_POST) {
/*** TO BE OPTIONALLY CHANGED END ***/

			printf("method not implemented\n");
			SEND_RESPONSE(client_fd, 501, UIDcookie,
#ifdef INCaPACHE_8_1
				      1, connection_no, thread_idx,
#endif
				      method_str, NULL);
			break;
		}
		printf("\n-----------------------------------------------\n");
		if (strcmp(filename, "/") == 0)
			filename = "index.html";
		else
			filename += 1; /* remove leading '/' */
		debug("http_method=%d, filename=%s\n", http_method, filename);
		stat_p = my_malloc(sizeof(*stat_p));
		if (access(filename, R_OK) != 0 || stat(filename, stat_p) < 0) {
			debug("    ... file %s not found!\n", filename);
			free(stat_p);
			SEND_RESPONSE(client_fd, RESPONSE_CODE_NOT_FOUND, UIDcookie,
#ifdef INCaPACHE_8_1
				      is_http1_0, connection_no, thread_idx,
#endif
				      filename, NULL);
		} else {
			if (MethodIsConditional(http_method)) {

				/*** compare file last modification time and decide
				 *** whether to transform http_method to METHOD_NOT_CHANGED
				 *** Use something like timegm() to convert from struct tm to time_t
				 ***/
/*** TO BE DONE 8.0 START ***/
			
			debug("...	Comparing file last modification time with since_tm\n");
			
			//stat-p = server; since_tm = client
			if(difftime(stat_p->st_mtime, timegm(&since_tm)) <= 0) { //NOTA: se il file che richiede il client è più vecchio o aggiornato come il file che il server ha, allora il file non è stato modificato
				http_method = METHOD_NOT_CHANGED;	//NOTA: se il file non è stato modificato, il metodo HTTP diventa METHOD_NOT_CHANGED
			}
			else { //NOTA: se il server ha la copia del file più recente rispetto a quella del client, entro qui
				if (http_method == 18) { //NOTA: se il metodo HTTP è 18 (cioè METHOD_CONDITIONAL+METHOD_GET), allora il metodo HTTP condizionale diventa METHOD_GET
				//DA FARE: sarebbe da scrivere meglio la guardia dell'if perché è un po' brutto 18
					http_method = METHOD_GET;
					}
				else if (http_method == 17) {
					http_method = METHOD_HEAD; //NOTA: se il metodo HTTP è 17 (cioè METHOD_CONDITIONAL+METHOD_HEAD), allora il metodo HTTP condizionale diventa METHOD_HEAD
					}
				else {
					fail("The conditional request was not made with an appropriate method (HEAD or GET)");
					}
			}
	
/*** TO BE DONE 8.0 END ***/
			
			}
			switch (http_method) {
			case METHOD_HEAD :
				debug("    ... sending header for file %s\n", filename);
				free(stat_p);
				SEND_RESPONSE(client_fd, RESPONSE_CODE_OK, UIDcookie, /*** OK, without body ***/
#ifdef INCaPACHE_8_1
					      is_http1_0, connection_no, thread_idx,
#endif
					      filename, NULL);
				break;
			case METHOD_NOT_CHANGED :
				debug("    ... file %s not modified\n", filename);
				free(stat_p);
				SEND_RESPONSE(client_fd, RESPONSE_CODE_NOT_MODIFIED, UIDcookie, /*** Not Modified, without body ***/
#ifdef INCaPACHE_8_1
					      is_http1_0, connection_no, thread_idx,
#endif
					      NULL, NULL);
				break;
			case METHOD_GET :
				debug("    ... sending file %s\n", filename);
				SEND_RESPONSE(client_fd, RESPONSE_CODE_OK, UIDcookie, /*** OK, with body ***/
#ifdef INCaPACHE_8_1
					      is_http1_0, connection_no, thread_idx,
#endif
					      filename, stat_p);
				break;
			case METHOD_POST :

/*** TO BE OPTIONALLY DONE START ***/

/*** TO BE OPTIONALLY DONE END ***/

			default:
				assert(0);
			}
		}
#ifdef INCaPACHE_8_1
		if (is_http1_0)
#endif
			break;
	}
#ifdef INCaPACHE_8_1
	join_all_threads(connection_no);
#endif
	if (close(client_fd))
		fail_errno("Cannot close the connection");
}

