// Arquivo: sfp.h
#ifndef SFP_H
#define SFP_H

// Tamanho do bloco de dados (Payload) conforme enunciado
#define BLOCK_SIZE 16
#define MAX_PATH 100

// Tipos de Mensagem (Comandos)
typedef enum {
    REQ_READ = 1,
    REQ_WRITE = 2,
    REQ_CREATE_DIR = 3,
    REQ_REM_DIR = 4,
    REQ_LIST = 5,
    // Respostas correspondentes
    REP_READ = 11,
    REP_WRITE = 12,
    REP_CREATE_DIR = 13,
    REP_REM_DIR = 14,
    REP_LIST = 15,
    REP_ERROR = 99
} MessageType;

// A Estrutura da Mensagem (O "Pacote" UDP)
typedef struct {
    int tipo;               // Um dos valores do enum MessageType
    int owner;              // Quem pediu? (PID do processo Ax)
    int offset;             // Posição para ler/escrever no arquivo
    int status;             // 0 = Sucesso, < 0 = Erro (usado na resposta)
    int size_path;          // Tamanho da string path
    int size_payload;       // Tamanho útil do payload
    char path[MAX_PATH];    // Caminho do arquivo/diretório (ex: "/A1/meu_arquivo.txt")
    char payload[BLOCK_SIZE]; // Dados lidos ou a serem escritos (16 bytes)
} SFP_Message;

#endif