#include <stdio.h>
#include <stdlib.h>

typedef struct fila Fila;
struct fila{
    int pid;
    Fila* prox;
};


Fila* insere_fila (int pid, Fila* minhaFila){
    if (minhaFila == NULL){
        Fila* nova = (Fila*)malloc(sizeof(Fila));
        if (!nova){ perror("malloc"); exit(1); }
        nova->pid = pid;
        nova->prox = NULL;
        return nova;                 
    }
    if (minhaFila->prox == NULL){
        Fila* nova = (Fila*)malloc(sizeof(Fila));
        if (!nova){ perror("malloc"); exit(1); }
        nova->pid = pid;
        nova->prox = NULL;
        minhaFila->prox = nova;      
        return minhaFila;            
    }
    minhaFila->prox = insere_fila(pid, minhaFila->prox);
    return minhaFila;
}

Fila* pop_fila(Fila* minhaFila, int* pid){

    if (minhaFila == NULL) return NULL;
    if (pid) *pid = minhaFila->pid;

    Fila* novo_head = minhaFila->prox;
    free(minhaFila);
    return novo_head;

}

int busca_fila(Fila* minhaFila, int pid) {
    Fila* atual = minhaFila;
    while (atual != NULL) {
        if (atual->pid == pid) {
            return 1;  
        }
        atual = atual->prox;
    }
    return 0;  
}

void libera_fila(Fila* minhaFila){
    Fila* atual = minhaFila;
    while (atual != NULL) {
        Fila* proximo = atual->prox;
        free(atual);
        atual = proximo;
    }
}