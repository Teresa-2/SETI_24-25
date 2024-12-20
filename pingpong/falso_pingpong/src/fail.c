/*
 * fail.c: funzioni ausiliarie per il ping-pong
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
#include <stdio.h>
#include "pingpong.h"

void fail_errno(const char * const msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

// NOTA: fail_errno stampa il messaggio msg seguito da una descrizione dell'errore corrispondente al valore corrente di errno. Il messaggio viene stampato su stderr e il programma termina con EXIT_FAILURE
// NOTA: metodo preferenziale di fail tra i due qui proposti

void fail(const char *const msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

//NOTA: fprintf(FILE *stream, const char *format, ...) stampa il messaggio formattato specificato da format e dagli argomenti successivi sullo stream specificato (stderr in questo caso).