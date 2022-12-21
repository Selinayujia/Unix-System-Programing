#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/time.h> //select

#define MAXLINE 4096
#define PORT 13000
#define LISTENQ 1024

void initializeSocket (int* socketfd, struct sockaddr_in* serveraddr, int port);
void getUserInput(int socketfd, char* username);
bool getClientMessage(int connectionfd);

int main(int argc, char* argv[]){
	int port = PORT;
    char* username;

    int socketfd;
    struct sockaddr_in serveraddr;
    
    int fd;
    fd_set set;
    FD_ZERO(&set);
    
    int connectionfd;

    if (argc > 3 || argc == 1)
    {
        fprintf(stderr, "Usage: server username [port]\n");
        exit(1);
    }
    else
    {
        username = argv[1];

        if (argc == 3)
        {
            port = strtol(argv[2], NULL, 10);
        }
    }
    
    initializeSocket(&socketfd, &serveraddr, port);
    
    while (true)
    {
        connectionfd = accept(socketfd, NULL, NULL);
        if (connectionfd < 0)
        {
            perror("accept() failed");
            exit(1);
        }
        printf("Connection established.\n");
        
        while (true)
        {
            FD_SET(STDIN_FILENO, &set);
            FD_SET(connectionfd, &set);
            
            fd = select(FD_SETSIZE, &set, NULL, NULL, NULL);
            
            if (fd < 0)
            {
                perror("select() failed");
                exit(1);
            }
            
            if (FD_ISSET(STDIN_FILENO, &set))
            {
                getUserInput(connectionfd, username);
            }
            else if (FD_ISSET(connectionfd, &set))
            {
                if (!getClientMessage(connectionfd))
                {
                    break;
                }
            }
        }
    }
}

void initializeSocket (int* socketfd, struct sockaddr_in* serveraddr, int port)
{
    if (((*socketfd) = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed");
        exit(1);
    }

    memset(serveraddr, 0, sizeof(*serveraddr));
    (*serveraddr).sin_family = AF_INET;
    (*serveraddr).sin_addr.s_addr = htonl(INADDR_ANY);
    (*serveraddr).sin_port = htons(port);

    if ((bind((*socketfd), (struct sockaddr*) serveraddr, sizeof(*serveraddr))) < 0)
    {
        perror("bind() failed");
        exit(1);
    }

    if ((listen((*socketfd), LISTENQ)) < 0)
    {
        perror("listen() failed");
        exit(1);
    }
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

bool getClientMessage(int connectionfd)
{
    char message[MAXLINE];
    memset(message, 0, MAXLINE);
    int read_length = read(connectionfd, message, MAXLINE);
    if (read_length < 0) {
        perror("read() failed");
        exit(1);
    }
    else if (read_length == 0)
    {
        printf("Client disconnected\n");
        return false;
    }
    else
    {
        puts(message);
        return true;
    }
}
