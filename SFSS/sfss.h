#ifndef SFSS_H
#define SFSS_H

#include "../kernel_simulator/sfp.h"

// Cria diretórios iniciais (A1..A5)
void init_root();

// Gera o caminho real dentro de SFSS-root-dir
void make_real_path(char *dst, const char *relpath);

// Handlers de cada operação SFP
void handle_read(sfp_msg_t *req, sfp_msg_t *rep);
void handle_write(sfp_msg_t *req, sfp_msg_t *rep);
void handle_add(sfp_msg_t *req, sfp_msg_t *rep);
void handle_rem(sfp_msg_t *req, sfp_msg_t *rep);
void handle_listdir(sfp_msg_t *req, sfp_msg_t *rep);

#endif

