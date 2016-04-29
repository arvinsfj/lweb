
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

char* fileread(const char *filename, unsigned long *length)
{
    *length = 0;
    FILE *fd;
    if ((fd = fopen(filename, "rb")) == NULL) {
        printf("file %s can not open\n", filename);
        exit(1);
    }
    fseek(fd, 0, SEEK_END);
    *length = ftell(fd);
    rewind(fd);
    char *ctt = malloc(*length);
    memset(ctt, '\0', *length);
    
    *length = fread(ctt, 1, *length, fd);
    
    fclose(fd);
    
    return ctt;
}

void filewrite(const char *filename, const char *data, const unsigned long length)
{
    FILE *fd;
    if ((fd = fopen(filename, "wb")) == NULL) {
        printf("file %s can not open\n", filename);
        exit(1);
    }
    
    fwrite(data, 1, length, fd);
    
    fclose(fd);
}

char* netread(int sock, unsigned long *length)
{
    *length = 0;
    const unsigned long addsize = sizeof(char) * 1024 * 1024; // 1mb
    unsigned long size = addsize;
    char *ctt = malloc(size);
    memset(ctt, '\0', size);
    char c;
    char *cttp = ctt;
    int num = 0;
    while ((num = read(sock, &c, 1)) > 0) {
        if (*length >= size) {
            size += addsize;
            char *cttr = realloc(ctt, size);
            if (cttr == NULL) {
                return ctt;
            }
            if (ctt != cttr) {
                cttp = cttr + (size - addsize);
                ctt = cttr;
            }
        }
        (*length)++;
        *cttp++ = c;
    }
    return ctt;
}

void client(char *ip, char *port, char *file, char *mode)
{
    int sock = -1;
    struct sockaddr_in addr;
    
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0){
        perror("socket");
        exit(1);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr.s_addr = inet_addr(ip);
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("connet");
        exit(1);
    }
    
    if (!strcmp(mode, "-ci")) {
        unsigned long cttlen = 0;
        char *ctt = fileread(file, &cttlen);
        if (cttlen > 0) {
            write(sock, ctt, cttlen);
        }
    }
    
    if (!strcmp(mode, "-co")) {
        unsigned long cttlen = 0;
        char *ctt = netread(sock, &cttlen);
        if (cttlen > 0) {
            filewrite(file, ctt, cttlen);
        }
    }
    
    if (!strcmp(mode, "-cx")) {
        unsigned long cttlen = 0;
        char *ctt = fileread(file, &cttlen);
        if (cttlen > 0) {
            write(sock, ctt, cttlen);
        }
        free(ctt);
        ctt = netread(sock, &cttlen);
        if (cttlen > 0) {
            int size = strlen(file) + 10;
            char *outfile = malloc(size);
            memset(outfile, 0, size);
            strcat(outfile, "O_");
            strcat(outfile, file);
            filewrite(outfile, ctt, cttlen);
        }
    }
    
    close(sock);
}

void server(char *port, char *file, char *mode)
{
    int sock_server = -1;
    struct sockaddr_in server_addr;
    int sock_client = -1;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = 0;
    
    sock_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_server < 0){
        perror("socket");
        exit(1);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(sock_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("bind");
        exit(1);
    }
    
    if (listen(sock_server, 5) < 0){
        perror("listen");
        exit(1);
    }
    
    int once = 1;
    while(once){
        sock_client = accept(sock_server, (struct sockaddr *)&client_addr, &client_addr_len);
        if (sock_client < 0){
            perror("accept");
            exit(1);
        }
        
        if (!strcmp(mode, "-si")) {
            unsigned long cttlen = 0;
            char *ctt = fileread(file, &cttlen);
            if (cttlen > 0) {
                write(sock_client, ctt, cttlen);
            }
        }
        
        if (!strcmp(mode, "-so")) {
            unsigned long cttlen = 0;
            char *ctt = netread(sock_client, &cttlen);
            if (cttlen > 0) {
                filewrite(file, ctt, cttlen);
            }
        }
        
        if (!strcmp(mode, "-sx")) {
            unsigned long cttlen = 0;
            char *ctt = fileread(file, &cttlen);
            if (cttlen > 0) {
                write(sock_client, ctt, cttlen);
            }
            free(ctt);
            ctt = netread(sock_client, &cttlen);
            if (cttlen > 0) {
                int size = strlen(file) + 10;
                char *outfile = malloc(size);
                memset(outfile, 0, size);
                strcat(outfile, "O_");
                strcat(outfile, file);
                filewrite(outfile, ctt, cttlen);
            }
        }
        
        close(sock_client);
        once = 0;
    }
    
    close(sock_server);
}


int main(int ac, char **av)
{
    char *mode;
    char *ip;
    char *port;
    char *filename;
    
    if (ac != 4 && ac != 5) {
        fprintf(stderr, "usage: netcs <mode> [ip] <port> <filename>\n");
        exit(1);
    }
    
    if (ac == 4) {
        //server mode
        mode = av[1];
        if (!strcmp(mode, "-si") || !strcmp(mode, "-so") || !strcmp(mode, "-sx")) {
            ip = NULL;
            port = av[2];
            filename = av[3];
            server(port, filename, mode);
        }
    }
    
    if (ac == 5) {
        //client mode
        mode = av[1];
        if (!strcmp(mode, "-ci") || !strcmp(mode, "-co") || !strcmp(mode, "-cx")) {
            ip = av[2];
            port = av[3];
            filename = av[4];
            client(ip, port, filename, mode);
        }
    }
    
    return 0;
}







