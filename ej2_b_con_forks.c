#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    setbuf(stdout, NULL);

    volatile unsigned long long dummy = 0;
    pid_t pid1, pid2, pid3;
    clock_t inicio, fin;

    inicio = clock();

    pid1 = fork(); 
    if (pid1 == 0) {
        pid2 = fork(); 
        if (pid2 == 0) {
            pid3 = fork(); 
            if (pid3 == 0) {
                
                for (int i = 0; i < 1000000; i++) dummy += i;
                return 0;
            } else {
              
                for (int i = 0; i < 1000000; i++) dummy += i;
                wait(NULL);
                return 0;
            }
        } else {
            for (int i = 0; i < 1000000; i++) dummy += i;
            wait(NULL);
            return 0;
        }
    } else {
        wait(NULL);
        fin = clock();

        double tiempo = (double)(fin - inicio) / CLOCKS_PER_SEC;
        printf("Tiempo (segundos): %f\n", tiempo);
    }

    return 0;
}
