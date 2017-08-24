#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

// Returns number of pipelines
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

void write_string(int fd, char* str)
{
    write(fd, str, strlen(str));
}

void make_pipename(char** result, int no, char** command0, char** command1)
{
    asprintf(result, "[%d] %s -> %s\n", no, *command0, *command1);
}

int count_char(char* str, int len, char c)
{
    int count = 0;
    for (int i = 0 ; i < len ; ++i) {
        if (str[i] == c) {
            count++;
        }
    }
    return count;
}

bool is_ascii(char* str, int len)
{
    for (int i = 0 ; i < len ; ++i) {
        if (str[i] > 127) return false;
    }
    return true;
}

void analyze(int* filedes, int logfd)
{
    int buf_size = 16;
    char buf[buf_size];
    int total_bytes_read = 0;
    int nb_bytes_read;
    int total_lines = 0;
    bool ascii = true;
    while ( (nb_bytes_read = read(filedes[0], buf, buf_size)) > 0 ) {
        total_bytes_read += nb_bytes_read;
        total_lines += count_char(buf, nb_bytes_read, '\n');
        ascii &= is_ascii(buf, nb_bytes_read);
    }

    char* bytes_str;
    asprintf(&bytes_str, "%d bytes\n", total_bytes_read);
    write_string(logfd, bytes_str);

    char* lines_str;
    asprintf(&lines_str, "%d lines\n", total_lines);
    write_string(logfd, lines_str);

    char* ascii_data = "ASCII data\n";
    char* binary_data = "Binary data\n";
    if (ascii) {
        write_string(logfd, ascii_data);
    } else {
        write_string(logfd, binary_data);
    }
}

int main(int argc, char* argv[])
{
    // skip program name
    argc--;
    argv++;

    int nb_pipes = count_pipes(argc, argv);
    int nb_commands = nb_pipes + 1;
    if (nb_commands != 3) {
        fprintf(stderr, "Number of commands should be 3, e.g. pa cmd1 | cmd2 | cmd3\n");
        exit(1);
    }

    int logfd;
    if ( (logfd = open("pa.log", O_CREAT | O_WRONLY | O_TRUNC, 0644)) < 0 ) {
        fprintf(stderr, "IO Error: pa.log\n");
        exit(1);
    }

    char** command0;
    char** command1;
    char** command2;
    int* start = (int*) malloc(sizeof(int));
    
    command0 = get_next_command(start, argc, argv);
    command1 = get_next_command(start, argc, argv);
    command2 = get_next_command(start, argc, argv);

    // First process
    int pfd1[2];
    if (pipe(pfd1) < 0) {
        fprintf(stderr, "Could not create pipe.\n");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Could not fork.\n");
        exit(1);
    } else if (pid == 0) {
        close(pfd1[0]);
        dup2(pfd1[1], 1);
        execvp(*command0, command0);
        exit(1);
    } else {
        close(pfd1[1]);
    }

    wait(NULL);

    // Second process
    int pfd2[2];
    if (pipe(pfd2) < 0) {
        fprintf(stderr, "Could not create pipe.\n");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Could not fork.\n");
        exit(1);
    } else if (pid == 0) {
        close(pfd1[1]);
        close(pfd2[0]);

        // [1] seq -> sort
        char* pipe_title;
        make_pipename(&pipe_title, 1, command0, command1);
        write_string(logfd, pipe_title);

        dup2(pfd1[0], 0);
        dup2(pfd2[1], 1);

        analyze(pfd1, logfd);

        close(logfd);
        execvp(*command1, command1);
        exit(1);
    } else {
        close(pfd1[0]);
        close(pfd2[1]);
    }

    wait(NULL);

    // Third process
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Could not fork.\n");
        exit(1);
    } else if (pid == 0) {
        // [2] sort -> wc -l
        char* pipe_title;
        make_pipename(&pipe_title, 2, command1, command2);
        write_string(logfd, pipe_title);

        dup2(pfd2[0], 0);
        analyze(pfd2, logfd);
        close(logfd);
        execvp(*command2, command2);
    }

    wait(NULL);

    close(logfd);

    return 0;

}

