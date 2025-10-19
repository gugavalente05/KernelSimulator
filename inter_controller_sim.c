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
    int prob;
    char interrupcao[2];


    srand(time(NULL));

    if ((fpFifo = open(FIFO, O_WRONLY)) < 0) {
        perror("openFifoIPC");
        exit(1);
    }
    
    printf("InterControllerSim iniciado. Enviando interrupções...\n");

    for (;;) {
        
        usleep(500000);

        prob = rand() % 100;

        strcpy(interrupcao, IRQ0);
        write(fpFifo, interrupcao, sizeof(interrupcao));

        if (prob < 10) {
            strcpy(interrupcao, IRQ1);
            write(fpFifo, interrupcao, sizeof(interrupcao));
        }

        if (prob >= 10 && prob < 15) {
            strcpy(interrupcao, IRQ2);
            write(fpFifo, interrupcao, sizeof(interrupcao));
        }
    }

    close(fpFifo);
}
