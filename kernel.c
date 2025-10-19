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
#define QUANTUM_TICKS 2

pid_t atual = -1;       
int quantum = 0;
Fila* proc_D1 = NULL;
Fila* proc_D2 = NULL;
Fila* prontos = NULL; 

// Auxiliares para Round Robin
void despacha_prox(void){ // Escolhe qual o proximo processo a rodar

    pid_t pid_prox = -1;
    prontos = pop_fila(prontos, &pid_prox);

    if (pid_prox != -1){ // Novo processo
        atual = pid_prox;
        quantum = QUANTUM_TICKS;
        kill(atual, SIGCONT);
        printf("[KERNEL] despachou %d (quantum = %d)\n", atual, quantum);
    } 
    else { // Sem processos prontos...
        atual = -1;
        quantum = 0;
    }

}

void preempta_atual(void){
    if (atual == -1) return;
    kill(atual, SIGSTOP);
    prontos = insere_fila(atual, prontos);
    printf("[KERNEL] PREEMPCAO de %d — acabou quantum\n", atual);
    atual = -1;
    quantum = 0;
    despacha_prox();
}

// Handlers
void handler_sys (const char* syscall){

    pid_t pid;
    char tipo[3], modo[2];

    if (sscanf(syscall, "%d;%2s;%1s", &pid, tipo, modo) != 3){
        perror("Leitura Syscall");
        exit(1);
    }

    printf("[KERNEL] SYSCALL de PID %d — tipo = %s — bloqueando\n", pid, tipo);
    kill(pid, SIGSTOP);

    if (pid == atual){ // se for do atual, despachar.
        atual = -1;
        quantum = 0;
    }

    if (strcmp(tipo, "D1") == 0) proc_D1 = insere_fila(pid, proc_D1);    
    else proc_D2 = insere_fila(pid, proc_D2); 

    printf("[KERNEL] PID %d colocado na fila de %s\n", pid, tipo);

    if (atual == -1) despacha_prox();

    return;
}

void handler_irq (const char* irq){

    switch (irq[0]){
        case '0' : {

            printf("[KERNEL] IRQ0 (tick de tempo) — quantum agora = %d\n", quantum - 1);
            if (atual == -1){
                despacha_prox();
            } 
            else {
                quantum -= 1;
                if (quantum <= 0){
                    preempta_atual();
                }
            }
            break;

        }

        case '1': {

            printf("[KERNEL] IRQ1 — liberando 1 processo esperando D1\n");
            pid_t pid = -1;
            proc_D1 = pop_fila(proc_D1, &pid);

            if (pid != -1){ // A fila de processos a espera de D1 NÃO estava vazia
                prontos = insere_fila(pid, prontos);
                if (atual == -1) despacha_prox();
            }
            break;
            
        }
            
        case '2':{

            printf("[KERNEL] IRQ2 — liberando 1 processo esperando D2\n");
            pid_t pid = -1;
            proc_D2 = pop_fila(proc_D2, &pid);

            if (pid != -1){ // A fila de processos a espera de D2 NÃO estava vazia
                prontos = insere_fila(pid, prontos);
                if (atual == -1) despacha_prox();
            }
            break;

        } 

        default:
            break;
        }

    return;
}

// Kernel Simulator
int kernel_sim(void){

    printf("[KERNEL] INICIADO — aguardando mensagens...\n");

    int fpFifoIpc, fpFifoSys;
    if ((fpFifoIpc = open(FIFOIPC, O_RDONLY | O_NONBLOCK)) < 0){
        perror("openFifoIPC");
        return 2;
    }
    if ((fpFifoSys = open(FIFOSYS, O_RDONLY | O_NONBLOCK)) < 0){
        perror("openFifoSYS");
        return 3;
    }

    char irq[2];
    char systemcall[50];

    // Cria os processos de usuário e coloca na fila de prontos
    pid_t processos[NUM];
    int status;

    for (int i = 0; i < NUM; i++){
        processos[i] = fork();

        if (processos[i] < 0){
            fprintf(stderr, "Erro na criacao dos processos Ax\n");
            return 1;
        }

        if (processos[i] == 0){
            // Filho usuário: começa parado; kernel decide quando ele roda
            raise(SIGSTOP);
            processo_aplicacao();
            exit(0);
        } else {
            // Pai (kernel): enfileira para Round-Robin
            prontos = insere_fila(processos[i], prontos);
        }
    }

    for (;;){
        // SYS
        ssize_t rs = read(fpFifoSys, systemcall, sizeof(systemcall) - 1);
        if (rs > 0){
            systemcall[rs] = '\0';
            handler_sys(systemcall);
        } else if (rs == 0) {
            close(fpFifoSys);
            // reabre FIFO para continuar recebendo
            fpFifoSys = open(FIFOSYS, O_RDONLY | O_NONBLOCK);
        }

        // IRQ
        ssize_t ri = read(fpFifoIpc, irq, sizeof(irq) - 1);
        if (ri > 0){
            for (ssize_t k = 0; k < ri; k++){
                handler_irq(&irq[k]); // trata byte a byte ('0','1','2')
            }
        } else if (ri == 0) {
            close(fpFifoIpc);
            // reabre FIFO para continuar recebendo
            fpFifoIpc = open(FIFOIPC, O_RDONLY | O_NONBLOCK);
        }

        // Se ninguém estiver em CPU, despachar alguém...
        if (atual == -1) despacha_prox();

    }

    return 0;
}




int main (void){
      
    if (access(FIFOIPC, F_OK) < 0) mkfifo(FIFOIPC, 0600);
    if (access(FIFOSYS, F_OK) < 0) mkfifo(FIFOSYS, 0600);

    // Forka o "kernel_sim"
    pid_t kpid = fork();
    if (kpid == 0) {
        kernel_sim();
        exit(1);
    }

    // Forka o "inter_controller_sim"
    pid_t icpid = fork();
    if (icpid == 0) {
        inter_controller_sim();
        exit(1);
    }

    return 0;
}