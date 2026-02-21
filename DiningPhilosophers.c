#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    int id;
} Philosopher;

static int N = 5;
static int RUN_SECONDS = 60;

static pthread_t *threads;
static Philosopher *philos;
static pthread_mutex_t *forks;  // un mutex por tenedor
static sem_t room;              // semáforo N-1 (el "mayordomo")

static volatile int running = 1;

// contador de comidas (una por filósofo)
static int *eat_count = NULL;

// para logs ordenados 
static pthread_mutex_t log_mx = PTHREAD_MUTEX_INITIALIZER;

static void log_line(int id, const char *msg) {
    pthread_mutex_lock(&log_mx);
    printf("[F%d] %s\n", id, msg);
    fflush(stdout);
    pthread_mutex_unlock(&log_mx);
}

// dormir aleatorio 
static void rand_sleep_ms(int min_ms, int max_ms) {
    int span = max_ms - min_ms + 1;
    int ms = min_ms + (rand() % span);
    usleep(ms * 1000);
}

static void *philosopher_thread(void *arg) {
    Philosopher *p = (Philosopher *)arg;
    int id = p->id;

    int left  = id;           // tenedor izquierdo
    int right = (id + 1) % N; // tenedor derecho

    while (running) {
        // THINK
        log_line(id, "pensando");
        rand_sleep_ms(80, 250);

        // Pedir permiso al room (semáforo)
        sem_wait(&room);

        // Agarrar tenedores (mutex)
        pthread_mutex_lock(&forks[left]);
        pthread_mutex_lock(&forks[right]);

        // CRITICAL SECTION: "comer"
        log_line(id, ">>> entra a comer (tiene ambos tenedores)");
        eat_count[id]++;  // contar que comió
        rand_sleep_ms(80, 220);
        log_line(id, "<<< sale de comer (va a soltar tenedores)");

        // Soltar tenedores
        pthread_mutex_unlock(&forks[right]);
        pthread_mutex_unlock(&forks[left]);

        // Liberar cupo en room
        sem_post(&room);
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc >= 2) N = atoi(argv[1]);
    if (argc >= 3) RUN_SECONDS = atoi(argv[2]);

    if (N < 2) {
        fprintf(stderr, "N debe ser >= 2\n");
        return 1;
    }
    if (RUN_SECONDS < 1) RUN_SECONDS = 60;

    srand((unsigned)time(NULL));

    forks   = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * N);
    threads = (pthread_t *)malloc(sizeof(pthread_t) * N);
    philos  = (Philosopher *)malloc(sizeof(Philosopher) * N);
    eat_count = (int *)calloc(N, sizeof(int));

    if (!forks || !threads || !philos || !eat_count) {
        fprintf(stderr, "Error de memoria.\n");
        return 1;
    }

    for (int i = 0; i < N; i++) {
        pthread_mutex_init(&forks[i], NULL);
        philos[i].id = i;
    }

    // Semáforo inicializado a N-1
    sem_init(&room, 0, (unsigned int)(N - 1));

    printf("== Dining Philosophers (N-1) ==\n");
    printf("N=%d, duration=%d seconds\n\n", N, RUN_SECONDS);

    for (int i = 0; i < N; i++) {
        pthread_create(&threads[i], NULL, philosopher_thread, &philos[i]);
    }

    sleep(RUN_SECONDS);
    running = 0;

    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    // Resumen
    printf("\n== Resumen de comidas ==\n");
    int min = eat_count[0], max = eat_count[0], total = 0;
    for (int i = 0; i < N; i++) {
        printf("Filosofo %d comio %d veces\n", i, eat_count[i]);
        if (eat_count[i] < min) min = eat_count[i];
        if (eat_count[i] > max) max = eat_count[i];
        total += eat_count[i];
    }
    printf("Total: %d | Min: %d | Max: %d\n", total, min, max);

    if (min == 0) {
        printf("OJO: al menos un filosofo no comio (posible starvation).\n");
    } else if (max >= 3 * min) {
        printf("OJO: hay mucha desigualdad (posible starvation / falta de fairness).\n");
    }

    // limpieza
    sem_destroy(&room);
    for (int i = 0; i < N; i++) pthread_mutex_destroy(&forks[i]);

    free(eat_count);
    free(forks);
    free(threads);
    free(philos);

    printf("\nFin.\n");
    return 0;
}