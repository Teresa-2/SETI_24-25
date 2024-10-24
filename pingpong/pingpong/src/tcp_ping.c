/*
 * tcp_ping.c: esempio di implementazione del processo "ping" con
 *             socket di tipo STREAM.
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

#include "pingpong.h"

/*
 * This function sends and wait for a reply on a socket.
 * int msg_size: message length
 * int msg_no: message sequence number (written into the message)
 * char message[msg_size]: buffer to send
 * int tcp_socket: socket file descriptor
 */
double do_ping(size_t msg_size, int msg_no, char message[msg_size], int tcp_socket)
{
	char rec_buffer[msg_size];
	ssize_t recv_bytes, sent_bytes;
	size_t offset;
	struct timespec send_time, recv_time;

    /*** write msg_no at the beginning of the message buffer ***/
/*** TO BE DONE START ***/

	//NOTA: il metodo scrive una stringa formattata in un buffer. In particolare, formatta il numero interno msg_no come una stringa e lo memorizza nel buffer 'message'. Poichè 'message' è un array di char, la call trasformerà msg_no in forato char 

	if(sprintf(message, "%d", msg_no)<0) fail_errno(strerror(errno));

/*** TO BE DONE END ***/

    /*** Store the current time in send_time ***/
/*** TO BE DONE START ***/

	//DA CHIEDERE: perché non va bene CLOCK_MONOTONIC?
	if(clock_gettime(CLOCK_MONOTONIC, &send_time) ==-1 ) fail_errno(strerror(errno));

/*** TO BE DONE END ***/

    /*** Send the message through the socket ***/
/*** TO BE DONE START ***/

	//NOTA: ssize_t send(int sockfd, const void *buf, size_t len, int flags);
	sent_bytes= send(tcp_socket, message, msg_size, 0); 
	//NOTA: con i flags equivalenti a zero, la send è equivalente alla write 
	if(sent_bytes<0) fail_errno("Error sending data");  	

/*** TO BE DONE END ***/

    /*** Receive answer through the socket (blocking) ***/
	for (offset = 0; (offset + (recv_bytes = recv(tcp_socket, rec_buffer + offset, sent_bytes - offset, MSG_WAITALL))) < msg_size; offset += recv_bytes) {
		debug(" ... received %zd bytes back\n", recv_bytes);
		if (recv_bytes < 0)
			fail_errno("Error receiving data");
	}

    /*** Store the current time in recv_time ***/
/*** TO BE DONE START ***/

if(clock_gettime(CLOCK_REALTIME,&recv_time) !=0 ) fail_errno(strerror(errno)); 

/*** TO BE DONE END ***/

	printf("tcp_ping received %zd bytes back\n", recv_bytes);
	return timespec_delta2milliseconds(&recv_time, &send_time);
}

int main(int argc, char **argv)
{
	struct addrinfo gai_hints, *server_addrinfo; 
	int msgsz, norep;
	int gai_rv; 
	char ipstr[INET_ADDRSTRLEN];
	struct sockaddr_in *ipv4;
	int tcp_socket;
	char request[MAX_REQ], answer[MAX_ANSW];
	ssize_t nr; //NOTA: utilizzato per rappr numero di byte di READ e WRITE

	if (argc < 4)
		fail("Incorrect parameters provided. Use: tcp_ping PONG_ADDR PONG_PORT SIZE [NO_REP]\n");
	for (nr = 4, norep = REPEATS; nr < argc; nr++)
		if (*argv[nr] >= '1' && *argv[nr] <= '9')
			sscanf(argv[nr], "%d", &norep); //NOTA: trasforma una stringa in un numero e la mette nella zona puntata da norep
	if (norep < MINREPEATS)
		norep = MINREPEATS;
	else if (norep > MAXREPEATS)
		norep = MAXREPEATS;

    /*** Initialize hints in order to specify socket options ***/
	memset(&gai_hints, 0, sizeof gai_hints); //NOTA: Inizializza tutti a zero e alloca alla struttura &gai_hints uno spazio di memoria pari a sizeof gai_hints espressa in byte  

/*** TO BE DONE START ***/ 
gai_hints.ai_family = AF_INET; 
gai_hints.ai_socktype = SOCK_STREAM;

/*** TO BE DONE END ***/

    /*** call getaddrinfo() in order to get Pong Server address in binary form ***/
/*** TO BE DONE START ***/
	gai_rv= getaddrinfo(*argv[1],*argv[2],&gai_hints,&server_addrinfo); //NOTA: restituisce 0 se ha successo 
	if(gai_rv!=0) fail_errno(strerror(errno)); //NOTE: controllo del valore di ritorno e stampa di errore in caso di insuccesso 

/*** TO BE DONE END ***/

    /*** Print address of the Pong server before trying to connect ***/
	ipv4 = (struct sockaddr_in *)server_addrinfo->ai_addr; 
	printf("TCP Ping trying to connect to server %s (%s) on port %s\n", argv[1], inet_ntop(AF_INET, &ipv4->sin_addr, ipstr, INET_ADDRSTRLEN), argv[2]);

    /*** create a new TCP socket and connect it with the server ***/
/*** TO BE DONE START ***/
	tcp_socket=socket(server_addrinfo->ai_family,server_addrinfo->ai_socktype,server_addrinfo->ai_protocol); //NOTA: in caso di insuccesso, restituisce un errore
	if(tcp_socket==-1){ 
		fail_errno("Problem with socket creation during the connection inizialization");
	}
	if(connect(tcp_socket,server_addrinfo->ai_addr,server_addrinfo->ai_addrlen)!=0) {//NOTA: in caso di successo, resituisce 0, sennò -1
		fail_errno("Problem with socket connection in TCP");
	}

/*** TO BE DONE END ***/

	freeaddrinfo(server_addrinfo); //NOTA: libera l'indirizzo
	if (sscanf(argv[3], "%d", &msgsz) != 1) //NOTA: restituisce il numero di variabili a cui è riuscito ad asegnare un valore (in questo caso ne ha una sola. Se non è uguale a 1, quindi, ha fallito)
		fail("Incorrect format of size parameter");
	if (msgsz < MINSIZE)
		msgsz = MINSIZE;
	else if (msgsz > MAXTCPSIZE)
		msgsz = MAXTCPSIZE;
	printf(" ... connected to Pong server: asking for %d repetitions of %d bytes TCP messages\n", norep, msgsz);
	sprintf(request, "TCP %d %d\n", msgsz, norep); //NOTA: stampa request, TCP msgsz, norep e poi un byte nullo

    /*** Write the request on socket ***/
/*** TO BE DONE START ***/
	nr = blocking_write_all(tcp_socket, request, strlen(request)); //a differenza della write, blocking_write_all ripete la write finchè non ha scritto tutti i byte richiesti
	if(nr!=strlen(request) || nr<0) { //se non sono stati scritti tutti i byte, da errore
		fail_errno("Problem with Write"); 
	}

/*** TO BE DONE END ***/

	nr = read(tcp_socket, answer, sizeof(answer));
	if (nr < 0)
		fail_errno("TCP Ping could not receive answer from Pong server");
		

    /*** Check if the answer is OK, and fail if it is not ***/
/*** TO BE DONE START ***/
	/*NOTA: prende due stringhe, le confrontano: 
		- se ritorna 0, successo 
		- in caso contrario, le due stringhe non sono identiche
	*/
	//TERESA
	//errore come in udp, cambio strncmp con strcmp perché la risposta deve esattamente essere OK
	//if(strncmp("OK",answer), size_of(answer)!= 0) fail_errno("... Pong Server denied :-(\n"); 

	if (strcmp("OK",answer) != 0) //NOTA: se le stringhe sono uguali, strcmp restituisce 0
		fail_errno("... Pong Server denied :-(\n");

/*** TO BE DONE END ***/

    /*** else ***/
	printf(" ... Pong server agreed :-)\n");

	{
		double ping_times[norep];
		struct timespec zero, resolution;
		char message[msgsz];
		int rep;
		memset(message, 0, (size_t)msgsz);
		for(rep = 1; rep <= norep; ++rep) {
			double current_time = do_ping((size_t)msgsz, rep, message, tcp_socket);
			ping_times[rep - 1] = current_time;
			printf("Round trip time was %lg milliseconds in repetition %d\n", current_time, rep);
		}
		memset((void *)(&zero), 0, sizeof(struct timespec));
		if (clock_getres(CLOCK_TYPE, &resolution))
			fail_errno("TCP Ping could not get timer resolution");
		print_statistics(stdout, "TCP Ping: ", norep, ping_times, msgsz, timespec_delta2milliseconds(&resolution, &zero));
	}

	shutdown(tcp_socket, SHUT_RDWR);
	close(tcp_socket);
	exit(EXIT_SUCCESS);
}

