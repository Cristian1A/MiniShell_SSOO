#include "parser.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>

#define PROMPT "msh>"
#define MAX_LINE_SIZE 1024
#define REDIRECTION_SOURCES 3

int line_executer(tline* line);

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
    /* --- Original file descriptors --- */
    int original_stdin = dup(STDIN_FILENO);
    int original_stdout = dup(STDOUT_FILENO);
    int original_stderr = dup(STDERR_FILENO);
    /* --- Creating common pipe --- */
    int comunication_pipe[2];
    if (pipe(comunication_pipe) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    /* --- Creating child process --- */
    pid_t pid;
    pid = fork();
    if (pid < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0){ /* --- Child code --- */
        close(comunication_pipe[0]); // Cerrar extremo de lectura
        dup2(comunication_pipe[1], STDOUT_FILENO);
        close(comunication_pipe[1]); // Cerrar extremo de escritura

        execv(line->commands->filename, line->commands->argv);
        // En caso de error al ejecutar el comando
        perror("execv");
        exit(EXIT_FAILURE);
    } else{ /* --- Parent code --- */
        close(comunication_pipe[1]); // Cerrar extremo de escritura
        dup2(comunication_pipe[0], STDIN_FILENO);
        close(comunication_pipe[0]); // Cerrar extremo de lectura
        wait(NULL);
        // Utilizar memoria dinámica para leer datos desde la tubería
        ssize_t bytesRead;
        char *buffer = malloc(4096); // Puedes ajustar el tamaño según tus necesidades

        if (buffer == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        while ((bytesRead = read(STDIN_FILENO, buffer, 4096)) > 0) {
            // Realizar operaciones con los datos leídos, por ejemplo, imprimirlos en stdout
            write(STDOUT_FILENO, buffer, bytesRead);
        }

        // Manejar cualquier error de lectura
        if (bytesRead == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        // Liberar la memoria dinámica
        free(buffer);
        // Devolver STDIN, STDOUT y STDERR al estado original
        dup2(original_stdin, STDIN_FILENO);
        dup2(original_stdout, STDOUT_FILENO);
        dup2(original_stderr, STDERR_FILENO);
    }
}