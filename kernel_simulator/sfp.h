#ifndef SFP_H
#define SFP_H

#define SFP_MAX_PATH 128
#define SFP_MAX_DATA 256
#define SFP_MAX_NAMES 512

// Tipos de requisições/respostas
enum {
    SFP_RD_REQ = 1,
    SFP_WR_REQ,
    SFP_DC_REQ,
    SFP_DR_REQ,
    SFP_DL_REQ,

    SFP_RD_REP = 11,
    SFP_WR_REP,
    SFP_DC_REP,
    SFP_DR_REP,
    SFP_DL_REP
};

// Estrutura de mensagem
typedef struct {
    int  type;
    int  owner;
    int  offset;

    char path[SFP_MAX_PATH];
    char data[SFP_MAX_DATA];

    // usado no LISTDIR
    int  nrnames;
    char allfilenames[SFP_MAX_NAMES];
    struct {
        int start;
        int end;
    } fstlstpositions[64];

} sfp_msg_t;

#endif