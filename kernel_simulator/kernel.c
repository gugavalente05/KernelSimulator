#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/unistd.h>

#include "processo.h"
#include "inter_controller_sim.h"
#include "shmem.h"
#include "sfp.h"
#include "fila.h"
#include "udpclient.h"


#define NUM 5
#define FIFOIPC "FifoIPC"
#define QUANTUM_TICKS 2

static pid_t kernel_pid;
static pid_t ic_pid;


typedef enum {
    ST_PRONTO = 0,
    ST_EXEC,
    ST_BLOQ_FILE,
    ST_BLOQ_DIR,
    ST_TERM
} estado_t;

typedef struct {
    pid_t   pid;
    int     pc;           // "program counter"
    estado_t st;
    char    op_bloq[2];   // não usado no T2, mas mantido
    int     cnt_file;     // quantas vezes recebeu resposta FILE
    int     cnt_dir;      // quantas vezes recebeu resposta DIR
} procinfo_t;

static procinfo_t T[NUM];  // tabela simples indexada por i=0..NUM-1

pid_t atual = -1;       
int quantum = 0;
Fila* wait_file = NULL;   // processos esperando READ/WRITE
Fila* wait_dir  = NULL;   // processos esperando ADD/REM/LISTDIR
Fila* prontos   = NULL; 

extern ax_shmem_t *shm[6]; // [1..5], declarado em shmem.h

// ---------------------------------------------------------------------
// Auxiliar: idx_por_pid
// ---------------------------------------------------------------------
int idx_por_pid(pid_t p){
    for (int i=0; i<NUM; i++) {
        if (T[i].pid == p) return i;
    }
    return -1;
}

// ---------------------------------------------------------------------
// Auxiliar: imprime uma fila de PIDs (sem mexer em fila.c)
// ---------------------------------------------------------------------
static void print_fila(const char *nome, Fila *f) {
    printf("%s: [", nome);
    Fila *aux = f;
    while (aux != NULL) {
        printf(" %d", aux->pid);
        aux = aux->prox;
    }
    printf(" ]\n");
}

// ---------------------------------------------------------------------
// dump_estado (SIGUSR1) — T2
// ---------------------------------------------------------------------

void dump_estado(int signo) {
    (void)signo;

    printf("\n================= DUMP DO KERNEL (T2) =================\n");

    // 1) ESTADO DOS PROCESSOS A1..A5
    printf("\n--- Estado dos processos ---\n");
    for (int i = 0; i < NUM; i++) {
        const char *txt =
            (T[i].st == ST_PRONTO)     ? "PRONTO" :
            (T[i].st == ST_EXEC)       ? "EXECUTANDO" :
            (T[i].st == ST_BLOQ_FILE)  ? "BLOQ_FILE" :
            (T[i].st == ST_BLOQ_DIR)   ? "BLOQ_DIR" :
                                         "TERMINADO";

        printf("AP%d (pid=%d): PC=%d | estado=%s | FILE_REP=%d | DIR_REP=%d\n",
               i+1, T[i].pid, T[i].pc, txt, T[i].cnt_file, T[i].cnt_dir);
    }

    // FILAS DE ESPERA: FILE E DIR
    printf("\n--- Filas de processos bloqueados ---\n");
    print_fila("WAIT_FILE (READ/WRITE)", wait_file);
    print_fila("WAIT_DIR  (ADD/REM/LISTDIR)", wait_dir);

    // FILA DE PRONTOS
    printf("\n--- Fila de PRONTOS ---\n");
    print_fila("PRONTOS", prontos);

    // SHARED MEMORY dos 5 processos (REQ/REP pendentes)
    printf("\n--- Shared Memory (shm[1..5]) ---\n");
    for (int owner = 1; owner <= 5; owner++) {
        ax_shmem_t *S = shm[owner];

        printf("A%d: req_ready=%d | rep_ready=%d | req.type=%d | rep.type=%d\n",
               owner,
               S->req_ready,
               S->rep_ready,
               S->req.type,
               S->rep.type);

        printf("    req.path=%s | req.offset=%d\n",
               S->req.path,
               S->req.offset);
        printf("    rep.path=%s | rep.offset=%d\n",
               S->rep.path,
               S->rep.offset);
    }

    printf("\n(Obs: Respostas ainda “presas” no SFSS/udpclient e que "
           "não geraram IRQ1/IRQ2 ainda não são visíveis aqui; "
           "mas qualquer processo em WAIT_FILE/WAIT_DIR está aguardando "
           "exatamente uma dessas respostas.)\n");

    printf("\n================ FIM DO DUMP DO KERNEL ================\n\n");
}

// ---------------------------------------------------------------------
// Round Robin: despacha_prox e preempta_atual
// ---------------------------------------------------------------------
void despacha_prox(void){ // Escolhe qual o proximo processo a rodar
    pid_t pid_prox = -1;
    prontos = pop_fila(prontos, &pid_prox);

    if (pid_prox != -1){ // Novo processo
        atual = pid_prox;
        quantum = QUANTUM_TICKS;
        kill(atual, SIGCONT);
        int ix = idx_por_pid(atual);

        if (ix >= 0){
            T[ix].st = ST_EXEC;
        } 

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

    int ix = idx_por_pid(atual);
    if (ix >= 0){
        T[ix].st = ST_PRONTO;
    } 

    printf("[KERNEL] PREEMPCAO de %d — acabou quantum\n", atual);
    atual = -1;
    quantum = 0;
    despacha_prox();
}

// ---------------------------------------------------------------------
// Trata syscalls dos Ax via SHM → envia REQ via UDP e bloqueia processo
// ---------------------------------------------------------------------
void trata_syscalls_dos_Ax() {

    for (int owner = 1; owner <= 5; owner++) {
        if (shm[owner]->req_ready) {
            sfp_msg_t msg = shm[owner]->req;

            // Descobre o PID correspondente a esse owner (APx)
            int ix = owner - 1;              // AP1 -> T[0], AP2 -> T[1]...
            pid_t pid = T[ix].pid;

            // BLOQUEIA o processo que fez a syscall
            kill(pid, SIGSTOP);

            // Atualiza estado e fila de espera (FILE ou DIR)
            if (msg.type == SFP_RD_REQ || msg.type == SFP_WR_REQ) {
                T[ix].st = ST_BLOQ_FILE;
                wait_file = insere_fila(pid, wait_file);
            } else { // DC/DR/DL
                T[ix].st = ST_BLOQ_DIR;
                wait_dir = insere_fila(pid, wait_dir);
            }
            T[ix].pc++;

            if (pid == atual) {
                atual = -1;
                quantum = 0;
            }

            // Envia REQ para o SFSS via udpclient (black box)
            sfp_send_req(&msg);

            // Limpa flag da shmem
            shm[owner]->req_ready = 0;

            printf("[KERNEL] Syscall de owner=%d (pid=%d) enviada ao SFSS\n",
                   owner, pid);
        }
    }
}

// ---------------------------------------------------------------------
// Tratamento de IRQs vindas do InterControllerSim
// ---------------------------------------------------------------------
void handler_irq (const char* irq){

    switch (irq[0]){

        // IRQ0: Tick de clock (Round-Robin)
        case '0' : {

            if (atual == -1){    
                despacha_prox();
            } 
            else {
                quantum -= 1;
                printf("[KERNEL] IRQ0 — quantum agora = %d\n", quantum);
                
                int ix = idx_por_pid(atual);
                if (ix >= 0) {
                    T[ix].pc++;
                }
                if (quantum <= 0){
                    preempta_atual();
                }
            }
            break;
        }

        // IRQ1: uma resposta FILE (RD/WR) ficou pronta
        case '1': {

            printf("[KERNEL] IRQ1 — entregando 1 resposta de FILE\n");

            sfp_msg_t rep;
            if (!sfp_pop_file_rep(&rep)) return; // nenhuma resposta pendente

            int owner = rep.owner;
            shm[owner]->rep = rep;
            shm[owner]->rep_ready = 1;

            // tira um processo da fila de espera de FILE
            pid_t pid = -1;
            wait_file = pop_fila(wait_file, &pid);

            if (pid != -1){
                prontos = insere_fila(pid, prontos);

                int ix = idx_por_pid(pid);
                if (ix >= 0){
                    T[ix].st = ST_PRONTO;
                    T[ix].cnt_file++;
                    T[ix].op_bloq[0] = '\0';
                }

                if (atual == -1) despacha_prox();
            }
            break;
        }

        // IRQ2: uma resposta DIR (DC/DR/DL) ficou pronta
        case '2': {

            printf("[KERNEL] IRQ2 — entregando 1 resposta de DIR\n");

            sfp_msg_t rep;
            if (!sfp_pop_dir_rep(&rep)) return;

            int owner = rep.owner;
            shm[owner]->rep = rep;
            shm[owner]->rep_ready = 1;

            pid_t pid = -1;
            wait_dir = pop_fila(wait_dir, &pid);

            if (pid != -1){
                prontos = insere_fila(pid, prontos);

                int ix = idx_por_pid(pid);
                if (ix >= 0){
                    T[ix].st = ST_PRONTO;
                    T[ix].cnt_dir++;
                    T[ix].op_bloq[0] = '\0';
                }

                if (atual == -1) despacha_prox();
            }
            break;
        }

        default:
            break;
    }
}

// ---------------------------------------------------------------------
// Kernel Simulator
// ---------------------------------------------------------------------
int kernel_sim(void){

    signal(SIGINT, SIG_IGN);
    signal(SIGUSR1, dump_estado);
    printf("[KERNEL] (%d) INICIADO — aguardando mensagens...\n", getpid());
    

    // inicializa cliente UDP apontando para o SFSS
    udp_init("127.0.0.1", 8080);  // mesma porta do SFSS

    int fpFifoIpc;
    if ((fpFifoIpc = open(FIFOIPC, O_RDONLY | O_NONBLOCK)) < 0){
        perror("openFifoIPC");
        return 2;
    }

    char irq[2];

    shmem_init();
    // Cria os processos de usuário e coloca na fila de prontos
    pid_t processos[NUM];

    for (int i = 0; i < NUM; i++){
        processos[i] = fork();

        if (processos[i] < 0){
            fprintf(stderr, "Erro na criacao dos processos Ax\n");
            return 1;
        }

        if (processos[i] == 0){
            // FILHO: processo de usuário (APx)
            MEU_ID = i+1;              // <=== define dono correto pro processo
            raise(SIGSTOP);            // Kernel decide quando ele roda
            processo_aplicacao();      // loop de syscalls
            exit(0);
        } else {
            // PAI (kernel): enfileira para Round-Robin
            prontos = insere_fila(processos[i], prontos);
            T[i].pid   = processos[i];
            T[i].pc    = 0;
            T[i].st    = ST_PRONTO;
            T[i].op_bloq[0] = '\0';
            T[i].cnt_file = 0;
            T[i].cnt_dir  = 0;
        }
    }

    for (;;) {

        // IRQs vindas do ICS
        int ri = read(fpFifoIpc, irq, sizeof(irq) - 1);
        if (ri > 0){
            for (int k = 0; k < ri; k++){
                handler_irq(&irq[k]); // trata byte a byte ('0','1','2')
            }
        } else if (ri == 0) {
            close(fpFifoIpc);
            // reabre FIFO para continuar recebendo
            fpFifoIpc = open(FIFOIPC, O_RDONLY | O_NONBLOCK);
        }

        // Syscalls feitas pelos Ax via SHM
        sfp_recv_any();
        trata_syscalls_dos_Ax();

        // Se ninguém estiver em CPU, despachar alguém...
        if (atual == -1) despacha_prox();
    }
    
    shmem_destroy();
    udp_close();
    return 0;
}

// ---------------------------------------------------------------------
// main: cria FIFO, fork kernel e ICS
// ---------------------------------------------------------------------
static int paused_ics = 0;

void toggle_ics(int sig) {
    (void)sig;

    if (!paused_ics) {
        printf("\n[MAIN] CTRL+C → kernel irá dumpar, ICS será pausado.\n");
        kill(kernel_pid, SIGUSR1); // mostra dump
        paused_ics = 1;
    }
    else {
        printf("\n[MAIN] CTRL+C → retomando ICS.\n");
        kill(ic_pid, SIGCONT);
        paused_ics = 0;
    }
}

int main(void) {
    if (access(FIFOIPC, F_OK) < 0) mkfifo(FIFOIPC, 0600);

    // Forka o "kernel_sim"
    pid_t kpid = fork();
    if (kpid == 0) {
        kernel_sim();
        exit(1);
    }
    kernel_pid = kpid;
    set_kernel_pid(kpid);

    // Forka o ICS
    pid_t icpid_fork = fork();
    if (icpid_fork == 0) {
        inter_controller_sim();
        exit(1);
    }
    ic_pid = icpid_fork;

    // Pai: instala handler de Ctrl+C
    signal(SIGINT, toggle_ics);

    // Pai apenas fica vivo
    for (;;) pause();
}
