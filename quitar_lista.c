

#include <string.h>


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

int switch_(struct SharedData* my_lista, int pos);

typedef struct SharedData {
    int numJobs;
    Job listaJobs[MAX_JOBS];
} SharedData;
typedef struct Job{
    int pid;
    char command[HIPER_SUPER_REDUCED_LINE_SIZE];
    int id;
    char state[HIPER_SUPER_REDUCED_LINE_SIZE];
} Job;

int main(int argc, char const *argv[])
{
    /* code */
    return 0;
}


int order(struct SharedData* my_lista){
    for (int i = my_lista->numJobs; i > 0; i--){
        if (strcmp(my_lista->listaJobs[i].state, "DONE\0") == 0){
            return switch_(my_lista, i);
        }   
    }
    return 0;
}

int switch_(struct SharedData* my_lista, int pos){
    for (int i = pos; i < my_lista->numJobs - 2; i++){
        my_lista->listaJobs[pos] = my_lista->listaJobs[pos + 1];
        pos += 1;
    }
    return 0;
}