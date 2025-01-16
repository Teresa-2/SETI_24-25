#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void signal_handler(int sig) {
    printf("Process with PID = %d changed state due to signal %d: Terminated\n", getpid(), sig);
    _exit(0);
}

int main() {
    signal(SIGTERM, signal_handler); // Cattura il segnale SIGTERM
    signal(SIGINT, signal_handler); // Cattura il segnale SIGINT (Ctrl+C)
    printf("Process with PID = %d is running in an infinite loop.\n", getpid());
    while (1) {
        sleep(1); // Mantiene il processo in esecuzione consumando meno risorse
    }
    return 0;
}