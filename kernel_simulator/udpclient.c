#include "udpclient.h"
#include "sfp.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h> // Para ver erros detalhados

#define FIFOIPC "FifoIPC"

static int sockfd;
static struct sockaddr_in servaddr;
static sfp_msg_t file_reps[64];
static int file_head = 0, file_tail = 0;
static sfp_msg_t dir_reps[64];
static int dir_head = 0, dir_tail = 0;

void udp_init(const char *ip, int port)
{
    printf("[DEBUG] Inicializando UDP para %s:%d\n", ip, port);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("[ERRO] Falha ao criar socket");
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);
}

void udp_close()
{
    close(sockfd);
}

void sfp_send_req(const sfp_msg_t *req)
{
    printf("[DEBUG] Tentando enviar requisicao (Tipo: %d)...\n", req->type);
    int n = sendto(sockfd, req, sizeof(*req), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (n < 0) {
        perror("[ERRO] Falha no sendto");
    } else {
        printf("[DEBUG] Pacote enviado (%d bytes)\n", n);
    }
}

void sfp_recv_any()
{
    sfp_msg_t rep;
    socklen_t len = sizeof(servaddr);

    // Tenta receber
    int n = recvfrom(sockfd, &rep, sizeof(rep), MSG_DONTWAIT, (struct sockaddr*)&servaddr, &len);

    if (n > 0) {
        printf("[DEBUG] Resposta recebida do servidor! (Tipo: %d)\n", rep.type);
        
        int isFile = (rep.type == SFP_RD_REP || rep.type == SFP_WR_REP);
        if (isFile) {
            file_reps[file_tail++] = rep;
            if (file_tail >= 64) file_tail = 0;
        } else {
            dir_reps[dir_tail++] = rep;
            if (dir_tail >= 64) dir_tail = 0;
        }

        // Tenta abrir FIFO
        printf("[DEBUG] Tentando abrir FIFO para IRQ...\n");
        int fifo = open(FIFOIPC, O_WRONLY | O_NONBLOCK);
        if (fifo >= 0) {
            char irq = isFile ? '1' : '2';
            write(fifo, &irq, 1);
            close(fifo);
            printf("[DEBUG] IRQ enviada com sucesso!\n");
        } else {
            // Se der erro, mostra qual foi
            printf("[ERRO] Falha ao abrir FIFO: %s\n", strerror(errno));
        }
    }
}

// Funcoes POP (sem alteracao, apenas logica)
int sfp_pop_file_rep(sfp_msg_t *out)
{
    if (file_head == file_tail) return 0;
    *out = file_reps[file_head++];
    if (file_head >= 64) file_head = 0;
    return 1;
}

int sfp_pop_dir_rep(sfp_msg_t *out)
{
    if (dir_head == dir_tail) return 0;
    *out = dir_reps[dir_head++];
    if (dir_head >= 64) dir_head = 0;
    return 1;
}