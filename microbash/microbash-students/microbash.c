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
	/*for (int i = c -> n_args - 1; i >= 0; --i) {
		free(c -> args[i]);
	}*/

	//più intuitivo
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

	/*for (int i = l -> n_commands - 1; i >= 0; --i)
		free_command(l -> commands[i]);*/
	
	//più intuitivo
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
	while (tmp) { //finché ci sono token da leggere, per ogni token letto si esegue una analisi sintattica 
		result->args = my_realloc(result->args, (result->n_args + 2)*sizeof(char *)); //vengono allocate due nuove celle nell'array di argomenti del comando (result->args) per contenere 1) il token appena letto 2) il terminatore NULL che segnerà la fine dell'iterazione in atto per il parsing del comando
		if (*tmp=='<') { //se c'è redirezione in input
			if (result->in_pathname) {
				fprintf(stderr, "Parsing error: cannot have more than one input redirection\n");
				goto fail;
			}
			if (!tmp[1]) {
				fprintf(stderr, "Parsing error: no path specified for input redirection\n");
				goto fail;
			}
			result->in_pathname = my_strdup(tmp+1);
		} else if (*tmp == '>') {
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
			if (*tmp=='$') {
				/* Make tmp point to the value of the corresponding environment variable, if any, or the empty string otherwise */
				/*** TO BE DONE START ***/

				if (!tmp[1]) {
					fprintf(stderr, "Parsing error: no variable name specified\n");
					goto fail;
				}
				if (!(tmp = getenv(tmp + 1)))
					tmp = "";

				/*** TO BE DONE END ***/
			}
			result->args[result->n_args++] = my_strdup(tmp);
			result->args[result->n_args] = 0;
		}
		tmp = strtok_r(0, BLANKS, &saveptr); //lettura del token successivo. dalla seconda iterazione in poi, strtok_r leggerà il token successivo a quello letto nella chiamata precedente di strtok_r e la stringa passata come primo argomento sarà SEMPRE NULL (per definizione metodo da man)
	}
	if (result->n_args)
		return result;
	fprintf(stderr, "Parsing error: empty command\n");
fail:
	free_command(result);
	return 0;
}

line_t *parse_line(char * const line)
{
	static const char * const PIPE = "|";
	char *cmd, *saveptr;
	cmd = strtok_r(line, PIPE, &saveptr);
	if (!cmd)
		return 0;
	line_t *result = my_malloc(sizeof(*result));
	memset(result, 0, sizeof(*result));
	while (cmd) {
		command_t * const c = parse_cmd(cmd);
		if (!c) {
			free_line(result);
			return 0;
		}
		result->commands = my_realloc(result->commands, (result->n_commands + 1)*sizeof(command_t *));
		result->commands[result->n_commands++] = c;
		cmd = strtok_r(0, PIPE, &saveptr);
	}
	return result;
}

check_t check_redirections(const line_t * const l)
{
	assert(l);
	/* This function must check that:
	 * - Only the first command of a line can have input-redirection
	 * - Only the last command of a line can have output-redirection
	 * and return CHECK_OK if everything is ok, print a proper error
	 * message and return CHECK_FAILED otherwise
	 */
	/*** TO BE DONE START ***/

	if (l -> n_commands != 1)
		for (int i = 0; i < l -> n_commands; ++i) {
			if (i != 0)
				if (l -> commands[i] -> in_pathname) {
					fprintf(stderr, "Input redirection in a non-first command\n");
					return CHECK_FAILED; }
			if (i != l -> n_commands - 1)
				if (l -> commands[i] -> out_pathname) {
					fprintf(stderr, "Output redirection in a non-last command\n");
					return CHECK_FAILED;
				}
		}

	/*** TO BE DONE END ***/
	return CHECK_OK;
}

check_t check_cd(const line_t * const l)
{
	assert(l);
	/* This function must check that if command "cd" is present in l, then such a command
	 * 1) must be the only command of the line
	 * 2) cannot have I/O redirections
	 * 3) must have only one argument
	 * and return CHECK_OK if everything is ok, print a proper error
	 * message and return CHECK_FAILED otherwise
	 */
	/*** TO BE DONE START ***/

	for (int i = 0; i < l -> n_commands; ++i)
		if (strncmp(l -> commands[i] -> args[0], CD, 2) == 0) {
			if (l -> n_commands != 1) {
				fprintf(stderr, "cd is not the only command\n");
				return CHECK_FAILED;
			}
			if (l -> commands[0] -> in_pathname || l -> commands[0] -> out_pathname) {
				fprintf(stderr, "cd cannot have I/O redirections\n");
						return CHECK_FAILED;
			}
			if (l -> commands[0] -> n_args != 2) {
				fprintf(stderr, "cd must have one argument\n");
				return CHECK_FAILED;
			}
		}

	/*** TO BE DONE END ***/
	return CHECK_OK;
}

void wait_for_children()
{
	/* This function must wait for the termination of all child processes.
	 * If a child exits with an exit-status!=0, then you should print a proper message containing its PID and exit-status.
	 * Similarly, if a child is killed by a signal, then you should print a message specifying its PID, signal number and signal name.
	 */
	/*** TO BE DONE START ***/

	int status = 0;
	while(1) {
		pid_t pid;
		if ((pid = wait(&status)) == -1) {
			if (errno == ECHILD)
				return;
			fatal_errno("error in wait");
		}
		if (WIFEXITED(status)) {
			intmax_t e_status = WEXITSTATUS(status);
			if (e_status != 0) {
				fprintf(stderr, "process with PID = %d exited with status %jd\n", pid, e_status);
			}
		}
		if (WIFSIGNALED(status)) {
			intmax_t sig_num = WTERMSIG(status);
			fprintf(stderr, "process with PID = %d changed state due to signal %jd: %s\n", pid, sig_num, strsignal(sig_num));
		}
	}

	/*** TO BE DONE END ***/
}

void redirect(int from_fd, int to_fd)
{
	/* If from_fd!=NO_REDIR, then the corresponding open file should be "moved" to to_fd.
	 * That is, use dup/dup2/close to make to_fd equivalent to the original from_fd, and then close from_fd
	 */
	/*** TO BE DONE START ***/

	if (from_fd != NO_REDIR) {
		if (dup2(from_fd, to_fd) == -1)
			fatal_errno("error in redirect, cannot dup file descriptor");
		if (close(from_fd))
			fatal_errno("cannot close fd in redirect");
	}

	/*** TO BE DONE END ***/
}

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

	pid_t pid = fork();

	if (pid == -1) 
		fatal_errno("fail in fork");

	if (pid == 0) {
		redirect(c_stdin, STDIN_FILENO);
		redirect(c_stdout, STDOUT_FILENO);
		execvp(c -> args[0], c -> args);
		fatal_errno("error in exec");
	}

	/*** TO BE DONE END ***/
}

void change_current_directory(char *newdir)
{
	/* Change the current working directory to newdir
	 * (printing an appropriate error message if the syscall fails)
	 */
	/*** TO BE DONE START ***/

	if (chdir(newdir))
		perror("fail in cd");

	/*** TO BE DONE END ***/
}

void close_if_needed(int fd)
{
	if (fd==NO_REDIR)
		return; // nothing to do
	if (close(fd))
		perror("close in close_if_needed");
}

void execute_line(const line_t * const l)
{
	if (strcmp(CD, l->commands[0]->args[0])==0) {
		assert(l->n_commands == 1 && l->commands[0]->n_args == 2);
		change_current_directory(l->commands[0]->args[1]);
		return;
	}
	int next_stdin = NO_REDIR;
	for(int a=0; a<l->n_commands; ++a) {
		int curr_stdin = next_stdin, curr_stdout = NO_REDIR;
		const command_t * const c = l->commands[a];
		if (c->in_pathname) {
			assert(a == 0);
			/* Open c->in_pathname and assign the file-descriptor to curr_stdin
			 * (handling error cases) */
			/*** TO BE DONE START ***/

			if ((curr_stdin = open(c -> in_pathname, O_RDONLY)) == -1) {
				perror("cannot open input file");
				break;
			}

			/*** TO BE DONE END ***/
		}
		if (c->out_pathname) {
			assert(a == (l->n_commands-1));
			/* Open c->out_pathname and assign the file-descriptor to curr_stdout
			 * (handling error cases) */
			/*** TO BE DONE START ***/

			if ((curr_stdout = open(c -> out_pathname, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
				perror("cannot open output file");
				close_if_needed(curr_stdin);
				break;
			}

			/*** TO BE DONE END ***/
		} else if (a != (l->n_commands - 1)) { /* unless we're processing the last command, we need to connect the current command and the next one with a pipe */
			int fds[2];
			/* Create a pipe in fds, and set FD_CLOEXEC in both file-descriptor flags */
			/*** TO BE DONE START ***/

			if (pipe2(fds,O_CLOEXEC))
			  fatal_errno("fail in creating pipe");

			/*** TO BE DONE END ***/
			curr_stdout = fds[1];
			next_stdin = fds[0];
		}
		run_child(c, curr_stdin, curr_stdout);
		close_if_needed(curr_stdin);
		close_if_needed(curr_stdout);
	}
	wait_for_children();
}

void execute(char * const line)
{
	line_t * const l = parse_line(line);
#ifdef DEBUG
	print_line(l);
#endif
	if (l) {
		if (check_redirections(l)==CHECK_OK && check_cd(l)==CHECK_OK)
			execute_line(l);
		free_line(l);
	}
}

int main()
{
	const char * const prompt_suffix = " $ ";
	const size_t prompt_suffix_len = strlen(prompt_suffix);
	for(;;) {
		char *pwd;
		/* Make pwd point to a string containing the current working directory.
		 * The memory area must be allocated (directly or indirectly) via malloc.
		 */
		/*** TO BE DONE START ***/

		if ((pwd = getcwd(NULL, 0)) == NULL)
			fatal_errno("can't get current directory");

		/*** TO BE DONE END ***/
		pwd = my_realloc(pwd, strlen(pwd) + prompt_suffix_len + 1);
		strcat(pwd, prompt_suffix);
#ifdef NO_READLINE
		const int max_line_size = 512;
		char * line = my_malloc(max_line_size);
		printf("%s", pwd);
		if (!fgets(line, max_line_size, stdin)) {
			free(line);
			line = 0;
			putchar('\n');
		} else {
			size_t l = strlen(line);
			if (l && line[--l]=='\n')
				line[l] = 0;
		}
#else
		char * const line = readline(pwd);
#endif
		free(pwd);
		if (!line) break;
		execute(line);
		free(line);
	}
}

