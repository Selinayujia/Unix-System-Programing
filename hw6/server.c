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
#include <stdio.h>       // perror, snprintf
#include <unistd.h>      // close, write
#include <string.h>      // strlen
#include <stdbool.h>     // bool
#include <sys/socket.h>  // socket, AF_INET, SOCK_STREAM, bind, listen, accept
#include <netinet/in.h>  // servaddr, INADDR_ANY, htons
#include <errno.h>       // errno
#include <pthread.h>     // pthread_create pthread_detach
#define BUFFSIZE 8192    // buffer size for reads and writes
#define LISTENQ  1024    // 2nd argument to listen()
#define DFL_PORT 13000
#define MAX_CLIENT_NUM 10
#define MAX_MESSAGE_NUM 20
#define MAX_CLIENT_USERNAME 20

typedef struct {
    int fd;            // message source
    pthread_t tid;  // the corresponding thread_id for each client, used for detach
    bool exited;           // if the client thread is done, used for detach
    char name[MAX_CLIENT_USERNAME + 1];    // name of the client
    
}client;


client* clients[MAX_CLIENT_NUM];
char* messages[MAX_MESSAGE_NUM];
pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t messages_lock  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_counter = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t message_counter  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condc = PTHREAD_COND_INITIALIZER;
pthread_cond_t condp = PTHREAD_COND_INITIALIZER;
int client_num = 0;
int message_num = 0;


void establishConn(char* username, int port_num); //server function to establish connection with clients
void* server_consume(void* param); // thread function for the server
void* client_produce(void* fd);// thread function for clients
void client_detach_check(); // check if client threads terminated and detach them
void akCurrMembers(client* client); // tell new joined memmber who is already in chatroom
void clientInit(client* client, int clientfd); // initialize the client
void addClient(client* client);  // add new client to clients
void notifyOldClients(client* client, char* mode);  // tell the old chatroom members who is here
void addMessage(char* message);  // add the message to the messages
void removeClient(client* client);  // remove a client from chatroom



void clientInit(client* client, int clientfd){ // Initialization of the client obj
    client->fd = clientfd;
    char name[MAX_CLIENT_USERNAME + 1];
    memset(name, 0, sizeof(name));
    if (read(clientfd, name, MAX_CLIENT_USERNAME + 1) < 0){
        perror("Failed to read name from client");
        client->exited = true;
        pthread_exit(&errno);
    }
    strcpy(client->name, name);
    client->tid = pthread_self();
    client->exited = false;
}

void notifyOldClients(client* client, char* mode){
    char buffer[BUFFSIZE];
    memset(buffer, 0, sizeof(buffer));
    strcat(buffer, client->name);
    if(strcmp(mode, "join") == 0){
        strcat(buffer, " has joined, say hi~\n");
    }
    else{  // leave
        strcat(buffer, " has left\n");
    }
    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < MAX_CLIENT_NUM; i++) {
        if (clients[i] && clients[i] != client && clients[i]->exited != true) {
            if (write(clients[i]->fd, buffer, strlen(buffer)) < 0) {
                perror("Failed to notify old clients");
            }
        }
    }
    pthread_mutex_unlock(&clients_lock);
    
}

void akCurrMembers(client* client){
    char buffer[BUFFSIZE];
    memset(buffer, 0, sizeof(buffer));
    strcat(buffer, "People in the chat room: ");
    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < MAX_CLIENT_NUM; i++) {
        if (clients[i]) {
            strcat(buffer, clients[i]->name);
            strcat(buffer, " ");
        }
    }
    strcat(buffer, "\n");
    if (write(client->fd, buffer, strlen(buffer)) < 0) {
        perror("Failed to aknowledge current client");
    }
    pthread_mutex_unlock(&clients_lock);
}

void addMessage(char* message){
    pthread_mutex_lock(&messages_lock);
    for (int i = 0; i < MAX_MESSAGE_NUM; i++) {
        if (!messages[i]) {
            messages[i] = (char*)malloc(sizeof(char) * strlen(message));
            if(messages[i] == NULL){
                perror("Failed to malloc");
            }
            strcpy(messages[i], message);
            break;
        }
    }
    pthread_mutex_unlock(&messages_lock);
}
void addClient(client* client){ // If initialization success, then officially add this client
    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < MAX_CLIENT_NUM; i++) {
        if (!clients[i]) {
            clients[i] = client;
            break;
        }
    }
    pthread_mutex_unlock(&clients_lock);
    pthread_mutex_lock(&client_counter);
    client_num++;
    pthread_mutex_unlock(&client_counter);
    akCurrMembers(client);
}
void removeClient(client* client){
    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < MAX_CLIENT_NUM; i++) {
        if (clients[i] == client) {
            clients[i]->exited = true;
            break;
        }
    }
    pthread_mutex_unlock(&clients_lock);
    pthread_mutex_lock(&client_counter);
    client_num--;
    pthread_mutex_unlock(&client_counter);
}

void* client_produce(void* fd){
    int clientfd = *((int*)fd);
    client* newclient = (client*)malloc(sizeof(client));
    if(newclient == NULL){
        perror("Failed to malloc");
        pthread_exit(&errno);
    }
    clientInit(newclient, clientfd);
    printf("%s joined!\n",newclient->name);
    addClient(newclient);
    notifyOldClients(newclient, "join");
    char buffer[BUFFSIZE];
    memset(buffer, 0, sizeof(buffer));
    while (read(clientfd, buffer, sizeof(buffer)) > 0) {
        if (strstr(buffer, "quitchat\n") != NULL) {
            break;
        }
        pthread_mutex_lock(&message_counter);
        while (message_num == MAX_MESSAGE_NUM) {
            pthread_cond_wait(&condp, &message_counter);
        }
        addMessage(buffer);
        memset(buffer, 0, sizeof(buffer));
        message_num++;
        pthread_cond_signal(&condc);
        pthread_mutex_unlock(&message_counter);
    }
    
    // At this point, the client left due to either quitchat or signals
    removeClient(newclient);
    notifyOldClients(newclient, "leave");
    printf("%s left\n",newclient->name);
    if (close(clientfd) < 0 ) {
        perror("Failed to close socket");
    }
    pthread_exit(0);
}

void* server_consume(void* param){
    for(; ;){
        pthread_mutex_lock(&message_counter);
        while (message_num == 0){
            pthread_cond_wait(&condc, &message_counter);
        }
        pthread_mutex_lock(&messages_lock);
        for (int i = 0; i < MAX_MESSAGE_NUM; i++) {
            if (messages[i]) {
                for(int j = 0; j < MAX_CLIENT_NUM; j++){
                    if(clients[j] && !clients[j]->exited){
                        if (write(clients[j]->fd, messages[i], strlen(messages[i])) < 0) {
                            perror("Failed to write clients the new message");
                        }
                    }
                }
                free(messages[i]);
                messages[i] = NULL;
                message_num --;
            }
        }
        pthread_mutex_unlock(&messages_lock);
        pthread_cond_signal(&condp);
        pthread_mutex_unlock(&message_counter);
    }
    pthread_exit(0);
    
}

void client_detach_check(){
     for (int i = 0; i < MAX_CLIENT_NUM; i++) {
         if(clients[i] && clients[i]->exited == true){
             pthread_detach(clients[i]->tid);
         }
     }
}

void establishConn(char* username, int port_num){
    int listenfd, connfd;
    struct sockaddr_in servaddr;
    
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to open socket");
        exit(1);
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port_num);
    
    if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
        perror("Failed to bind to socket ");
        exit(1);
    }
    if (listen(listenfd, LISTENQ) < 0) {
        perror("Failed to listen");
        exit(1);
    }
    printf("Ready for Connections\n");
    pthread_t server_t;
    pthread_create(&server_t, NULL, server_consume, NULL); // server starts a thread to wait for clients' inputs
    
    for ( ; ; ) {  //waiting for connections
        client_detach_check();
        if (( connfd = accept(listenfd, NULL, NULL)) < 0){
            perror("Failed to accept the incoming connection");
            exit(1);
        }
        
        if(client_num == MAX_CLIENT_NUM){
            char* message = "Chat room reaches its maximum capacity. Try again later!\n";
            if (write(connfd, message, strlen(message)) < 0) {
                perror("Failed to write to client: ");
            }
            close(connfd);
            continue;  // do not create a thread
        }
        pthread_t client_t;
        pthread_create(&client_t, NULL, client_produce, &connfd); // a client starts a thread to potentially write to server
    }
     for (int i = 0; i < MAX_CLIENT_NUM; i++) {
         if(clients[i]){
             clients[i] = NULL;
             free(clients[i]);
         }
     }
    close(connfd);
    
    
}

int main(int argc, char **argv) {
    char* username;
    int port_num = DFL_PORT;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <Username [Port number]>\n", argv[0]);
        exit(1);
    }
    username = argv[1];
    if(argc == 3){
        port_num = strtol(argv[2], NULL, 10);
        if (errno != 0 && port_num == 0) {  // make sure it is a valid port number
            perror("strtol");
            exit(1);
        }
    }
    establishConn(username, port_num);
}
