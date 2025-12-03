#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "sfp.h" 

#define PORT 8080
#define ROOT_DIR "./SFSS-root-dir" 

void build_path(char *dest, const char *remote_path) {
    sprintf(dest, "%s%s", ROOT_DIR, remote_path);
}

void handle_read(sfp_msg_t *msg) {
    char full_path[512];
    build_path(full_path, msg->path);

    FILE *f = fopen(full_path, "rb");
    if (!f) {
        printf("[SFSS] Erro ao ler: %s\n", full_path);
        msg->type = -1; 
        return;
    }

    fseek(f, msg->offset, SEEK_SET);
    int n = fread(msg->data, 1, 16, f); 
    if (n < 16) msg->data[n] = '\0';

    fclose(f);
    msg->type = SFP_RD_REP;
    printf("[SFSS] READ OK: %s (Offset %d)\n", msg->path, msg->offset);
}

void handle_write(sfp_msg_t *msg) {
    char full_path[512];
    build_path(full_path, msg->path);

    FILE *f = fopen(full_path, "r+b");
    if (!f) f = fopen(full_path, "wb"); 

    if (!f) {
        printf("[SFSS] Erro ao escrever: %s\n", full_path);
        msg->type = -1;
        return;
    }

    fseek(f, msg->offset, SEEK_SET);
    fwrite(msg->data, 1, 16, f); 
    fclose(f);

    msg->type = SFP_WR_REP;
    printf("[SFSS] WRITE OK: %s (Offset %d)\n", msg->path, msg->offset);
}

void handle_create_dir(sfp_msg_t *msg) {
    char full_path[512];
    build_path(full_path, msg->path);

    if (mkdir(full_path, 0777) == 0 || errno == EEXIST) {
        msg->type = SFP_DC_REP;
        printf("[SFSS] MKDIR OK: %s\n", msg->path);
    } else {
        msg->type = -1;
    }
}

void handle_remove(sfp_msg_t *msg) {
    char full_path[512];
    build_path(full_path, msg->path);

    if (rmdir(full_path) == 0) {
        msg->type = SFP_DR_REP;
        printf("[SFSS] RMDIR OK: %s\n", msg->path);
    } 
    else if (remove(full_path) == 0) {
        msg->type = SFP_DR_REP;
        printf("[SFSS] REMOVE FILE OK: %s\n", msg->path);
    } else {
        msg->type = -1;
    }
}

void handle_listdir(sfp_msg_t *msg) {
    char full_path[512];
    build_path(full_path, msg->path);

    DIR *d = opendir(full_path);
    if (!d) {
        msg->type = -1;
        return;
    }

    struct dirent *dir;
    int current_pos = 0;
    int count = 0;
    msg->allfilenames[0] = '\0'; 

    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
            continue;

        int len = strlen(dir->d_name);
        if (current_pos + len + 1 >= SFP_MAX_NAMES) break;

        strcat(msg->allfilenames, dir->d_name);
        strcat(msg->allfilenames, "|"); 

        msg->fstlstpositions[count].start = current_pos;
        msg->fstlstpositions[count].end = current_pos + len;
        
        current_pos += len + 1; 
        count++;
        if (count >= 64) break;
    }
    closedir(d);

    msg->nrnames = count;
    msg->type = SFP_DL_REP;
    printf("[SFSS] LISTDIR OK: %s (%d itens)\n", msg->path, count);
}

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    sfp_msg_t msg;

    if (argc != 2) {
        printf("Uso: %s <porta>\n", argv[0]);
        exit(1);
    }
    int port = atoi(argv[1]);

    mkdir(ROOT_DIR, 0777);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) exit(EXIT_FAILURE);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) exit(EXIT_FAILURE);

    printf("=== SFSS REAL Rodando na porta %d ===\n", port);
    printf("Armazenamento em: %s\n", ROOT_DIR);

    while (1) {
        len = sizeof(cliaddr);
        int n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cliaddr, &len);
        
        if (n > 0) {
            switch (msg.type) {
                case SFP_RD_REQ: handle_read(&msg); break;
                case SFP_WR_REQ: handle_write(&msg); break;
                case SFP_DC_REQ: handle_create_dir(&msg); break;
                case SFP_DR_REQ: handle_remove(&msg); break;
                case SFP_DL_REQ: handle_listdir(&msg); break;
                default: msg.type = -1;
            }
            sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cliaddr, len);
        }
    }
    return 0;
}
