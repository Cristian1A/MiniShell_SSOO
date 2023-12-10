
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

// Compile with -> gcc -Wall aux.c libparser.a -o aux -static

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
    printf("%s", line->commands[0].filename);
    printf("%s", line->commands[1].filename);

    return 0;
}