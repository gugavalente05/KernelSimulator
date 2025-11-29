#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include "sfp.h"  // Certifique-se que sfp.h está na mesma pasta

#define MAX_ITERACOES 50
#define SYSCALL_FIFO "FifoSyscall" 
#define KEY_BASE 1234 // Chave base para gerar IDs de memória única

// Variáveis globais para memória compartilhada
int shm_id;
SFP_Message *shm_msg; // Ponteiro para a "Lousa" (Memória Compartilhada)

// Função auxiliar para gerar nomes de arquivo aleatórios
void gera_nome_arquivo(char *buffer, int owner) {
    sprintf(buffer, "/A%d/arq_%d.txt", owner, rand() % 5);
}

// Função auxiliar para gerar nomes de diretório aleatórios
void gera_nome_dir(char *buffer, int owner) {
    sprintf(buffer, "/A%d/dir_%d", owner, rand() % 3);
}

// --- AQUI ESTÁ A MAIN QUE FALTAVA ---
int main(int argc, char *argv[]) {
    // Validação: O processo precisa receber seu ID (1 a 5) ao ser lançado pelo Kernel
    // Nota: Vamos configurar o Kernel na Fase 3 para enviar esse número.
    if (argc < 2) {
        fprintf(stderr, "Uso: ./aplicacao <ID_DO_PROCESSO 1-5>\n");
        exit(1);
    }
    
    int id_processo = atoi(argv[1]); // Converte o argumento ("1") para inteiro
    pid_t pid = getpid();
    int fpFifo;

    // --- 1. CONFIGURAÇÃO DA MEMÓRIA COMPARTILHADA ---
    // Cria uma chave única: Se id_processo for 1, key será 1235
    key_t key = KEY_BASE + id_processo; 
    
    // shmget: Localiza ou cria o segmento de memória
    if ((shm_id = shmget(key, sizeof(SFP_Message), IPC_CREAT | 0666)) < 0) {
        perror("Erro no shmget em processo.c");
        exit(1);
    }

    // shmat: "Gruda" a memória no ponteiro shm_msg
    if ((shm_msg = (SFP_Message *)shmat(shm_id, NULL, 0)) == (void *) -1) {
        perror("Erro no shmat em processo.c");
        exit(1);
    }

    // Abre o FIFO para "cutucar" o Kernel (Write Only)
    if ((fpFifo = open(SYSCALL_FIFO, O_WRONLY)) < 0) {
        // Tenta criar se não existir (segurança), mas o Kernel já deve ter criado
        perror("Erro ao abrir FifoSyscall");
        exit(1);
    }

    // Inicializa semente randômica baseada no tempo e PID
    srand(time(NULL) ^ pid);
    printf("Processo A%d (PID: %d) iniciado. Memória ID: %d\n", id_processo, pid, shm_id);

    // --- 2. LOOP PRINCIPAL ---
    for (int pc = 0; pc < MAX_ITERACOES; pc++) {
        // Simula processamento
        usleep(500000); // 0.5s

        // Decide se faz syscall (Chance de 15%, como no T1)
        if ((rand() % 100) < 15) {
            
            // --- PREENCHIMENTO DA MEMÓRIA ---
            shm_msg->owner = id_processo; // Assina a mensagem
            shm_msg->status = 0;          // Limpa status
            
            int escolha = rand() % 100;
            
            // Lógica do Enunciado T2: Ímpar = Arquivo, Par = Diretório
            if (escolha % 2 != 0) { 
                // --- OPERAÇÃO DE ARQUIVO ---
                gera_nome_arquivo(shm_msg->path, id_processo);
                shm_msg->offset = (rand() % 5) * 16; // Offset: 0, 16, 32...
                
                if ((rand() % 3) == 1) { // Chance de Escrita
                    shm_msg->tipo = REQ_WRITE;
                    strcpy(shm_msg->payload, "DADOS_T2_TESTE_"); // Exemplo de dados (16 bytes)
                    printf("[A%d] Preparando WR-REQ em %s\n", id_processo, shm_msg->path);
                } else { // Leitura
                    shm_msg->tipo = REQ_READ;
                    printf("[A%d] Preparando RD-REQ em %s\n", id_processo, shm_msg->path);
                }
            } 
            else {
                // --- OPERAÇÃO DE DIRETÓRIO ---
                gera_nome_dir(shm_msg->path, id_processo);
                shm_msg->tipo = REQ_CREATE_DIR; 
                printf("[A%d] Preparando DC-REQ (Criar Dir) em %s\n", id_processo, shm_msg->path);
            }

            // --- NOTIFICAÇÃO AO KERNEL ---
            // Escreve APENAS o PID no FIFO para acordar o Kernel
            char buffer_pid[10];
            sprintf(buffer_pid, "%d;", pid); 
            write(fpFifo, buffer_pid, strlen(buffer_pid));

            // --- BLOQUEIO (Wait for Reply) ---
            // O processo para aqui. O Kernel vai acordá-lo com SIGCONT quando a resposta chegar do servidor.
            printf("A%d (PID %d): Bloqueando aguardando Kernel...\n", id_processo, pid);
            kill(pid, SIGSTOP); 
            
            // --- RETORNO (ACORDADO PELO KERNEL) ---
            // Quando a execução chega aqui, a memória compartilhada já tem a resposta!
            if (shm_msg->status < 0) {
                printf("A%d: Erro retornado na operação!\n", id_processo);
            } else {
                printf("A%d: Sucesso! Operação concluída.\n", id_processo);
                if (shm_msg->tipo == REP_READ) {
                    // Se foi leitura, mostra o que leu
                    printf("   -> Dados lidos do servidor: %.16s\n", shm_msg->payload);
                }
            }
        }
    }

    printf("Processo A%d completou %d ciclos e terminou.\n", id_processo, MAX_ITERACOES);
    
    // Limpeza
    shmdt(shm_msg); // Desanexa memória
    close(fpFifo);
    return 0;
}