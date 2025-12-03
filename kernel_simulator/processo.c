#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sfp.h"
#include "shmem.h"

#include "processo.h"

extern ax_shmem_t *shm[6];
int MEU_ID = 0;

// ---------------------------------------------------------------------
// FUNÇÃO AUXILIAR: BLOQUEIO (Passo 2)
// ---------------------------------------------------------------------
void esperar_resposta(void) {
    // Fica em loop até o Kernel colocar a resposta na memória (rep_ready = 1)
    // Isso impede que o processo atropele o pedido anterior
    while (shm[MEU_ID]->rep_ready == 0) {
        usleep(1000); // Espera ativa leve (1ms)
    }
    // Consome a resposta
    shm[MEU_ID]->rep_ready = 0; // Limpa flag para a próxima iteração
    shm[MEU_ID]->req_ready = 0; // Garante que a requisição antiga foi limpa
}

// ---------------------------------------------------------------------
// SYS_READ
// ---------------------------------------------------------------------
void sys_read(const char *path, int offset)
{
    shm[MEU_ID]->req.type = SFP_RD_REQ;
    shm[MEU_ID]->req.owner = MEU_ID;
    shm[MEU_ID]->req.offset = offset;

    snprintf(shm[MEU_ID]->req.path, SFP_MAX_PATH, "%s", path);
    shm[MEU_ID]->req_ready = 1;

    printf("[AP%d] READ '%s' (offset=%d) - Aguardando...\n", MEU_ID, path, offset);
    
    // BLOQUEIA AQUI
    esperar_resposta();

    printf("[AP%d] READ Concluido.\n", MEU_ID);
}

// ---------------------------------------------------------------------
// SYS_WRITE
// ---------------------------------------------------------------------
void sys_write(const char *path)
{
    shm[MEU_ID]->req.type = SFP_WR_REQ;
    shm[MEU_ID]->req.owner = MEU_ID;
    shm[MEU_ID]->req.offset = 0;

    snprintf(shm[MEU_ID]->req.path, SFP_MAX_PATH, "%s", path);

    for (int i = 0; i < 16; i++)
        shm[MEU_ID]->req.data[i] = 'A' + (rand() % 26);

    shm[MEU_ID]->req.data[16] = '\0';
    shm[MEU_ID]->req_ready = 1;

    printf("[AP%d] WRITE '%s' - Aguardando...\n", MEU_ID, path);

    // BLOQUEIA AQUI
    esperar_resposta();

    printf("[AP%d] WRITE Concluido.\n", MEU_ID);
}

// ---------------------------------------------------------------------
// SYS_ADD (cria diretório)
// ---------------------------------------------------------------------
void sys_add(const char *path)
{
    shm[MEU_ID]->req.type = SFP_DC_REQ;
    shm[MEU_ID]->req.owner = MEU_ID;

    snprintf(shm[MEU_ID]->req.path, SFP_MAX_PATH, "%s", path);
    shm[MEU_ID]->req_ready = 1;

    printf("[AP%d] ADD DIR '%s' - Aguardando...\n", MEU_ID, path);

    // BLOQUEIA AQUI
    esperar_resposta();

    printf("[AP%d] ADD DIR Concluido.\n", MEU_ID);
}

// ---------------------------------------------------------------------
// SYS_REM (remove diretório)
// ---------------------------------------------------------------------
void sys_rem(const char *path)
{
    shm[MEU_ID]->req.type = SFP_DR_REQ;
    shm[MEU_ID]->req.owner = MEU_ID;

    snprintf(shm[MEU_ID]->req.path, SFP_MAX_PATH, "%s", path);
    shm[MEU_ID]->req_ready = 1;

    printf("[AP%d] REMOVE DIR '%s' - Aguardando...\n", MEU_ID, path);

    // BLOQUEIA AQUI
    esperar_resposta();

    printf("[AP%d] REMOVE DIR Concluido.\n", MEU_ID);
}

// ---------------------------------------------------------------------
// SYS_LISTDIR
// ---------------------------------------------------------------------
void sys_listdir(const char *path)
{
    shm[MEU_ID]->req.type = SFP_DL_REQ;
    shm[MEU_ID]->req.owner = MEU_ID;

    snprintf(shm[MEU_ID]->req.path, SFP_MAX_PATH, "%s", path);
    shm[MEU_ID]->req_ready = 1;

    printf("[AP%d] LISTDIR '%s' - Aguardando...\n", MEU_ID, path);

    // BLOQUEIA AQUI
    esperar_resposta();

    printf("[AP%d] LISTDIR Concluido.\n", MEU_ID);
}

// ---------------------------------------------------------------------
// Processo de aplicação
// ---------------------------------------------------------------------
void processo_aplicacao()
{
    srand(getpid());

    // Loop infinito de requisições
    while (1) {

        int r = rand() % 5;

        // Agora cada chamada sys_... vai bloquear até o Kernel responder
        switch (r) {
            case 0: sys_read("/A1/arq.txt", 0); break;
            case 1: sys_write("/A1/arq.txt"); break;
            case 2: sys_add("/A1/teste"); break;
            case 3: sys_rem("/A1/teste"); break;
            case 4: sys_listdir("/A1"); break;
        }

        // Dorme um pouco antes da próxima requisição para não spammar o log
        usleep(200000);
    }
}