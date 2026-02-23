#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // Hijo
        printf("Hijo termina. PID=%d | PPID=%d\n", getpid(), getppid());
        return 0;
    } else {
        // Padre
        printf("Padre se queda en while(1). PID=%d\n", getpid());
        while (1) { }
    }
    return 0;
}
