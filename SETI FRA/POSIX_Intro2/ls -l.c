/*Esegue il comando ls -l, attraverso l’uso di fork(2) ed exec(3). Notate che exec(3) documenta una
famiglia di funzioni e scegliete quella che vi sembra pi`u comoda per il vostro caso.
    • Serve usare wait(2) in questo caso? Perch´e?
    • S`ı, lo sappiamo che esiste system(3); no, non potete usarla. 
        In compenso, re-implementarsi system potrebbe essere un utile esercizio.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(){ 

    pid_t pid;
    int status;

    if((pid = fork()) < 0){ // fork() crea un processo figlio
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if(pid == 0){ // processo figlio
        execlp("ls", "ls",  "-l", NULL); // execlp() esegue il comando passato come primo argomento con gli argomenti successivi (NULL termina la lista)
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    if(wait(&status) < 0){  // wait() attende la terminazione del processo figlio
        perror("wait");
        exit(EXIT_FAILURE);
    }

    if(WIFEXITED(status)){  // WIFEXITED() ritorna true se il processo figlio è terminato normalmente
        printf("Child process exited with status %d\n", WEXITSTATUS(status)); // WEXITSTATUS() ritorna lo status del processo figlio
    }

    return 0;
}