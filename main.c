/*************************************
 * Lab 2 Exercise 2
 * Name: Chaitanya Baranwal
 * Student No: A0184716X
 * Lab Group: 01
 *************************************
 Warning: Make sure your code works on
 lab machine (Linux on x86)
 *************************************/

#include <stdio.h>
#include <fcntl.h>      // For stat()
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>   // for waitpid()
#include <unistd.h>     // for fork(), wait()
#include <string.h>     // for string comparison etc
#include <stdlib.h>     // for malloc()

char** readTokens(int maxTokenNum, int maxTokenSize, int* readTokenNum, char* buffer);
char** readCommands(int maxCommandNum, int maxCommandSize, int* readCommandNum, char* buffer);
void freeTokenArray(char** strArr, int size);
int checkIfCommandExists(char* cmd);
char* getCommandPath(char* cmd);
char* getCommand(char* input);

#define MAX_COMMAND_NUM 10
#define MAX_COMMAND_SIZE 250
#define MAX_TOKEN_NUM 11
#define MAX_TOKEN_SIZE 20

int main() {
    char input[MAX_COMMAND_NUM * MAX_COMMAND_SIZE];
    char** tokens;
    char** commands;
    int *numCommands = (int*) malloc(sizeof(int*));
    int *numTokens = (int*) malloc(sizeof(int*));
    int i, j;
    
    while (1) {
        printf("GENIE > ");
        fgets(input, sizeof(input), stdin);
        input[strlen(input)-1] = '\0';  // to terminate the input with null character
        
        // exit if input entered is 'quit'
        if (strcmp(input, "quit") == 0) {
            break;
        }

        commands = readCommands(MAX_COMMAND_NUM, MAX_COMMAND_SIZE, numCommands, input);

        // check the index till which commands can be executed in the pipeline
        int ending = *numCommands;
        for (j = 0; j < *numCommands; j++) {
            char* command = getCommand(commands[j]);
            if (checkIfCommandExists(command) != 0) {
                ending = j + 1;
                break;
            }
        }
        
        // run through commands while maintaining a pipe
        // So many pipes created so processes do not have sync problems
        // when passing data through pipeline
        int fd[*numCommands * 2];

        // parent initialises all the pipes
        for (i = 0; i < ending * 2; i++) {
            if (pipe(fd + (2 * i)) < 0) {
                printf("Pipeline creation problem.");
                _exit(EXIT_FAILURE);
            }
        }

        for (i = 0; i < ending; i++) {
            tokens = readTokens(MAX_TOKEN_NUM, MAX_TOKEN_SIZE, numTokens, commands[i]);
            int childpid = fork();

            if (childpid < 0) {
                perror("Could not create fork.");
                _exit(EXIT_FAILURE);
            }
            else if (childpid == 0) {
                if (i > 0) {
                    dup2(fd[(i - 1) * 2], STDIN_FILENO);
                }
                if ((i + 1) < ending) {
                    dup2(fd[(i * 2) + 1], STDOUT_FILENO);
                }
                
                // close all pipes
                int k;
                for (k = 0; k < 2 * ending; k++)
                    close(fd[k]);
                
                // execute command
                if (checkIfCommandExists(tokens[0]) != 0) {
                    printf("%s not found\n", getCommandPath(tokens[0]));
                    _exit(EXIT_FAILURE);
                }
                execv(getCommandPath(tokens[0]), tokens);
                _exit(EXIT_FAILURE);
            }
            // Free the array of tokens
            freeTokenArray(tokens, *numTokens);
        }

        // Free the array of commands
        freeTokenArray(commands, *numCommands);

        // Parent closes pipes and waits for children
        for (i = 0; i < 2 * ending; i++)
            close(fd[i]);
        for (i = 0; i < ending; i++)
            wait(NULL);

        printf("\n");
    }


    printf("Goodbye!\n");
    return 0;
}

char** readTokens(int maxTokenNum, int maxTokenSize, int* readTokenNum, char* buffer)
// Tokenize buffer
// Assumptions:
//  - the tokens are separated by " " and ended by newline
// Return: Tokenized substrings as array of strings
//        readTokenNum stores the total number of tokens
// Note:
//  - should use the freeTokenArray to free memory after use!
{
    char** tokenStrArr;
    char* token;
    int i;

    // allocate token array, each element is a char*
    tokenStrArr = (char**) malloc(sizeof(char*) * maxTokenNum);
    
    // Nullify all entries
    for (i = 0; i < maxTokenNum; i++) {
        tokenStrArr[i] = NULL;
    }

    token = strtok(buffer, " \n");
    
    i = 0;
    while (i < maxTokenNum && (token != NULL)) {
         // Allocate space for token string
        tokenStrArr[i] = (char*) malloc(sizeof(char*) * maxTokenSize);

        // Ensure at most 19 + null characters are copied
        strncpy(tokenStrArr[i], token, maxTokenSize - 1);

        // Add NULL terminator in the worst case
        tokenStrArr[i][maxTokenSize-1] = '\0';
        
        i++;
        token = strtok(NULL, " \n");
    }

    *readTokenNum = i;
    
    return tokenStrArr;
}

char** readCommands(int maxCommandNum, int maxCommandSize, int* readCommandNum, char* buffer)
// Tokenize a command into multiple commands based on the pipe (`|`) operator.
// Assumptions:
//  - the tokens are separated by "|" and ended by newline
// Return: Tokenized substrings as array of strings
//        readTokenNum stores the total number of commands
// Note:
//  - should use the freeTokenArray to free memory after use!
{
    char** commandStrArr;
    char* command;
    int i;

    // allocate command array, each element is a (char*)
    commandStrArr = (char**) malloc(sizeof(char*) * maxCommandNum);

    // Nullify all entries
    for (i = 0; i < maxCommandNum; i++) {
        commandStrArr[i] = NULL;
    }

    command = strtok(buffer, "|\n");
    
    i = 0;
    while (i < maxCommandNum && (command != NULL)) {
        // Allocate space for the command
        commandStrArr[i] = (char*) malloc(sizeof(char*) * maxCommandSize);

        // Ensure the command size stays within limits
        strncpy(commandStrArr[i], command, maxCommandSize - 1);

        // Assign a null character to the end just in case
        commandStrArr[i][maxCommandSize - 1] = '\0';

        i++;
        command = strtok(NULL, "|\n");
    }

    *readCommandNum = i;
    return commandStrArr;
}

void freeTokenArray(char** tokenStrArr, int size) {
    int i = 0;

    // Free every string stored in tokenStrArr
    for (i = 0; i < size; i++) {
        if (tokenStrArr[i] != NULL) {
            free(tokenStrArr[i]);
            tokenStrArr[i] = NULL;
        }
    }
    // Free entire tokenStrArr
    free(tokenStrArr);

    // Note: Caller still need to set the strArr parameter to NULL afterwards
}

// Function to check whether the entered command is valid
int checkIfCommandExists(char* cmd) {
    struct stat buf;
    return stat(getCommandPath(cmd), &buf);
}

// Function to get full command path
char* getCommandPath(char* cmd) {
    if (cmd[0] == '/') {
        return cmd;
    }
    char* fullCmd = (char*) malloc(sizeof(cmd) + 5);
    sprintf(fullCmd, "/bin/%s", cmd);
    return fullCmd;
}

// Function to read a command
char* getCommand(char* input) {
    char* command = (char*) malloc(sizeof(char*) * MAX_TOKEN_SIZE);
    int i = 0;
    while (input[i] == ' ') {
        i++;
    }
    int offset = i;
    while (input[i] != ' ' && input[i] != '\0' && input[i] != '\n') {
        command[i - offset] = input[i];
        i++;
    }
    command[i] = '\0';
    return command;
}
