
/****************************************************************/
/*  __  __   _           _     ____   _   _   _____  _     _    */
/* |  \/  | (_)         (_)   / ___| | | | | | ___/ | |   | |   */
/* | \  / |  _  _ _ _    _   | |__   | |_| | | |___ | |   | |   */
/* | |\/| | | | | '_ \  | |   \__ \  |  _  | |  __/ | |   | |   */
/* | |  | | | | | | | | | |   ___) | | | | | | |__  | |_  | |_  */
/* |_|  |_| |_| |_| |_| |_|  |____/  |_| |_| |___/  |___| |___| */
/*                                                              */
/****************************************************************/

/* AUTHORS:                                                     */
/*                              David Paúl Limaylla Ticlavilca  */
/*                                        Cristian Andrei Vlad  */


                /*<<<<<<<<<<<<<---->>>>>>>>>>>>>*/
                /*                              */
                /*       EXECUTION STEPS        */
                /*                              */
                /*<<<<<<<<<<<<<---->>>>>>>>>>>>>*/


/*              	1) Display the promt                        */
/*                                                              */
/*              	2) Read a line from stdin                   */
/*                                                              */
/*              	3) Analyze that line with the parser        */
/*                                                              */
/*              	4) Execute the comands of the line          */

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
#define FILE_DESCRIPTORS 3

// Compile with -> gcc -Wall minishell.c libparser.a -o minishell -static

int upper_executer(tline* line);
int executer(int nprocesses, tcommand* command, int* new_fds, int* old_fds, int first);

int main(int argc, char const *argv[])
{
    char shell_line[MAX_LINE_SIZE]; tline* parsed_line; int shell_status = 0;
    while (!shell_status){
        /********************************/
        /*             <0>              */
        /*        Resetting fds         */
        /********************************/
        if ((dup2(dup(STDIN_FILENO), STDIN_FILENO) == -1) + 
            (dup2(dup(STDOUT_FILENO), STDOUT_FILENO) == -1) + 
            (dup2(dup(STDERR_FILENO), STDERR_FILENO) == -1))
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
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
    }
    return shell_status;
}

int upper_executer(tline* line){
    /* --- ORIGINAL FILE DESCRIPTORS CLONED --- */
    int original_fds[FILE_DESCRIPTORS] = {
        dup(STDIN_FILENO), dup(STDOUT_FILENO), dup(STDERR_FILENO)
    };
    /* --- AUXILIARY FILE DESCRIPTORS --- */
    int new_fds[FILE_DESCRIPTORS] = {
        original_fds[0], original_fds[1], original_fds[2]
    };
    /* --- REDIRECTION SOURCES --- */
    char* redirection_src[FILE_DESCRIPTORS] = {
        line->redirect_input, line->redirect_output, line->redirect_error
    };
    for (int i = 0; i < FILE_DESCRIPTORS; i++){
        /* --- Cheking if there is a file for redirection --- */
        if (redirection_src[i]){
            /* --- INPUT REDIRECTION --- */
            if (i == 0){
                new_fds[i] = open(redirection_src[i], O_RDONLY); 
            } else{ /* --- OUTPUT or ERROR REDIRECTION --- */
                new_fds[i] = open(redirection_src[i], O_WRONLY | O_CREAT | O_TRUNC, 0666); 
            }
            /* --- Crecking error while opening file*/
            if (new_fds[i] == -1){
                perror("open");
                exit(EXIT_FAILURE);
            }
            printf("SE HA DETECTADO UNA REDIRECCIÓN\n"); /**/
        }
    }
    /* --- Executing the line --- */
    int aux = executer(line->ncommands, line->commands, new_fds, original_fds, 1);
    /* --- Closing file descriptors --- */
    for (int i = 0; i < FILE_DESCRIPTORS; i++){
        close(new_fds[i]);
    }
    dup2(original_fds[0], STDIN_FILENO);
    dup2(original_fds[1], STDOUT_FILENO);
    dup2(original_fds[2], STDERR_FILENO);
    return aux;
}

int executer(int nprocesses, tcommand* command, int* new_fds, int* old_fds, int first){
    int comunication_pipe[2];
    pid_t pid;

    /* --- PIPE CREATION --- */
    if (pipe(comunication_pipe) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    /* --- FORK --- */
    pid = fork();
    if (pid < 0){
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0){
        printf("ACCEDIENDO AL HIJO\n"); /**/
        // Cerrar extremo de lectura
        close(comunication_pipe[0]);
        /*
            Redirigir STDIN:
            1) A ningún lado, si nprocesses == 1 y output_fd == STDOUT_FILENO
            2) A ningún lado, si no es el primer mandato ??
            3) A fichero, si nprocesses == 1 y output_fd != STDOUT_FILENO
        */
        if ((nprocesses == 1) && (first)){
            if (new_fds[0] != old_fds[0]){
                /* --- REDIRECT TO FILE --- */
                dup2(new_fds[0], STDIN_FILENO);
            } else{
                /* --- REDIRECT TO KEYBOARD --- */
                dup2(old_fds[0], STDIN_FILENO);
            }
        }
        /*
            Redirigir STDOUT:
            1) A ningún lado, si nprocesses == 1 y output_fd == STDOUT_FILENO
            2) A la pipe, si nprocesses > 1
            3) A fichero, si nprocesses == 1 y output_fd != STDOUT_FILENO
        */
        if (nprocesses == 1){
            if (new_fds[1] != old_fds[1]){
                /* --- REDIRECT TO FILE --- */
                dup2(new_fds[1], STDOUT_FILENO);
            } else{
                /* --- REDIRECT TO SCREEN --- */
                dup2(old_fds[1], STDOUT_FILENO);
            }
        } else{
            /* --- REDIRECT TO PIPE --- */
            dup2(comunication_pipe[1], STDOUT_FILENO);
        }
        /*
            Redirigir STDERR:
            1) A file si error_fd != STDERR_FILENO
        */
        if (new_fds[2] != old_fds[2]){
            /* --- REDIRECT TO FILE --- */
            dup2(new_fds[2], STDERR_FILENO);
        } else{
            /* --- REDIRECT TO SCREEN --- */
            dup2(old_fds[2], STDERR_FILENO);
        }
        

        close(comunication_pipe[1]);
        execv(command->filename, command->argv);

        // Si falla y error_fd != NULL escribir en el file asociado al descriptor de fichero
        printf("execv: Bad address\n");
        exit(EXIT_FAILURE);
    } else{
        waitpid(pid, NULL, 0);
        int new_input = comunication_pipe[1];
        close(new_fds[0]);
        close(comunication_pipe[0]);
        close(comunication_pipe[1]);
        if (nprocesses - 1 > 1){
            new_fds[0] = new_input;
            executer(nprocesses - 1, command + 1, new_fds, old_fds, 0);
        }
        return 0;
    }
}