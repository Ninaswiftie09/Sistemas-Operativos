#include <stdio.h>
#include <time.h>

int main() {
    volatile unsigned long long dummy = 0;  
    clock_t inicio = clock();

    for (int i = 0; i < 1000000; i++) dummy += i;
    for (int i = 0; i < 1000000; i++) dummy += i;
    for (int i = 0; i < 1000000; i++) dummy += i;

    clock_t fin = clock();
    double tiempo = (double)(fin - inicio) / CLOCKS_PER_SEC;

    printf("Tiempo (segundos): %f\n", tiempo);
    return 0;
}
