#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "sfss.h"

// Pasta raiz usada pelo SFSS
static const char *ROOTDIR = "./SFSS-root-dir";

// ------------------------------------------------------------
// Cria as pastas A1..A5
// ------------------------------------------------------------
void init_root() {
    mkdir(ROOTDIR, 0700);

    char p[256];
    for (int i = 1; i <= 5; i++) {
        snprintf(p, sizeof(p), "%s/A%d", ROOTDIR, i);
        mkdir(p, 0700);
    }
}

// ------------------------------------------------------------
// Monta caminho real: "./SFSS-root-dir" + caminho vindo do Kernel
// ------------------------------------------------------------
void make_real_path(char *dst, const char *relpath) {
    snprintf(dst, SFP_MAX_PATH, "%s%s", ROOTDIR, relpath);
}

// ------------------------------------------------------------
// READ (RD_REQ) — lê até 16 bytes e devolve em rep->data
// ------------------------------------------------------------
void handle_read(sfp_msg_t *req, sfp_msg_t *rep) {

    char full[300];
    make_real_path(full, req->path);

    FILE *f = fopen(full, "rb");
    if (!f) {
        rep->type = SFP_RD_REP;
        rep->data[0] = '\0';
        return;
    }

    memset(rep->data, 0, SFP_MAX_DATA);
    size_t n = fread(rep->data, 1, 16, f);
    fclose(f);

    if (n < 16)
        rep->data[n] = '\0';

    rep->type = SFP_RD_REP;
}

// ------------------------------------------------------------
// WRITE (WR_REQ) — grava req->data no arquivo
// ------------------------------------------------------------
void handle_write(sfp_msg_t *req, sfp_msg_t *rep) {

    char full[300];
    make_real_path(full, req->path);

    FILE *f = fopen(full, "wb");
    if (!f) {
        rep->type = SFP_WR_REP;
        return;
    }

    fwrite(req->data, 1, strlen(req->data), f);
    fclose(f);

    rep->type = SFP_WR_REP;
}

// ------------------------------------------------------------
// ADD DIRECTORY (DC_REQ)
// req->path = "/A3/novo_dir"
// ------------------------------------------------------------
void handle_add(sfp_msg_t *req, sfp_msg_t *rep) {

    char full[300];
    make_real_path(full, req->path);

    mkdir(full, 0700);

    rep->type = SFP_DC_REP;
}

// ------------------------------------------------------------
// REMOVE DIRECTORY (DR_REQ)
// req->path = "/A1/meudir"
// ------------------------------------------------------------
void handle_rem(sfp_msg_t *req, sfp_msg_t *rep) {

    char full[300];
    make_real_path(full, req->path);

    rmdir(full);

    rep->type = SFP_DR_REP;
}

// ------------------------------------------------------------
// LISTDIR (DL_REQ)
// Envia:
//  - rep->allfilenames (todos nomes concatenados)
//  - rep->fstlstpositions[i]
//  - rep->nrnames
// ------------------------------------------------------------
void handle_listdir(sfp_msg_t *req, sfp_msg_t *rep) {

    char full[300];
    make_real_path(full, req->path);

    DIR *d = opendir(full);
    if (!d) {
        rep->nrnames = 0;
        rep->type = SFP_DL_REP;
        return;
    }

    memset(rep->allfilenames, 0, SFP_MAX_NAMES);

    int pos = 0;
    int count = 0;

    struct dirent *ent;

    int max_entries =
        sizeof(rep->fstlstpositions) / sizeof(rep->fstlstpositions[0]);

    while ((ent = readdir(d)) != NULL) {

        if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
            continue;

        int len = strlen(ent->d_name);
        if (pos + len + 1 >= SFP_MAX_NAMES) break;
        if (count >= max_entries) break;

        memcpy(rep->allfilenames + pos, ent->d_name, len);

        rep->fstlstpositions[count].start = pos;
        rep->fstlstpositions[count].end   = pos + len - 1;

        pos += len;
        rep->allfilenames[pos++] = '\n';

        count++;
    }

    closedir(d);

    rep->nrnames = count;
    rep->type = SFP_DL_REP;
}

// ------------------------------------------------------------
// MAIN DO SFSS  (Servidor UDP)
// ------------------------------------------------------------
int main() {

    init_root();

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8080);

    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    printf("[SFSS] Servidor iniciado na porta 8080\n");

    while (1) {

        sfp_msg_t req, rep;
        socklen_t len = sizeof(cliaddr);

        recvfrom(sockfd, &req, sizeof(req), 0,
                 (struct sockaddr*)&cliaddr, &len);

        rep.owner = req.owner;
        rep.type  = -1;

        switch (req.type) {
            case SFP_RD_REQ: handle_read(&req, &rep);     break;
            case SFP_WR_REQ: handle_write(&req, &rep);    break;
            case SFP_DC_REQ: handle_add(&req, &rep);      break;
            case SFP_DR_REQ: handle_rem(&req, &rep);      break;
            case SFP_DL_REQ: handle_listdir(&req, &rep);  break;
            default:
                rep.type = -1;
        }

        sendto(sockfd, &rep, sizeof(rep), 0,
               (struct sockaddr *)&cliaddr, len);
    }

    return 0;
}
