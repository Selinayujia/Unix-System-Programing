/*
 Author @ Selina Zhang
 Chat Room
 Focus:
 Networking
 Concurrency
 Spec
 Application: A server allows multiple clients to join and leave a chat room
 */

#include <stdlib.h>         // exit
#include <stdio.h>          // printf, fprintf, stderr
#include <unistd.h>         // read
#include <string.h>         // memset
#include <sys/socket.h>     // socket, AF_INET, SOCK_STREAM
#include <arpa/inet.h>      // inet_pton
#include <netinet/in.h>     // servaddr
#include <errno.h>          // errno
#include <stdbool.h>

#define BUFFSIZE  8192    /* buffer size for reads and writes */
#define DFL_PORT 13000
#define DFL_IP "127.0.0.1"
#define MAX_CLIENT_USERNAME 20

bool quited = false;

void commandArgs(char** IPaddress, int* port_num, int argc, char** argv){
    if (argc < 4){
        *IPaddress = argv[2];
    }
    if (argc == 4){
        *port_num = strtol(argv[3], NULL, 10);
        if (errno != 0 && port_num == 0) {
            perror("strtol");
            exit(1);
        }
    }
        
}
void blockingListen(fd_set set, int sockfd, char* username){
    char buffer[BUFFSIZE];
    for(;;) {
        FD_SET(sockfd, &set);
        FD_SET(0, &set);
        int fd = select(FD_SETSIZE, &set, NULL, NULL, NULL);
        if (fd < 0) {
            perror("Failed to select: ");
            exit(1);
        }
        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET(fd, &set)) {
                if (fd) {
                    memset(&buffer, 0, sizeof(buffer));
                    if (read(fd, buffer, BUFFSIZE) < 0) {
                        perror("Failed to read from server: ");
                        exit(1);
                    }
                    printf("%s", buffer);
                }
                
                else if (fd == 0) {
                    memset(&buffer, 0, sizeof(buffer));
                    if (fgets(buffer, BUFFSIZE, stdin) == NULL) {
                        perror("Failed to get from to stdin: ");
                        exit(1);
                    }
                    if(strcmp(buffer,"quitchat\n") == 0){
    
                        quited = true;
                    }
                    char copybuf[BUFFSIZE];
                    snprintf(copybuf, BUFFSIZE, "%s: %s", username, buffer);
                    int len = strlen(copybuf);
                    if (len!= write(sockfd, copybuf, len)) {
                        perror("Failed to write to host: ");
                        exit(1);
                    }
                    if(quited){
                        exit(0);
                    }
                }
            }
        }
    
    }
    
}
void establishConn(char* username, char* IPaddress,int port_num){
    int sockfd;
    struct sockaddr_in servaddr;
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "socket error");
        exit(1);
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port_num);
    if (inet_pton(AF_INET, IPaddress, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error for: %s\n", IPaddress);
        exit(1);
    }
    
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("Connection error:");
        exit(1);
    }

    printf("Get connected to the server, to exit, type 'quitchat'\n");
    if(write(sockfd, username, strlen(username)) < 0){
        perror("Failed to write username to host: ");
        exit(1);
    }
    fd_set set;
    FD_ZERO(&set);
    blockingListen(set,sockfd,username);
    close(sockfd);
    
    
}

int main(int argc, char **argv) {
    char* username;
    char* IPaddress = DFL_IP;
    int port_num = DFL_PORT;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <Username [IPaddress] [Port number]>\n", argv[0]);
        exit(1);
    }
    username = argv[1];
    if(strlen(username) > MAX_CLIENT_USERNAME){
        fprintf(stderr, "Username cannot exceed 20 characters\n");
        exit(1);
    }
    if(argc > 2) commandArgs(&IPaddress,&port_num,argc, argv);
    establishConn(username, IPaddress, port_num);
    
    }



