
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
#define PROMPT "msh>"
#define MAX_LINE_SIZE 1024

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
        line_executer(parsed_line);
    }
    return 0;
}

/*
    Executes a specific command
    Args:
    - tcommand* command - command to execute
*/
int command_executer(tcommand* command){
    return execv(command->filename, command->argv);
}

/*
    Analyzes a parsed line
    Args:
    - tline* line - the parsed line
*/
int line_executer(tline* line){
    if (line->ncommands == 1){
        command_executer(line->commands);
    }
    
    for (int i = 0; i < line->ncommands; i++){
        printf("%s\n", line->commands[i].filename);
    }
    
    return 0;
}