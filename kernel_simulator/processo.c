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
// SYS_READ
// ---------------------------------------------------------------------
void sys_read(const char *path, int offset)
{
    shm[MEU_ID]->req.type = SFP_RD_REQ;
    shm[MEU_ID]->req.owner = MEU_ID;
    shm[MEU_ID]->req.offset = offset;

    snprintf(shm[MEU_ID]->req.path, SFP_MAX_PATH, "%s", path);
    shm[MEU_ID]->req_ready = 1;

    printf("[AP%d] READ '%s' (offset=%d)\n",
           MEU_ID, path, offset);
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

    printf("[AP%d] WRITE '%s'\n", MEU_ID, path);
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

    printf("[AP%d] ADD DIR '%s'\n", MEU_ID, path);
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

    printf("[AP%d] REMOVE DIR '%s'\n", MEU_ID, path);
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

    printf("[AP%d] LISTDIR '%s'\n", MEU_ID, path);
}

// ---------------------------------------------------------------------
// Processo de aplicação
// ---------------------------------------------------------------------
void processo_aplicacao()
{
    srand(getpid());

    while (1) {

        int r = rand() % 5;

        switch (r) {
            case 0: sys_read("/A1/arq.txt", 0); break;
            case 1: sys_write("/A1/arq.txt"); break;
            case 2: sys_add("/A1/teste"); break;
            case 3: sys_rem("/A1/teste"); break;
            case 4: sys_listdir("/A1"); break;
        }

        usleep(200000);
    }
}
