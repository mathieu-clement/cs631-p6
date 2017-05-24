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

    // char** command;
    int* start = (int*) malloc(sizeof(int));
    // while ( (command = get_next_command(start, argc, argv)) ) {
    //     printf("%s\n", command[0]);
    //     int i = 1;
    //     while (command[i]) {
    //         printf("   %s\n", command[i]);
    //         i++;
    //     }
    //     execvp(command[0], command);
    // }

    char** command0 = get_next_command(start, argc, argv);
    char** command1 = get_next_command(start, argc, argv);

    int pipes[2];
    if (pipe(pipes) < 0) {
        fprintf(stderr, "Could not create pipe.\n");
    }

    pid_t child_id = fork();
    if (child_id < 0) {
        fprintf(stderr, "Could not fork.\n");
    } else if (child_id == 0) {
        // we are in the child
        dup2(pipes[0], 0); // redirect stdin
        close(pipes[0]); 
        close(pipes[1]); 
        execvp(command1[0], command1);
        fprintf(stderr, "Could not execute command (child).\n");
    } else {
        // we are in the parent
        child_id = fork();
        if (child_id == 0) {
            dup2(pipes[1], 1);
            close(pipes[0]); 
            close(pipes[1]); 
            execvp(command0[0], command0);
        } else {
            close(pipes[0]);
            close(pipes[1]);
            wait(NULL);
        }
    }

    return 0;

}

