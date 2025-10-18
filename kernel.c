#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/unistd.h>
#include "inter_controller_sim.c"
#include "processo.c"
#include "fila.c"

#define NUM 5
#define FIFOIPC "FifoIPC"
#define FIFOSYS "FifoSystemCall"


Fila* proc_D1 = NULL;
Fila* proc_D2 = NULL;


void handler_sys (const char* syscall){

    int pid = getpid();
    char tipo[3];
    strncpy(tipo, &syscall[6], 2);
    tipo[2] = '\0';

    kill(pid, SIGSTOP);
    if (tipo == "D1") insere_fila(pid, proc_D1);    
    else insere_fila(pid, proc_D2); 

    return;
}

void handler_irq (const char* irq){

    int pid = getpid();

    switch (irq[0]){
        case '0':
            kill(pid, SIGSTOP);
            break;
        case '1':
            if (busca_fila(proc_D1, pid) == 1){
                kill(pid, SIGCONT);
            }
            break;
        case '2':
            if (busca_fila(proc_D2, pid) == 1){
                kill(pid, SIGCONT);
            }
            break;
        default:
            break;
        }

    return;
}

int kernel_sim(void){
    int fpFifoIpc, fpFifoSys;
    
    if ((fpFifoIpc = open(FIFOIPC, O_RDONLY)) < 0){
        perror("openFifoIPC");
        return 2;
    }
    if ((fpFifoSys = open(FIFOSYS, O_RDONLY)) < 0){
        perror("openFifoSYS");
        return 3;
    }
    
    char irq[2];
    char systemcall[50]; 
    int quantum;

    for (;;){
        int pid = getpid();
        if (read(fpFifoSys, systemcall, sizeof(systemcall)) > 0){
            handler_sys(systemcall);
        }
        if (read(fpFifoIpc, irq, sizeof(irq)) > 0){
            handler_irq(irq);    
        }
    }
}



int main (void){
      
    if (access(FIFO, F_OK) < 0){
        if (mkfifo(FIFO, 0600) < 0){
            perror("Erro na criacao do fifo.");
            exit(1);
        }
    }

    // Forka o "kernel_sim"
    pid_t kpid = fork();
    if (kpid == 0) {
        kernel_sim();
    }

    // Forka o "inter_controller_sim"
    pid_t icpid = fork();
    if (icpid == 0) {
        inter_controller_sim();
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
            processo_aplicacao();
        }
    }

    for (int i = 0; i < NUM ; i ++){
        waitpid(processos[i], &status, 0);
    }

    return 0;
}