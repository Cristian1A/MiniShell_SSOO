
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
#include <sys/mman.h>
#include <sys/shm.h>

/*******************************/
/*        CONSTANT VALUES      */
/*******************************/

#define PROMPT "msh>"
#define MAX_LINE_SIZE 1024
#define REDUCED_LINE_SIZE 256
#define SUPER_REDUCED_LINE_SIZE 128
#define HIPER_SUPER_REDUCED_LINE_SIZE 16
#define FILE_DESCRIPTORS 3
#define WORD_DELIMITER " "
#define MAX_JOBS 15
#define FILE_INDENTIFICATOR 'A'
#define JOBS_IDENTIFICATOR 0

/*******************************/
/*    FUNCTION DECLARATIONS    */
/*******************************/

int upper_executer(tline* line, char* shell_line);
int background_executer(tline* line, char* shell_line);
int foreground_executer(tline* line, char* shell_line);
int multicommand(tline* line, char* shell_line);
int unicommand(tcommand* command, char* input, char* output, int background, char* error, char* command_name);
int multicommand_executer(int command_counter, tcommand* command, int input_fd, int output_fd, int error_fd, char* aux_file_name);
int intern_command(char* shell_line);
int change_cdir(char* new_path);
void SIGINT_handler(int sig);
int create_job(int pidJob, char* commandJob);
int umask_func(char* new_mask);
int show_jobs();

/*******************************/
/*           STRUCTS           */
/********************************/

typedef struct Job{
    pid_t pid;
    char command[SUPER_REDUCED_LINE_SIZE];
    int id;
    char state[HIPER_SUPER_REDUCED_LINE_SIZE];
} Job;

/*******************************/
/*       GLOBAL VARIABLES      */
/*******************************/

Job jobs[MAX_JOBS];
int num_jobs = 0;
int sigint_received = 0;
key_t clave;	//Clave de acceso a la zona de memoria
long int id;	//Identificador de la zona de memoria
int *pmem = NULL;	//Puntero a la zona de memoria

/*******************************/
/*        SIGNAL HANDLERS      */
/*******************************/

void SIGINT_handler(int sig){
    sigint_received = 1;
}

void sigchld_handler(int signum) {
    (void) signum;
    pid_t child_pid;
    int status;
    while ((child_pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < num_jobs; ++i) {
            if (jobs[i].pid == child_pid) {
                if (WIFEXITED(status)) {
                    strcpy(jobs[i].state, "Hecho");
                    num_jobs--;
                } else {
                    strcpy(jobs[i].state, "En ejecución");
                }
            }
        }
    }
}

/*******************************/
/*         MAIN FUNCTION       */
/*******************************/

int main(int argc, char const *argv[]){
    char shell_line[MAX_LINE_SIZE]; tline* parsed_line; int shell_status = 0;
    /* --- Creating shared memory area --- */
    char dir[50];
    getcwd(dir, 50);
	clave = ftok(dir, 33); //Cualquier fichero existente y cualquier int
	id = shmget(clave, sizeof(int) * 100, 0777 | IPC_CREAT);
	pmem = (int *) shmat(id, (char *)0, 0);
    pmem[5] = 0;
    while (!shell_status){
        /********************************/
        /*             <0>              */
        /*        SIGINT handling       */
        /********************************/
        sigint_received = 0;
        if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
            perror("Signal handler\n");
            return -1;
        }
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
        if(intern_command(shell_line) == 2){
            /********************************/
            /*             <3>              */
            /*   Analyze with the parser    */
            /********************************/
            parsed_line = tokenize(shell_line);
            /********************************/
            /*             <4>              */
            /*     Execute the comands      */
            /********************************/
            shell_status = upper_executer(parsed_line, shell_line);
        }        
    }
    return shell_status;
}

int upper_executer(tline* line, char* shell_line){
    if (line->background){
        return background_executer(line, shell_line);
    } else{
        return foreground_executer(line, shell_line);
    }
}

int background_executer(tline* line, char* shell_line){
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
            return unicommand(line->commands, line->redirect_input, line->redirect_output, line->background, line->redirect_error, shell_line);
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
            return multicommand(line, shell_line);
        }else{ /* -> PARENT PROCESS */
            return 0;
        }
        break;
    }
}

int foreground_executer(tline* line, char* shell_line){
    char* first_token; int result;
    switch (line->ncommands){
    case 0:
        return -1;
        break;
    case 1:
        first_token = strtok(shell_line, WORD_DELIMITER);
        result = unicommand(line->commands, line->redirect_input, line->redirect_output, line->background, line->redirect_error, first_token);
        //free(first_token);
        return result;
        break;
    default:
        result = multicommand(line, shell_line);
        return result;
        break;
    }
}

int unicommand(tcommand* command, char* input, char* output, int background, char* error, char* command_name){
    int comunication_pipe[2];
    pid_t pid;

    /* --- SIGNAL HANDLING --- */
    if(background){
        signal(SIGCHLD, sigchld_handler);
    }

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
                strcat(input, ": Error\n");
                perror(input);
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
                strcat(output, ": Error\n");
                perror(output);
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        /* --- Error redirection --- */
        if (error){
            error_fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
            /* --- Crecking error command_namewhile opening file --- */
            if (error_fd == -1){
                strcat(error, ": Error\n");
                perror(error);
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
        strcat(command_name, ": No se encuentra el mandato\n");
        perror(command_name);
        exit(EXIT_FAILURE);
    } else{ /* -> PARENT PROCESS */
        if (background == 0){
            waitpid(pid, NULL, 0);
        }else{
            create_job(getpid(), command->filename);
            shmdt((char *)id); //Desconecta el segmento de memoria compartida
		    shmctl(id,IPC_RMID,0); //Elimina el segmento de memoria compartida
        }
        int aux_df = dup(STDOUT_FILENO);
        dup2(aux_df, STDOUT_FILENO);
        printf("\n");
        close(aux_df);
        close(comunication_pipe[1]);
        close(comunication_pipe[0]);
        return 0;
    }
}

int create_job(int pidJob, char* commandJob){
    struct Job newJob;  
    newJob.pid = pidJob;
    strcpy(newJob.command, commandJob); 
    newJob.id = *pmem;
    jobs[*pmem] = newJob;
    printf("[%i] %i\n", *pmem, newJob.pid);
    printf("Valor antes: %i ", *pmem);
    *pmem = *pmem + 1;
    printf("Valor de num_jobs despues: %i\n", *pmem);
    return 0;
}

int show_jobs(){
    printf("mostrando jobs ");
    printf("%i\n", *pmem);
    for (int i = 0; i < *pmem; ++i) {
        printf("[%i] %s        %s\n", jobs[i].id, jobs[i].state, jobs[i].command);
    }
    return 0;
}

int multicommand(tline* line, char* shell_line){
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
            strcat(line->redirect_input, ": Error\n");
            perror(line->redirect_input);
            return -1;
        }
    }
    /* --- Output redirection --- */
    if (line->redirect_output){
        output_fd = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
        /* --- Crecking error while opening file --- */
        if (output_fd == -1){
            strcat(line->redirect_output, ": Error\n");
            perror(line->redirect_output);
            return -1;
        }
    }
    /* --- Error redirection --- */
    if (line->redirect_error){
        error_fd = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
        /* --- Crecking error while opening file --- */
        if (error_fd == -1){
            strcat(line->redirect_error, ": Error\n");
            perror(line->redirect_error);
            return -1;
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
            /*if(background){
                num_jobs++;
                if (num_jobs < MAX_JOBS) {
                    jobs[num_jobs].pid = getpid();
                    jobs[num_jobs].id = JOBS_IDENTIFICATOR+num_jobs;
                    strcpy(jobs[num_jobs].command, command->filename);
                    printf("[%d] %i\n", num_jobs + 1, pid);
                    num_jobs++;
                    waitpid(pid, NULL, 0);
                } else {
                    printf("Max number of jobs executing in background was reached\n");
                }
            }*/
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
            exit(EXIT_SUCCESS);
        } /* --- FG --- */
        else if (strcmp(first_token, "fg\n\0") == 0){
            printf("fg");
        } /* --- JOBS --- */
        else if (strcmp(first_token, "jobs\n\0") == 0){
            show_jobs();
            return 0;
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