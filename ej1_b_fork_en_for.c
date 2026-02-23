#include <stdio.h>
#include <unistd.h>

int main() {
    setbuf(stdout, NULL);

    printf("[Inicio] PID=%d | PPID=%d\n", getpid(), getppid());

    for (int i = 0; i < 4; i++) {
        fork();
        printf("Iter=%d | PID=%d | PPID=%d\n", i, getpid(), getppid());
    }

    return 0;
}

