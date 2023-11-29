
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

int execute_line(tline* line);

int main(int argc, char const *argv[])
{
    char shell_line[MAX_LINE_SIZE];
    tline* parsed_line;
    int shell_status = 0;
    while (shell_status == 0){
        // 1) Display the promt
        printf("%s ", PROMPT);
        // 2) Read a line from stdin
        if(!fgets(shell_line, MAX_LINE_SIZE, stdin)){
            printf("The line cannot be read\n");
            exit(-1);
        }
        // 3) Analyze that line with the parser
        parsed_line = tokenize(shell_line);
        // 4) Execute the comands of the line
        shell_status = execute_line(parsed_line);
    }
    return shell_status;
}

int execute_command(tcommand* command, char* input, char* output, char* error){
    int file_descriptor;
    // Comprobar si el comando existe
    if (command->filename){
        printf("El mandato existe\n");
        // Verificar la redirección de entrada
        if (input != NULL){
            printf("%s\n", input);
            file_descriptor = open(input, O_RDONLY);
            if (file_descriptor == -1){
                perror("open");
                exit(-1);
            }
            printf("Se ha podido abrir el fichero %d\n", file_descriptor);
            // Modificar descriptor de stdin
            if (dup2(file_descriptor, STDIN_FILENO) == -1){
                perror("dup2");
                exit(-1);
            }
            printf("Se ha redirigido la entrada correctamente\n");
        }
        // Ejecutar comando
        return execv(command->filename, command->argv);
    }
    printf("El mandato no existe\n");
    return -1;
}

int execute_line(tline* line){
    // Create a son proccess
    int status;
    int fd[2];
    pid_t pid = fork();

    /* --- Pipe creation --- */
    if (pipe(fd) == -1){
        perror("pipe");
        return -1;
    }
    /* --- Child process creation --- */
    if (pid < 0){
        fprintf(stderr, "Failure at fork function");
        return -1; //
    } else if (pid == 0){ /* --- Child process --- */
        int result;
        close(fd[0]);
        result = (execute_command(line->commands, line->redirect_input, line->redirect_output, line->redirect_error) == NULL);
        write(fd[1], &result, sizeof(result));
    } else{ /* --- Parent process --- */
        int result;
        waitpid(pid, &status, 0);
        /* --- Verifying child process' result --- */
        close(fd[1]);
        read(fd[0], &result, sizeof(result));
        close(fd[0]);
        return 0;
    }
}