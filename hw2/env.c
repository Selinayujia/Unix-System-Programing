/*env.c
 hw2
 implement env.c
 env [-i] [name=value]... [command [argument]...]
 The purpose of env with options is to execute a command with a modified environment. The command being run can have its own arguments.
 If the -i option is used, then the name/value pairs that are provided, completely replace the environment when running the command.
 If the -i option is not provided, then the name/value pairs modify the environment.
 If no command is provided then env simply displays the new / modified environment.
 author@ Selina Zhang
 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <stdbool.h>
#include <unistd.h>

void displayEnviron(char** environ, int lengthOfEnviron){ //display environ in given length
    int counter = 0;
    while(counter != lengthOfEnviron){
        if (environ[counter] != NULL){
            puts(environ[counter]);
            counter ++;
        }
        else{
            break;
        }
    }
}

int numOfFinalKVPair(char* argv[], int curr, int end){
    int res = 0;
    int start = curr;
    while(curr != end){
        bool equal = false;
        bool reassigned = true;
        for(int i = start; i < curr; ++i){  //check the case env[] a=b a=b repeating kv pairs
            if(argv[i] != NULL){   // the previous equal / reassigned kv pairs is set to NULL in argv
                if(strcmp(argv[curr],argv[i]) == 0){   // this kv is the repeating the previous, reassign last kv
                    equal = true;
                    argv[curr] = NULL;
                    break;
                }
                else{  // not equal, but needed to check the case env[] a=b a=v reassigned kv pairs
                    int j = 0;
                    while((argv[curr][j] != '=') || (argv[i][j] != '=')){  // reassignment, only first half to '=' need to be checked
                        if(argv[curr][j] != argv[i][j]){
                            reassigned = false;
                            break;
                        }
                        j++;
                    }
                    if(reassigned && (argv[curr][j] == argv[i][j])){   //both are equal until equal sign
                        argv[i] = NULL;
                        break;
                    }
                }
            }
        }
        if((curr == start) || ((!reassigned)&&(!equal))){  // if first kv, kv cannot be equal to previous or being reassigned
            res ++;
        }
        curr ++;
    }
    return res;
    
}

void appendNewEnviron(int indexOfCommand, int argc, char* argv[], char** environ, int lengthOfEnviron){
    int endOfKV = indexOfCommand== -1 ? argc : indexOfCommand;
    int environ_i = 0;
    int argv_i = 1;               // index in argv, 2 for skipping the first arg in argv, env
    if(argv_i == endOfKV){        //no key-value pair provided, no need to append
        execvp(argv[indexOfCommand], &argv[indexOfCommand]);
        perror("Error executing: ");
    }
    else{
        int lengthOfKV = numOfFinalKVPair(argv, argv_i, endOfKV) + lengthOfEnviron;
        char** newEnv = (char**)malloc(sizeof(char*) * (lengthOfKV));
        if(environ == NULL){
            perror("Memory Allocation Error: ");
            exit(1);
        }
        while(environ_i != lengthOfEnviron){       // put the old environ key-value pair to new environ
            newEnv[environ_i] = environ[environ_i];
            environ_i ++;
        }
        while(environ_i != lengthOfKV){          // append the rest of the new environ
            if(argv[argv_i] != NULL){
                newEnv[environ_i] = argv[argv_i];
                environ_i ++;
            }
            argv_i ++;
        }
        if(indexOfCommand == -1){           //display if no commands given
            displayEnviron(newEnv, lengthOfKV);
        }
        else{
            execvp(argv[indexOfCommand], &argv[indexOfCommand]);
            perror("Error executing: ");   // if the exec ever comes back, it means there is a problem in executing
        }
        free(newEnv);                // free up the memory allocated
        newEnv = NULL;
    }
        
}

void createNewEnviron(int indexOfCommand, int argc, char* argv[], char** environ, int lengthOfEnviron){
    int endOfKV = indexOfCommand== -1 ? argc : indexOfCommand;
    int environ_i = 0;                // index in environ
    int argv_i = 2;                   // index in argv, 2 for skipping the first two args in argv, env and -i
    int lengthOfKV = numOfFinalKVPair(argv, argv_i, endOfKV);   //the length of non-repeat, final kv pairs
    if(lengthOfKV <= lengthOfEnviron){      // no need to allocate memory
        while(environ_i != lengthOfKV){          // update the kp list to old environ
            if(argv[argv_i] != NULL){
                environ[environ_i] = argv[argv_i];
                environ_i ++;
            }
            argv_i ++;
        }
        while(environ_i != lengthOfEnviron){  // if the old one too big for the new kv pairs, set the rest of array to null
            environ[environ_i] = NULL;
            environ_i += 1;
        }
        if(indexOfCommand == -1){     // no command, display
            displayEnviron(environ, lengthOfEnviron);
        }
        else{
            execvp(argv[indexOfCommand], &argv[indexOfCommand]);
            perror("Error executing: ");    // if ever return back, need to print errono
        }
    }
    else{                // need to allocate memory
        char** newEnv = (char**)malloc(sizeof(char*) * (lengthOfKV));
        if(newEnv == NULL){
            perror("Memory Allocation Error: ");
            exit(1);
        }
        while(environ_i != lengthOfKV){     // put the kp pairs to new environ
            if(argv[argv_i] != NULL){
                newEnv[environ_i] = argv[argv_i];
                environ_i ++;
            }
            argv_i ++;
        }
        if(indexOfCommand == -1){
            displayEnviron(newEnv, lengthOfKV);
        }
        else{
            execvp(argv[indexOfCommand], &argv[indexOfCommand]);
            perror("Error executing: ");
        }
        free(newEnv);        // free is allocated memory
        newEnv = NULL;
    }
    
}

int separateKVandCommands(int argc, char* argv[], bool replace){
    int commandStartIndex = -1;        // to track the start of command in argv
    int i = replace ? 2 : 1;  // if replace environ, it means the first possible key-value pair starts with index
    while(i < argc){            // this loop's purpose is to find the start of command
        bool isKeyValuePair = false;
        for(int j = 0; j < strlen(argv[i]); ++j){
            if(argv[i][j] == '=' && j != 0){
                isKeyValuePair = true;
                break;
            }
            else if (argv[i][j] == '='){  // j == 0,  =dev not allowed
                fprintf(stderr, "Please give valid key value pair!\n");
                exit(1);
            }
        }
        if(isKeyValuePair == false){   // not a key value pair, command started
            commandStartIndex = i;
            break;
        }
        i++;
    }
    return commandStartIndex;
}

int findLenOfEnviron(char** environ){     // find the system's default environ length
    int i = 0;
    while(environ[i] != NULL){
        i ++;
    }
    return i;
}

int main(int argc, char* argv[]){
    extern char** environ;
    int lengthOfEnviron = findLenOfEnviron(environ);
    if(argc == 1){    // the command is env, simply display the environment
        displayEnviron(environ,lengthOfEnviron);  // do not free because nothing changes
    }
    else{
        bool replace = strcmp(argv[1], "-i") == 0 ? true : false;     
        int indexOfCommand = separateKVandCommands(argc, argv, replace);
        if(replace){  // argv[i] == "-i"
            createNewEnviron(indexOfCommand, argc, argv, environ, lengthOfEnviron);
        }
        else{
            appendNewEnviron(indexOfCommand, argc, argv, environ, lengthOfEnviron);
        }
    }
    return 0;
}

