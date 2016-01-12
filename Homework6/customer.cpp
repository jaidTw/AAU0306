#include <cstdio>
#include <cstdlib>
#include <deque>
#include <algorithm>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#define ERR_EXIT(str, ...) { fprintf(stderr, str, ## __VA_ARGS__); exit(-1); }

struct event_t {
    int type;
    int signo;
    double duration;
    static const int EVENT_SEND = 0;
    static const int EVENT_DEADLINE = 1;
    bool operator<(const event_t &rhs) const { return duration < rhs.duration; }
};

void handler(int);
void alarm_hand(int);
static inline int get_signo(int code) { return code == 0 ? SIGINT : code == 1 ? SIGUSR1 : SIGUSR2; }
static inline int get_code(int signo) { return signo == SIGINT ? 0 : signo == SIGUSR1 ? 1 : 2; }

static int serial[3] = {1, 1, 1};
static bool receive[3] = {true, true, true};
static const double deadline[3] = {0, 1.0, 0.3};
static std::deque<event_t> event_queue;
static FILE *logfp;

int main(int argc, char **argv) {
    if(argc != 2)
        ERR_EXIT("*** THIS PROGRAM SHOULD ONLY BE FORKED BY bidding_system\n");
    
    FILE *datafp;
    datafp = fopen(argv[1], "r");
    if(!(logfp = fopen("customer_log", "w+")))
        ERR_EXIT("Can't create the log file.\n");
    
    int code;
    double send_time;
    while(fscanf(datafp, "%d %lf", &code, &send_time) != EOF) {
        event_queue.push_back(event_t({event_t::EVENT_SEND, get_signo(code), send_time}));
        if(code != 0)
            event_queue.push_back(event_t({event_t::EVENT_DEADLINE,
                        get_signo(code), send_time + deadline[code]}));
    }
    std::sort(event_queue.begin(), event_queue.end());

    for(auto iter = event_queue.rbegin(); iter != event_queue.rend() - 1; ++iter)
        (*iter).duration -= (*(iter + 1)).duration;
    
    // prepare signal handling
    struct sigaction sigact;
    sigact.sa_handler = handler;
    sigact.sa_flags = 0;
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGALRM);
    sigaction(SIGINT, &sigact, NULL);
    sigaddset(&sigact.sa_mask, SIGINT);
    sigaction(SIGUSR1, &sigact, NULL);
    sigaddset(&sigact.sa_mask, SIGUSR1);
    sigaction(SIGUSR2, &sigact, NULL);
    sigact.sa_handler = alarm_hand;
    sigaction(SIGALRM, &sigact, NULL);

    raise(SIGALRM);
    while(!event_queue.empty());
    return 0;
}

void handler(int signo) {
    fprintf(logfp, "finish %d %d\n", get_code(signo), serial[get_code(signo)] - 1);
    receive[get_code(signo)] = true;
}

// critical, block all revieve signal
void alarm_hand(int signo) {
    auto &event = event_queue.front();
    event_queue.pop_front();

    if(event.type == event_t::EVENT_SEND) {
        fprintf(logfp, "send %d %d\n", get_code(event.signo), serial[get_code(event.signo)]);
        ++serial[get_code(event.signo)];
        if(event.signo == SIGINT) {
            printf("ordinary\n");
            fflush(stdout);
        } else
            kill(getppid(), event.signo);
        receive[get_code(event.signo)] = false;
    }
    else if(event.type == event_t::EVENT_DEADLINE && !receive[get_code(event.signo)]) {
        fprintf(logfp, "timeout %d %d\n", get_code(event.signo), serial[get_code(event.signo)] - 1);
        fflush(logfp);
        exit(0);
    }
    if(event_queue.empty())
        return;

    struct itimerval timer({
        timeval({0, 0}),
        timeval({((suseconds_t)(event_queue.front().duration * 1000000L)) / 1000000L
            , ((suseconds_t)(event_queue.front().duration * 1000000L)) % 1000000L})
    });
    setitimer(ITIMER_REAL, &timer, NULL);
}
