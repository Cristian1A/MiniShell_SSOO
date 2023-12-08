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

int main(int argc, char const *argv[]){
    char shell_line[MAX_LINE_SIZE]; tline* parsed_line; int shell_status = 0;
    while (!shell_status){
        /********************************/
        /*             <1>              */
        /*      Print the prompt        */
        /********************************/
        printf("%s ", PROMPT);
        /********************************/
        /*             <2>              */
        /*   Read a line from stdin     */
        /********************************/
        if(!fgets(shell_line, MAX_LINE_SIZE, stdin)){
            printf("The line cannot be read\n");
            exit(-1);
        }
        /********************************/
        /*             <3>              */
        /*   Analyze with the parser    */
        /********************************/
        parsed_line = tokenize(shell_line);
        /********************************/
        /*             <4>              */
        /*     Execute the comands      */
        /********************************/
        shell_status = line_executer(parsed_line);
    }
    return shell_status;
}

int line_executer(tline* line){
    return executer(line->ncommands, line->commands, line->redirect_input, line->redirect_output, line->redirect_error, 1);
}

int input_redir(char* file_v, int input_to_file){
    int file_descriptor; int status = 0; int pipe_fd[2];
    if (input_to_file * (file_v != NULL)){
        file_descriptor = open(file_v, O_RDONLY);
        if (file_descriptor == -1){
            status = -1;
        }
        if (dup2(file_descriptor, STDIN_FILENO) == -1){
            status = -2;
        }
    } else if (input_to_file * (file_v == NULL)){
        if ((dup(STDIN_FILENO), STDIN_FILENO) == -1){
            status = -2;
        }
    } else{
        if (pipe(pipe_fd) == -1) {
            status = -3;
        } else{
            write(pipe_fd[1], file_v, strlen(file_v));
            close(pipe_fd[1]);
            dup2(pipe_fd[0], STDIN_FILENO);

    // Ahora stdin está r 
    close(pipe_fd[0]);
        }

    }
    
    return status;
}

int executer(int nprocesses, tcommand* command, char* input, char* output, char* error, int redir_input_to_file){
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
        /* ----- 1) Redirect STDIN ----- */
        if (input_redir(input, redir_input_to_file) <= 0){
            perror("Input file descriptor eror");
            exit(EXIT_FAILURE);
        }
        
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
        // TODO Write error file if existed
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
            executer(nprocesses - 1, command + 1, message_received, output, error, 0) * (nprocesses > 1);
            // TODO Write output file if existed
            // write in output file if (nprocesses == 1)
            // TODO Write error file if existed
            // write in error file if (nprocesses == 1) POSITIVE ANSWER
            /* --- Freeing memory --- */
            free(message_received);
        } else{
            perror("read");
            exit(EXIT_FAILURE);
        }
        return 0;
    }
}