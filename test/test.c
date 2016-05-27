#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>

#define PIPE_READ_END_INDEX 0
#define PIPE_WRITE_END_INDEX 1
int childStdoutPipe[2];
int childStderrPipe[2];
int childStdinPipe[2];

int initPipes(void)
{
    int r;
    r = pipe(childStdoutPipe);
    if (r != 0)
    {
        return r;
    }
    r = pipe(childStderrPipe);
    if (r != 0)
    {
        return r;
    }
    r = pipe(childStdinPipe);
    return r;
}


void parentProcess(void)
{
    printf("I am the parent\n");

    // Close the fds not required by the parent process
    close(childStdinPipe[PIPE_READ_END_INDEX]);
    close(childStdoutPipe[PIPE_WRITE_END_INDEX]);
    close(childStderrPipe[PIPE_WRITE_END_INDEX]);

    printf(
        "The fds I will be checking are: childStdOut=%d, childStdErr=%d, stdin=%d\n",
        childStdoutPipe[PIPE_READ_END_INDEX],
        childStderrPipe[PIPE_READ_END_INDEX],
        STDIN_FILENO);

    while (true)
    {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(childStdoutPipe[PIPE_READ_END_INDEX], &set);
        FD_SET(childStderrPipe[PIPE_READ_END_INDEX], &set);
        FD_SET(STDIN_FILENO, &set);
        int selectResult = select(FD_SETSIZE, &set, NULL, NULL, NULL);
        if (selectResult == -1)
        {
            fprintf(stderr, "select() failed\n");
            exit(1);
        }

        char readBuffer[128];
        ssize_t count;
        if (FD_ISSET(childStdoutPipe[PIPE_READ_END_INDEX], &set))
        {
            count = read(
                childStdoutPipe[PIPE_READ_END_INDEX], readBuffer, sizeof(readBuffer) - 1);
            if (count > 0)
            {
                readBuffer[count] = 0;
                printf("Child stdout: %s\n", readBuffer);
            }
            else if (count == 0)
            {
                fprintf(stderr, "Weird, EOF reached on child stdout\n");
                exit(1);
            }
            else
            {
                fprintf(stderr, "IO Error while reading from child process stdout\n");
                exit(1);
            }
        }
        else if (FD_ISSET(childStderrPipe[PIPE_READ_END_INDEX], &set))
        {
            count = read(
                childStderrPipe[PIPE_READ_END_INDEX], readBuffer, sizeof(readBuffer) - 1);
            if (count > 0)
            {
                readBuffer[count] = 0;
                printf("Child stderr: %s\n", readBuffer);
            }
            else if (count == 0)
            {
                fprintf(stderr, "Weird, EOF reached on child stderr\n");
                exit(1);
            }
            else
            {
                fprintf(stderr, "IO Error while reading from child process stderr\n");
                exit(1);
            }
        }
        else if (FD_ISSET(STDIN_FILENO, &set))
        {
            count = read(STDIN_FILENO, readBuffer, sizeof(readBuffer) - 1);
            if (count > 0)
            {
                readBuffer[count] = 0;
                printf("writing to the stdin of the child: %s\n", readBuffer);
                write(childStdinPipe[PIPE_WRITE_END_INDEX], readBuffer, count);
            }
            else if (count == 0)
            {
                fprintf(stderr, "Weird, EOF reached on stdin\n");
                close(childStdinPipe[PIPE_WRITE_END_INDEX]);
                //exit(1);
            }
            else
            {
                fprintf(stderr, "IO Error while reading stdin from the parent process stdin\n");
                exit(1);
            }
        }
    }
}


void childProcess(void)
{
    // This is the child process
    printf("I am the child\n");

    //close(STDIN_FILENO);
    //close(STDOUT_FILENO);
    //close(STDERR_FILENO);

    dup2(childStdinPipe[PIPE_READ_END_INDEX], STDIN_FILENO);
    dup2(childStdoutPipe[PIPE_WRITE_END_INDEX], STDOUT_FILENO);
    dup2(childStderrPipe[PIPE_WRITE_END_INDEX], STDERR_FILENO);
    //dup2(STDIN_FILENO, childStdinPipe[PIPE_READ_END_INDEX]);
    //dup2(STDOUT_FILENO, childStdoutPipe[PIPE_WRITE_END_INDEX]);
    //dup2(STDERR_FILENO, childStderrPipe[PIPE_WRITE_END_INDEX]);

    // Close fds not required by the child process
    //close(childStdinPipe[PIPE_READ_END_INDEX]);
    close(childStdinPipe[PIPE_WRITE_END_INDEX]);
    close(childStdoutPipe[PIPE_READ_END_INDEX]);
    //close(childStdoutPipe[PIPE_WRITE_END_INDEX]);
    close(childStderrPipe[PIPE_READ_END_INDEX]);
    //close(childStderrPipe[PIPE_WRITE_END_INDEX]);

    //char* argv[] = {"ls", "-al", 0};
    //char* argv[] = {"sed", "--version", 0};
    //char* argv[] = {"sed", "-u", "s/f/F/g", 0};
    char* argv[] = {"sed", "s/f/F/g", 0};
    //char* argv[] = {"tr", "[:lower:]", "[:upper:]", 0};
    int r = execvp(argv[0], argv);
    if (r == -1)
    {
        // error has ocurred
    }
}


int main(int argc, char **argv)
{
    if (initPipes() != 0)
    {
        fprintf(stderr, "Failed to initialize pipes\n");
        exit(1);
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        childProcess();
    }
    else if (pid == -1)
    {
        fprintf(stderr, "Call to fork() has failed!\n");
        exit(1);
    }
    else
    {
        parentProcess();
    }

    return 0;
}


typedef void (*OutStreamHandler)(const char* data, unsigned numChars);
typedef void (*InStreamWriter)(const char* data, unsigned numChars);

InStreamWriter interact(
    const char* program,
    const char** args,
    unsigned numArgs,
    OutStreamHandler stdoutHandler,
    OutStreamHandler stderrHandler);
