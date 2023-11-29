
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

int execute_command(tcommand* command, char** redirections){
    int file_descriptor[REDIRECTION_SOURCES];
    /* --- Checking whether the command exists or not --- */
    if (command->filename){
        /* --- Cheking file redirections --- */
        for (int i = 0; i < REDIRECTION_SOURCES; i++){
            if (redirections[i]){
                if (i == 0){
                    file_descriptor[i] = open(redirections[i], O_RDONLY);
                } else{
                    file_descriptor[i] = open(redirections[i], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                }
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
        return execv(command->filename, command->argv);
    } /* --- The command does not exist --- */
    return -1;
}

int execute_line(tline* line){
    char* redirections[REDIRECTION_SOURCES] = {
        line->redirect_input,
        line->redirect_output,
        line->redirect_error
    };

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
        result = execute_command(line->commands, redirections);
        /* --- 0 for success and any other value for failure --- */
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