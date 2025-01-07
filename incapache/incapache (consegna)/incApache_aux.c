/* 
 * incApache_aux.c: funzioni ausiliarie per il web server
 *
 * Programma sviluppato a supporto del laboratorio di
 * Sistemi di Elaborazione e Trasmissione del corso di laurea
 * in Informatica classe L-31 presso l'Universita` degli Studi di
 * Genova per l'anno accademico 2023/2024.
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

void fail(const char *const msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

void fail_errno(const char *const msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void *my_malloc(size_t size)
{
	void *result = malloc(size);
	if (result == NULL)
		fail("Cannot allocate memory with malloc");
	return result;
}

char *my_strdup(const char *const s)
{
	char *result = strdup(s);
	if (result == NULL)
		fail("Cannot allocate memory for strdup");
	return result;
}

ssize_t send_all(int fd, const char *ptr, size_t n, int flags) /*
La funzione send_all è progettata per inviare tutti i dati specificati su un socket, gestendo eventuali interruzioni e assicurandosi che tutti i dati siano inviati prima di restituire il controllo al chiamante.
Parametri della Funzione: 
- int fd: Il file descriptor del socket su cui inviare i dati. (client_fd)
- const char *ptr: Un puntatore ai dati da inviare. (http_header)
- size_t n: La dimensione dei dati da inviare in byte. (header_size)
- int flags: I flag, se presenti, da passare alla chiamata di sistema send*/
{
	size_t n_left = n;
	while (n_left > 0) {
		ssize_t n_written = send(fd, ptr, n_left, flags); //NOTA: restituisce il numero di byte inviati oppure '-1' in caso di errore 
		if (n_written < 0) { 
			if (n_left == n)
				return -1; /* nothing has been sent */
			else //NOTA: qualcosa è stato inviato e c'è stato un errore nella send 
				break; /* we have sent something */
		} else if (n_written == 0) { //NOTA: non c'erano byte da inviare 
			break;
		}
		n_left -= n_written;
		ptr += n_written;
	}
	assert(n - n_left >= 0);
	return n - n_left; //NOTA: ritorna il numero di byte che è riuscita a scrivere  
}


static pthread_mutex_t my_timegm_mutex = PTHREAD_MUTEX_INITIALIZER;

time_t my_timegm(struct tm *tm)
{
               time_t ret;
               char *tz;

        /*** Guess what is missing here ... ***/
/*** TO BE DONE 8.0 START ***/

pthread_mutex_lock(&my_timegm_mutex);
tz = my_malloc(64); // allocazione di uno spazio di memoria con la funzione sovrastante mymalloc, che restituisce un puntatore ad uno spazio di 64 byte
memset(tz, 0, 64); // inizializzazione di tutti i 64 byte della memoria allocata a zero (default value più sicuro)

/*** TO BE DONE 8.0 END ***/

               setenv("TZ", "", 1);
               tzset();
               ret = mktime(tm);
               if (tz) {
                   setenv("TZ", tz, 1);
                   free(tz);
               } else
                   unsetenv("TZ");

        /*** Guess what is missing here ... ***/
/*** TO BE DONE 8.0 START ***/

pthread_mutex_unlock(&my_timegm_mutex);

/*** TO BE DONE 8.0 END ***/

               return ret;
}
