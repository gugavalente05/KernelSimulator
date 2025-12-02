// shmem.h
#ifndef SHMEM_H
#define SHMEM_H

#include "sfp.h"

typedef struct {
    sfp_msg_t req;   // mensagem de requisição A -> Kernel
    sfp_msg_t rep;   // mensagem de resposta Kernel -> A
    int req_ready;   // 1 = tem syscall pendente
    int rep_ready;   // 1 = tem resposta pronta
} ax_shmem_t;

void shmem_init(void);
void shmem_destroy(void);

extern ax_shmem_t *shm[6];

#endif
