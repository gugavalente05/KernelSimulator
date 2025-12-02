
typedef struct fila Fila;
struct fila{
    int pid;
    Fila* prox;
};

Fila* insere_fila (int pid, Fila* minhaFila);
Fila* pop_fila(Fila* minhaFila, int* pid);
int busca_fila(Fila* minhaFila, int pid);
void libera_fila(Fila* minhaFila);