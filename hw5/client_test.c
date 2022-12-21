#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <sys/select.h>
#include <sys/time.h> //select
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 13000
#define MAXLINE 4096

void initializeSocket (int* socketfd, struct sockaddr_in* clientaddr, char* IPaddress, int port);
void getServerMessage(int socketfd, fd_set set);
void getUserInput(int socketfd, char* username);

int main(int argc, char* argv[]){
	int port = PORT;
    char* username;
    char* IPaddress;

    int socketfd;
    struct sockaddr_in clientaddr;
    
    int fd;
    fd_set set;
    FD_ZERO(&set);

    if (argc > 4 || argc < 3)
    {
        fprintf(stderr, "Usage: client username IP_address [Port number]\n");
        exit(1);
    }
    else
    {
        username = argv[1];
        IPaddress = argv[2];

        if (argc == 4)
        {
            port = strtol(argv[3], NULL, 10);
        }
    }
    
    initializeSocket(&socketfd, &clientaddr, IPaddress, port);
  
    while(true)
    {
        FD_SET(STDIN_FILENO, &set);
        FD_SET(socketfd, &set);

        fd = select(socketfd + 1, &set, NULL, NULL, NULL);

        if (fd < 0)
        {
            perror("select() failed");
            exit(1);
        }

        if (FD_ISSET(STDIN_FILENO, &set))
        {
            getUserInput(socketfd, username);
        }
        else if (FD_ISSET(socketfd, &set))
        {
            getServerMessage(socketfd, set);
        }
    }
}

void initializeSocket (int* socketfd, struct sockaddr_in* clientaddr, char* IPaddress, int port)
{
    if (((*socketfd) = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed");
        exit(1);
    }

    memset(clientaddr, 0, sizeof(*clientaddr));
    (*clientaddr).sin_family = AF_INET;
    (*clientaddr).sin_port = htons(port);

    if((inet_pton(AF_INET, IPaddress, &((*clientaddr).sin_addr))) <= 0)
    {
        fprintf(stderr, "inet_pton() failed for %s\n", IPaddress);
        exit(1);
    }

    if ((connect((*socketfd), (struct sockaddr*) clientaddr, sizeof(*clientaddr))) < 0)
    {
        perror("connect() failed");
        exit(1);
    }
}

void getServerMessage(int socketfd, fd_set set)
{
    char message[MAXLINE];
    memset(message, 0, MAXLINE);
    if (read(socketfd, message, MAXLINE) < 0) {
        perror("read() failed");
        exit(1);
    }
    puts(message);
}

void getUserInput(int socketfd, char* username)
{
    char input[MAXLINE];
    char message[MAXLINE];
    memset(message, 0, MAXLINE);
    memset(input, 0, MAXLINE);
    if (fgets(input, MAXLINE, stdin) == NULL)
    {
        perror("fgets() failed");
        exit(1);
    }
    input[strlen(input) - 1] = '\0';
    snprintf(message, strlen(username) + 1, "%s", username);
    snprintf(message + strlen(message), 4, ": ");
    snprintf(message + strlen(message), MAXLINE - strlen(username) - 3, "%s", input);
    
    int message_length = strlen(message);
    int written_length = write(socketfd, message, message_length);
    
    if (written_length < 0)
    {
        perror("write() failed");
        exit(1);
    }
    else if (written_length != message_length)
    {
        perror("write() malfunctioned");
    }
}
