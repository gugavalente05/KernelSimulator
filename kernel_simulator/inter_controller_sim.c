#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>

#define FIFO "FifoIPC"

pid_t KERNEL_PID = -1;     // definido no main (kernel.c)
pid_t MY_PID     = -1;     // pid do ICS

void set_kernel_pid(pid_t p) { 
    KERNEL_PID = p;
}

// ------------------------------------------------------
// Tratamento de Ctrl+C (SIGINT)
// ------------------------------------------------------
static int paused = 0;

void interromper(int sinal) {
    (void)sinal;

    // PRIMEIRA VEZ → dump + pausa ICS
    if (!paused) {
        printf("\n[IC] CTRL+C → dumping estado e PAUSANDO ICS\n");

        // manda dump pro kernel
        kill(KERNEL_PID, SIGUSR1);

        usleep(200000);

        paused = 1;

        // pausa ICS
        kill(getpid(), SIGSTOP);

        // quando acordar, continua daqui
        printf("[IC] ICS retomado.\n");
    }
}
// ------------------------------------------------------
// InterControllerSim principal
// ------------------------------------------------------
void inter_controller_sim(void) {

    MY_PID = getpid();

    int fpFifo;
    char irq;

    srand(time(NULL));

    if ((fpFifo = open(FIFO, O_WRONLY)) < 0) {
        perror("openFifoIPC");
        exit(1);
    }

    printf("[IC] InterControllerSim iniciado. Enviando IRQs...\n");

    signal(SIGINT, interromper);  // Ctrl+C → dump + encerra tudo

    for (;;) {

        usleep(500000);

        int prob = rand() % 100;

        // IRQ0 (sempre) - clock do Round-Robin
        irq = '0';
        write(fpFifo, &irq, 1);

        // IRQ1 → prob 10% (0.1)
        if (prob < 10) {
            irq = '1';
            write(fpFifo, &irq, 1);
        }

        // IRQ2 → prob 2% (0.02)
        if (prob >= 10 && prob < 12) {
            irq = '2';
            write(fpFifo, &irq, 1);
        }
    }

    close(fpFifo);
}
