#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid;

    // Crear el pipe
    if (pipe(pipefd) == -1) {
        perror("Error al crear el pipe");
        exit(EXIT_FAILURE);
    }

    // Crear un nuevo proceso
    pid = fork();

    if (pid == -1) {
        perror("Error al crear el proceso hijo");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Proceso hijo
        printf("entrando en el proceso hijo\n");

        // Cerrar el extremo de lectura del pipe en el proceso hijo
        close(pipefd[0]);

        // Redirigir la salida estándar al extremo de escritura del pipe
        dup2(pipefd[1], STDOUT_FILENO);

        // Cerrar el extremo de escritura del pipe en el proceso hijo
        close(pipefd[1]);

        // Especificar la ruta completa del comando ls
        char *args[] = {"/bin/ls", "/bin", NULL};
        execv("/bin/ls", args);

        // En caso de error al ejecutar el comando
        perror("Error al ejecutar el comando en el proceso hijo");
        exit(EXIT_FAILURE);
    } else { // Proceso padre
        printf("entrando en el proceso padre\n");
        // Cerrar el extremo de escritura del pipe en el proceso padre
        close(pipefd[1]);

        // Leer desde el extremo de lectura del pipe
        char buffer[128];
        ssize_t bytesRead;

        printf("leyendo los datos de la pipe desde el padre\n");
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            // Procesar los datos leídos, puedes imprimirlos por ejemplo
            write(STDOUT_FILENO, buffer, bytesRead);
        }

        // Cerrar el extremo de lectura del pipe en el proceso padre
        close(pipefd[0]);

        // Esperar a que el proceso hijo termine
        waitpid(pid, NULL, 0);
    }

    return 0;
}




