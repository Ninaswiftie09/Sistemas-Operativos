#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>

int main() {
    pid_t a = fork();
    if (a == 0) {
        execl("./ipc", "ipc", "10", "a", (char*)NULL);
        perror("execl a");
        return 1;
    }

    pid_t b = fork();
    if (b == 0) {
        execl("./ipc", "ipc", "10", "b", (char*)NULL);
        perror("execl b");
        return 1;
    }

    wait(NULL);
    wait(NULL);
    return 0;
}
