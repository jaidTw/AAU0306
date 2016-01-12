#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    int i, pid;

    int pipefd[2];
    pipe(pipefd);
    for(i = 1; i < argc; ++i) {
        pid = fork();
        if(pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            execlp( argv[i], argv[i], (char *) 0 );
        }
    }
    close(pipefd[1]);
    for(i = 1; i < argc; ++i) {
        wait(NULL);
    }
    close(pipefd[0]);
}
