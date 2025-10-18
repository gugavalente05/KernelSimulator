#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_ITERACOES 50
#define SYSCALL_FIFO "FifoSytemCall"

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
        perror("Erro ao abrir Fifo");
        exit(1);
    }

    printf("Processo (PID: %d) iniciado.\n", pid);

    for (int count = 0; count < MAX_ITERACOES; count++) {
        printf("Processo (PID: %d): PC = %d\n", pid, count);
        
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

        sleep(1); 
    }
    
    printf("Processo (PID: %d) terminou.\n", pid);
    close(fpFifo);  
}

int main(void) {
    if (access(SYSCALL_FIFO, F_OK) == -1) {
        if (mkfifo(SYSCALL_FIFO, 0666) != 0) {
            perror("Erro ao criar FIFO");
            return 1;
        }
        printf("FIFO '%s' criado.\n", SYSCALL_FIFO);
    }

    processo_aplicacao();
    return 0;
}

