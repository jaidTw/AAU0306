#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <stdio.h>

#include "apue.h"
#include <errno.h> /* for definition of errno */
#include <stdarg.h> /* ISO C variable aruments */
static void err_doit(int, int, const char *, va_list);
/*
* Nonfatal error related to a system call.
* Print a message and return.
*/
void
err_ret(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, errno, fmt, ap);
    va_end(ap);
}
/*
* Fatal error related to a system call.
* Print a message and terminate.
*/
void
err_sys(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, errno, fmt, ap);
    va_end(ap);
    exit(1);
}
/*
* Fatal error unrelated to a system call.
* Error code passed as explict parameter.
* Print a message and terminate.
*/
void
err_exit(int error, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, error, fmt, ap);
    va_end(ap);
    exit(1);
}
/*
* Fatal error related to a system call.
* Print a message, dump core, and terminate.
*/
void
err_dump(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, errno, fmt, ap);
    va_end(ap);
    abort(); /* dump core and terminate */
    exit(1); /* shouldn't get here */
}
/*
* Nonfatal error unrelated to a system call.
* Print a message and return.
*/
void
err_msg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(0, 0, fmt, ap);
    va_end(ap);
}
/*
* Fatal error unrelated to a system call.
* Print a message and terminate.
*/
void
err_quit(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(0, 0, fmt, ap);
    va_end(ap);
    exit(1);
}
/*
* Print a message and return to caller.
* Caller specifies "errnoflag".
*/
static void
err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
    char buf[MAXLINE];
   vsnprintf(buf, MAXLINE, fmt, ap);
   if (errnoflag)
       snprintf(buf+strlen(buf), MAXLINE-strlen(buf),": %s",
         strerror(error));
   strcat(buf, " ");
   fflush(stdout); /* in case stdout and stderr are the same */
   fputs(buf, stderr);
   fflush(NULL); /* flushes all stdio output streams */
}

void pr_mask(const char *str)
{
        sigset_t sigset;
        int errno_save;

        errno_save = errno;
        if(sigprocmask(0, NULL, &sigset) < 0)
                err_sys("sigprocmask error");

        printf("%s", str);
        if(sigismember(&sigset, SIGABRT)) printf("SIGABRT ");
    if(sigismember(&sigset, SIGALRM)) printf("SIGALRM ");
#ifdef SIGBUS
        if(sigismember(&sigset, SIGBUS)) printf("SIGBUS ");
#endif
        if(sigismember(&sigset, SIGCHLD)) printf("SIGCHLD ");
        if(sigismember(&sigset, SIGCONT)) printf("SIGCONT ");
#ifdef SIGEMT
        if(sigismember(&sigset, SIGEMT)) printf("SIGEMT ");
#endif
        if(sigismember(&sigset, SIGFPE)) printf("SIGFPE ");
        if(sigismember(&sigset, SIGHUP)) printf("SIGHUP ");
    if(sigismember(&sigset, SIGILL)) printf("SIGILL ");
        if(sigismember(&sigset, SIGINT)) printf("SIGINT ");
#ifdef SIGIO
        if(sigismember(&sigset, SIGIO)) printf("SIGIO ");
#endif
        if(sigismember(&sigset, SIGKILL)) printf("SIGKILL ");
        if(sigismember(&sigset, SIGPIPE)) printf("SIGPIPE ");
#ifdef SIGPOLL
        if(sigismember(&sigset, SIGPOLL)) printf("SIGPOLL ");
#endif
        if(sigismember(&sigset, SIGQUIT)) printf("SIGQUIT ");
        if(sigismember(&sigset, SIGSEGV)) printf("SIGSEGV ");
        if(sigismember(&sigset, SIGSTOP)) printf("SIGSTOP ");
#ifdef SIGSYS 
        if(sigismember(&sigset, SIGSYS)) printf("SIGSYS ");
#endif
        if(sigismember(&sigset, SIGTERM)) printf("SIGTERM ");
        if(sigismember(&sigset, SIGTSTP)) printf("SIGTSTP ");
        if(sigismember(&sigset, SIGTTIN)) printf("SIGTTIN ");
        if(sigismember(&sigset, SIGTTOU)) printf("SIGTTOU ");
        if(sigismember(&sigset, SIGUSR1)) printf("SIGUSR1 ");
        if(sigismember(&sigset, SIGUSR2)) printf("SIGUSR2 ");
#ifdef SIGWINCH
        if(sigismember(&sigset, SIGWINCH)) printf("SIGWINCH ");
#endif
    
        printf("\n");
        fflush(stdout);
}

void catchit(int signo) {
    pr_mask("before Line C\n");
    printf("Line C\n");
    pr_mask("After Line C\n");
}

int main(void) {
    pid_t pid;
    sigset_t newmask, waitmask, oldmask;
    if((pid = fork()) == 0) {
        struct sigaction action;

        sigemptyset(&newmask);
        sigemptyset(&waitmask);
        sigaddset(&newmask, SIGUSR1);
        sigaddset(&waitmask, SIGUSR2);

        sigprocmask(SIG_BLOCK, &newmask, &oldmask);

        action.sa_flags = 0;
        sigemptyset(&action.sa_mask);
        action.sa_handler = catchit;
        sigaction(SIGUSR2, &action, NULL);
        sigaction(SIGINT, &action, NULL);
        sigaddset(&action.sa_mask, SIGINT);
        sigaction(SIGUSR1, &action, NULL);

        pr_mask("before Line A\n");
        printf("Line A\n");
        pr_mask("After Line A\n");

        pr_mask("Before Line B\n");
        printf("Line B\n");
        sigsuspend(&waitmask); //Line B
        pr_mask("After Line B\n");
    }
    else {
        int stat;
//        kill(pid, SIGUSR1);
//        kill(pid, SIGUSR2);
//        kill(pid, SIGINT);
        pid = wait(&stat);
    }
    _exit(0);
}
