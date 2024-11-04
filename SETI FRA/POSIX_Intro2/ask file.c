/*2. Chiede all’utente il nome di un file, usando per esempio fgets(3), ed esegue /bin/nome-inserito-dall’utente
    • Attenzione: non deve cercare in tutto il PATH
    • Usate valgrind/sanitizer per controllare l’uso della memoria

– In generale, abituatevi a testare i vostri programmi con input “strani” a piacere; per esempio,
stringa vuota, stringhe contenenti caratteri non stampabili, stringhe lunghissime, . . .
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAXLEN 256

int main(){

    char filename[MAXLEN];
    char *args[2];
    pid_t pid;
    int status;

    printf("Insert filename: ");
    if(fgets(filename, MAXLEN, stdin) == NULL){ // fgets() legge al massimo MAXLEN caratteri da stdin e li salva in filename
        perror("fgets");
        exit(EXIT_FAILURE);
    }

    filename[strlen(filename) - 1] = '\0'; // fgets() inserisce anche il carattere di newline, che va eliminato

    if(strlen(filename) == 0) { // se la stringa è vuota, termina il programma
    fprintf(stderr, "Error: Filename is empty\n");
    exit(EXIT_FAILURE);
}

    if(access(filename, F_OK | X_OK) == -1) { // access() controlla se il file
        fprintf(stderr, "Error: File '%s' does not exist or isn't executable\n", filename);
        exit(EXIT_FAILURE);
    }

    args[0] = filename;
    args[1] = NULL;

    if((pid = fork()) < 0){ // fork() crea un processo figlio
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if(pid == 0){ // processo figlio
        execv(filename, args); // execv() esegue il comando passato come primo argomento con gli argomenti successivi (NULL termina la lista)
        perror("execv");
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


