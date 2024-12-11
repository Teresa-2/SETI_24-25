/* 
 * incApache_main.c: implementazione del main per il web server del corso di SETI
 *
 * Programma sviluppato a supporto del laboratorio di
 * Sistemi di Elaborazione e Trasmissione del corso di laurea
 * in Informatica classe L-31 presso l'Universita` degli Studi di
 * Genova per l'anno accademico 2024/2025.
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

int listen_fd;
FILE *mime_request_stream, *mime_reply_stream;

void create_listening_socket(const char *const port_as_str)
{
	struct addrinfo hints;
	struct addrinfo *server_addr;
	int gai_rv;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_TCP;
	if ((gai_rv = getaddrinfo(NULL, port_as_str, &hints, &server_addr))) //NOTA: vedi pingpong
		fail(gai_strerror(gai_rv));
	if ((listen_fd = socket(server_addr->ai_family, server_addr->ai_socktype, server_addr->ai_protocol)) == -1)
		fail_errno("Could not allocate socket descriptor");
	if (bind(listen_fd, server_addr->ai_addr, server_addr->ai_addrlen) == -1)
		fail_errno("Could not bind socket");
	freeaddrinfo(server_addr);
	if (listen(listen_fd, BACKLOG) == -1)
		fail_errno("Could not listen on socket");
}

void drop_privileges() //NOTA: metodo che fa in modo che il processo che lo esegue perda i privilegi di root e diventi un processo normale
{
	uid_t uid = getuid(); //NOTA: getuid() è un metodo creato nel progetto che restituisce l'effective UID dell'utente che sta eseguendo in quel momento il processo. NB) se uid == 0 allora l'utente è root
	assert(uid); //NOTA: se l'utente che esegue il processo è root, allora termina l'esecuzione del programma
	if (setuid(uid)) //NOTA: se l'utente non è root, allora setuid() fa in modo che il processo perda i privilegi di root e diventi un processo normale. Se non riesce a farlo allora stampa un messaggio di errore e termina l'esecuzione
		fail_errno("Cannot drop privileges");
}

void run_file(const int *p_to_file, const int *p_from_file)
{
	//DA RIVEDERE teoria lagorio
	drop_privileges(); //NOTA: fa in modo che il processo perda i privilegi di root e diventi un processo normale. se fosse un processo root, allora terminerebbe l'esecuzione del programma. ATTENZIONE: è necessario che ci siano 1 utente root (= client) + 1 utente non root (= server) per far funzionare correttamente il programma. Il server è NON root affinché i file che mette a disposizione siano accessibili a tutti gli utenti + perché i file a cui può accedere siano solo quelli della directory www_root
	if (close(p_to_file[PIPE_WRITE_END])) //NOTA: chiude l'estremo di scrittura della pipe p_to_file. Se non riesce a chiuderlo allora stampa un messaggio di errore e termina l'esecuzione.
		fail_errno("close p_to_file write-end");
	if (dup2(p_to_file[PIPE_READ_END], STDIN_FILENO) == -1) //NOTA: duplica l'estremo di scrittura della pipe p_to_file e lo associa al file descriptor STDIN_FILENO. Se non riesce a duplicare l'estremo di lettura allora stampa un messaggio di errore e termina l'esecuzione.
		fail_errno("dup2 stdin");
	if (close(p_from_file[PIPE_READ_END])) //NOTA: chiude l'estremo di lettura della pipe p_from_file. Se non riesce a chiuderlo allora stampa un messaggio di errore e termina
		fail_errno("close p_from_file read-end");
	if (dup2(p_from_file[PIPE_WRITE_END], STDOUT_FILENO) == -1) //NOTA: duplica l'estremo di lettura della pipe p_from_file e lo associa al file descriptor STDOUT_FILENO. Se non riesce a duplicare l'estremo di scrittura allora stampa un messaggio di errore e termin
		fail_errno("dup2 stdout");
	execlp("file", "file", "--no-buffer", "--brief", "--mime", "--files-from", "-", (char *) NULL); //NOTA: esegue il file "file" con i parametri "--no-buffer", "--brief", "--mime", "--files-from", "-". Se non riesce a eseguire il file allora stampa un messaggio di errore e termina l'esecuzione
	fprintf(stderr, "Cannot exec \"file\"\n");
	exit(-1);
}

void run_webserver(const char *const port_as_str, char *www_root, const int *const p_to_file,
		   const int *const p_from_file)
{
	int i;

	/*** perform chroot to www_root; then, create, bind, and listen to
	 *** listen_fd, and eventually drop root privileges ***/
	if (chroot(www_root) < 0) {
#ifndef PRETEND_TO_BE_ROOT
		fail_errno("Cannot chroot");
#endif /* #ifndef PRETEND_TO_BE_ROOT */
	}

/*** TO BE DONE 8.0 START ***/

	create_listening_socket(port_as_str); //NOTA: crea un socket in ascolto sulla porta specificata da port_as_str che agirà da server con il protocollo TCP/IP
	drop_privileges(); //NOTA: rimozione dei privilegi da root in quanto server. Il server non può essere root per due ragioni: sicurezza + accessibilità limitata dei file (si può accedere solo ai file della directory www_root)
	
/*** TO BE DONE 8.0 END ***/

#ifdef INCaPACHE_8_1
	printf("Server HTTP 1.1 (with pipelining support)");
#else /* #ifdef INCaPACHE_8_1 */
	printf("Server HTTP 1.0");
#endif /* #ifdef INCaPACHE_8_1 */
	printf(" listening on port %s\nwith WWW root set to %s\n\n", port_as_str, www_root);
	free(www_root); //NOTA: libera la memoria allocata dinamicamente per la directory delle pagine HTML; in quanto ora accessibili tramite il server
	mime_request_stream = fdopen(p_to_file[1], "w");
	if (!mime_request_stream)
		fail_errno("Cannot create MIME request stream");
	if (close(p_to_file[0]))
		fail_errno("Cannot close p_to_file read-end");
	mime_reply_stream = fdopen(p_from_file[0], "r");
	if (!mime_reply_stream)
		fail_errno("Cannot create MIME reply stream");
	if (close(p_from_file[1]))
		fail_errno("Cannot close p_from_file write-end");
#ifdef INCaPACHE_8_1
	for (i = MAX_CONNECTIONS; i < MAX_THREADS; i++) //NOTA: inizializza l'array connection_no[] che descrive per ogni thread la connessione (cioè il client) a cui questo è associato. Inizialmente tutti i thread sono liberi (FREE_SLOT) perché nessuno è stato ancora associato ad un client. L'inizializzazione dell'array non è necessaria per i primi MAX_CONNECTIONS (cioè per i primi 4 elementi dell'array) perché sono thread di connessione riservati ai client (in particolare, 1 thread per ogni client). Gli altri thread sono thread di risposta ai client.
		connection_no[i] = FREE_SLOT;
#endif /* #ifdef INCaPACHE_8_1 */
	for (i = 0; i < MAX_CONNECTIONS; i++) { //NOTA: inizializza i thread di connessione dell'array connection_no[], cioè attrbiuisce ad ogni client un proprio thread di connessione. Sia protocollo HTTP 1.0 che 1.1.
		connection_no[i] = i;
#ifdef INCaPACHE_8_1
		no_response_threads[i] = 0; //NOTA: nel protocollo HTTP 1.1, inizializza le prime quattro celle dell'array no_response_threads[] con 0. Tale array descrive per ogni client il numero di thread di risposta associati al relativo client. Inizialmente nessun client ha thread di risposta associati quindi tutti i client hanno 0 thread di risposta associati.
#endif /* #ifdef INCaPACHE_8_1 */

		/*** create PTHREAD number i, running client_connection_thread() ***/
/*** TO BE DONE 8.0 START ***/

		client_connection_thread((void *) &connection_no[i]); //NOTA: crea un thread di connessione per il client i-esimo (ed essendo in un ciclo lo farà per tutti i 4 possibili client). Il thread di connessione è un thread che si occupa di gestire la connessione con il client. Il thread di connessione è associato ad un client e rimane attivo finché il client non chiude la connessione. Il thread di connessione è un thread di tipo PTHREAD. Il thread di connessione è creato con il metodo client_connection_thread() che prende come argomento un puntatore all'indice del client a cui il thread è associato.

/*** TO BE DONE 8.0 END ***/

	}
	for (i = 0; i < MAX_CONNECTIONS; i++) //NOTA: per ogni client, si attende la terminazione di thread_ids[i] (cioè del thread di connessione associati a ciascun client)
		if (pthread_join(thread_ids[i], NULL))
			fail_errno("Could not join thread");
	if (close(listen_fd))
		fail_errno("Cannot close listening socket");
	if (fclose(mime_request_stream))
		fail_errno("Cannot close MIME request stream");
	if (fclose(mime_reply_stream))
		fail_errno("Cannot close MIME reply stream");
}

void check_uids() //NOTA: controlla se l'utente che esegue il programma con PRETEND_TO_BE_ROOT è root o meno e lancia errore se non è root (cioè quando il flag è commentato)
{
#ifndef PRETEND_TO_BE_ROOT //NOTA: le operazioni all'interno di questo if verranno eseguite SOLO SE la variabile PRETEND_TO_BE_ROOT NON è DEFINITA (If Not Defined = ifndef) nel makefile
	if (geteuid()) { //NOTA: geteuid() è un metodo creato nel progetto che restituisce l'effective UID dell'utente che sta eseguendo in quel momento il processo. Essendo che per essere ROOT devi avere id=0 questo if controlla che tu sia ROOT o meno. Zero per un if è equivalente ad un false. Pertanto entra nell'if quando l'utente non è root.
		fprintf(stderr, "The effective UID should be zero (that is, the executable should be owned by root and have the SETUID flag on).\n");
		exit(EXIT_FAILURE);
	}
#endif /* #ifndef PRETEND_TO_BE_ROOT */
	if (getuid() == 0) { //NOTA: se l'utente che esegue il processo è root, allora stampa un messaggio di errore e termina l'esecuzione
		fprintf(stderr,
			"The real UID should be non-zero (that is, the executable should be run by a non-root account).\n");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	int p_to_file[2], p_from_file[2]; //NOTA: array di dimensione 2, rappresentano rispettivamente i due estremi della pipe: p_from_file[2] è l'estremo di lettura e p_to_file[2] è l'estremo di scrittura. entrambi sono file descriptor.
	const char *port_as_str; //NOTA: puntatore a stringa che rappresenta la porta in cui mettere in ascolto il server. è la seconda stringa passata come argomento al programma (argv[2]), se omessa viene sostituita con la porta di default "8000"
	const char *const default_port = "8000"; //NOTA: porta di default se non viene indicata una porta in cui mettere in ascolto in server
	char *www_root; //NOTA: puntatore a directory di Pagine HTML di prova
	pid_t pid; //NOTA: numero univoco associato dal SO ad un programMa quando va in esecuzione (cioè quando diventa un processo)
	signal(SIGPIPE, SIG_IGN); //NOTA: system call che fa in modo che nel caso in cui venga inviato dal processo un segnale di tipo SIGPIPE esso venga sostituito dal segnale SIG_IGN (= che lo ignora). Quindi se un processo cercasse di accedere ad un socket/pipe chiuso, grazie a questa chiamata di metodo il processo non si blocca ma viene semplicemente ignorato. In questo modo il processo continuerà la sua esecuzione e restituirà il valore di errore (-1) solo se si cercheranno di eseguire le system call WRITE e READ.
#ifdef PRETEND_TO_BE_ROOT //NOTA: se PRETEND_TO_BE_ROOT è definito, allora il programma si comporta come se fosse root
	fprintf(stderr, "\n\n\n*** Debug UNSAFE version - DO NOT DISTRIBUTE ***\n\n");
#endif /* #ifdef PRETEND_TO_BE_ROOT */
	check_uids(); //NOTA: controlla se l'utente che esegue il programma è root o meno
	if (argc < 2 || argc > 3) { //NOTA: se il numero di argomenti passati al programma è diverso da 2 o 3 allora stampa un messaggio di errore e termina l'esecuzione. Gli argomenti sono: argv[0] = nome del programma , argv[1] = directory delle pagine HTML + (fac.) argv[2] = porta in cui mettere in ascolto il server
		fprintf(stderr, "Usage: %s <www-root> [<port-number>]\nDefault port: %s\n", *argv, default_port);
		return EXIT_FAILURE;
	}
	port_as_str = argc == 3 ? argv[2] : default_port; //NOTA: se il numero di argomenti è 3 allora la porta è la seconda stringa passata come argomento al programma (argv[2]), altrimenti è la porta di default "8000" che viene assegnata a port_as_str
	www_root = argv[1]; //NOTA: la directory delle pagine HTML è la prima stringa passata come argomento al programma (argv[1])
	if (pipe(p_to_file)) //NOTA: crea una pipe tra i due estremi partendo da p_to_file[2]
		fail_errno("Cannot create pipe to-file");
	if (pipe(p_from_file)) //NOTA: crea una pipe tra i due estremi partendo da p_from_file[2]. Ora la pipe è completa e pronta per essere utilizzata
		fail_errno("Cannot create pipe from-file");
	if (chdir(www_root)) { //NOTA: cambia la directory corrente in quella specificata da www_root. Se non riesce a cambiare la directory allora stampa un messaggio di errore e termina l'esecuzione
		fail_errno("Cannot chdir to www-root directory");
	}
	www_root = getcwd(NULL, 0); //NOTA: getcwd() salva il path della directory corrente in un array allocato dinamicamente. Se non riesce a restituire il path allora stampa un messaggio di errore e termina l'esecuzione
	if (!www_root) //NOTA: se il path della directory corrente non è stato salvato nella variabile www_root, allora stampa un messaggio di errore e termina l'esecuzione
		fail_errno("Cannot get current directory");
	pid = fork(); //NOTA: crea un processo figlio il cui pid è salvato nella variabile "pid" (vedi fork)
	if (pid < 0) //NOTA: Se non si riesce a creare il processo figlio allora stampa un messaggio di errore e termina l'esecuzione
		fail_errno("Cannot fork");
	if (pid == 0) //NOTA: se il pid del processo figlio è 0 (come la root) allora esegue il metodo run_file, altrimenti esegue il metodo run_webserver. il metodo run_file toglie i privilegi da root al processo figlio e lo esegue (vedi run_file)
		run_file(p_to_file, p_from_file);
            else {
	        run_webserver(port_as_str, www_root, p_to_file, p_from_file); //NOTA: esegue il webserver con i parametri passati quando il pid del processo figlio è maggiore di 0
			}
	return EXIT_SUCCESS; //NOTA: termina l'esecuzione del programma con successo dopo che sono terminate (con successo) le esecuzioni di run_file o run_webserver
}

