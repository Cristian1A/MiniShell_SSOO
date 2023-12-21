
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


/*              	1) Display the prompt                       */
/*                                                              */
/*              	2) Read a line from stdin                   */
/*                                                              */
/*              	3) Analyze that line with the parser        */
/*                                                              */
/*              	4) Execute the commands of the line         */


// Compile with -> gcc -Wall minishell.c libparser.a -o minishell -static


/*******************************/
/*           LIBRARIES         */
/*******************************/

#include "parser.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <pwd.h>
#include <ctype.h>

/*******************************/
/*        CONSTANT VALUES      */
/*******************************/

#define PROMPT "msh>"
#define MAX_LINE_SIZE 1024
#define REDUCED_LINE_SIZE 256
#define SUPER_REDUCED_LINE_SIZE 128
#define FILE_DESCRIPTORS 3
#define WORD_DELIMITER " "
#define MAX_JOBS 15
#define FILE_INDENTIFICATOR 'A'

/*******************************/
/*    FUNCTION DECLARATIONS    */
/*******************************/

int upper_executer(tline* line);
int background_executer(tline* line);
int foreground_executer(tline* line);
int multicommand(tline* line);
int unicommand(tcommand* command, char* input, char* output, int background, char* error);
int multicommand_executer(int command_counter, tcommand *command, int input_fd, int output_fd, int error_fd, char *aux_file_name);
int intern_command(char* shell_line);
int change_cdir(char* new_path);
int exit_func();
int show_jobs();
int umask_func(char* new_mask);
void SIGINT_handler(int sig);

/*******************************/
/*           STRUCTS           */
/*******************************/

struct Job {
    pid_t pid;
    char command[REDUCED_LINE_SIZE];
};

/*******************************/
/*       GLOBAL VARIABLES      */
/*******************************/

struct Job jobs[MAX_JOBS];
int num_jobs = 0;
int sigint_received = 0;

/*******************************/
/*        SIGNAL HANDLERS      */
/*******************************/

void SIGINT_handler(int sig){
    sigint_received = 1;
}

void sigchld_handler(int signum) {
    /* --- Used to avoid unused parameter warning --- */
    (void) signum;
    pid_t child_pid;
    int status;

    /* --- Waiting to all the childs which have changed their state to be handled --- */
    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* --- Seeking job associated to child's pid --- */
        for (int i = 0; i < num_jobs; ++i) {
            if (jobs[i].pid == child_pid) {
                if (WIFEXITED(status)) {
                    printf("[%d] %s ha terminado. Estado: %d\n", i + 1, jobs[i].command, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)){
                    printf("[%d] %s ha terminado debido a la señal %d\n", i + 1, jobs[i].command, WTERMSIG(status));
                }
                break;
            }
        }
    }
}

/*******************************/
/*         MAIN FUNCTION       */
/*******************************/

int main(int argc, char const *argv[]){
    char shell_line[MAX_LINE_SIZE]; tline* parsed_line; int shell_status = 0;
    if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
        perror("Signal handler\n");
        return -1;
    }
    while (!shell_status){
        /********************************/
        /*             <0>              */
        /*        SIGINT handling       */
        /********************************/
        sigint_received = 0;
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
            exit(EXIT_FAILURE);
        }
        int int_res = intern_command(shell_line);
        //printf("%d\n", int_res);
        if(int_res == 2){
            printf("PASSING HERE\n");
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
    }
    return shell_status;
}

int upper_executer(tline* line){
    if (line->background){
        return background_executer(line);
    } else{
        return foreground_executer(line);
    }
}

int background_executer(tline* line){
    pid_t pid;
    switch (line->ncommands){
    case 0:
        return -1;
        break;
    case 1:
        pid = fork();
        if (pid < 0){ /* -> FORK ERROR */
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0){ /* -> CHILD PROCESS */
            return unicommand(line->commands, line->redirect_input, line->redirect_output, line->background, line->redirect_error);
        } else{ /* -> PARENT PROCESS */
            return 0;
        }
        break;
    default:
        pid = fork();
        if (pid < 0){  /* -> FORK ERROR */
            perror("fork");
            exit(EXIT_FAILURE);
        }else if (pid == 0){ /* -> CHILD PROCESS */
            return multicommand(line);
        }else{ /* -> PARENT PROCESS */
            return 0;
        }
        break;
    }
}

int foreground_executer(tline* line){
    switch (line->ncommands){
    case 0:
        return -1;
        break;
    case 1:
        return unicommand(line->commands, line->redirect_input, line->redirect_output, line->background, line->redirect_error);
        break;
    default:
        return multicommand(line);
        break;
    }
}

int unicommand(tcommand* command, char* input, char* output, int background, char* error){
    int comunication_pipe[2];
    pid_t pid;

    /* --- SIGNAL HANDLING --- */
    signal(SIGCHLD, sigchld_handler);

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
        int input_fd; int output_fd; int error_fd;
        if (background){
            signal(SIGINT, SIGINT_handler);
        } else{
            signal(SIGINT, SIG_DFL);
        }
        /* --- Closing pipe for reading --- */
        close(comunication_pipe[0]);
        /* --- Input redirection --- */
        if (input){
            input_fd = open(input, O_RDONLY);
            /* --- Crecking error while opening file --- */
            if (input_fd == -1){
                perror("fichero: Error");
                exit(EXIT_FAILURE);
            }
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        /* --- Output redirection --- */
        if (output){
            output_fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
            /* --- Crecking error while opening file --- */
            if (output_fd == -1){
                perror("fichero: Error");
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        /* --- Error redirection --- */
        if (error){
            error_fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
            /* --- Crecking error while opening file --- */
            if (error_fd == -1){
                perror("fichero: Error");
                exit(EXIT_FAILURE);
            }
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);
        }
        if (!background && sigint_received){
            sigint_received = 0;
            exit(EXIT_FAILURE);
        }
        execv(command->filename, command->argv);
        printf("mandato: No se encuentra el mandato\n");
        exit(EXIT_FAILURE);
    } else{ /* -> PARENT PROCESS */
        if (background == 0){
            waitpid(pid, NULL, 0);
            int aux_df = dup(STDOUT_FILENO);
            dup2(aux_df, STDOUT_FILENO);
            close(aux_df);
            printf("\n");
        } else if(background == 1){
            if (num_jobs < MAX_JOBS) {
                jobs[num_jobs].pid = pid;
                strcpy(jobs[num_jobs].command, command->filename);
                printf("[%d] %i\n", num_jobs + 1, pid);
                num_jobs++;
            } else {
                printf("Max number of jobs executing in background was reached\n");
            }
        }
        close(comunication_pipe[1]);
        close(comunication_pipe[0]);
        return 0;
    }
}

int multicommand(tline* line){
    signal(SIGCHLD, sigchld_handler);
    if (line->background){
        signal(SIGINT, SIGINT_handler);
    } else{
        signal(SIGINT, SIG_DFL);
    }
    static int adder = 0;
    /* --- Default redirections --- */
    int input_fd = dup(STDIN_FILENO);
    int output_fd = dup(STDOUT_FILENO);
    int error_fd = dup(STDERR_FILENO);
    /* --- Input redirection --- */
    if (line->redirect_input){
        input_fd = open(line->redirect_input, O_RDONLY);
        /* --- Crecking error while opening file --- */
        if (input_fd == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }
    }
    /* --- Output redirection --- */
    if (line->redirect_output){
        output_fd = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
        /* --- Crecking error while opening file --- */
        if (output_fd == -1){
            perror("fichero: Error");
            exit(EXIT_FAILURE);
        }
    }
    /* --- Error redirection --- */
    if (line->redirect_error){
        error_fd = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
        /* --- Crecking error while opening file --- */
        if (error_fd == -1){
            perror("fichero: Error");
            exit(EXIT_FAILURE);
        }
    }
    /* --- Auxiliary file --- */
    char aux_file_name[SUPER_REDUCED_LINE_SIZE];
    /* --- Generating file name --- */
    if(line->background){
        adder++;
    }
    sprintf(aux_file_name, "file_%c.ms", FILE_INDENTIFICATOR + adder);

    int result = multicommand_executer(line->ncommands, line->commands, input_fd, output_fd, error_fd, aux_file_name);
    close(output_fd);
    close(error_fd);
    return result;
}

int multicommand_executer(int command_counter, tcommand* command, int input_fd, int output_fd, int error_fd, char* aux_file_name){
    int first = 1; int despl = 0;
    char* destination;
    char** argsv;
    while (command_counter > 0){
        int comunication_pipe[2];
        pid_t pid;

        /* --- PIPE CREATION --- */
        if (pipe(comunication_pipe) == -1){
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        /* --- FORK --- */
        pid = fork();
        if (pid < 0){ /* -> FORK ERROR */
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0){ /* -> CHILD PROCESS */
            /* --- Closing pipe for reading --- */
            close(comunication_pipe[0]);
            /* --- Input redirection --- */
            if (!first){
                input_fd = open(aux_file_name, O_RDONLY);
                /* --- Crecking error while opening file --- */
                if (input_fd == -1){
                    perror("fichero: Error");
                    exit(EXIT_FAILURE);
                }
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            } else{
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }

            /* --- Output redirection to pipe --- */
            if(dup2(comunication_pipe[1], STDOUT_FILENO) == -1){
                perror("redir to pipe");
                exit(EXIT_FAILURE);
            }
            close(comunication_pipe[1]);

            /* --- Error redirection --- */
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);

            destination = (command + despl)->filename;
            argsv = (command + despl)->argv;
            execv(destination, argsv);
            printf("mandato: No se encuentra el mandato\n");
            exit(EXIT_FAILURE);
        } else{ /* -> PARENT PROCESS */
            waitpid(pid, NULL, 0);
            /* --- Closing pipe for writing --- */
            close(comunication_pipe[1]);
            /* --- Resetting output redirection --- */
            int new_out_fd = dup(STDOUT_FILENO);
            dup2(new_out_fd, STDOUT_FILENO);
            close(new_out_fd);
            /* --- Opening auxiliary file for writing --- */
            int aux_file_fd = open(aux_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (aux_file_fd == -1){
                perror("open");
                exit(EXIT_FAILURE);
            }

            char buffer[SUPER_REDUCED_LINE_SIZE];
            ssize_t bytesRead;
            while ((bytesRead = read(comunication_pipe[0], buffer, sizeof(buffer))) > 0){
                // Procesando los datos leídos
                if (command_counter == 1){
                    write(output_fd, buffer, bytesRead);
                } else{
                    write(aux_file_fd, buffer, bytesRead);
                }
            }
            close(aux_file_fd);
            close(comunication_pipe[0]);
            first = 0;
            despl += 1;
            command_counter -= 1;
        }
    }
    return 0;
}

int intern_command(char* shell_line){
    char* second_line = strdup(shell_line);
    if (!second_line){
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    char* first_token = strtok(second_line, WORD_DELIMITER);
    char* second_token = strtok(NULL, WORD_DELIMITER);
    if (first_token != NULL){
        /* --- CD --- */
        if ((strncmp(first_token, "cd\0", 3) == 0) || (strncmp(first_token, "cd\n", 3) == 0)){
            return change_cdir(second_token);
        } /* --- EXIT --- */
        else if (strcmp(first_token, "exit\n\0") == 0){
            return exit_func();
        } /* --- FG --- */
        else if (strcmp(first_token, "fg\n\0") == 0){
            printf("fg");
        } /* --- JOBS --- */
        else if (strcmp(first_token, "jobs\n\0") == 0){
            return show_jobs();
        } /* --- UMASK --- */
        else if ((strncmp(first_token, "umask\0", 6) == 0) || (strncmp(first_token, "umask\n", 6) == 0)){
            return umask_func(second_token);
        } /* --- EMPTY LINE --- */
        else if (strcmp(first_token, "\n\0") == 0){
            return 3; // -> Empty line
        }
    }
    return 2; // Try other command
}

int change_cdir(char* new_path){
    char new_cwd[MAX_LINE_SIZE];
    /* --- cd - no params --- */
    if (new_path == NULL){
        struct passwd *pw = getpwuid(getuid());
        if (pw != NULL){
            if (chdir(pw->pw_dir) != 0){
                perror("chdir");
                return -1; // Error
            }
            printf("%s\n", getcwd(new_cwd, sizeof(new_cwd)));
            return 0;
        }
        return -1;
    } else{
        int string_size = strcspn(new_path, "\n");
        memmove(new_path, new_path, string_size);
        new_path[string_size] = '\0';
        if (chdir(new_path) != 0){
            perror("chdir");
            return -1; // Error
        }
        printf("%s\n", getcwd(new_cwd, sizeof(new_cwd)));
    }
    return -1;
}

int exit_func(){
    exit(EXIT_SUCCESS);
}

int show_jobs(){
    return 0;
}

int umask_func(char* new_mask){
    mode_t aux_mask; int mask_str_len;
    /* --- Umask execution without parameter --- */
    if (new_mask == NULL){
        aux_mask = umask(0);
        printf("%d\n", aux_mask);
        umask(aux_mask);
    } else{ /* --- Umask execution with parameter --- */
        mask_str_len = strlen(new_mask);
        if (mask_str_len != 5){
            printf("Invalid parameter\n");
            return -1;
        }
        /* --- Checking if all new_mask's characters are in octal --- */
        for (int i = 0; i < mask_str_len - 1; i++){
            if (new_mask[i] < '0' && new_mask[i] > '7'){
                printf("Invalid parameter\n");
                return -1;
            }
        }
        aux_mask = atoi(new_mask);
        printf("%d\n", aux_mask);
        umask(aux_mask);
        printf("Applied new umask value\n");
    }
    return 0;
}