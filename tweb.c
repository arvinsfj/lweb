#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>

/*Service Logic Interface Define*/
#define SERVER_STRING "Server: tweb/0.1.0\r\n"
void worker(int client_sock);
int read_line(int client_sock, char *buf, int size);
void serve_file(int client_sock, const char *file);
void serve_cgi(int client_sock, const char *file, const char *method, const char *query_str);
void error_request(int client_sock, int error_code);

void error_request(int client_sock, int error_code)
{
    char buf[1024];
    
    switch (error_code) {
        case 400:{
            //bad request
            sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
            send(client_sock, buf, sizeof(buf), 0);
            sprintf(buf, "Content-type: text/html\r\n");
            send(client_sock, buf, sizeof(buf), 0);
            sprintf(buf, "\r\n");
            send(client_sock, buf, sizeof(buf), 0);
            sprintf(buf, "<P>Your browser sent a bad request, ");
            send(client_sock, buf, sizeof(buf), 0);
            sprintf(buf, "such as a POST without a Content-Length.\r\n");
            send(client_sock, buf, sizeof(buf), 0);
            break;
        }
        case 404:{
            //not found
            sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, SERVER_STRING);
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "Content-Type: text/html\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "your request because the resource specified\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "is unavailable or nonexistent.\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "</BODY></HTML>\r\n");
            send(client_sock, buf, strlen(buf), 0);
            break;
        }
        case 500:{
            //internal server error
            sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "Content-type: text/html\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
            send(client_sock, buf, strlen(buf), 0);
            break;
        }
        case 501:{
            //method not implemented
            sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, SERVER_STRING);
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "Content-Type: text/html\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "</TITLE></HEAD>\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
            send(client_sock, buf, strlen(buf), 0);
            sprintf(buf, "</BODY></HTML>\r\n");
            send(client_sock, buf, strlen(buf), 0);
            break;
        }
        default:
            break;
    }
}

int read_line(int client_sock, char *buf, int size)
{
    int i = 0, n = 0;
    char c = '\0';
    
    while (i < size - 1 && c != '\n') {
        n = recv(client_sock, &c, 1, 0);
        if (n > 0) {
            if (c == '\r') {
                n = recv(client_sock, &c, 1, MSG_PEEK);
                if (n > 0 && c =='\n') {
                    recv(client_sock, &c, 1, 0);
                }else{
                    c = '\n';
                }
            }
            buf[i++] = c;
        }else{
            c = '\n';
        }
    }
    buf[i] = '\0';
    
    return i;
}

void serve_file(int client_sock, const char *file)
{
    FILE *fd = NULL;
    int num = 1;
    char buf[1024] = {'A', '\0'};

    while (num > 0 && strcmp("\n", buf)) {
        num = read_line(client_sock, buf, sizeof(buf));
    }
    
    fd = fopen(file, "r");
    if (fd < 0) {
        perror(file);
        error_request(client_sock, 404);
    }else{
        /*send header*/
        char buf[1024];
        strcpy(buf, "HTTP/1.0 200 OK\r\n");
        send(client_sock, buf, strlen(buf), 0);
        strcpy(buf, SERVER_STRING);
        send(client_sock, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: text/html\r\n");
        send(client_sock, buf, strlen(buf), 0);
        strcpy(buf, "\r\n");
        send(client_sock, buf, strlen(buf), 0);
        /*send file*/
        fgets(buf, sizeof(buf), fd);
        while (!feof(fd))
        {
            send(client_sock, buf, strlen(buf), 0);
            fgets(buf, sizeof(buf), fd);
        }
    }
    fclose(fd);
}

void serve_cgi(int client_sock, const char *file, const char *method, const char *query_str)
{
    char buf[1024] = {'A', '\0'};
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status, i, num = 1, cttlen = -1;
    char c;
    if (!strcasecmp(method, "GET")) {
        while ((num > 0) && strcmp("\n", buf)) {
            num = read_line(client_sock, buf, sizeof(buf));
        }
    }else{
        num = read_line(client_sock, buf, sizeof(buf));
        while ((num > 0) && strcmp("\n", buf)) {
            buf[15] = '\0';
            if (!strcasecmp(buf, "Content-Length:")) {
                cttlen = atoi(&(buf[16]));
            }
            num = read_line(client_sock, buf, sizeof(buf));
        }
        if (cttlen == -1) {
            error_request(client_sock, 400);
            return;
        }
    }
    
    /*send header*/
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client_sock, buf, sizeof(buf), 0);
    
    if (pipe(cgi_output) < 0) {
        error_request(client_sock, 500);
        return;
    }
    if (pipe(cgi_input) < 0) {
        error_request(client_sock, 500);
        return;
    }
    if ((pid = fork()) < 0) {//error
        error_request(client_sock, 500);
        return;
    }else if (pid == 0) {//child: CGI SCRIPT RUN
        //
        char meth_env[255];
        char query_env[255];
        char length_env[255];
        
        dup2(cgi_output[1], 1);
        dup2(cgi_input[0], 0);
        close(cgi_output[0]);
        close(cgi_input[1]);
        
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        
        if (!strcasecmp(method, "GET")) {
            sprintf(query_env, "QUERY_STRING=%s", query_str);
            putenv(query_env);
        }else{
            sprintf(length_env, "CONTENT_LENGTH=%d", cttlen);
            putenv(length_env);
        }
        execl(file, file, NULL);
        exit(0);
        
    }else{//parent
        //
        close(cgi_output[1]);
        close(cgi_input[0]);
        
        if (!strcasecmp(method, "POST")) {
            for (i = 0; i < cttlen; i++) {
                recv(client_sock, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        }
        while (read(cgi_output[0], &c, 1) > 0) {
            send(client_sock, &c, 1, 0);
        }
        
        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);
    }
}

void worker(int client_sock)
{
    char buf[1024];
    char method[255];
    char url[255];
    char path[512];
    int num;
    long i = 0, j = 0;
    struct stat sta;
    int cgi = 0;
    char *query_str = NULL;
    
    num = read_line(client_sock, buf, sizeof(buf));
    while (!isspace(buf[j]) && (i < sizeof(method) - 1)) {
        method[i++] = buf[j++];
    }
    method[i] = '\0';
    
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
        error_request(client_sock, 501);
        return;
    }
    
    if (!strcasecmp(method, "POST")) {
        cgi = 1;
    }
    
    i = 0;
    while (isspace(buf[j]) && (j < sizeof(buf))) {
        j++;
    }
    while (!isspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
        url[i++] = buf[j++];
    }
    url[i] = '\0';
    
    if (!strcasecmp(method, "GET")) {
        query_str = url;
        while ((*query_str != '?') && (*query_str != '\0')) {
            query_str++;
        }
        if (*query_str == '?') {
            cgi = 1;
            *query_str = '\0';
            query_str++;
        }
    }
    
    sprintf(path, "htdocs%s", url);
    if (path[strlen(path) - 1] == '/') {
        strcat(path, "index.html");
    }
    if (stat(path, &sta) == -1) {
        while (num > 0 && strcmp("\n", buf)) {
            num = read_line(client_sock, buf, sizeof(buf));
        }
        error_request(client_sock, 404);
    }else{
        if ((sta.st_mode & S_IFMT) == S_IFDIR){
            strcat(path, "/index.html");
        }
        if ((sta.st_mode & S_IXUSR) ||
            (sta.st_mode & S_IXGRP) ||
            (sta.st_mode & S_IXOTH) ){
            cgi = 1;
        }
        if (!cgi) {
            serve_file(client_sock, path);
        }else{
            serve_cgi(client_sock, path, method, query_str);
        }
    }
    
    close(client_sock);
}


/*Server Interface Define*/
int startup(short *);
void workloop(int);
void error_die(const char *);

void error_die(const char *info)
{
	perror(info);
	exit(1);
}

int startup(short *port)
{	
	int server_sock = -1;
	struct sockaddr_in server_addr;
	
	server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_sock == -1){
		error_die("socket");
	}
	
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(*port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		error_die("bind");
	}

	if (*port == 0){
        int server_addr_len = sizeof(server_addr);
		if (getsockname(server_sock, (struct sockaddr *)&server_addr, &server_addr_len) == -1){
            error_die("getsockname");
		}
        *port = ntohs(server_addr.sin_port);
	}
	if (listen(server_sock, 5) < 0){
		error_die("listen");
	}

	return server_sock;
}

void workloop(int server_sock)
{
	int client_sock = -1;
	struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
	pthread_t work_thread;
    
	while(1){
		client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
		if (client_sock == -1){
			error_die("accept");
		}
        if (pthread_create(&work_thread, NULL, worker, client_sock) != 0){
			perror("pthread_create");
		}
	}
}

/*Program Interface: main*/
int main(int ac, char **av)
{
	char *port;
	int server_sock = -1, server_port = 0;
    
    if (ac > 2) {
        fprintf(stderr, "usage: tweb [port]");
        exit(1);
    }

	if (ac == 2){
		port = av[1];
        server_port = atoi(port);
	}
	
	server_sock = startup((short *)&server_port);
    
	printf("tweb running on port %d\n", server_port);
	
	workloop(server_sock);

	close(server_sock);
    
	return 0;
}

