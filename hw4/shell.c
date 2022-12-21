/*
 Implement of a simple shell.
 
 I/O redirection (<, >, >>, 2>)
 Handle of SIGINT, SIGQUIT
 cd and exit
 exec regular commands
 author @ Selina Zhang
 */
#define _GNU_SOURCE
#include <stdio.h>       // printf, perror, getline
#include <stdlib.h>      // exit
#include <string.h>      // strcat, strcpy, strtok, strlen
#include <unistd.h>      // getcwd fork
#include <stdbool.h>     // boolean
#include <sys/wait.h>   // wait
#include <signal.h>     // signal
#include <setjmp.h>  // setsigjmp() setlongjmp()
#include <fcntl.h> //open()

#define INIT_CAP 10
volatile bool jmp_flag = false;  // handle if the signal sent before we got to sigsetjmp
sigjmp_buf buf;

void signalSetting();
void handler(int signalNum);
void setChildsigaction();
char** parseIo(int argc, char** argv);
bool processAndTok(char* input);
void execute(int argc, char** argv);

int main(int argc, char* argv[]){
    signalSetting();    // set the handler for sigint and sigquit
    if(argc > 1){
        printf("Usage: %s\n", argv[0]);
        exit(1);
    }
    char* ps1 = getenv("PS1");    // if ps1 set in env, get that instead
    if(ps1 == NULL) ps1 = "myShell$ ";
    ssize_t nread;
    char* line = NULL;
    size_t len = 0;
    sigsetjmp(buf, 1);    //the point to jump back after sigint and sigquit, restore handler
    jmp_flag = true;
    printf("%s ", ps1);
    while((nread = getline(&line, &len, stdin)) != -1 ){
        bool cont = processAndTok(line);
        if(!cont){ //exit get called
            free(line);
            exit(0);
        }
        printf("%s ", ps1);
    }
}

void handler(int signalNum) {
    printf("\n");
    if(!jmp_flag) return;  // if not set jump yet, just return
    siglongjmp(buf, 1); //jump back to the point we set
}
void signalSetting(){
    struct sigaction action;
    
    sigset_t block;
    sigemptyset (&block);
    sigaddset (&block, SIGINT);
    sigaddset (&block, SIGQUIT);
    action.sa_mask = block;
    action.sa_handler = handler;
    action.sa_flags = SA_RESTART;  // making certain system calls restartable across signals.
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGQUIT, &action, NULL);
}

void setChildsigaction(){
    struct sigaction sigintAction;
    struct sigaction sigquitAction;
    sigintAction.sa_handler = SIG_DFL;  //want child to ended from sigint and sigquit, so set back dfl
    sigquitAction.sa_handler = SIG_DFL;
    sigaction( SIGINT, &sigintAction, NULL);
    sigaction( SIGQUIT, &sigquitAction, NULL);
}
char** parseIo(int argc, char** argv){
    int number_cmd = 0;
    for (int i = 0; i < argc; ++i){
        if (argv[i] != NULL){  // if not the end or not just set to null
            if (strcmp(argv[i], ">") == 0){  // trunc and write
                if (!argv[i+1]){
                    fprintf(stderr, "syntax error near unexpected token");
                    exit(1);
                }
                else{
                    int fd = open(argv[i+1], O_CREAT | O_WRONLY | O_TRUNC, 666); //replace with o_trunc
                    if (fd < 0){
                        perror("");
                        exit(1);
                    }
                    dup2(fd, 1);
                    close(fd);
                    argv[i] = NULL;
                    argv[i+1] = NULL;
                   
                }
            }
            else if (strcmp(argv[i], ">>") == 0){ //append
                if (!argv[i+1]){
                    fprintf(stderr, "syntax error near unexpected token");
                    exit(1);
                }
                else{
                    int fd = open(argv[i+1], O_RDWR | O_APPEND); //append with o_append
                    if (fd < 0){
                        perror("");
                        exit(1);
                    }
                    dup2(fd, 1);
                    close(fd);
                    argv[i] = NULL;
                    argv[i+1] = NULL;
                }
            }
            else if (strcmp(argv[i], "2>") == 0){  //append to stderr
                if (!argv[i+1]){
                    fprintf(stderr, "syntax error near unexpected token");
                    exit(1);
                }
                else{
                    int fd = open(argv[i+1], O_CREAT | O_WRONLY | O_TRUNC, 666);
                    if (fd < 0){
                        perror("");
                        exit(1);
                    }
                    dup2(fd, 2);
                    close(fd);
                    argv[i] = NULL;
                    argv[i+1] = NULL;
                }
            }
            else if (strcmp(argv[i], "<") == 0){ //read to stdin
                if (!argv[i+1]){
                    fprintf(stderr, "syntax error near unexpected token");
                    exit(1);
                }
                else{
                    int fd = open(argv[i+1], O_RDONLY);
                    if (fd < 0) perror("");
                    dup2(fd, 0);
                    close(fd);
                    argv[i] = NULL;
                    argv[i+1] = NULL;
                }
            }
            else{          // number of real commands
                number_cmd ++;
            }
        }
    }
    char** new_argv = (char**)malloc(sizeof(char*)* (number_cmd+1)); // +1 for the null at end
    if(new_argv == NULL){
        printf("Memory allocate failed\n");
        exit(1);
    }
    int old_cur = 0;
    int new_cur = 0;
    while(old_cur != argc){
        if(argv[old_cur] != NULL) new_argv[new_cur] = argv[old_cur];
        old_cur ++;
        new_cur ++;
    }
    new_argv[new_cur] = NULL;
    free(argv);
    return new_argv;
}

bool processAndTok(char* input){
    char* token = strtok(input, "\n");
    token = strtok(input, " ");  //tokenize
    if(strcmp(token, "\n") == 0) return true;  // do not need to exec
    char ** argv = (char**)malloc(sizeof(char*) * INIT_CAP);
    if(argv == NULL){
        printf("Memory allocate failed\n");
        exit(1);
    }
    int cap = INIT_CAP;
    int count= 0;
    while (token != NULL) {
        if(strcmp(token, "exit") == 0) return false;
        if(count == cap){
            cap *= 2;
            argv = (char**)realloc(argv,sizeof(char*) * (cap));
            if(argv == NULL){
                printf("Memory reallocate failed\n");
                exit(1);
            }
        }
        argv[count] = token;
        count ++;
        token = strtok(NULL, " ");
    }
    if(count == cap){
        argv = (char**)realloc(argv,sizeof(char*) * (cap+1));
        if(argv == NULL){
            printf("Memory reallocate failed\n");
            exit(1);
        }
    }
    argv[count] = NULL;
    execute(count, argv);
    free(argv);
    return true;
}
void execute(int argc, char** argv){
    if(strcmp(argv[0],"cd") ==0){   //build in cd
        int status = 0;
        if(argc == 1 || strcmp(argv[1],"~") == 0) status = chdir(getenv("HOME")) == -1;
        else{
            if(argv[1][0] == '~'){ //relative path
                char* home_path = getenv("HOME");
                char* path = (char*)malloc(sizeof(char)*(strlen(home_path) + strlen(argv[1])));
                if(path == NULL){
                    printf("Memory allocate failed\n");
                    exit(1);
                }
                strcpy(path,home_path);  //e.g ~/dir
                strcat(path, &argv[1][1]);
                chdir(path);
                
            }
            else status = chdir(argv[1]);
        }
        if(status == -1) perror("");
    }
    else{
        pid_t child = fork();
        if(child == 0){  //child process
            setChildsigaction();
            argv = parseIo(argc, argv);
            // two special cases  : > filename and  > filename
            if((argv[0] == NULL) || (strcmp(argv[0], ":") == 0)) {
                free(argv);
                exit(0);
            }
            execvp(argv[0], &argv[0]);
            perror("");
            free(argv);
            exit(1);
        }
        else{  //parent process
            int wstatus = 0;
            wait(&wstatus);
            if(wstatus == -1) perror("");
        }
    }
}
