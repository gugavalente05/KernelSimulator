#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_ITERACOES 50
#define SYSCALL_FIFO "FifoSystemCall"

void processo_aplicacao() {

    pid_t pid = getpid();
    char systemcall[50]; 
    char dispositivo[3];
    char tipo[2];
    int fpFifo;

    int valorsystemcall;
    int valorDispositivo;
    int valortipo;

    srand((time(NULL) ^ pid));
    
    if ((fpFifo = open(SYSCALL_FIFO, O_WRONLY)) < 0) {
        perror("openFifoSYS");
        exit(1);
    }


    for (int count = 0; count < MAX_ITERACOES; count++) {

        usleep(500000);

        if ((count % 10) == 0) {
            printf("AP(pid=%d) : PC = %d\n", pid, count);
        }
        
        valorsystemcall = rand() % 100;

        if (valorsystemcall < 15) {

            valorDispositivo = rand() % 2;
            if (valorDispositivo == 0) {
                strcpy(dispositivo, "D1");
            } else {
                strcpy(dispositivo, "D2");
            }

            valortipo = rand() % 3;
            if (valortipo == 0) {
                strcpy(tipo, "R");
            } else if (valortipo == 1) {
                strcpy(tipo, "W");
            } else {
                strcpy(tipo, "X");
            }

            sprintf(systemcall, "%d;%s;%s", pid, dispositivo, tipo);
            write(fpFifo, systemcall, strlen(systemcall) + 1);
            printf("--- SYSCALL GERADA: %s ---\n", systemcall);
        }

        usleep(500000); 
    }
    
    printf("Processo (PID: %d) terminou.\n", pid);
    close(fpFifo);  
}

