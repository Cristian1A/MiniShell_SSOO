
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

int line_executer(tline* line);
// Compile with -> gcc -Wall Practica_2.c libparser.a -o Practica_2 -static


int main(int argc, char const *argv[])
{
    char shell_line[MAX_LINE_SIZE];
    tline* parsed_line;
    while (1){
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
        
        if (line_executer(parsed_line) != 0){
            printf("Error -> An error ocurred while executing the current line\n");
            exit(-1);
        }

        
    }
    return 0;
}

/*
    Executes a specific command
    Args:
    - tcommand* command - command to execute
*/
int command_executer(tcommand* command, char** redirections){
    //int descriptors[REDIRECTION_SOURCES];
    int file_descriptor;
    int default_file_descriptors[REDIRECTION_SOURCES] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
    for (int i = 0; i < REDIRECTION_SOURCES; i++){
        if (redirections[i]){
            // TODO ABRIR EL FICHERO Y CAMBIAR EL DESCRIPTOR DE FICHERO CON dup2
            file_descriptor = open(redirections[i], O_RDONLY);
            // An error ocurred when reading the file
            if (file_descriptor == -1){
                perror("open");
                exit(-1);
            }
            // Dulpicating the file descriptor
            if (dup2(file_descriptor, default_file_descriptors[i]) == -1){
                perror("dup2");
                exit(-1);
            }
        }
    }
    
    if(execv(command->filename, command->argv) == -1){
        printf("Error -> Executing command\n");
        exit(-1);
    }
    printf("\n");
    return 0;
}

/*
    Analyzes a parsed line
    Args:
    - tline* line - the parsed line
*/
int line_executer(tline* line){
    if (line->ncommands > 0){
        int status;
        char* file_name = line->commands->filename;
        if (!file_name){
            printf("The command does not exist\n");
            exit(-1);
        } else{
            printf("%s\n", file_name);
        }
        
        if(fork() == 0){
            char* redirect[3] = {line->redirect_input, line->redirect_output, line->redirect_error};
        if(command_executer(line->commands, redirect) != 0){
                printf("Error -> Executing command in child\n");
                exit(-1);
            }
        } else {
            wait(&status);
            if(WIFEXITED(status) && WEXITSTATUS(status) != 0){
                printf("Error -> Child state is wrong\n");
            }
        }
    }
    return 0;
}
