/*
 * readwrite.c: funzioni ausiliarie per il ping-pong
 *
 * versione 24.1 
 *
 * Programma sviluppato a supporto del laboratorio di
 * Sistemi di Elaborazione e Trasmissione del corso di laurea
 * in Informatica classe L-31 presso l'Universita` degli Studi di
 * Genova, anno accademico 2024/2025.
 *
 * Copyright (C) 2013-2014 by Giovanni Chiola <chiolag@acm.org>
 * Copyright (C) 2015-2016 by Giovanni Lagorio <giovanni.lagorio@unige.it>
 * Copyright (C) 2017-2024 by Giovanni Chiola <chiolag@acm.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdlib.h>
#include "pingpong.h"

/* read_all and write_all, inspired by readn and writen of the 
   book "Advanced Programming in the UNIX Environment" */

ssize_t read_all(int fd, void *ptr, size_t n)
{
	size_t n_left = n;
	while (n_left > 0) {
		ssize_t n_read = read(fd, ptr, n_left);
		if (n_read < 0) {
			if (n_left == n)
				return -1; /* nothing has been read */
			else
				break; /* we have read something */
		} else if (n_read == 0) {
			break; /* EOF */
		}
		n_left -= n_read;
		ptr += n_read;
	}
	assert(n - n_left >= 0);
	return n - n_left;
}

ssize_t blocking_write_all(int fd, const void *ptr, size_t n)
{
	size_t n_left = n;
	while (n_left > 0) {
		ssize_t n_written = write(fd, ptr, n_left);
		if (n_written < 0) {
			if (n_left == n)
				return -1; /* nothing has been written */
			else
				break; /* we have written something */
		} else if (n_written == 0) {
			break;
		}
		n_left -= n_written;
		ptr += n_written;
	}
	assert(n - n_left >= 0);
	return n - n_left;
}

/*NOTA: tenta di scrivere tutti i dati forniti in modalita' non bloccante su un file descriptor 
fd: file descroptor in cui scrivere 
ptr: puntatore ai dati su cui scrivere 
n: dimensione dei dati da scrivere
return: 
CASO 1 :  numero di byte effettivamente scritti, che è la differenza tra il numero totale di byte da scrivere (n) e il numero di byte rimanenti (n_left)
CASO 2: -1 come errore cioè la write ha fallito per errore bloccante o non è stato scritto alcun byte
*/
ssize_t nonblocking_write_all(int fd, const void *ptr, size_t n)
{
	size_t n_left = n; //NOTA: n_left sono i dai rimanenti da scrivere (espressi in numero di byte)
	while (n_left > 0) {
		ssize_t n_written = write(fd, ptr, n_left);
		if (n_written < 0) { //NOTA: se la write da' errore (return value = -1), verifichiamo se l'errore è bloccante o meno 

/*** TO BE DONE START ***/
// DA CHIEDERE: vanno bene questi due errori, a parte EWOULDBLOCK 
			if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){  //NOTA: se l'errore è non bloccante, va a riga 89 grazie alla continue 
					printf("non-blocking error (data couldn't be written immediately)\n");
					continue;
			}
			else return -1; //NOTA: se l'errore è bloccante, la write fallisce 
			
/*** TO BE DONE END ***/

			if (n_left == n)
				return -1; /* nothing has been written */
			else
				break; /* we have written something */
		} else if (n_written == 0) {
			break;
		}
		n_left -= n_written;
		ptr += n_written;
	}
	assert(n - n_left >= 0);
	return n - n_left;
}

