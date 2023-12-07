#include "parser.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define PROMPT "msh>"
#define MAX_LINE_SIZE 1024
#define AUX_BUFFER_SIZE 25000

int main(int argc, char const *argv[])
{
    /* code */
    return 0;
}

int executer(int nprocesses, tcommand* command, char* input, char* output, char* error){
    int comunication_pipe[2];
    pid_t pid;

    /* --- Creating common PIPE --- */
    if (pipe(comunication_pipe) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /* --- Creating a child process --- */
    pid = fork();

    if (pid < 0){ 
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0){ /* --- Child process --- */
        /* --- Close pipe for reading --- */
        close(comunication_pipe[0]);
        /* --- STDOUT STDERR redirections --- */
        dup2(comunication_pipe[1], STDOUT_FILENO);
        // TODO STDERR redirection
        /* --- Close pipe for writing --- */
        close(comunication_pipe[1]);
        /* Executing the command --- */
        execv(command->filename, command->argv);
        /* --- In the event of failure of execv --- */
        perror("execv");
        exit(EXIT_FAILURE); // Or compulsory return
    } else{ /* --- Parent process --- */
        /* --- Close pipe for writing --- */
        close(comunication_pipe[1]);

        dup2(comunication_pipe[0], STDIN_FILENO);
        /* --- Close pipe for reading --- */
        close(comunication_pipe[0]);
        // Leer desde la pipe y almacenar en una variable char*
        char buffer[AUX_BUFFER_SIZE]; ssize_t bytesRead;
        
        bytesRead = read(comunication_pipe[0], buffer, sizeof(buffer));
        if (bytesRead >= 0){ /* --- Child content read successfuly --- */
            buffer[bytesRead] = '\0';
            char *message_received = strdup(buffer);
            // If there are more commands
            executer(nprocesses - 1, command + 1, message_received, output, error) * (nprocesses > 1);
            /* --- Freeing memory --- */
            free(message_received);
        } else{
            perror("read");
            exit(EXIT_FAILURE);
        }
        return 0;
    }
}