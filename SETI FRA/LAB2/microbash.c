//#error Please read the accompanying microbash.pdf before hacking this source code (and removing this line).
/*
 * Micro-bash v2.2
 *
 * Programma sviluppato a supporto del laboratorio di Sistemi di
 * Elaborazione e Trasmissione dell'Informazione del corso di laurea
 * in Informatica presso l'Università degli Studi di Genova, a.a. 2022/2023.
 *
 * Copyright (C) 2020-2022 by Giovanni Lagorio <giovanni.lagorio@unige.it>
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
#include <readline/readline.h>
#include <readline/history.h>
#include <stdint.h>

/***Piccolo accorgimento per evitare di cospargere il codice di #ifdef #endif ***/
#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

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

void *my_realloc(void *ptr, size_t size)
{
	void *rv = realloc(ptr, size);
	if (!rv)
		fatal_errno("my_realloc");
	return rv;
}

char *my_strdup(char *ptr) //crea una copia di una stringa, allocando la memoria necessaria e restituendo un puntatore alla nuova stringa.
{
	debug("my_strdup, myptr: %s\n", ptr);
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
	int n_args; // number of arguments (including the command itself)
	char **args; // in an execv*-compatible format; i.e., args[n_args]=0 
	char *out_pathname; // 0 if no output-redirection is present
	char *in_pathname; // 0 if no input-redirection is present
} command_t;

typedef struct {
	int n_commands;	// number of commands in the line
	command_t **commands; 		// array of commands
} line_t;

void free_command(command_t * const c)
{
	assert(c==0 || c->n_args==0 || (c->n_args > 0 && c->args[c->n_args] == 0)); /* sanity-check: if c is not null, then it is either empty (in case of parsing error) or its args are properly NULL-terminated */
	/*** TO BE DONE START ***/
	if(c) //se c è diverso da NULL
	{
		for(int i=0; i<c->n_args; i++) //libero la memoria allocata per ogni argomento
			free(c->args[i]);
		free(c->args);					//libero la memoria allocata per l'array di argomenti
		free(c->in_pathname);			//libero la memoria allocata per il pathname di input
		free(c->out_pathname);			//libero la memoria allocata per il pathname di output
		free(c); 						//libero la memoria allocata per la struttura command_t
	}
	/*** TO BE DONE END ***/
}

void free_line(line_t * const l)
{
	assert(l==0 || l->n_commands>=0); /* sanity-check */
	/*** TO BE DONE START ***/
	 if (l) {
        for (int i = 0; i < l->n_commands; i++) {
            	free_command(l->commands[i]);            
        }
        free(l->commands);
        free(l);
    }
	/*** TO BE DONE END ***/
}

#ifdef DEBUG
void print_command(const command_t * const c)
{
	if (!c) {
		printf("Command == NULL\n");
		return;
	}
	printf("[ ");
	for(int a=0; a<c->n_args; ++a)
		printf("%s ", c->args[a]);
	assert(c->args[c->n_args] == 0);
	printf("] ");
	printf("in: %s out: %s\n", c->in_pathname, c->out_pathname);
}

void print_line(const line_t * const l)
{
	if (!l) {
		printf("Line == NULL\n");
		return;
	}
	printf("Line has %d command(s):\n", l->n_commands);
	for(int a=0; a<l->n_commands; ++a)
		print_command(l->commands[a]);
}
#endif

command_t *parse_cmd(char * const cmdstr)
{
	static const char *const BLANKS = " \t";
	command_t * const result = my_malloc(sizeof(*result));
	memset(result, 0, sizeof(*result));
	char *saveptr, *tmp;
	tmp = strtok_r(cmdstr, BLANKS, &saveptr); //strtok_r divide la stringa cmdstr in sottostringhe, utilizzando come delimitatori i caratteri contenuti in BLANKS
	while (tmp) {							//e restituisce un puntatore alla prima sottostringa trovata
		result->args = my_realloc(result->args, (result->n_args + 2)*sizeof(char *));//alloco la memoria necessaria per contenere un nuovo argomento
		if (*tmp=='<') {				//se il primo carattere di tmp è '<'
			if (result->in_pathname) {	//se in_pathname è diverso da NULL
				fprintf(stderr, "Parsing error: cannot have more than one input redirection\n");
				goto fail;
			}
			if (!tmp[1]) {              //se il secondo carattere di tmp è NULL
				fprintf(stderr, "Parsing error: no path specified for input redirection\n");
				goto fail;
			}
			result->in_pathname = my_strdup(tmp+1); //alloco la memoria necessaria per contenere il pathname di input
		} else if (*tmp == '>') {     //se il primo carattere di tmp è '>'
			if (result->out_pathname) { //se out_pathname è diverso da NULL
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
				debug("tmp: %s\n", tmp);
				tmp = getenv(tmp+1);	//getenv restituisce il puntatore alla variabile d'ambiente specificata come argomento
				debug("val: %s\n", tmp);
				if (tmp == NULL){
					tmp = "";	//se la variabile d'ambiente non è definita, tmp punta alla stringa vuota"";
				}	
				/*** TO BE DONE END ***/
			}
			result->args[result->n_args++] = my_strdup(tmp);
			result->args[result->n_args] = 0;
		}
		tmp = strtok_r(0, BLANKS, &saveptr);
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
	for (int i = 0; i < l->n_commands; i++) {
		if (strcmp(l->commands[i]->args[0], CD) != 0) {
			if ((i != 0 && l->commands[i]->in_pathname != NULL) || 
				(i != l->n_commands - 1 && l->commands[i]->out_pathname != NULL)) {
					printf("Error: Only the first command can have input-redirection and only the last command can have output-redirection.\n");
					return CHECK_FAILED;
			}
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
	debug("quanti comandi ha cd? %d\n", l->n_commands);
	debug("quanti argomenti ha cd? %d\n", l->commands[0]->n_args);
	debug("primo argomento? %s\n", l->commands[0]->args[0]);
	debug("secondo argomento? %s\n", l->commands[0]->args[1]);


	int syntax_error = 0;	
	for (int i = 0; i < l->n_commands; i++) {
        if (strcmp(l->commands[i]->args[0], CD) == 0 ) {
            if (l->n_commands > 1 ) {
                printf("Error: 'cd' must be the only command of the line\n");
                syntax_error = 1;
			}else if (l->commands[i]->in_pathname != NULL || l->commands[i]->out_pathname != NULL) {
				printf("Error: 'cd'  cannot have I/O redirections\n");
				syntax_error = 1;
            } else if (l->commands[i]->n_args != 2 ) {
				printf("Error: 'cd' must have only one argument.\n");
				syntax_error = 1;
			}
			if (syntax_error == 1) {
				return CHECK_FAILED;
			}
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
	int status;
    pid_t pid;

    // Wait for any child process.
    // If there is no child process, wait() returns -1 and we break the loop.
    while ((pid = wait(&status)) != -1){
        if (WIFEXITED(status)){ // Check if the child process ended normally
            printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status)){ // Check if the child process was terminated by a signal
            printf("Child process %d was terminated by signal %d: %s\n", pid, WTERMSIG(status), strsignal(WTERMSIG(status)));
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
	if (from_fd != NO_REDIR && from_fd != to_fd)
	{
		int dup2_fd = dup2(from_fd, to_fd);
		if (dup2_fd == -1 || dup2_fd != to_fd)
			fatal_errno("dup2 in redirect");
		if (close(from_fd) == -1)
			fatal_errno("close in redirect");
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
	int pid = fork();
	if (pid == -1)
		fatal_errno("fork");
	if (pid == 0)
	{
		redirect(c_stdin, STDIN_FILENO);
		redirect(c_stdout, STDOUT_FILENO);
		if (execvp(c->args[0], c->args) == NO_REDIR)
			fatal_errno("execvp");
		//exit(0);
	} else {
		if (pid != waitpid(pid, NULL, 0))
			fatal_errno("waitpid");
		//wait_for_children();
	}

	/*** TO BE DONE END ***/
}

void change_current_directory(char *newdir)
{
	/* Change the current working directory to newdir
	 * (printing an appropriate error message if the syscall fails)
	 */
	/*** TO BE DONE START ***/
	if (chdir(newdir) == -1)
		perror("error chdir");
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
			curr_stdin = open(c->in_pathname, O_RDONLY);
			if (curr_stdin == NO_REDIR)
				fatal_errno("open curr_stdin");
			/*** TO BE DONE END ***/
		}
		if (c->out_pathname) {
			assert(a == (l->n_commands-1));
			/* Open c->out_pathname and assign the file-descriptor to curr_stdout
			 * (handling error cases) */
			/*** TO BE DONE START ***/
			curr_stdout = open(c->out_pathname, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if (curr_stdout == NO_REDIR)
				fatal_errno("open curr_stdout");
			/*** TO BE DONE END ***/
		} else if (a != (l->n_commands - 1)) { /* unless we're processing the last command, we need to connect the current command and the next one with a pipe */
			int fds[2];
			/* Create a pipe in fds, and set FD_CLOEXEC in both file-descriptor flags */
			/*** TO BE DONE START ***/
			if (pipe(fds) == -1)
				fatal_errno("pipe");
			if (fcntl(fds[0], F_SETFD, FD_CLOEXEC) == -1 || fcntl(fds[1], F_SETFD, FD_CLOEXEC) == -1)
				fatal_errno("fcntl");
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
		const size_t MAX_LEN = 1024;
		char *cwd;
		cwd = my_malloc(MAX_LEN);
		if ((getcwd(cwd, MAX_LEN)) == NULL)
			fatal_errno("getcwd");
		pwd = my_strdup(cwd);
		free(cwd);

		/*** TO BE DONE END ***/
		pwd = my_realloc(pwd, strlen(pwd) + prompt_suffix_len + 1);
		strcat(pwd, prompt_suffix);
		char * const line = readline(pwd);
		free(pwd);
		if (!line) break;
		execute(line);
		free(line);
	}
}

