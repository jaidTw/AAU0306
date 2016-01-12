#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>

#define ERR_EXIT(str, ...) { fprintf(stderr, str, ## __VA_ARGS__); exit(-1); }

static pid_t customer_pid;
static FILE *logfp;
static int serial[3] = {1, 1, 1};

void handler(int);
static inline int code(int signo) { return signo == SIGINT ? 0 : signo == SIGUSR1 ? 1 : 2; }

int main(int argc, char **argv) {
    if(argc != 2)
        ERR_EXIT("Usage : ./bidding_system [test_data]\n");
    
    FILE *datafp;
    if(!(datafp = fopen(argv[1], "r")))
        ERR_EXIT("Test data file not found.\n");
    fclose(datafp);    
    if(!(logfp = fopen("bidding_system_log", "w+")))
        ERR_EXIT("Can't create the log file.\n");
   
    int pipefd[2];
    if(pipe(pipefd) == -1)
        ERR_EXIT("Error while creating pipe.\n");
    FILE *pipefp = fdopen(pipefd[0], "r");

    // prepare signal handling
    struct sigaction sigact = {.sa_handler = handler};
    sigemptyset(&sigact.sa_mask);
    sigaction(SIGUSR1, &sigact, NULL);
    sigaddset(&sigact.sa_mask, SIGUSR1);
    sigaction(SIGUSR2, &sigact, NULL);

    // use for pipe
    fd_set mread_set, read_set;
    FD_ZERO(&mread_set);
    FD_SET(pipefd[0], &mread_set);
    
    if((customer_pid = fork()) < 0) {
        ERR_EXIT("Error while forking the customer program.\n");
    }
    else if(customer_pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        execl("./customer", "./customer", argv[1], (char *)NULL);
    }
    close(pipefd[1]);

    // handle
    for(;;) {
        memcpy(&read_set, &mread_set, sizeof(fd_set));
        if(select(pipefd[0] + 1, &read_set, NULL, NULL, &(struct timeval){0, 5000}) > 0) {
            if(fscanf(pipefp, "%*s") == EOF)
                break;
            handler(SIGINT);
        }
    }
    fprintf(logfp, "terminate\n");
    waitpid(customer_pid, NULL, 0);
    return 0;
}

void handler(int signo) {
    struct timespec time, rem = {.tv_sec = 0, .tv_nsec = 0};
    if(signo == SIGINT)
        rem.tv_sec = 1;
    else if(signo == SIGUSR1)
        rem.tv_nsec = 500000000;
    else if(signo == SIGUSR2)
        rem.tv_nsec = 200000000;

    fprintf(logfp, "reveive %d %d\n", code(signo), serial[code(signo)]);
    do time = rem;
    while(nanosleep(&time, &rem));
    
    fprintf(logfp, "finish %d %d\n", code(signo), serial[code(signo)]);
    ++serial[code(signo)];
    kill(customer_pid, signo);
}
