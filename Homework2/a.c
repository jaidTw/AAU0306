#include <stdio.h>

int main(void) {
    char buf[100];
    fgets(buf, 100, stdin);
    fputs(buf, stdout);
    fputs("err\n", stderr);
    return 0;
}
