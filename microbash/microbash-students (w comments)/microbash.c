//error Please read the accompanying microbash.pdf before hacking this source code (and removing this line).
/*
 * Micro-bash v2.2
 *
 * Programma sviluppato a supporto del laboratorio di Sistemi di
 * Elaborazione e Trasmissione dell'Informazione del corso di laurea
 * in Informatica presso l'Università degli Studi di Genova, a.a. 2024/2025.
 *
 * Copyright (C) 2020-2024 by Giovanni Lagorio <giovanni.lagorio@unige.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#ifndef NO_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif
#include <stdint.h>
#include <asm-generic/fcntl.h>

void fatal(const char * const msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

void fatal_errno(const char * const msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void *my_malloc(size_t size)
{
	void *rv = malloc(size);
	if (!rv)
		fatal_errno("my_malloc");
	return rv;
}

void *my_realloc(void *ptr, size_t size) //funzione che alloca memoria dinamica ad un puntatore ptr di dimensione size. L'allocazione serve solo per aumentare la dimensione del blocco di memoria associato al puntatore ptr e aggiunge i byte necessari (non inizializzati) per raggiungere la dimensione size. se ptr = NULL la realloc funziona come una malloc; se invece la realloc va a ridurre le dimensione di ptr, allora si comporta come una free(ptr). 
{
	void *rv = realloc(ptr, size);
	if (!rv)
		fatal_errno("my_realloc");
	return rv;
}

char *my_strdup(char *ptr)
{
	char *rv = strdup(ptr);
	if (!rv)
		fatal_errno("my_strdup");
	return rv;
}

#define malloc I_really_should_not_be_using_a_bare_malloc
#define realloc I_really_should_not_be_using_a_bare_realloc
#define strdup I_really_should_not_be_using_a_bare_strdup

static const int NO_REDIR = -1;

typedef enum { CHECK_OK = 0, CHECK_FAILED = -1 } check_t;

static const char *const CD = "cd";

typedef struct {
	int n_args; // number of arguments
	char **args; // in an execv*-compatible format; i.e., args[n_args]=0
	char *out_pathname; // 0 if no output-redirection is present
	char *in_pathname; // 0 if no input-redirection is present
} command_t;

typedef struct {
	int n_commands; // number of commands in the line
	command_t **commands; //array dei comandi della riga
} line_t;

//NOTA: la funzione free_command libera la memoria allocata dinamicamente per il comando c
void free_command(command_t * const c)
{
	assert(c==0 || c->n_args==0 || (c->n_args > 0 && c->args[c->n_args] == 0)); /* sanity-check: if c is not null, then it is either empty (in case of parsing error) or its args are properly NULL-terminated */
	/*** TO BE DONE START ***/

	//svuotamento degli elementi contenuti nell'array args associato al comando c (perché devo svuotare l'array e poi eliminare la struttura anzichè eliminare direttamente l'array?: per evitare memory leak! cancellando direttamente l'array lascerei allocata la memoria associata agli argomenti dell'array stesso)
	for (int i = 0; i < c -> n_args; i++) {
		free(c -> args[i]);
	}

	free(c -> args); //rimozione dell'array di argomenti
	free(c -> out_pathname); //rimozione del pathname di output
	free(c -> in_pathname); //rimozione del pathname di input
	free(c); //rimozione della struttura del comando

	//NOTA BENE: non viene deallocata la memoria associata all'interno n_args perché è un intero e non è allocato dinamicamente

	/*** TO BE DONE END ***/
}

void free_line(line_t * const l) //libera la memoria allocata dinamicamente per la linea l di comandi
{
	assert(l==0 || l->n_commands>=0); /* sanity-check */
	/*** TO BE DONE START ***/
	for (int i = 0; i < l -> n_commands; i++) //rimozione dei comandi (contenuto dell'array di comandi della linea)
		free_command(l -> commands[i]);
	
	free(l -> commands); //rimozione dell'array di comandi
	free(l); //rimozione della struttura della linea

	/*** TO BE DONE END ***/
}

#ifdef DEBUG
void print_command(const command_t * const c) //stampa il comando c
{
	if (!c) { //se il comando è nullo (NOTA BENE: NULL=0 in C)
		printf("Command == NULL\n");
		return;
	} //stampa del comando (se non è nullo)
	printf("[ ");
	for(int a=0; a<c->n_args; ++a)
		printf("%s ", c->args[a]);
	assert(c->args[c->n_args] == 0); //The array of pointers for execve must be terminated by a null pointer
	printf("] ");
	printf("in: %s out: %s\n", c->in_pathname, c->out_pathname); //stampa del pathname di input e di output del comando
}

void print_line(const line_t * const l)
{
	if (!l) { //se la linea dei comandi è nulla (NOTA BENE: NULL=0 in C)
		printf("Line == NULL\n");
		return;
	}
	printf("Line has %d command(s):\n", l->n_commands); //stampa del numero di comandi presenti nella linea
	for(int a=0; a<l->n_commands; ++a) //stampa di tutti i comandi presenti nella linea
		print_command(l->commands[a]);
}
#endif

command_t *parse_cmd(char * const cmdstr) //analisi sintattica del comando cmdstr passato come argomento
{
	static const char *const BLANKS = " \t";
	command_t * const result = my_malloc(sizeof(*result)); //allocazione della memoria per il puntatore (result) che punta ad una struct command_t la quale è il valore di ritorno della funzione
	memset(result, 0, sizeof(*result)); //inizializzazione della struct result con soli zeri (per evitare garbage values)
	char *saveptr, *tmp;
	tmp = strtok_r(cmdstr, BLANKS, &saveptr); //parsing della stringa cmdstr attraverso il metodo strtok_r (che è una versione thread-safe di strtok). Nel parsing viene usalto BLANKS come delimitatore tra un token e quello successivo. Strtok_r legge un token per volta e restituisce un puntatore al punto in cui è arrivato nella lettura affinché alla chiamata successiva di strtok_r possa riprendere da dove aveva lasciato. strtok_r quando arriva alla fine della stringa da leggere ritornerà NULL.
	//ATTENZIONE: in questo momento strtok_r passa al while solo il primo token della stringa cmdstr
	//tmp = TOKEN ATTUALE, è una stringa (char *)
	while (tmp) { //finché ci sono token da leggere, per ogni token letto si esegue una analisi sintattica 
		result->args = my_realloc(result->args, (result->n_args + 2)*sizeof(char *)); //vengono allocate due nuove celle nell'array di argomenti del comando (result->args) per contenere 1) il token appena letto 2) il terminatore NULL che segnerà la fine dell'iterazione in atto per il parsing del comando
		if (*tmp=='<') { //se il token specifica per una redirezione in input (che inizia con <)
			if (result->in_pathname) { //si controlla che non sia già stata specificata una redirezione in input, se è già stata specificata si stampa un messaggio di errore e si dealloca la memoria allocata per il comando e ritorna errore (vedi label FAIL in parse_cmd)
				fprintf(stderr, "Parsing error: cannot have more than one input redirection\n");
				goto fail;
			}
			if (!tmp[1]) { //si controlla se è stato specificato nel token successivo un pathname per la redirezione in input. se non è stato specificato si stampa un messaggio di errore e si dealloca la memoria allocata per il comando (vedi label FAIL in parse_cmd)
				fprintf(stderr, "Parsing error: no path specified for input redirection\n");
				goto fail;
			}
			result->in_pathname = my_strdup(tmp+1); //se la redirezione è stata specificata correttamente, si copia il pathname specificato nel token successivo a quello corrente (che è "<") e si inserisce tale valore nel campo in_pathname della struct result
		} else if (*tmp == '>') {
			//nota: vedi sopra, come per input redirection
			if (result->out_pathname) {
				fprintf(stderr, "Parsing error: cannot have more than one output redirection\n");
				goto fail;
			}
			if (!tmp[1]) {
				fprintf(stderr, "Parsing error: no path specified for output redirection\n");
				goto fail;
			}
			result->out_pathname = my_strdup(tmp+1);
		} else {
			//caso in cui il token letto è una variabile d'ambiente da leggere
			if (*tmp=='$') { //ATTENZIONE: il simbolo $ è usato per indicare una variabile d'ambiente. In microbash NON è possibile dichiarare nuove variabili d'ambiente, quindi bisogna usare solo quelle predefinite (es. PATH)

				/* Make tmp point to the value of the corresponding environment variable, if any, or the empty string otherwise */
				/*** TO BE DONE START ***/

				if (!tmp[1]) { //se non è specificato il nome della variabile d'ambiente si stampa un messaggio di errore e si dealloca la memoria allocata per il comando (vedi label FAIL in parse_cmd)
					fprintf(stderr, "Parsing error: no variable name specified\n");
					goto fail;
				}
				if (!(tmp = getenv(tmp + 1))) //recupero tramite la funzione getenv (che trova una variabile data nell'ambiente corrente) il valore della variabile d'ambiente specificata nel token corrente (che inizia con $). Se la variabile d'ambiente non esiste, la funzione getenv restituisce NULL
					tmp = ""; //se la variabile d'ambiente non esiste, tmp punterà ad una stringa vuota

				//NOTA BENE: questo controllo garantisce che il programma non tenti di dereferenziare un puntatore NULL, prevenendo potenziali crash o comportamenti indefiniti

				/*** TO BE DONE END ***/
			}
			//se si sta accedendo ad una variabile d'ambiente oppure in tutti gli altri casi (escluse le redirezioni in input e output):
			result->args[result->n_args++] = my_strdup(tmp); //my_strdup(tmp) restituisce una copia della stringa puntata da tmp allocata dinamicamente. Questa copia viene inserita nell'array di argomenti del comando (nella posizone result->n_args) e il contatore n_args viene incrementato di 1
			result->args[result->n_args] = 0; //imposta il successivo elemento dell'array args a NULL(cioè 0) per segnalare la fine dell'array degli argomenti del comando
		}
		tmp = strtok_r(0, BLANKS, &saveptr); //lettura del token successivo. dalla seconda iterazione in poi, strtok_r leggerà il token successivo a quello letto nella chiamata precedente di strtok_r e la stringa passata come primo argomento sarà SEMPRE NULL (per definizione metodo da man)
	}
	if (result->n_args) //se il comando ha almeno un argomento (cioè se il comando non è vuoto)
		return result; //ritorna il comando correttamente parsato
	fprintf(stderr, "Parsing error: empty command\n"); //altrimenti; se il comando è vuoto si stampa un messaggio di errore
fail:
	free_command(result);
	return 0;
}

line_t *parse_line(char * const line) //analisi sintattica della linea di comandi line passata come argomento
{
	static const char * const PIPE = "|";
	char *cmd, *saveptr;
	cmd = strtok_r(line, PIPE, &saveptr); //parsing della stringa line attraverso il metodo strtok_r (che è una versione thread-safe di strtok). Nel parsing viene usato PIPE come delimitatore tra un comando e quello successivo. Strtok_r legge un token per volta e salva in cmd un puntatore al punto in cui è arrivato nella lettura affinché alla chiamata successiva di strtok_r possa riprendere da dove aveva lasciato. strtok_r quando arriva alla fine della stringa da leggere ritornerà NULL.
	if (!cmd) //se la stringa line è vuota, la funzione restituisce NULL (cioè 0)
		return 0;
	//altrimenti, se la stringa line non è vuota, si procede con l'analisi sintattica della linea di comandi
	line_t *result = my_malloc(sizeof(*result)); //allocazione della memoria per il puntatore (result) che punta ad una struct line_t la quale è il valore di ritorno della funzione
	memset(result, 0, sizeof(*result)); //inizializzazione della struct result con soli zeri (per evitare garbage values)
	while (cmd) { //finché ci sono comandi da leggere, per ogni comando letto si esegue una analisi sintattica
		command_t * const c = parse_cmd(cmd); //analisi sintattica del comando cmd
		if (!c) { //se il comando è nullo (NOTA BENE: NULL=0 in C)
			free_line(result); //si dealloca la memoria allocata per la linea di comandi
			return 0; //si restituisce NULL (cioè 0) e il programma termina
		}
		//se il comando è stato correttamente parsato e non è null, si inserisce il comando nell'array di comandi della linea di comandi
		result->commands = my_realloc(result->commands, (result->n_commands + 1)*sizeof(command_t *)); //allocazione di una nuova cella nell'array di comandi della linea
		result->commands[result->n_commands++] = c; //inserimento del comando nell'ultima cella appena inserita nell'array dei comandi della linea + incremento del contatore dei comandi della linea
		cmd = strtok_r(0, PIPE, &saveptr); //aggiornamento della variabile cmd con il comando successivo che verrà parsato nella prossima iterazione di parse_line
	}
	return result; //se la linea di comandi non è vuota ed è stata correttamente parsata, si restituisce la linea di comandi
}

check_t check_redirections(const line_t * const l)
{
	assert(l); //controllo che la linea di comandi non sia nulla, altrimenti ritorna errore
	
	/* This function must check that:
	 * - Only the first command of a line can have input-redirection
	 * - Only the last command of a line can have output-redirection
	 * and return CHECK_OK if everything is ok, print a proper error
	 * message and return CHECK_FAILED otherwise
	 */
	/*** TO BE DONE START ***/
	
	if (l -> n_commands != 1) //escludendo i casi in cui ci sia al masismo solo un comando nella linea
		for (int i = 0; i < l -> n_commands; ++i) { //scorrimento di tutti i comandi della linea
			if (i != 0) //se il comando non è il primo della linea
				if (l -> commands[i] -> in_pathname) { //se a partire dal 2° comando ha una redirezione in input (cioè se un comando diverso dal 1° ha un pathname di input diverso da NULL)
					fprintf(stderr, "Input redirection in a non-first command\n"); //si stampa un messaggio di errore
					return CHECK_FAILED; }
			if (i != l -> n_commands - 1) //se il comando non è l'ultimo della linea
				if (l -> commands[i] -> out_pathname) { //se un comando (che non è l'ultimo della riga a cui appartiene) ha una redirezione in output (cioè se ha un pathname di output diverso da NULL)
					fprintf(stderr, "Output redirection in a non-last command\n"); //si stampa un messaggio di errore
					return CHECK_FAILED;
				}
		}

	/*** TO BE DONE END ***/
	return CHECK_OK; //in tutti gli altri casi, si restituisce CHECK_OK	
}

check_t check_cd(const line_t * const l)
{
	assert(l); //controllo che la linea di comandi non sia nulla, altrimenti ritorna errore

	/* This function must check that if command "cd" is present in l, then such a command
	 * 1) must be the only command of the line
	 * 2) cannot have I/O redirections
	 * 3) must have only one argument
	 * and return CHECK_OK if everything is ok, print a proper error
	 * message and return CHECK_FAILED otherwise
	 */
	/*** TO BE DONE START ***/

	for (int i = 0; i < l -> n_commands; ++i) //scorrimento di tutti i comandi della linea
		if (strncmp(l -> commands[i] -> args[0], CD, 2) == 0) { //confronta il comando corrente con il comando "cd" per un totale di 2 char. Se il confronto è positivo, allora il comando corrente è "cd" e si esegue il controllo dell'if 
			if (l -> n_commands != 1) {
				fprintf(stderr, "cd is not the only command \n");
				return CHECK_FAILED;
			}
			if (l -> commands[0] -> in_pathname || l -> commands[0] -> out_pathname) {
				fprintf(stderr, "cd cannot have I/O redirections\n");
						return CHECK_FAILED;
			}
			if (l -> commands[0] -> n_args != 2) { //se il comando "cd" ha un numero di argomenti diverso da 2 (cioè se non ha un solo argomento, atteso: cd + argomento per un tot n_args = 2)
				fprintf(stderr, "cd must have one argument\n");
				return CHECK_FAILED;
			}
		}

	/*** TO BE DONE END ***/

	return CHECK_OK; //in tutti gli altri casi, si restituisce CHECK_OK
}

void wait_for_children()
{
	/* This function must wait for the termination of all child processes.
	 * If a child exits with an exit-status!=0, then you should print a proper message containing its PID and exit-status.
	 * Similarly, if a child is killed by a signal, then you should print a message specifying its PID, signal number and signal name.
	 */
	/*** TO BE DONE START ***/

	int status = 0; //variabile per lo stato di terminazione del processo figlio
	while(1) { //ciclo infinito per la gestione dei processi figli
		pid_t pid;
		if ((pid = wait(&status)) == -1) { //se la wait fallisce, restituisce -1
			if (errno == ECHILD) //se errno è ECHILD, non ci sono processi figli da attendere (vedi man wait)
				return; //si esce dal ciclo while perché non ci sono processi figli da attendere
			fatal_errno("error in wait"); //se la wait ha fallito ma ci sono dei p. figli ancora da attendere, allora c'è un problema e si stampa un messaggio di errore
		}
		//la wait ha avuto successo e il suo valore di ritorno (cioè il PID del p. figlio) è stato salvato nella variabile status
		if (WIFEXITED(status)) { //la macro WIFEXITED ritorna TRUE se il processo figlio è terminato correttamente
			intmax_t e_status = WEXITSTATUS(status); //la macro WEXITSTATUS restituisce il valore di uscita del processo figlio, che viene salvato nella variabile e_status
			if (e_status != 0) { //se il valore di uscita del processo figlio è diverso da 0 allora c'è stato un problema e viene stampato un messaggio di errore
				fprintf(stderr, "process with PID = %d exited with status %jd\n", pid, e_status);
			}
		}
		//se la wait ha avuto successo e il valore di uscita del p. figlio ha senso
		if (WIFSIGNALED(status)) { //la macro WIFSIGNALED ritorna TRUE se il processo figlio è stato terminato da un segnale di errore
			intmax_t sig_num = WTERMSIG(status); //la macro WTERMSIG restituisce il numero del segnale che ha terminato il processo figlio, che viene salvato nella variabile sig_num
			fprintf(stderr, "process with PID = %d changed state due to signal %jd: %s\n", pid, sig_num, strsignal(sig_num)); //stampa del messaggio di errore, per stampare il valore del numero di segnale che ha causato la terminazione del p.figlio si deve usare la funzione strsignal che traduce il numero del segnale in una stringa leggibile
		}
	}

	/*** TO BE DONE END ***/
}

//NOTA: la funzione redirect cambia il fd per la redirezione
void redirect(int from_fd, int to_fd)
{
	/* If from_fd!=NO_REDIR, then the corresponding open file should be "moved" to to_fd.
	 * That is, use dup/dup2/close to make to_fd equivalent to the original from_fd, and then close from_fd
	 */
	/*** TO BE DONE START ***/

	if (from_fd != NO_REDIR) { //se il file descriptor di i/o non è NO_REDIR
		if (dup2(from_fd, to_fd) == -1) //duplicazione del file descriptor from_fd in to_fd. Se la syscall dup2 fallisce, si stampa un messaggio di errore
			fatal_errno("error in redirect, cannot dup file descriptor");
		if (close(from_fd)) //dopo la duplicazione del file descriptor, si chiude il file descriptor from_fd. Se la syscall close fallisce, si stampa un messaggio di errore
			fatal_errno("cannot close fd in redirect");
	}

	/*** TO BE DONE END ***/
}


//NOTA: crea un processo figlio a cui viene impostata la redirezione di i/o (std) e viene eseguito il comando c
void run_child(const command_t * const c, int c_stdin, int c_stdout)
{
	/* This function must:
	 * 1) create a child process, then, in the child
	 * 2) redirect c_stdin to STDIN_FILENO (=0)
	 * 3) redirect c_stdout to STDOUT_FILENO (=1)
	 * 4) execute the command specified in c->args[0] with the corresponding arguments c->args
	 * (printing error messages in case of failure, obviously)
	 */
	/*** TO BE DONE START ***/

	pid_t pid = fork(); //creazione di un processo figlio

	/* FORK, return value
	   On success, the PID of the child process is returned in the
       parent, and 0 is returned in the child.  On failure, -1 is
       returned in the parent, no child process is created, and errno is
       set to indicate the error */

	/* Dopo la chiamata a fork(), sia il processo padre che il processo figlio continuano l'esecuzione dallo stesso punto del codice, immediatamente dopo la chiamata a fork().
	Tuttavia, non si entra direttamente nel processo figlio, perché entrambi i processi (padre e figlio) riprendono l'esecuzione nel medesimo punto del codice, ma con differenze nel valore restituito da fork().
	
	Valore restituito da fork():
	Nel processo padre, fork() restituisce il PID del processo figlio (un valore positivo).
	Nel processo figlio, fork() restituisce 0.
	Se la creazione del processo fallisce, fork() restituisce -1 (solo nel processo padre).*/

	//IL CODICE SEGUENTE VIENE UTILIZZATO SIA DAL P. PADRE CHE DAL P. FIGLIO

	//solo il padre può entrare in questo if. se il padre riceve -1, c'è stato un errore nella creazione del processo figlio. analizzando subito questo caso, si evita di fare operazioni di run per il figlio inutili
	if (pid == -1)
		fatal_errno("fail in fork");

	//a questo punto sappiamo che il p. figlio esiste ed è pronto per eseguire il codice successivo; infatti pid = 0 è un valore possibile da ottenere con la fork solo da parte del processo figlio (Pid=0 nel padre significa che il figlio è la radice dell'albero dei processi, ma questo è impossibile in quanto figlio di un processo esistente)
	if (pid == 0) {
		redirect(c_stdin, STDIN_FILENO); //redirezione in input del p. figlio
		redirect(c_stdout, STDOUT_FILENO); //redirezione in output del p. figlio
		execvp(c -> args[0], c -> args); //esecuzione del comando ( int execvp(const char *file, char *const argv[]) - dove file = nome del file eseguibile e args = array di stringhe contenente gli argomenti del comando, args[0] è il nome del comando stesso)
		fatal_errno("error in exec"); //se la syscall execvp fallisce, si stampa un messaggio di errore
	}

	/*** TO BE DONE END ***/
}

void change_current_directory(char *newdir)
{
	/* Change the current working directory to newdir
	 * (printing an appropriate error message if the syscall fails)
	 */
	/*** TO BE DONE START ***/

	if (chdir(newdir)) //cambio della directory corrente con la directory specificata come argomento del comando "cd", se la syscall ritorna 0 ha fallito
		perror("fail in cd");

	/*** TO BE DONE END ***/
}

void close_if_needed(int fd)
{
	if (fd==NO_REDIR) //se il file descriptor è NO_REDIR, la funzione termina senza fare nulla
		return; // nothing to do
	if (close(fd)) //chiude il fd (con redirezione) e se ha successo, segnala errore
		perror("close in close_if_needed");
}

void execute_line(const line_t * const l) //esecuzione della linea di comandi l
{
	//comando CD, viene valutato separatamente perché è un comando speciale che non può essere eseguito insieme ad altri (è un lupo solitario)
	if (strcmp(CD, l->commands[0]->args[0])==0) { //se il (1° e unico) comando è "cd"
		//NOTA: esistono due versioni di strcmp, questa (senza n° di char da confrontare) e l'altra con un terzo argomento che indica il numero di char da confrontare tra le 2 stinghe. Entrambe restituiscono 0 se il confronto ha successo, altrimenti restituiscono un valore diverso da 0 (in base all'errore)
		assert(l->n_commands == 1 && l->commands[0]->n_args == 2); //controllo che il comando "cd" sia l'unico comando della linea e che abbia un solo argomento (secondo controllo, era già stato usato check_cd nella execute)
		change_current_directory(l->commands[0]->args[1]); //CAMBIO DIRECTORY. cambio della directory corrente con la directory specificata come argomento del comando "cd", che è il secondo argomento del comando stesso
		return;
	}

	int next_stdin = NO_REDIR;
	for(int a=0; a<l->n_commands; ++a) { //scorrimento di tutti i comandi della linea l
		int curr_stdin = next_stdin, curr_stdout = NO_REDIR; //inizializzazione dei file-descriptor di input e output
		const command_t * const c = l->commands[a]; //assegnazione del comando corrente alla variabile c
		if (c->in_pathname) { //se il comando corrente ha un pathname di input
			assert(a == 0); //controlla che il comando corrente sia il primo della linea perché la redirezione in input è ammessa solo per il primo comando
			
			/* Open c->in_pathname and assign the file-descriptor to curr_stdin
			 * (handling error cases) */
			/*** TO BE DONE START ***/
			curr_stdin = open(c -> in_pathname, O_RDONLY); //apertura del file specificato dal pathname di input del comando corrente e salvataggio nel fd curr_stdin. curr_stdin ha il flag O_RDONLY che indica che il file è aperto in sola lettura
			if (curr_stdin == -1) { //se la syscall open fallisce
				perror("cannot open input file");
				break; //si esce dal ciclo for perché non è possibile continuare senza il file di input
			}

			/*** TO BE DONE END ***/
		}

		if (c->out_pathname) {
			
			assert(a == (l->n_commands-1)); //controlla che il comando corrente sia l'ultimo della linea perché la redirezione in output è ammessa solo per l'ultimo comando
			
			/* Open c->out_pathname and assign the file-descriptor to curr_stdout
			 * (handling error cases) */
			/*** TO BE DONE START ***/

			curr_stdout = open(c -> out_pathname, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
			/* apertura del file specificato dal pathname di output del comando corrente e salvataggio nel fd curr_stdout. curr_stdout ha i flag:
			- O_WRONLY (apertura in sola scrittura di un file già esistente)
			- O_CREAT (crea il file se non esiste)
			- O_TRUNC (se il file selezionato esiste ma non è vuoto, in questo caso lo ripristina mettendo solo 0 per tutta la sua dimensione).
			Il flag 0666 indica che il file è aperto con i permessi di lettura e scrittura per l'utente, il gruppo e gli altri
			*/

			if (curr_stdout == -1) { //se la syscall open fallisce
				perror("cannot open output file");
				close_if_needed(curr_stdin); //chiusura del file di input (solo per casi in cui ci sono fd con redirezione in input/output)
				break; //si esce dal ciclo for perché non è possibile continuare senza il file di output
			}

			/*** TO BE DONE END ***/
		} else if (a != (l->n_commands - 1)) { /* unless we're processing the last command, we need to connect the current command and the next one with a pipe */
			int fds[2]; //array di file-descriptor per la pipe

			/* Create a pipe in fds, and set FD_CLOEXEC in both file-descriptor flags */
			/*** TO BE DONE START ***/

			/*NOTE PIPE
			pipe() creates a pipe, a unidirectional data channel that can be
			used for interprocess communication.  The array fds is used to
			return two file descriptors referring to the ends of the pipe.
			fds[0] refers to the read end of the pipe.  fds[1] refers
			to the write end of the pipe.  Data written to the write end of
			the pipe is buffered by the kernel until it is read from the read
			end of the pipe.  For further details, see pipe(7).
			*/

			//la pipe viene utilizzata per mettere in comunicazione due processi figli consecutivi che eseguono comandi in una pipeline. Il primo processo scrive l'output sulla pipe e il secondo processo legge l'input dalla pipe

			if (pipe2(fds,O_CLOEXEC)) //creazione di una pipe (struttura per il flusso unidirezionale di dati utile alla comunicazione tra processi diversi, con un lato di ingresso/lettura e un lato di uscita/scrittura). L'operazione è fatta tramite il comando pipe2 che ammette la presenza di FLAG (O_CLOEXEC come da richiesta), a differenza di pipe. O_CLOEXEC è un flag che indica che i file descriptor creati dalla pipe devono essere chiusi automaticamente quando il processo figlio viene eseguito.
			  fatal_errno("fail in creating pipe");

			/*** TO BE DONE END ***/
			curr_stdout = fds[1]; //lato scrittura della pipe
			next_stdin = fds[0]; //lato lettura della pipe
		}
		run_child(c, curr_stdin, curr_stdout); //esecuzione del comando corrente (solo dei suoi processi figli)
		close_if_needed(curr_stdin); //chiusura del fd di input
		close_if_needed(curr_stdout); //chiusura del fd di output
	}
	wait_for_children();
}

void execute(char * const line)
{
	line_t * const l = parse_line(line); //analisi sintattica della linea di comandi line, puntata dal puntatore l
#ifdef DEBUG
	print_line(l); //stampa della linea di comandi l
#endif
	if (l) { //se la linea di comandi l non è nulla
		if (check_redirections(l)==CHECK_OK && check_cd(l)==CHECK_OK) //se le redirezioni e il comando "cd" sono corretti, allora procede con l'esecuzione della linea di comandi
			execute_line(l); //esecuzione della linea di comandi l
		free_line(l); //libera la memoria allocata dinamicamente per la linea l di comandi
	}
}


/*NOTA: Il ciclo for(;;) è un ciclo infinito che continua a eseguire finché non viene interrotto esplicitamente da una delle seguenti situazioni:
	- la linea di comando è nulla
	- la funzione getcwd fallisce
*/

int main()
{
	const char * const prompt_suffix = " $ ";
	const size_t prompt_suffix_len = strlen(prompt_suffix);
	for(;;) { //ciclo infinito
		char *pwd;
		/* Make pwd point to a string containing the current working directory.
		 * The memory area must be allocated (directly or indirectly) via malloc.
		 */
		/*** TO BE DONE START ***/

		/* 
		NOTE SU GETCWD 
		*char *getcwd(char *buf, size_t size);*
		//buf = NULL: la funzione deve allocare dinamicamente un buffer sufficientemente grande per contenere il percorso corrente
		//size = 0: dal 2001, la funzione alloca un buffer di dimensione sufficiente grande per contenere il percorso corrente
		//return value: se ha successo, getcwd restituisce un puntatore ad un buffer contenente il pathname della current working directory. Se fallisce, restituisce NULL
		*/

		pwd = getcwd(NULL, 0);
		
		if (pwd == NULL)
			fatal_errno("cannot get current working directory");

		/*** TO BE DONE END ***/
		pwd = my_realloc(pwd, strlen(pwd) + prompt_suffix_len + 1);
		strcat(pwd, prompt_suffix);
#ifdef NO_READLINE
		const int max_line_size = 512; //max_line_size = 512 bytes è un ragionevole compromesso flessibilità e sicurezza (limitare la dimensione dell'input aiuta a prevenire buffer overflow, che possono causare comportamenti imprevisti o vulnerabilità di sicurezza)
		char * line = my_malloc(max_line_size); //allocazione della memoria per la stringa line di dimensione max_line_size
		printf("%s", pwd); //stampa del prompt di microbash
		if (!fgets(line, max_line_size, stdin)) { //lettura di tutti i caratteri dello stream std_in
		// NOTA SU FDGETS() - fgets() legge al massimo un numero di caratteri pari a (max_line_size - 1) dallo stream stdin e li memorizza nel buffer puntato da line. La lettura si interrompe quando viene rilevato un EOF o un carattere di nuova linea (newline). Se viene letto un carattere di nuova linea, esso viene memorizzato nel buffer. Un byte nullo di terminazione (\0) viene memorizzato dopo l'ultimo carattere nel buffer

		//se la lettura dello stdin fallisce...
			free(line); //deallocazione della memoria allocata per la stringa line
			line = 0; //deallocazione del puntatore line al buffer
			putchar('\n'); //scrive il singolo carattere newline sullo std output
		} else { //se la lettura dello stdin ha successo...
			size_t l = strlen(line); //salva in l la lunghezza della stringa line
			if (l && line[--l]=='\n') //se la stringa line non è vuota e l'ultimo carattere è un newline, allora la stringa è stata presa correttamente e inviata al buffer di microbash in modo corretto
				line[l] = 0; //affinché la stringa line sia compatibile con i metodi di microbash, l'ultimo carattere ad essere inserito nel buffer deve essere un byte nullo di terminazione (=NULL=0)
		}
#else
		char * const line = readline(pwd); //microbash legge e interpreta la stringa di comando ricevuta da stdin e precedentemente controllata
#endif
		free(pwd); //deallocazione della memoria allocata per il prompt del terminale
		if (!line) break; //se la stringa line è nulla, il programma termina la sua esecuzione (break esce dal ciclo infinito e fa un return/exit)
		execute(line); //esecuzione della stringa line
		free(line); //deallocazione della memoria allocata per la stringa line
	}
}