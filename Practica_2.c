
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
#include <stdlib.h>
#include <stdio.h>
#define PROMPT "msh>"
#define MAX_LINE_SIZE 1024

int main(int argc, char const *argv[])
{
    char shell_line[MAX_LINE_SIZE];
    tline* parsed_line;

	// 1) Display the promt
    printf("%s ", PROMPT);
    // 2) Read a line from stdin
    if(!fgets(shell_line, MAX_LINE_SIZE, stdin)){
        printf("The line cannot be read\n");
        exit(-1);
    }
    // 3) Analyze that line with the parser
    parsed_line = tokenize(shell_line);

    // Commands in line
    printf("%d", parsed_line->ncommands);

    // 4) Execute the comands of the line
    // TODO
    return 0;
}