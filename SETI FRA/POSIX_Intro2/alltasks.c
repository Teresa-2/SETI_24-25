#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#define MAXCMD 256

void my_exec(char *cmd)
{
    char cmd_copy[MAXCMD];
     if ( snprintf(cmd_copy, sizeof(cmd_copy), "%s", cmd) < 0) { // snprintf() scrive la stringa formattata in cmd_copy
        perror("snprintf");
        exit(EXIT_FAILURE);
    }
    char *args[MAXCMD];// Define an array of pointers to hold the command and its arguments
    int i = 0;
    // Use strtok to split the command string into separate words
    args[i] = strtok(cmd_copy, " ");     // The first call to strtok should pass the command string and the delimiter (" ")
    while(args[i] != NULL) { // Continue calling strtok with a null pointer as the first argument
        args[++i] = strtok(NULL, " "); //to get the rest of the words in the string
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }else if(pid == 0){
        if ( execvp(args[0], args) == -1 ){ // execvp() ritorna solo se c'è un errore
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else wait(NULL);

    /*if(wait(&status) < 0){  // wait() attende la terminazione del processo figlio
        perror("wait");
        exit(EXIT_FAILURE);
    }
    if(WIFEXITED(status)){  // WIFEXITED() ritorna true se il processo figlio è terminato normalmente
        printf("Child process exited with status %d\n", WEXITSTATUS(status)); // WEXITSTATUS() ritorna lo status del processo figlio
    }*/
}

int main(int argc, char *argv[]){

    printf ("%d\n", argc);
    printf ("%s\n", argv[1]);

    char cmd[MAXCMD];
    
    /*TASK1
    Esegue il comando ls -l, attraverso l’uso di fork(2) ed exec(3). Notate che exec(3) documenta una
    famiglia di funzioni e scegliete quella che vi sembra pi`u comoda per il vostro caso.
    • Serve usare wait(2) in questo caso? Perch´e?
    • S`ı, lo sappiamo che esiste system(3); no, non potete usarla. In compenso, re-implementarsi system potrebbe essere un utile esercizio.*/
    printf ("TASK1\n");
    my_exec("ls -l");

    /*TASK2
    Chiede all’utente il nome di un file, usando per esempio fgets(3), 
    ed esegue /bin /nome-inserito-dall’utente
    • Attenzione: non deve cercare in tutto il PATH
    • Usate valgrind/sanitizer per controllare l’uso della memoria*/
    printf ("TASK2\n");

    printf("Insert filename: ");
    if(fgets(cmd, MAXCMD, stdin) == NULL){ // fgets() legge al massimo MAXLEN caratteri da stdin e li salva in cmd
        perror("fgets");
        exit(EXIT_FAILURE);
    }

    cmd[strlen(cmd) - 1] = '\0'; // fgets() inserisce anche il carattere di newline, che va eliminato

    if(strlen(cmd) == 0) { // se la stringa è vuota, invia un messaggio
        fprintf(stderr, "Error: Filename is empty\n");
        //exit(EXIT_FAILURE);
    }

    char cmd_with_path[MAXCMD];
    if ( snprintf(cmd_with_path, sizeof(cmd_with_path), "/bin/%s", cmd) < 0) { // snprintf() scrive la stringa formattata in cmd_with_path
        perror("snprintf");
        exit(EXIT_FAILURE);
    }

    if(access(cmd_with_path, F_OK | X_OK) == -1) { // access() controlla se il file
        fprintf(stderr, "Error: File '%s' does not exist or isn't executable\n", cmd_with_path);
        //exit(EXIT_FAILURE);
    }else my_exec (cmd_with_path);
    
    /*TASK3
    (a) stampa un prompt (per esempio, la stringa "nano-shell $") sullo standard-output
    (b) chiede all’utente il nome di un file
    (c) se l’utente inserisce exit o EOF (premendo ctrl-D all’inizio di una nuova linea), esce con exit status EXIT_SUCCESS
    (d) esegue /bin/nome-inserito-dall’utente, dando un appropriato messaggio di errore se l’esecuzione fallisce. Per stampare il messaggio di errore, vedete perror(3)
    (e) aspetta la terminazione del processo figlio, vedete wait(2)
        • cosa succede se non utilizzate wait?

    Possibile migliorie: potrebbe cercare in tutto il PATH*/
    printf ("TASK3\n");

    while(1){
        printf ("nano-shell $\n");
        printf("Insert filename: ");
        if(fgets(cmd, MAXCMD, stdin) == NULL){ // fgets() legge al massimo MAXLEN caratteri da stdin e li salva in filename
            perror("fgets");
            exit(EXIT_FAILURE);
        }

         if (strcmp(cmd, "exit\n") == 0 || feof(stdin)) {
            //exit(EXIT_SUCCESS);
            break;
        }
        cmd[strlen(cmd) - 1] = '\0'; // fgets() inserisce anche il carattere di newline, che va eliminato
        my_exec(cmd); 
    }
    

    /*TASK4
    Esegue il comando ls -l > filename; ovvero esegue il comando ls con argomento -l redirezionando il suo
    standard-output nel file di nome filename, che riceverete come primo argomento dalla linea di comando
    (ovvero, argv[1]); oltre alle system-call gi`a usate per gli esercizi precedenti, vi serviranno close(2) e
    dup(2), oppure dup2(2).*/
    printf ("TASK4\n");
    printf ("Opening %s\n", argv[1]);
    int stdout_fd = dup(STDOUT_FILENO); // Save the file descriptor of stdout
    int fd = open (argv[1], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(fd == -1){
        perror("Open failed");
        exit(EXIT_FAILURE);
    }
    if(dup2(fd, STDOUT_FILENO) == -1){
        perror("dup2");
        exit(EXIT_FAILURE);
    }
    if(close(fd) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }
    my_exec("ls -l");
    
    if(dup2(stdout_fd, STDOUT_FILENO) == -1){ // Restore stdout
        perror("dup2");
        exit(EXIT_FAILURE);
    }
    


    /*TASK5
    Esegue il comando ps aux | grep bash, usando, oltre a quella gi`a usate precedentemente, la system-call
    pipe(2)*/
    printf ("TASK5\n");
    int pipefd[2]; // pipefd[0] è la read end, pipefd[1] è la write end
    pid_t pid;

    if (pipe(pipefd) == -1) { // crea una pipe
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {    /* Child reads from pipe */
       if( close(pipefd[1]) == -1){ // Close unused write end
            perror("Close unused write end\n");
            exit(EXIT_FAILURE);
        }

        /* Duplicate stdin on pipefd[0] and close it */
        if(dup2(pipefd[0], STDIN_FILENO) == -1){
            perror("dup2 stdin on pipefd[0]\n");
            exit(EXIT_FAILURE);
        }
        if ( close(pipefd[0]) == -1){
            perror("close pipefd[0]\n");
            exit(EXIT_FAILURE);
        }

        /* Replace the child process with the grep bash process */
        if (execlp("grep", "grep", "bash", NULL) == -1){ // execlp() ritorna solo se c'è un errore
            perror("execlp grep");
            exit(EXIT_FAILURE);
        } 
    } else {           /* Parent writes argv[1] to pipe */  
        if (close(pipefd[0]) == -1){  /* Close unused read end */
            perror("close");
            exit(EXIT_FAILURE);
        }

        /* Duplicate stdout on pipefd[1] and close it */
        if (dup2(pipefd[1], STDOUT_FILENO) == -1){
            perror("dup2 stdout on pipefd[1]\n");
            exit(EXIT_FAILURE);
        }
        if (close(pipefd[1]) == -1){
            perror("close pipefd[1]\n");
            exit(EXIT_FAILURE);
        }
        /* Replace the parent process with the ps aux process */
        if (execlp("ps", "ps", "aux", NULL) == -1){ // execlp() ritorna solo se c'è un errore
            perror("execlp ps");  
            exit(EXIT_FAILURE);
        }
        printf("This will not be printed\n"); // This will not be printed because the process is replaced by the ps aux process
        wait(NULL); // Wait for the child process to terminate 
    }
    return 0;
}