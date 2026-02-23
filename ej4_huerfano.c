#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
       
        for (int i = 1; i <= 4000000; i++) {
            if (i % 1000000 == 0) {
                printf("Hijo PID=%d contando... i=%d | PPID=%d\n", getpid(), i, getppid());
                fflush(stdout);
            }
        }
        printf("Hijo terminó. PID=%d | PPID=%d\n", getpid(), getppid());
        return 0;
    } else {
        
        printf("Padre vivo (mátame con kill -9). PID=%d | hijo PID=%d\n", getpid(), pid);
        while (1) { }
    }

    return 0;
}
