#include <stdlib.h>
#include <string.h>
#include "shmem.h"

// Vetor global acessível por kernel.c
ax_shmem_t *shm[6];  // [1..5], índice 0 ignorado

void shmem_init(void) {
    for (int i = 1; i <= 5; i++) {
        shm[i] = malloc(sizeof(ax_shmem_t));
        memset(shm[i], 0, sizeof(ax_shmem_t));
    }
}

void shmem_destroy(void) {
    for (int i = 1; i <= 5; i++) {
        if (shm[i]) free(shm[i]);
        shm[i] = NULL;
    }
}