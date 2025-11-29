// Arquivo: sfss.c
// Compile com: gcc sfss.c -o sfss
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <errno.h>
#include "sfp.h" // Inclui a nossa definição de protocolo

#define PORT 8080
#define ROOT_DIR "./SFSS-root-dir" // Raiz dos arquivos 

// Função auxiliar para montar o caminho real no disco do PC
// Ex: Recebe "/A1/texto.txt" -> Transforma em "./SFSS-root-dir/A1/texto.txt"
void constroi_caminho(char *destino, const char *path_remoto) {
    sprintf(destino, "%s%s", ROOT_DIR, path_remoto);
}

// Implementação da Leitura (Read)
void tratar_leitura(SFP_Message *msg) {
    char full_path[200];
    constroi_caminho(full_path, msg->path);

    FILE *f = fopen(full_path, "rb"); // Abre para leitura binária
    if (!f) {
        msg->status = -1; // Erro: arquivo não existe
        msg->tipo = REP_ERROR;
        return;
    }

    fseek(f, msg->offset, SEEK_SET); // Pula para a posição desejada (Offset)
    int lidos = fread(msg->payload, 1, BLOCK_SIZE, f); // Lê 16 bytes
    
    msg->size_payload = lidos;
    msg->tipo = REP_READ;
    msg->status = 0; // Sucesso
    fclose(f); // Fecha imediatamente (Stateless)
}

// Implementação da Escrita (Write)
void tratar_escrita(SFP_Message *msg) {
    char full_path[200];
    constroi_caminho(full_path, msg->path);

    // Abre para leitura e escrita ("r+b"). Se não existir, use "wb" para criar.
    FILE *f = fopen(full_path, "r+b");
    if (!f) f = fopen(full_path, "wb"); // Cria se não existe

    if (!f) {
        msg->status = -1;
        msg->tipo = REP_ERROR;
        return;
    }

    fseek(f, msg->offset, SEEK_SET);
    fwrite(msg->payload, 1, BLOCK_SIZE, f); // Escreve os 16 bytes
    
    msg->tipo = REP_WRITE;
    msg->status = 0;
    fclose(f);
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    SFP_Message msg_buffer;
    
    // 1. Cria a pasta raiz se não existir
    mkdir(ROOT_DIR, 0777); 

    // 2. Criação do Socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // 3. Bind (Amarrar o socket à porta)
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Falha no bind");
        exit(EXIT_FAILURE);
    }

    printf("SFSS Servidor Rodando na porta %d...\nRaiz: %s\n", PORT, ROOT_DIR);

    // 4. Loop Infinito de Atendimento
    while (1) {
        socklen_t len = sizeof(cliaddr);
        
        // RECEBE MENSAGEM (Bloqueante)
        int n = recvfrom(sockfd, &msg_buffer, sizeof(SFP_Message), 0, (struct sockaddr *)&cliaddr, &len);
        
        if (n > 0) {
            printf("[SFSS] Recebido pedido Tipo %d do Owner %d para: %s\n", msg_buffer.tipo, msg_buffer.owner, msg_buffer.path);
            
            // PROCESSA O PEDIDO
            switch (msg_buffer.tipo) {
                case REQ_READ:
                    tratar_leitura(&msg_buffer);
                    break;
                case REQ_WRITE:
                    tratar_escrita(&msg_buffer);
                    break;
                // Tarefa para você: Implementar REQ_CREATE_DIR e REQ_LIST aqui depois
                default:
                    msg_buffer.tipo = REP_ERROR;
                    msg_buffer.status = -99; // Operação desconhecida
            }

            // ENVIA RESPOSTA
            sendto(sockfd, &msg_buffer, sizeof(SFP_Message), 0, (struct sockaddr *)&cliaddr, len);
            printf("[SFSS] Resposta enviada. Status: %d\n", msg_buffer.status);
        }
    }
    return 0;
}