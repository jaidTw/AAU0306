#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <chrono>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/wait.h>

#define SIGUSR3 SIGWINCH
#define ERR_EXIT(str, ...) { fprintf(stderr, str, ## __VA_ARGS__); exit(-1); }
using namespace std::chrono_literals;

struct customer_t {
    customer_t(int ctype, int cserial, std::chrono::high_resolution_clock::time_point arrive_time)
        : type(ctype), serial(cserial), remain({0, 0}), start(false) {
            if(ctype == SIGUSR1) {
                remain.tv_nsec = 5e+8;
                deadline = arrive_time + 2s;
            }
            else if(ctype == SIGUSR2) {
                remain.tv_sec = 1;
                deadline = arrive_time + 3s;
            }
            else if(ctype == SIGUSR3) {
                remain.tv_nsec = 2e+8;
                deadline = arrive_time + 300ms;
            }
        };
    int type;
    int serial;
    std::chrono::high_resolution_clock::time_point deadline;
    struct timespec remain;
    bool start;
    bool operator<(const customer_t &rhs) const { return deadline > rhs.deadline; }
};

static int serial[3] = {1, 1, 1};
static std::priority_queue<customer_t> customer_queue;

void handle(customer_t&, FILE*, pid_t);
void sig_handler(int signo);
static inline int code(int signo) { return signo == SIGUSR1 ? 0 : signo == SIGUSR2 ? 1 : 2; }

int main(int argc, char **argv) {
    if(argc != 2)
        ERR_EXIT("Usage : ./bidding_system_EDF [test_data]\n");
    
    FILE *datafp, *logfp;
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
    struct sigaction sigact;
    sigact.sa_flags = 0;
    sigact.sa_handler = sig_handler;
    sigemptyset(&sigact.sa_mask);
    sigaction(SIGUSR1, &sigact, NULL);
    sigaction(SIGUSR2, &sigact, NULL);
    sigaction(SIGUSR3, &sigact, NULL);

    // use for checking pipe
    fd_set mread_set, read_set;
    FD_ZERO(&mread_set);
    FD_SET(pipefd[0], &mread_set);
    
    pid_t customer_pid;
    if((customer_pid = fork()) < 0) {
        ERR_EXIT("Error while forking the customer program.\n");
    }
    else if(customer_pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        execl("./customer_EDF", "./customer_EDF", argv[1], (char *)NULL);
    }
    close(pipefd[1]);

    // handle
    char buf[20];
    struct timeval time{0, 0};
    for(;;) {
        memcpy(&read_set, &mread_set, sizeof(fd_set));
        if(!customer_queue.empty()) {
            auto customer = customer_queue.top();
            customer_queue.pop();
            handle(customer, logfp, customer_pid);
        }
        else if(select(pipefd[0] + 1, &read_set, NULL, NULL, &time) > 0
            && fscanf(pipefp, "%s", buf) != EOF
            && strncmp(buf, "terminate", 9) == 0)
            break;
    }
    fprintf(logfp, "terminate\n");
    waitpid(customer_pid, NULL, 0);
    return 0;
}

void handle(customer_t &customer, FILE *logfp, pid_t customer_pid) {
    if(!customer.start) {
        fprintf(logfp, "recieve %d %d\n", code(customer.type), customer.serial);
        customer.start = true;
    }

    do {
        auto time = customer.remain;
        if(nanosleep(&time, &customer.remain) == 0)
            break;
        else if(errno == EINTR && customer_queue.top().deadline < customer.deadline ) {
            customer_queue.push(customer);
            return; // return if interrupt by a customer with earlier deadline.
        }
    } while(true);

    fprintf(logfp, "finish %d %d\n", code(customer.type), customer.serial);
    kill(customer_pid, customer.type);
}

void sig_handler(int signo) {
    customer_queue.push(customer_t(signo, serial[code(signo)], std::chrono::high_resolution_clock::now()));
    ++serial[code(signo)];
}
