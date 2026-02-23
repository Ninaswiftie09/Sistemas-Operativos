#include <stdio.h>
#include <unistd.h>

int main() {
    setbuf(stdout, NULL);  

    printf("[Inicio] PID=%d | PPID=%d\n", getpid(), getppid());

    fork();
    fork();
    fork();
    fork();

    printf("Hola desde PID=%d | PPID=%d\n", getpid(), getppid());
    return 0;
}
