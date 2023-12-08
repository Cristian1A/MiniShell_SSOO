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

// Compile with -> gcc -Wall Practica_2.c libparser.a -o Practica_2 -static

int upper_executer(tline* line);
int executer(int nprocesses, tcommand* command, int input_fd, int output_fd, int error_fd);

int main(int argc, char const *argv[])
{
    char shell_line[MAX_LINE_SIZE]; tline* parsed_line; int shell_status = 0;
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
    shell_status = upper_executer(parsed_line);
    return shell_status;
}

int upper_executer(tline* line){
    int input_fd;
    // Verificar si hay redirección de entrada, porque hay que leer el archivo para sacar el descriptor de fich
    if (line->redirect_input){
        input_fd = open(line->redirect_input, O_RDONLY);
        if (input_fd == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }
    }
    printf("LLEGÓ HASTA DEPUÉS DE VERIFICAR LA REDIRECCIÓN DE ENTRADA\n");
    int aux = executer(
        line->ncommands,
        line->commands,
        input_fd * (line->redirect_input != NULL) + dup(STDIN_FILENO),
        dup(STDOUT_FILENO),
        dup(STDERR_FILENO)
    );
    close(input_fd);
    return aux;
}

int executer(int nprocesses, tcommand* command, int input_fd, int output_fd, int error_fd){
    //dup2(dup(STDOUT_FILENO), STDOUT_FILENO);
    printf("ENTRÓ A EXECUTER\n");
    printf("%d\n", input_fd);
    int comunication_pipe[2];
    pid_t pid;

    if (pipe(comunication_pipe) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    printf("CREÓ EL PIPE\n");
    pid = fork();

    printf("PID: %d\n", pid);
    printf("DESPUÉS DEL PID\n");
    if (pid < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0){
        printf("ACCEDIENDO AL HIJO\n");
        // Cerrar extremo de lectura
        close(comunication_pipe[0]);
        // Cambiar el descriptor de fichero asociado a STDIN
        if (dup2(input_fd, STDIN_FILENO) == -1){
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        /*
        // Cambiar el descriptor de fichero asociado a STDOUT
        if (dup2(comunication_pipe[1], STDOUT_FILENO) == -1){
            perror("dup2");
            exit(EXIT_FAILURE);
        }*/
        close(comunication_pipe[1]);
        printf("A PUNTO DE EJECUTAR EL MANDATO %s\n", command->filename);
        execv(command->filename, command->argv);

        // Si falla y error_fd != NULL escribir en el file asociado al descriptor de fichero
        perror("execv");
        exit(EXIT_FAILURE);
    } else{
        waitpid(pid, NULL, 0);
        int new_fd = comunication_pipe[1];
        close(input_fd); //?
        close(comunication_pipe[0]);
        close(comunication_pipe[1]);
        printf("AHORA ESTOY AQUÍ\n");
        if (nprocesses - 1 > 1){
            executer(nprocesses - 1, command + 1, new_fd, output_fd, error_fd);
        }
        
        return 0;
    }
}