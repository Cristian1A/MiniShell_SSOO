
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

#define PROMPT "msh>"
#define MAX_LINE_SIZE 1024
#define REDIRECTION_SOURCES 3


// Compile with -> gcc -Wall Practica_2.c libparser.a -o Practica_2 -static
int execute_line(tline* line);

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

/* --- Executes a specific command --- */
int command_executer(tcommand command, char** redirections){
    /* --- File descriptors state ---*/
    int file_descriptor[REDIRECTION_SOURCES];
    /* --- Cheking the existence of the command --- */
    if (!command.filename){
        return -1;
    }
    for (int i = 0; i < REDIRECTION_SOURCES; i++){
        if (redirections[i]){
            file_descriptor[i] = open(redirections[i], O_RDONLY) * (!i) + open(redirections[i], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            /* --- Checking if an error ocurred while opening the file --- */
            if (file_descriptor[i] == -1){
                perror("Open");
                return -1;
            }
            /* --- Modifying the file descriptor --- */
            if (dup2(file_descriptor[i], STDIN_FILENO * (i == 0) + STDOUT_FILENO * (i == 1) + STDERR_FILENO * (i == 2)) == -1){
                perror("dup2");
                return -1;
            }
        }
    }
    return execv(command.filename, command.argv);
}

/* --- Verifies whether the line needs to be executed in background or not --- */
int line_executer(tline* line){
    return (background_executer(line) * line->background) + foreground_executer(line);
}

/* --- Executes the line in background --- */
int foreground_executer(tline* line){
    /* --- Shared comunication pipe --- */
    int comunication_pipe[2];
    /* --- Shared STDOUT cache --- */
    char* stdout_cache;
    /* --- Shared redirections array --- */
    char* redirections[REDIRECTION_SOURCES] = {line->redirect_input, NULL, NULL};
    /* --- Indicates if an error ocurred while executing a command ---*/
    int exec_error = 0;
    /* --- Process pid --- */
    pid_t pid;
    int status;
    /* --- Indicates the current command position in the line --- */
    int command_counter = 0;
    /* --- Initialising comunication pipe --- */
    if (pipe(comunication_pipe) == -1){
        perror("pipe");
        exec_error = 1;
    }
    while (!exec_error + (command_counter < line->ncommands)){
        /* --- STDOUT and STDERR redirections --- */
        if (command_counter == line->ncommands - 1){
            redirections[1] = line->redirect_input;
            redirections[2] = line->redirect_error;
        }
        /* --- Child process creation --- */
        pid = fork();
        if (pid < 0){ /* --- Child creation failure ---*/
            fprintf(stderr, "Failure at fork function");
            exec_error = 1;
        } else if (pid == 0){ /* --- Child process --- */
            int result;
            close(comunication_pipe[0]);
            result = execute_command(line->commands[command_counter], redirections);
            /* --- 0 for success and any other value for failure --- */
            write(comunication_pipe[1], &result, sizeof(result));
        } else{ /* --- Parent process --- */
            int result;
            waitpid(pid, &status, 0);
            /* --- Verifying child process' result --- */
            close(comunication_pipe[1]);
            read(comunication_pipe[0], &result, sizeof(result));
            close(comunication_pipe[0]);
            exec_error = result;
        }
        ++command_counter;
        redirections[0] = NULL;
    }
    return exec_error;
}

/* --- Executes the line in background --- */
int background_executer(tline* line){
    // TODO Implement function
    return 0;
}