#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    int i, pid;

    int pipefd[3][2];
    for(i = 1; i < argc; ++i) {
        pipe(pipefd[i - 1]);
        pid = fork();
        if(pid == 0) {
            close(pipefd[i - 1][0]);
            dup2(pipefd[i - 1][1], STDOUT_FILENO);
            execlp( argv[i], argv[i], (char *) 0 );
        }
        close(pipefd[i - 1][1]);
    }
    for(i = 1; i < argc; ++i)
        wait(NULL);
    
    for(i = 1; i < argc; ++i)
        close(pipefd[i - 1][0]);
}
