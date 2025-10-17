#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/unistd.h>

#define NUM 5
#define FIFO "FifoIPC"

enum {
    IRQ0 = 0,
    IRQ1 = 1,
    IRQ2 = 2,
};

int kernel_sim(void){
    int fpFifo;

    if ((fpFifo = open(FIFO, O_RDONLY)) < 0){
        fprintf(stderr, "Erro ao abrir FIFO %s", FIFO);
        return 2;
    }

}

int inter_controller_sim(void){

    int fpFifo;

    if ((fpFifo = open(FIFO, O_WRONLY)) < 0){
        fprintf(stderr, "Erro ao abrir FIFO %s", FIFO);
        return 2;
    }

    for (;;){
        usleep(500000);
    } 

}


int main (void){
    
    if (access(FIFO, F_OK) < 0){
        if (mkfifo(FIFO, S_IRUSR | S_IWUSR)){
            fprintf(stderr, "Erro na criacao da FIFO %s\n", FIFO);
            return 1;
        }
    }

    // Forka o "kernel_sim"
    pid_t kpid = fork();
    if (kpid == 0) {
        // processo kernel
        exit(kernel_sim());
    }

    // Forka o "inter_controller_sim"
    pid_t icpid = fork();
    if (icpid == 0) {
        exit(inter_controller_sim());
    }

    // Processos - A1,A2 (...)
    pid_t processos[NUM];
    int status;
   
    for (int i = 0; i < NUM ; i ++){
        processos[i] = fork();

        if (processos[i] < 0){
            fprintf(stderr, "Erro na criacao da processos Ax\n");
            return 1;
        }

        if (processos[i] == 0){
            // Codigo processos Ax

        }
    }

    for (int i = 0; i < NUM ; i ++){
        waitpid(processos[i], &status, 0);
    }

    return 0;
}