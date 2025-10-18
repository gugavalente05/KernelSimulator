#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h> 
#define FIFO "FifoIPC"
#define IRQ0 "0"
#define IRQ1 "1"
#define IRQ2 "2"
void inter_controller_sim(void) {
    int fpFifo;
    int valor;
    char interrupcao[2];

    srand(time(NULL));

    if ((fpFifo = open(FIFO, O_WRONLY)) < 0) {
        perror("Erro ao abrir FIFO");
        exit(1);
    }
    
    printf("InterControllerSim iniciado. Enviando interrupções...\n");

    for (;;) {
        
        usleep(500000);

        valor = rand() % 100;

        strcpy(interrupcao, IRQ0);
        write(fpFifo, interrupcao, sizeof(interrupcao));
        printf("IRQ0 (Timeslice) gerada.\n");

        if (valor < 10) {
            strcpy(interrupcao, IRQ1);
            write(fpFifo, interrupcao, sizeof(interrupcao));
            printf("IRQ1 (I/O D1) gerada.\n");
        }

        if (valor >= 10 && valor < 15) {
            strcpy(interrupcao, IRQ2);
            write(fpFifo, interrupcao, sizeof(interrupcao));
            printf("IRQ2 (I/O D2) gerada.\n");
        }
    }

    close(fpFifo);
}


int main(void) {
    if (access(FIFO, F_OK) == -1) {
        if (mkfifo(FIFO, 0666) != 0) {
            perror("Erro ao criar FIFO");
            return 1;
        }
        printf("FIFO '%s' criado com sucesso.\n", FIFO);
    }

    inter_controller_sim();

    return 0;
}
