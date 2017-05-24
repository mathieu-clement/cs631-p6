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
    printf("There are %d pipe(s).\n", nb_pipes);
    
    printf("Commands:\n");
    char** command;
    int* start = (int*) malloc(sizeof(int));
    while ( (command = get_next_command(start, argc, argv)) ) {
        printf("%s\n", command[0]);
        int i = 1;
        while (command[i]) {
            printf("   %s\n", command[i]);
            i++;
        }
    }

    exit(0);

	pid_t id;
	int fd;

	if ((fd = open("out", O_CREAT | O_WRONLY, 0644)) < 0) {
			printf("cannot open out\n");
			exit(1);
	}
	
	id = fork();

	if (id == 0) {
			/* we are in the child */
			/* close stdout in child */
			close(1);
			dup(fd);   /* replace stdout in child with "out" */
			close(fd);
			execl("/bin/date", "date", NULL);
	}

	/* we are in the parent */
	close(fd);
	id = wait(NULL);

    return 0;
}

