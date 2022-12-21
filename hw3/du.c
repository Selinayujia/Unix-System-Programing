/*
 Implement the utility du.
 
 If no arguments, then do du on the current working directory.
 Otherwise, the argument (only handled) is a directory to run du on.
 do not support any options/flags.

author @ Selina Zhang
 */

#include <stdio.h>       // printf, perror
#include <dirent.h>      // DIR, dirent
#include <stdlib.h>      // exit
#include <string.h>      // strcat, strcpy
#include <sys/stat.h>    // stat, ISDIR
#include <unistd.h>      // getcwd
#include <linux/limits.h>// PATH_MAX
#include <stdbool.h>     // boolean
#include <errno.h>       // check errono value

#define INIT_CAP 10   // for the initial capacity of inodelist
typedef struct InodesList{  // this is used to keep track of appeared inodes
    int size;
    int capacity;
    int* exisitingInodes;
    
} InodesList;

char* string_process(char* arg);  // deal with the case that in ubantu ///path/// will become /path/
void initialize(char* flag, char* display); //process argv, append path, create inodelist
bool isNewInode(InodesList* inodes, int ino_num); // check if the ino_num exisied
int countDirBlk(char* path, InodesList* inodes, char* display_str);  // recursively list out the block usage of dir


int main(int argc, char **argv) {
    if (argc == 1  ){
        initialize("cwd", ".");
    }
    else if (argc == 2){
        char* arg = string_process(argv[1]);
        if(arg[0] == '/'){  // some absolute path is given
            initialize("root", arg);
        }
        else{   // a directory entry in current working dir
            initialize("cwd", arg);
        }
        free(arg);
    }
    else {
        fprintf(stderr, "Usage: %s [directory]\n", argv[0]);
        exit(1);
    }
}

char* string_process(char* arg){
    char* res = (char*)malloc(sizeof(char)* strlen(arg));
    if(res == NULL){
        perror("Memory Allocation Error: ");
        exit(2);
    }
    strcpy(res,arg);
    if(arg[0] == '/'){     // check the /// before path
        int i = 0;
        int j = 1;
        while(j < strlen(arg)){
            if(arg[i] == '/' && arg[j] == '/'){
                i++;
                j++;
            }
            else break;
        }
        if(j >= strlen(arg)) return "/";
        else strcpy(res,&arg[i]);
    }
    char* result = (char*)malloc(sizeof(char)* strlen(res));
    if(result == NULL){
        perror("Memory Allocation Error: ");
        exit(2);
    }
    int i = strlen(res)-2;
    int j = strlen(res)-1;
    while(i >= 0){                //check the /// after path
        if(res[i] == '/' && res[j] == '/'){
            i--;
            j--;
        }
        else break;
    }
    strncpy(result, res, j+1);
    free(res);
    return result;
}

void initialize(char* flag, char* display){
    InodesList inodes;
    inodes.size = 0;
    inodes.capacity = INIT_CAP;
    inodes.exisitingInodes = (int*)malloc(sizeof(int)* INIT_CAP);
    if(inodes.exisitingInodes == NULL){
        perror("Memory Allocation Error: ");
        exit(2);
    }
    
    char* path =(char*)malloc(sizeof(char)*PATH_MAX);
    if(path == NULL){
        perror("Memory Allocation Error: ");
        exit(2);
    }
    if(strcmp(flag, "cwd") == 0){    // need to append the path of cwd
        path = getcwd(path, (size_t)PATH_MAX);
        strcat(path, "/");
    }
    strcat(path, display);       // display is used to not display the path appended to the front
    char* display_str = (char*)malloc(sizeof(char)*PATH_MAX);
    if(display_str == NULL){
        perror("Memory Allocation Error: ");
        exit(2);
    }
    strcpy(display_str, display);
    int total = countDirBlk(path, &inodes, display_str);
    printf("%d\t%s\n",  total/2, display_str);
    free(path);
    free(display_str);
    free(inodes.exisitingInodes);
}


bool isNewInode(InodesList* inodes, int ino_num){
    if(inodes->size == inodes->capacity){
        inodes->capacity *= 2;    // if reach capacity, double it
        inodes->exisitingInodes = (int*)realloc(inodes->exisitingInodes, sizeof(int)*inodes->capacity);
        if(inodes->exisitingInodes == NULL){
            perror("Memory Reallocation Error: ");
            exit(2);
        }
    }
    int counter = 0;
    bool newInode = true;
    while(counter != inodes->size){
        if(inodes->exisitingInodes[counter] == ino_num){
            newInode = false;
            break;
        }
        counter ++;
    }
           
    if(newInode){
        inodes->exisitingInodes[inodes->size] = ino_num;
        inodes->size ++;
    }
    return newInode;
}

int countDirBlk(char* path, InodesList* inodes, char* display_str){
    DIR *dirp;
    struct dirent *direntp;
    struct stat statBuf;
    if ( (dirp = opendir(path)) == NULL ) {
        if(errno == EACCES){  // if permission denied, do not count
            return 0;
        }
        fprintf(stderr, "%s: ", display_str);
        perror("Failed to open directory");
        exit(2);
    }
    if(chdir(path) == -1){    //change cwd to the given path
        perror("Failed to change current working directory");
        exit(2);
    }
    char* newPath = (char*)malloc(sizeof(char)*PATH_MAX); //the path for the new path in recursive call on dir
    if(newPath == NULL){
        perror("Memory Allocation Error: ");
        exit(2);
    }
    
    char* newDisplay = (char*)malloc(sizeof(char)*PATH_MAX);  //the display for the new path in recursive call on dir
    if(newDisplay == NULL){
        perror("Memory Allocation Error: ");
        exit(2);
    }
    int total_bk_num = 0;
    while ( (direntp = readdir(dirp)) != NULL ) {
        if (lstat(direntp->d_name, &statBuf) == -1) {
            fprintf(stderr, "%s: ", direntp->d_name);
            perror("Failed to lstat");
            exit(2);
        }
        if (strcmp(direntp->d_name,"." )!= 0 && strcmp(direntp->d_name,".." ) != 0) {
            if (S_ISDIR(statBuf.st_mode)) {
                strcpy(newPath,path);
                strcat(newPath,"/");
                strcat(newPath,direntp->d_name);
                strcpy(newDisplay,display_str);
                if(newDisplay[strlen(newDisplay)-1] != '/') strcat(newDisplay,"/");
                strcat(newDisplay,direntp->d_name);
                int new_path_blk = countDirBlk(newPath, inodes, newDisplay);
                printf("%d\t%s\n",  new_path_blk/2, newDisplay); // divided two for linux system
                total_bk_num += new_path_blk;
                if(chdir(path) == -1){  //after recursion, since change back to current path
                    perror("Failed to change back to current working directory");
                    exit(2);
                }
            }
	       else{
                if(isNewInode(inodes, (int)direntp->d_ino)){
                    total_bk_num += (int)statBuf.st_blocks;
                }
           }
        }
        if (strcmp(direntp->d_name,"." )== 0){
            if(isNewInode(inodes, (int)direntp->d_ino)){
                total_bk_num += (int)statBuf.st_blocks;
            }
        }
    }
    free(newPath);
    free(newDisplay);
    closedir(dirp);
    return total_bk_num;
}

