
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
#define WORD_DELIMITER " "
volatile sig_atomic_t contador_sigint = 0;

// Compile with -> gcc -Wall minishell.c libparser.a -o minishell -static

int upper_executer(tline* line);
int background_executer(tline* line);
int foreground_executer(tline* line);
int fg_multicommand(tline* line);
int fg_unicommand(tcommand* command, char* input, char* output, char* error);
int fg_multicommand_executer(int command_counter, tcommand *command, int input_fd, int output_fd, int error_fd, char *aux_file_name);
int intern_command(char* shell_line);
int changeD(int counter, char** words);
void SIGINT_handler(int sig);

void SIGINT_handler(int sig){
    printf("\n%s", PROMPT);
}

void kill_son_handler(int sig){
    ++contador_sigint;
    exit(EXIT_SUCCESS);
}

int main(int argc, char const *argv[]){
    char shell_line[MAX_LINE_SIZE]; tline* parsed_line; int shell_status = 0;
    while (!shell_status){
        /********************************/
        /*             <0>              */
        /*        SIGINT handling       */
        /********************************/
        if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
            perror("Signar handler\n");
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
    return 0;
}

int foreground_executer(tline* line){
    switch (line->ncommands){
    case 0:
        return -1;
        break;
    case 1:
        return fg_unicommand(line->commands, line->redirect_input, line->redirect_output, line->redirect_error);
        break;
    default:
        return fg_multicommand(line);
        break;
    }
}

int fg_unicommand(tcommand* command, char* input, char* output, char* error){
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
        if (signal(SIGINT, kill_son_handler) == SIG_ERR) {
            perror("Error al registrar el manejador de señales\n");
            return -1;
        }
        int input_fd; int output_fd; int error_fd;
        // Cerrar extremo de lectura
        close(comunication_pipe[0]);
        // Redirección de entrada
        if (input){
            input_fd = open(input, O_RDONLY);
            /* --- Crecking error while opening file --- */
            if (input_fd == -1){
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        // Redirección de salida
        if (output){
            output_fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
            /* --- Crecking error while opening file --- */
            if (output_fd == -1){
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        // Redirección de error
        if (error){
            error_fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
            /* --- Crecking error while opening file --- */
            if (error_fd == -1){
                perror("open");
                exit(EXIT_FAILURE);
            }
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);
        }
        execv(command->filename, command->argv);
        // Si falla y error_fd != NULL escribir en el file asociado al descriptor de fichero
        printf("execv: Bad address\n");
        exit(EXIT_FAILURE);
    } else{
        waitpid(pid, NULL, 0);
        close(comunication_pipe[1]);
        close(comunication_pipe[0]);
        return 0;
    }
}

int fg_multicommand(tline* line){
    // Open redirection files
    int input_fd = dup(STDIN_FILENO);
    int output_fd = dup(STDOUT_FILENO);
    int error_fd = dup(STDERR_FILENO);
    // Redirección de entrada
    if (line->redirect_input){
        input_fd = open(line->redirect_input, O_RDONLY);
        /* --- Crecking error while opening file --- */
        if (input_fd == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }
    }
    // Redirección de salida
    if (line->redirect_output){
        output_fd = open(line->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
        /* --- Crecking error while opening file --- */
        if (output_fd == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }
    }
    // Redirección de error
    if (line->redirect_error){
        error_fd = open(line->redirect_error, O_WRONLY | O_CREAT | O_TRUNC, 0666); 
        /* --- Crecking error while opening file --- */
        if (error_fd == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }
    }
    // Fichero auxiliar
    char* aux_file_name = "aux_file.ms";

    int result = fg_multicommand_executer(line->ncommands, line->commands, input_fd, output_fd, error_fd, aux_file_name);
    close(output_fd);
    close(error_fd);
    return result;
}

int fg_multicommand_executer(int command_counter, tcommand* command, int input_fd, int output_fd, int error_fd, char* aux_file_name){
    int first = 1;
    int despl = 0;
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
        if (pid < 0){
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0){
            if (signal(SIGINT, kill_son_handler) == SIG_ERR) {
                perror("Error al registrar el manejador de señales\n");
                return -1;
            }
            if (contador_sigint != 0){
                contador_sigint = 0;
                exit(EXIT_SUCCESS);
            }
            
            // Cerrar extremo de lectura
            close(comunication_pipe[0]);
            // La entrada ya está siendo redirigida
            if (!first){
                input_fd = open(aux_file_name, O_RDONLY);
                /* --- Crecking error while opening file --- */
                if (input_fd == -1){
                    perror("open");
                    exit(EXIT_FAILURE);
                }
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            } else{
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }

            // Redirigir salida siempre a pipe
            if(dup2(comunication_pipe[1], STDOUT_FILENO) == -1){
                perror("redir to pipe");
                exit(EXIT_FAILURE);
            }
            close(comunication_pipe[1]);

            // Redirigir preventivamente el error
            dup2(error_fd, STDERR_FILENO);
            close(error_fd);

            char* destination = (command+despl)->filename;
            char** argsv = (command+despl)->argv;
            execv(destination, argsv);
            // Si falla y error_fd != NULL escribir en el file asociado al descriptor de fichero
            printf("execv: Bad address\n");
            exit(EXIT_FAILURE);
        } else{
            waitpid(pid, NULL, 0);
            // Cerrar el extremo de escritura
            close(comunication_pipe[1]);
            // Resetear la redirección de salida
            int new_out_fd = dup(STDOUT_FILENO);
            dup2(new_out_fd, STDOUT_FILENO);
            close(new_out_fd);
            // Abrir el fichero axiliar para la escritura
            int aux_file_fd = open(aux_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (aux_file_fd == -1){
                perror("open");
                exit(EXIT_FAILURE);
            }

            char buffer[128];
            ssize_t bytesRead;
            while ((bytesRead = read(comunication_pipe[0], buffer, sizeof(buffer))) > 0) {
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
        if (strcmp(first_token, "cd\n\0") == 0){
            printf("cd");
        } /* --- EXIT --- */
        else if (strcmp(first_token, "exit\n\0") == 0){
            exit(EXIT_SUCCESS);
        } /* --- FG --- */
        else if (strcmp(first_token, "fg\n\0") == 0){
            printf("fg");
        } /* --- JOBS --- */
        else if (strcmp(first_token, "jobs\n\0") == 0){
            printf("jobs");
        }/* --- UMASK --- */
        else if (strcmp(first_token, "umask\n\0") == 0){
            printf("umask");
        }
    } else{
        return 3; // -> Empty line
    }
    return 2; // Try other command
}

int changeD(int counter, char** words){
    char *dir;
	char buffer[512];
	
    printf( "El directorio ANTERIOR es: %s\n", getcwd(buffer, sizeof(buffer)));

	if(counter > 2)
	{
	  fprintf(stderr, "Numero de argumentos inválido.\n");
	  return 1;
	}
	
	if (counter == 1)
	{
		dir = getenv("HOME");
		if(dir == NULL)
		{
		  perror("No existe la variable $HOME\n");
          return 1;
		}
	}
	else 
	{
        //Elimina los saltos de línea del directorio al que se quiere cambiar
        if (words[1][strlen(words[1]) - 1] == '\n') {
        words[1][strlen(words[1]) - 1] = '\0';
        }
		dir = words[1];
	}
	
	// Comprobar si es un directorio
	if (chdir(dir) != 0) {
		perror("Error al cambiar de directorio\n");
        printf( "El directorio actual es: %s\n", getcwd(buffer, sizeof(buffer)));
        return 1;
    }
	printf( "El directorio ACTUAL es: %s\n", getcwd(buffer, sizeof(buffer)));

	return 0;
}
