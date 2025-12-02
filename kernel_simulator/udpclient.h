
#ifndef UDPCLIENT_H
#define UDPCLIENT_H

#include "sfp.h"    // <- IMPORTANTE! Define sfp_msg_t

void udp_init(const char *ip, int port);
void udp_close();
void sfp_send_req(const sfp_msg_t *msg);
void sfp_recv_any();
int sfp_pop_file_rep(sfp_msg_t *out);
int sfp_pop_dir_rep(sfp_msg_t *out);

#endif