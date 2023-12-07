#include "parser.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>

int redirection_handler(int* nredirections, char** vredirections){
    /* --- Original file descriptors --- */
    int original_descriptors[3] = {dup(STDIN_FILENO), dup(STDOUT_FILENO), dup(STDERR_FILENO)};
    switch (nredirections[0]){
    case 0:
        dup2(original_descriptors[0], STDIN_FILENO);
        break;
    case 1:
        /* code */
        break;
    default:
        //Error
        break;
    }
    
}