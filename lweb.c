//A web-like server for browsing or downloading files.

//Created by arvin in 2015,4
//email:arvin.sfj@gmail.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>

#include <arpa/inet.h>
#include <ifaddrs.h>

#include <dirent.h>
#include <fcntl.h>

#define VERSION 0.1

#define BUFSIZE 8096

#define ERROR      42
#define NOTFOUND  404

#ifndef SIGCLD
# define SIGCLD SIGCHLD
#endif

void logger(int type, char *s1, char *s2, int socket_fd);

//cmd function
void listcmd(int socket_fd,char *buffer);
void stopcmd(int socket_fd,char *buffer);
void restartcmd(int socket_fd,char *buffer);
void downloadcmd(int socket_fd,char *buffer);

typedef void (*CMDFUNC)(int,char *);

struct {
    char *cmd;
    CMDFUNC func;
} cmds[]={
    {"list",listcmd},
    {"ls",listcmd},
    {"stop",stopcmd},
    {"quit",stopcmd},
    {"qt",stopcmd},
    {"exit",stopcmd},
    {"restart",restartcmd},
    {"rst",restartcmd},
    {0,downloadcmd}
};

struct {
    char *path;
    char *ip;
    char *port;
    char *dir;
    char *pid;
} curConfig = {
    "./lweb",
    "127.0.0.1",
    "80",
    "./",
    "-1"
};

void
listcmd(int socket_fd,char *buffer)
{
    struct dirent *pdir;
    DIR *dir=opendir(curConfig.dir);
    while ((pdir=readdir(dir))) {
        (void)write(socket_fd, curConfig.ip, strlen(curConfig.ip));
        (void)write(socket_fd, ":", 1);
        (void)write(socket_fd, curConfig.port, strlen(curConfig.port));
        (void)write(socket_fd, "/", 1);
        (void)write(socket_fd, pdir->d_name, strlen(pdir->d_name));
        (void)write(socket_fd, "\n", 1);
    }
    sleep(1);
    closedir(dir);
}

void
stopcmd(int socket_fd,char *buffer)
{
    kill(atoi(curConfig.pid),SIGKILL);
}

void
restartcmd(int socket_fd,char *buffer)
{
    kill(atoi(curConfig.pid),SIGKILL);
    if (fork()==0) {
        char *namebuffer=malloc(sizeof(char)*strlen(curConfig.path));
        char *portbuffer=malloc(sizeof(char)*10);
        char *name=namebuffer;
        char *path=curConfig.path;
        for (; *path; path++);
        for (; *path!='/'&&*path!='\\'; path--);
        for (path++; *path; *name++=*path++);
        (void)sprintf(portbuffer,"%d",atoi(curConfig.port)-1);
        if(execl(curConfig.path,namebuffer,portbuffer,curConfig.dir,NULL)<0){
            return;
        }
    }else{
        exit(3);
    }
}

void
downloadcmd(int socket_fd,char *buffer)
{
    int file_fd;
    if(( file_fd = open(buffer+5,O_RDONLY)) == -1) {
        logger(NOTFOUND, "failed to open file",buffer+5,socket_fd);
    }
    (void)sprintf(buffer,"HTTP/1.1 200 OK\nContent-Length: %ld\nConnection: close\n\n", (long)lseek(file_fd, (off_t)0, SEEK_END));
    (void)write(socket_fd,buffer,strlen(buffer));
    (void)lseek(file_fd, (off_t)0, SEEK_SET);
    for (long pack=0; (pack=read(file_fd, buffer, BUFSIZE)) > 0; write(socket_fd,buffer,pack));
    sleep(1);
    close(file_fd);
}

void
commander(CMDFUNC func, int socket_fd, char *buffer)
{
    (*func)(socket_fd,buffer);
    close(socket_fd);
    exit(3);
}

void
logger(int type, char *s1, char *s2, int socket_fd)
{
    char *logbuffer=malloc(sizeof(char)*BUFSIZE*2);
    switch (type) {
        case ERROR: (void)sprintf(logbuffer,"ERROR: %s:%s exiting pid=%d",s1,s2,getpid());
            (void)write(socket_fd,logbuffer,224);
            break;
        case NOTFOUND:
            (void)sprintf(logbuffer,"NOT FOUND: %s:%s",s1, s2);
            (void)write(socket_fd,logbuffer,224);
            break;
        default:
            break;
    }
    close(socket_fd);
    exit(3);
}

//below is the core.
void
worker(int fd)
{
    static char buffer[BUFSIZE+1];
    
    read(fd,buffer,BUFSIZE);
    if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) ) {
        logger(ERROR,"Only simple GET operation supported",buffer,fd);
    }
    for (char *ptmp=buffer+4; *ptmp!=0; *ptmp==' '?(*ptmp=0):(*ptmp++));
    if( !strcmp(buffer,"GET /") || !strcmp(buffer,"get /") ){
        (void)strcpy(buffer,"GET /list");
    }
    
    //work out the cmd type
    for(int i=0;;i++) {
        if(!cmds[i].cmd || !strcmp(buffer+5, cmds[i].cmd)) {
            commander(cmds[i].func,fd,buffer);
        }
    }
    //end cmd
}

int
main(int argc, char **argv)
{
    curConfig.path=argv[0];//absolute not null
    if( argc !=3 ) {
        (void)printf("\nhint: %s Port-Number Top-Directory\tversion %.1f\n\n"
                     "Example: %s 9897 ./ &\n\n"
                     , curConfig.path, VERSION, curConfig.path);
        exit(0);
    }else{
        //have logic warnning.not think about all solutions.
        curConfig.ip=malloc(sizeof(char)*INET_ADDRSTRLEN);
        struct ifaddrs *ifAddr;
        for (getifaddrs(&ifAddr); ifAddr; ifAddr = ifAddr->ifa_next) {
            if (ifAddr->ifa_addr->sa_family==AF_INET&&!strcmp(ifAddr->ifa_name, "en0")){
                inet_ntop(AF_INET, &((struct sockaddr_in *)ifAddr->ifa_addr)->sin_addr, curConfig.ip, INET_ADDRSTRLEN);
                break;
            }
        }
        curConfig.ip=curConfig.ip?:"127.0.0.1";
        curConfig.port=argv[1]?argv[1]:"80";
        curConfig.dir=argv[2]?argv[2]:"./";
        curConfig.pid=malloc(sizeof(char)*10);
        (void)sprintf(curConfig.pid,"%d",getpid());
        curConfig.pid=curConfig.pid?:"-1";
    }
    
    if(atoi(curConfig.port) < 0 || atoi(curConfig.port) > 60000){
        (void)printf("ERROR: Invalid port number (try 1->60000) %s\n",curConfig.port);
        exit(4);
    }
    
    if(chdir(curConfig.dir) == -1){
        (void)printf("ERROR: Can't Change to directory %s\n",curConfig.dir);
        exit(4);
    }
    
    (void)signal(SIGCLD, SIG_IGN);
    (void)signal(SIGHUP, SIG_IGN);
    for(int i=0;i<32;i++,close(i));
    (void)setpgrp();
    
    /* setup the network socket */
    int pid, listenfd, socketfd;
    static struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(curConfig.port));
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) <0){
        logger(ERROR, "system call","socket",0);
    }
    if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <0){
        logger(ERROR,"system call","bind",0);
    }
    if(listen(listenfd,64) <0){
        logger(ERROR,"system call","listen",0);
    }
    for(;;) {
        if((socketfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) < 0){
            logger(ERROR,"system call","accept",0);
        }
        if((pid = fork()) < 0) {
            logger(ERROR,"system call","fork",0);
        } else {
            if(pid == 0) {//child
                (void)close(listenfd);
                worker(socketfd);
            } else {//parent
                (void)close(socketfd);
            }
        }
    }
}



