#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    int id;                 // id del filósofo: 0..N-1
} Philosopher;

static int N = 5;           // # filósofos / # tenedores
static int RUN_SECONDS = 60; // duración de la simulación

// Recursos globales compartidos
static pthread_t *threads;         // threads de los filósofos
static Philosopher *philos;        // info de cada filósofo (id)
static pthread_mutex_t *forks;     // mutex por tenedor
static sem_t room;                 // semáforo N-1 ("mayordomo")

static volatile int running = 1;   // para detener threads
static int *eat_count = NULL;      // contador de comidas por filósofo
static pthread_mutex_t log_mx = PTHREAD_MUTEX_INITIALIZER; // ordena prints

static void log_line(int id, const char *msg) {
    pthread_mutex_lock(&log_mx);
    printf("[F%d] %s\n", id, msg);
    fflush(stdout);
    pthread_mutex_unlock(&log_mx);
}

static void rand_sleep_ms(int min_ms, int max_ms) {
    int span = max_ms - min_ms + 1;
    int ms = min_ms + (rand() % span);
    usleep(ms * 1000);
}

static void *philosopher_thread(void *arg) {
    Philosopher *p = (Philosopher *)arg;
    int id = p->id;

    int left  = id;             // tenedor izquierdo
    int right = (id + 1) % N;   // tenedor derecho 

    while (running) {
        log_line(id, "pensando");
        rand_sleep_ms(80, 250);

        sem_wait(&room);                     // entra a "room" (máx N-1 compiten)
        pthread_mutex_lock(&forks[left]);    // toma left
        pthread_mutex_lock(&forks[right]);   // toma right

        log_line(id, ">>> entra a comer (tiene ambos tenedores)");
        eat_count[id]++;                     // cuenta una comida
        rand_sleep_ms(80, 220);
        log_line(id, "<<< sale de comer (va a soltar tenedores)");

        pthread_mutex_unlock(&forks[right]); // suelta right
        pthread_mutex_unlock(&forks[left]);  // suelta left
        sem_post(&room);                     // sale de "room"
    }

    return NULL;
}

int main(int argc, char **argv) {

    // Lee argumentos opcionales
    if (argc >= 2) N = atoi(argv[1]);              // N = cantidad de filósofos/tenedores
    if (argc >= 3) RUN_SECONDS = atoi(argv[2]);    // duración en segundos

    // Validaciones de entrada
    if (N < 2) {
        fprintf(stderr, "N debe ser >= 2\n");
        return 1;
    }
    if (RUN_SECONDS < 1) RUN_SECONDS = 60;         // fallback si mandan un valor inválido

    // Semilla para rand() (tiempos de espera aleatorios en los threads)
    srand((unsigned)time(NULL));

    // Reservas dinámicas según N
    forks   = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * N); // N mutexes (tenedores)
    threads = (pthread_t *)malloc(sizeof(pthread_t) * N);             // N threads
    philos  = (Philosopher *)malloc(sizeof(Philosopher) * N);         // N structs con ids
    eat_count = (int *)calloc(N, sizeof(int));                        // contadores en 0

    // Verifica que la memoria se asignó correctamente
    if (!forks || !threads || !philos || !eat_count) {
        fprintf(stderr, "Error de memoria.\n");
        return 1;
    }

    // Inicializa cada mutex (tenedor) y asigna id a cada filósofo
    for (int i = 0; i < N; i++) {
        pthread_mutex_init(&forks[i], NULL);
        philos[i].id = i;
    }

    // Semáforo "room" con N-1 cupos: limita cuántos compiten por tenedores
    sem_init(&room, 0, (unsigned int)(N - 1));

    // Mensaje inicial de configuración
    printf("== Dining Philosophers (N-1) ==\n");
    printf("N=%d, duration=%d seconds\n\n", N, RUN_SECONDS);

    // Crea un thread por filósofo y le pasa su struct (id)
    for (int i = 0; i < N; i++) {
        pthread_create(&threads[i], NULL, philosopher_thread, &philos[i]);
    }

    // Deja correr la simulación el tiempo indicado
    sleep(RUN_SECONDS);

    // Señal de parada para que los threads salgan del while(running)
    running = 0;

    // Espera a que todos los threads terminen antes de seguir
    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    // Resumen final de cuántas veces comió cada filósofo
    printf("\n== Resumen de comidas ==\n");

    int min = eat_count[0], max = eat_count[0], total = 0;
    for (int i = 0; i < N; i++) {
        printf("Filosofo %d comio %d veces\n", i, eat_count[i]);
        if (eat_count[i] < min) min = eat_count[i];
        if (eat_count[i] > max) max = eat_count[i];
        total += eat_count[i];
    }
    printf("Total: %d | Min: %d | Max: %d\n", total, min, max);

    // Mensajes de posible alerta
    if (min == 0) {
        printf("OJO: posible starvation (alguien no comio).\n");
    } else if (max >= 3 * min) {
        printf("OJO: posible falta de fairness (mucha desigualdad).\n");
    }

    // Limpieza de recursos del sistema
    sem_destroy(&room);
    for (int i = 0; i < N; i++) pthread_mutex_destroy(&forks[i]);

    // Liberación de memoria dinámica
    free(eat_count);
    free(forks);
    free(threads);
    free(philos);

    printf("\nFin.\n");
    return 0;
}