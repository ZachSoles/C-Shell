#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>
#define delim " \t\r\n\a"

void error(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO,error_message,strlen(error_message));
}

int getInput(char* str){
    size_t length = 0; char *line = NULL; int i = 0;
    if(getline(&line, &length, stdin) != -1) {
        line[strcspn(line, "\n")] = 0;
        strcpy(str, line);
    }
    if(strcmp(str,"") == 0) return 0;
    else if(strstr(str, "&") != NULL) return 1;
    else return 2;
}

int getFileRedirect(char* str, char** fileArray){
    if(strstr(str,">") == NULL) return 0;
    int i = 0;
    char* token = strtok(str, ">");
    while(token != NULL){
        fileArray[i] = token;
        token = strtok(0, delim);
        i++;
    }
    if(i > 2) {error(); return 0;}
    return 1;
}

int getFileInput(char* str){
    if(strcmp(str,"") == 0) return 0;
    else if(strstr(str, "&") != NULL) return 1;
    else return 2;
}

int tokenize(char** arguments, char* str){
    char* token = strtok(str, delim);
    int i = 0;
    while(token != NULL){
        arguments[i] = token;
        token = strtok(0, delim);
        i++;
    }
    return i-1;
}

int parallelCommands(char* str, char** pCommands){
    int i = 0;
    char* token = strtok(str,"&");
    while(token != NULL){
        pCommands[i] = token;
        i++;
        token = strtok(0, "&");
    }
    return i;
}

void builtInExit(char** arguments){
    if(arguments[1] == NULL) exit(0);
    error();
}

void builtInCd(char** arguments){
    if(arguments[2] == NULL && arguments[1] != NULL) chdir(arguments[1]);
    else error();
}

void builtInPath(char** arguments, char** path, int* pathCount){
    memset(path, 0, 1000); *pathCount = 1; int i = 1;
    while (arguments[i] != NULL){
        path[i - 1] = (char*) calloc(1000, sizeof(char));
        strcpy(path[i-1], arguments[i]);
        i++;
    }
    *pathCount = i-1;
}

void runExternalCommand(char** arguments, char** path, int *pathCount, int redirect, char **fileArray){
    int index = 0; int status = -1; int i = 0;
    char str[1000];
    for(int i = 0; i < *pathCount; i++){
        strcpy(str, path[i]);
        strcat(str,"/");
        strcat(str, arguments[0]);
        status = access(str,X_OK);
        if(status == 0){
            int pid = fork();
            if(pid == 0){
                if(redirect == 1){
                    int fd = open(fileArray[1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                    dup2(fd, 1);
                    dup2(fd, 2);
                    close(fd);
                }
                execv(str,arguments);
            }
            else{
                wait(NULL);
                return;
            }
        }
    }
}

void mainShell(char *str, char** arguments, char** path, int* pathCount, int redirect, char**fileArray){
    int count = tokenize(arguments, str);
    if(strcmp(arguments[0],"exit") == 0) builtInExit(arguments);
    else if(strcmp(arguments[0],"cd") == 0) builtInCd(arguments);
    else if(strcmp(arguments[0],"path") == 0) builtInPath(arguments, path, pathCount);
    else if(path[0] != NULL) runExternalCommand(arguments, path, pathCount, redirect, fileArray);
}

void shell(){
    char* path[1000]; char* arguments[1000]; char str[1000]; char* pCommands[1000]; int* pathCount = malloc(sizeof(int)); *pathCount = 1; char* fileArray[1000]; int redirect = 0;
    path[0] = "/bin";
    printf("grsh> ");
    while(1) {
        int status = getInput(str);
        int redirect = getFileRedirect(str, fileArray);
        if(status == 0) continue;
        if(status == 1){
            int count = parallelCommands(str, pCommands);
            for(int i = 0; i < count; i++) {
                mainShell(pCommands[i], arguments, path, pathCount, redirect, fileArray);
                memset(arguments, 0, (sizeof(arguments[0]) * 1000));
            }
        }
        if(status == 2) {
            mainShell(str, arguments, path, pathCount, redirect, fileArray);
        }
        memset(str, 0, (sizeof(str[0]) * 1000));
        memset(arguments, 0, (sizeof(arguments[0]) * 1000));
        memset(pCommands, 0, (sizeof(pCommands[0]) * 1000));
        printf("grsh> ");
    }
}

void bashShell(char* argv){
    char* path[1000]; char* arguments[1000]; char* pCommands[1000]; int* pathCount = malloc(sizeof(int)); *pathCount = 1; char* fileArray[1000]; int redirect = 0;
    path[0] = "/bin"; 
    FILE* f;
    f=fopen(argv, "r");
    size_t length = 0; char *str = NULL; int i = 0; ssize_t read;
    while((read = getline(&str, &length, f)) != -1) {
        int status = getFileInput(str);
        int redirect = getFileRedirect(str, fileArray);
        if(status == 0) continue;
        if(status == 1){
            int count = parallelCommands(str, pCommands);
            for(int i = 0; i < count; i++) {
                mainShell(pCommands[i], arguments, path, pathCount, redirect, fileArray);
                memset(arguments, 0, (sizeof(arguments[0]) * 1000));
            }
        }
        if(status == 2) mainShell(str, arguments, path, pathCount, redirect, fileArray);
        memset(arguments, 0, (sizeof(arguments[0]) * 1000));
        memset(pCommands, 0, (sizeof(pCommands[0]) * 1000));
    }
}

int main(int argc, char *argv[]){
    if(argc == 1) shell();
    else bashShell(argv[1]);
    return 0;
}