
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
#include <ctype.h>

#define PROMPT "msh>"
#define MAX_LINE_SIZE 1024
#define FILE_DESCRIPTORS 3
#define MAX_JOBS 15
// Compile with -> gcc -Wall minishell.c libparser.a -o minishell -static

int es_cadena_solo_espacios(char *cadena);
int upper_executer(tline* line);
int background_executer(tline* line);
int foreground_executer(tline* line, char* name);
int fg_multicommand(tline* line);
int fg_unicommand(tcommand* command, char* input, char* output, int background, char* error);
int fg_multicommand_executer(int command_counter, tcommand *command, int input_fd, int output_fd, int error_fd, char *aux_file_name, int background);
int InternOp(char* shell_line);
int changeD(int counter, char** words);
int show_jobs();

struct Job {
    pid_t pid;
    char command[256];
};

struct Job jobs[MAX_JOBS];
int num_jobs = 0;

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

        if(es_cadena_solo_espacios(shell_line) != 1){
            if(InternOp(shell_line) == 2){
                /********************************/
                /*             <3>              */
                /*   Analyze with the parser    */
                /********************************/

                parsed_line = tokenize(shell_line);
                /********************************/
                /*             <4>              */
                /*     Execute the comands      */
                /********************************/
                shell_status = foreground_executer(parsed_line, shell_line);
            }   
        }

    }
    return shell_status;
}

int es_cadena_solo_espacios(char *cadena) {
    while (*cadena) {
        if (!isspace((unsigned char)*cadena)) {
            return 0;  // La cadena contiene al menos un carácter que no es un espacio en blanco
        }
        cadena++;
    }
    return 1;  // La cadena está formada solo por espacios en blanco
}

int man_error(char* phrase){    
    char* primeraPalabra;
    // Utilizar strtok para obtener la primera palabra
    char* token = strtok(phrase, " ");

    // Verificar si se obtuvo la primera palabra
    if (token != NULL) {
        // Crear una copia de la palabra para devolverla
        primeraPalabra = strdup(token);
    }
    printf("%s: No se encuentra el mandato\n", primeraPalabra);
    return 0;
}
int foreground_executer(tline* line, char* name){

    /*if(line->commands->filename == NULL){
        man_error(name);
        return 0;
    }*/
    switch (line->ncommands){
    case 0:
        return -1;
        break;
    case 1:
        return fg_unicommand(line->commands, line->redirect_input, line->redirect_output, line->background, line->redirect_error);
        break;
    default:
        return fg_multicommand(line);
        break;
    }
}

int fg_unicommand(tcommand* command, char* input, char* output, int background, char* error){
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
        //if(background == 0){
        waitpid(pid, NULL, 0);
        /*}else if(background == 1){
            if (num_jobs < MAX_JOBS) {
                jobs[num_jobs].pid = pid;
                strcpy(jobs[num_jobs].command, name);
                printf("[%d] %i\n", num_jobs + 1, pid);
                num_jobs++;
            } else {
                printf("Número máximo de trabajos en segundo plano alcanzado.\n");
            }
        }*/
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

    int result = fg_multicommand_executer(line->ncommands, line->commands, input_fd, output_fd, error_fd, aux_file_name, line->background);
    close(output_fd);
    close(error_fd);
    return result;
}


int fg_multicommand_executer(int command_counter, tcommand* command, int input_fd, int output_fd, int error_fd, char* aux_file_name, int background){
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
            if(background == 0){
                waitpid(pid, NULL, 0);
            }
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

/* --- Comprobador de instrucciones internas  --- */
int InternOp(char* shell_line){
    char *miString = strdup(shell_line);
    if (miString == NULL) {
        perror("Error al duplicar la cadena");
        exit(EXIT_FAILURE);
    }
    // Declarar un puntero a un array de strings para almacenar las palabras
    char **miArray = NULL;

    // Contar la cantidad de palabras en myString
    char *token = strtok(miString, " ");
    int numPalabras = 0;

    while (token != NULL) {
        numPalabras++;
        token = strtok(NULL, " ");
    }

    // Asignar memoria para el array de strings
    miArray = (char **)malloc(numPalabras * sizeof(char *));
    if (miArray == NULL) {
        perror("Error al asignar memoria");
        free(miString);  // Liberar la memoria de miString antes de salir
        exit(EXIT_FAILURE);
    }

    // Reiniciar el string para volver a utilizar strtok
    strcpy(miString, shell_line);

    // Almacenar cada palabra en el array de strings
    token = strtok(miString, " ");
    int indice = 0;

    while (token != NULL) {
        // Asignar memoria para la palabra y copiarla al array
        miArray[indice] = strdup(token);

        // Obtener la siguiente palabra
        token = strtok(NULL, " ");

        indice++;
    }

    /* --- Filtrar instrucciones internas --- */
    if (strncmp(miArray[0], "cd\n", 3) == 0 || strncmp(miArray[0], "cd\0", 3) == 0) {
        changeD(numPalabras, miArray);
    }else if(strncmp(miArray[0], "exit\n", strlen("exit\n")) == 0){
        exit(1);
    }/*else if(strncmp(miArray[0], "jobs", strlen("jobs")) == 0){
        show_jobs();
    }*/else{
        // Liberar la memoria asignada para cada palabra y el array de strings
        for (int i = 0; i < numPalabras; i++) {
            free(miArray[i]);
        }
        free(miArray);
        free(miString);
        return 2;
    }

    // Liberar la memoria asignada para cada palabra y el array de strings
    for (int i = 0; i < numPalabras; i++) {
        free(miArray[i]);
    }
    
    free(miArray);
    free(miString);

    return 0;
}

/* --- Cambiar directorio --- */
int changeD(int counter, char** words){
    char *dir;
	char buffer[512];
	
	if(counter > 2)
	{
	  fprintf(stderr, "cd: demasiados argumentos\n");
	  return 1;
	}
	
	if (counter == 1)
	{
		dir = getenv("HOME");
        if(dir == NULL)
		{
		  fprintf(stderr, "cd: No existe la variable $HOME\n");
          return 1;
		}
	}
	else 
	{
        if (words[1][strlen(words[1]) - 1] == '\n') {
        words[1][strlen(words[1]) - 1] = '\0';
        }
		dir = words[1];
	}
	
	// Comprobar si es un directorio
	if (chdir(dir) != 0) {
		printf("cd: %s: No exite el archivo o el directorio\n", dir);
        return 1;
    }
	printf( "%s\n", getcwd(buffer, sizeof(buffer)));

	return 0;
}

/*int show_jobs(){
    //checkear los estado de cada proceso en background
    for (int i = 0; i < num_jobs; i++) {
        printf("[%d]                   %s\n", i + 1, jobs[i].command);
    }
    return 0;
}*/
