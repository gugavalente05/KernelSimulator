#include "shmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

// Ponteiros para a memória compartilhada
ax_shmem_t *shm[6]; // shm[1..5] serão usados

// Memória bruta (ponteiro para o bloco total)
static void *mem_block = NULL;

void shmem_init() {
    // Calcula o tamanho total: 6 estruturas (usamos indices 1 a 5)
    size_t total_size = 6 * sizeof(ax_shmem_t);

    // CRUCIAL: Usar mmap com MAP_SHARED e MAP_ANONYMOUS
    // Isso garante que a memória seja a mesma para Pai (Kernel) e Filhos (APx)
    mem_block = mmap(NULL, total_size, 
                     PROT_READ | PROT_WRITE, 
                     MAP_SHARED | MAP_ANONYMOUS, 
                     -1, 0);

    if (mem_block == MAP_FAILED) {
        perror("Erro no shmem_init (mmap)");
        exit(1);
    }

    // Ajusta os ponteiros shm[1], shm[2]... para apontarem dentro desse bloco
    ax_shmem_t *ptr = (ax_shmem_t *)mem_block;
    for (int i = 0; i < 6; i++) {
        shm[i] = &ptr[i];
        
        // Limpa a memória para começar zerada
        shm[i]->req_ready = 0;
        shm[i]->rep_ready = 0;
        shm[i]->req.type = 0;
        shm[i]->rep.type = 0;
    }

    printf("[SHMEM] Memoria compartilhada criada com sucesso (MAP_SHARED).\n");
}

void shmem_destroy() {
    size_t total_size = 6 * sizeof(ax_shmem_t);
    if (mem_block) {
        munmap(mem_block, total_size);
    }
}