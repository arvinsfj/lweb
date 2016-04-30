
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// handler class begin
/*class private method*/
void i_mode(int sock)
{
    char c;
    while (!feof(stdin)) {
        c = fgetc(stdin);
        write(sock, &c, 1);
    }
}

void o_mode(int sock)
{
    char c;
    while (read(sock, &c, 1) > 0) {
        fputc(c, stdout);
    }
}

void io_mode(int sock)
{
    i_mode(sock);
    o_mode(sock);
}

void oi_mode(int sock)
{
    o_mode(sock);
    i_mode(sock);
}

/*class private var*/
struct {
    char *mode;
    void (*func)(int);
    
} cmds[] = {
    {"-si", i_mode},
    {"-so", o_mode},
    {"-ci", i_mode},
    {"-co", o_mode},
    {"-cio", io_mode},
    {"NULL", NULL}
};

/*class public method*/
//api - find handler and exec
void io_handler(int sock, const char *mode)
{
    for (int i = 0; cmds[i].func; i++) {
        if (!strcmp(mode, cmds[i].mode)) {
            (*(cmds[i].func))(sock);
            break;
        }
    }
}

//api - find mode, if the mode is finded return true, or false.
int mode_find(const char *findmode, const char *mode)
{
    for (int i = 0; cmds[i].func; i++) {
        if (!strcmp(mode, cmds[i].mode) && strcmp(mode, findmode) > 0) {
            return 1;
        }
    }
    return 0;
}
// handler class end


//client mode
void client(char *ip, char *port, char *mode)
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
    
    io_handler(sock, mode);
    
    close(sock);
}

//server mode
void server(char *port, char *mode)
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
        
        io_handler(sock_client, mode);
        
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
    
    if (ac != 3 && ac != 4) {
        fprintf(stderr, "usage: netcsp <mode> [ip] <port> [< or >] [filename]\n");
        exit(1);
    }
    
    //client mode
    if (mode_find("-c", mode) && ac == 4) {
        ip = av[2];
        port = av[3];
        client(ip, port, mode);
    }
    //server mode
    mode = av[1];
    if (mode_find("-s", mode) && ac == 3) {
        ip = NULL;
        port = av[2];
        server(port, mode);
    }
    
    return 0;
}







