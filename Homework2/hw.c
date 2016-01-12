#include <unistd.h>
#include <fcntl.h>

int main(void) {
    int fd1, fd2;
    fd1 = open("infile", O_RDONLY);
    fd2 = open("outfile", O_WRONLY);
    
    dup2(fd1, STDIN_FILENO);
    dup2(STDOUT_FILENO, STDERR_FILENO);
    dup2(fd2, STDOUT_FILENO);

    execlp("./a.out", "./a.out", (char *)0);
    return 0;

}
