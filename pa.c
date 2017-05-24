#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

int count_pipes(int argc, char* argv[])
{
    int count = 0;
    for (int i = 0 ; i < argc ; ++i) {
        if (argv[i][0] == '|' && argv[i][1] == 0) {
            count++;
        }
    }
    return count;
}

// Returns all elements from argv[start] until the pipe symbol is encountered 
// or the end of the argv[] array is reached
char** get_next_command(int* start, int argc, char* argv[])
{
    if ((*start) + 1 >= argc) return NULL;

    char** result = (char**) malloc(sizeof(char*) * argc-(*start)+1); // max # of elements. +1 because NULL at the end
    int result_idx = 0;
    int i;
    for (i = *start ; i < argc && argv[i][0] != '|' ; ++i) {
        result[result_idx++] = argv[i];
    }
    *start = i + 1;
    result[result_idx++] = NULL;
    return result;
}

int main(int argc, char* argv[])
{
    // skip program name
    argc--;
    argv++;

    int nb_pipes = count_pipes(argc, argv);
    int nb_commands = nb_pipes + 1;
    printf("There are %d pipe(s).\n", nb_pipes);
    
    int pipes01[2];
    pipe(pipes01);

    int pipes12[2];
    pipe(pipes12);

    // stdin --> (pipes01[0] pipes01[1]) -> (pipes12[0] pipes12[1]) -> stdout

    char** command0;
    char** command1;
    char** command2;
    int* start = (int*) malloc(sizeof(int));
    
    command0 = get_next_command(start, argc, argv);
    command1 = get_next_command(start, argc, argv);
    command2 = get_next_command(start, argc, argv);

    pid_t pid = fork();
    if (pid == 0) {
        dup2(pipes01[1], 1);
        execvp(*command0, command0);
        exit(1);
    } else {
        close(pipes01[1]);
    }

    pid = fork();
    if (pid == 0) {
        dup2(pipes01[0], 0);
        dup2(pipes12[1], 1);
        execvp(*command1, command1);
        exit(1);
    } else {
        close(pipes12[1]);
    }

    pid = fork();
    if (pid == 0) {
        dup2(pipes12[0], 0);
        execvp(*command2, command2);
    }

    wait(NULL);

    return 0;

}

