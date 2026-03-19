#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <omp.h>

#define SIZE 9
#define SLOW_DEMO 1   

int sudoku[SIZE][SIZE];
int rows_ok[SIZE];
int cols_ok[SIZE];
int subs_ok[SIZE];

static long get_tid_linux(void) {
    return syscall(SYS_gettid);
}

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static bool validate_values(const int values[SIZE]) {
    int seen[SIZE + 1] = {0};

    for (int i = 0; i < SIZE; i++) {
        int value = values[i];

        if (value < 1 || value > 9) {
            return false;
        }

        if (seen[value]) {
            return false;
        }

        seen[value] = 1;
    }

    return true;
}

static bool check_single_row(int row) {
    int values[SIZE] = {0};

    
    omp_set_num_threads(9); 

    omp_set_nested(true); 

    #pragma omp parallel for schedule(dynamic)
    for (int col = 0; col < SIZE; col++) {
        values[col] = sudoku[row][col];
    }

    return validate_values(values);
}

static bool check_single_column(int col) {
    int values[SIZE] = {0};

    omp_set_num_threads(9);

    omp_set_nested(true); 

    #pragma omp parallel for schedule(dynamic) 
    for (int row = 0; row < SIZE; row++) {
        values[row] = sudoku[row][col];
    }

    return validate_values(values);
}

static bool check_subgrid(int start_row, int start_col) {
    int values[SIZE] = {0};

    omp_set_num_threads(9); 

    omp_set_nested(true); 

    #pragma omp parallel for schedule(dynamic) 
    for (int k = 0; k < SIZE; k++) {
        int row = start_row + (k / 3);
        int col = start_col + (k % 3);
        values[k] = sudoku[row][col];
    }

    return validate_values(values);
}

static void check_rows(void) {
    omp_set_num_threads(9); 

    omp_set_nested(true); 

    #pragma omp parallel for schedule(dynamic) 
    for (int row = 0; row < SIZE; row++) {
        rows_ok[row] = check_single_row(row) ? 1 : 0;
    }
}

static void check_columns(void) {
    omp_set_num_threads(9); 

    omp_set_nested(true); 

    
    #pragma omp parallel for
    for (int col = 0; col < SIZE; col++) {
        cols_ok[col] = check_single_column(col) ? 1 : 0;
        printf("En la revision de columnas el siguiente es un thread en ejecucion: %ld\n", get_tid_linux());
        fflush(stdout);

#if SLOW_DEMO
        usleep(100000);
#endif
    }
}

static void check_all_subgrids(void) {
    omp_set_num_threads(9); 

    omp_set_nested(true); 

    #pragma omp parallel for schedule(dynamic)
    for (int index = 0; index < SIZE; index++) {
        int start_row = (index / 3) * 3;
        int start_col = (index % 3) * 3;
        subs_ok[index] = check_subgrid(start_row, start_col) ? 1 : 0;
    }
}

static bool sudoku_is_valid(void) {
    for (int i = 0; i < SIZE; i++) {
        if (!rows_ok[i] || !cols_ok[i] || !subs_ok[i]) {
            return false;
        }
    }
    return true;
}

static void print_grid(void) {
    printf("Sudoku cargado:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", sudoku[i][j]);
        }
        printf("\n");
    }
}

static void load_sudoku_file(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        die("open");
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        die("fstat");
    }

    if (st.st_size <= 0) {
        close(fd);
        fprintf(stderr, "El archivo está vacío.\n");
        exit(EXIT_FAILURE);
    }

    char *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        die("mmap");
    }

    char digits[81];
    int count = 0;

    for (off_t i = 0; i < st.st_size && count < 81; i++) {
        if (mapped[i] >= '0' && mapped[i] <= '9') {
            digits[count++] = mapped[i];
        }
    }

    if (count != 81) {
        munmap(mapped, st.st_size);
        close(fd);
        fprintf(stderr, "Se esperaban 81 dígitos y se encontraron %d.\n", count);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 81; i++) {
        sudoku[i / 9][i % 9] = digits[i] - '0';
    }

    if (munmap(mapped, st.st_size) < 0) {
        close(fd);
        die("munmap");
    }

    if (close(fd) < 0) {
        die("close");
    }
}

static void run_ps_for_parent(pid_t parent_pid) {
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", parent_pid);
    execlp("ps", "ps", "-p", pid_str, "-lLf", (char *)NULL);
    perror("execlp");
    _exit(EXIT_FAILURE);
}

static void *columns_thread_start(void *arg) {
    (void)arg;
    printf("El thread que ejecuta el método de revisión de columnas es: %ld\n", get_tid_linux());
    fflush(stdout);
    check_columns();
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <archivo_sudoku>\n", argv[0]);
        return EXIT_FAILURE;
    }

     omp_set_num_threads(1);
    

    memset(rows_ok, 0, sizeof(rows_ok));
    memset(cols_ok, 0, sizeof(cols_ok));
    memset(subs_ok, 0, sizeof(subs_ok));

    load_sudoku_file(argv[1]);
    print_grid();

    check_all_subgrids();

    pid_t parent_pid = getpid();
    pid_t first_child = fork();
    if (first_child < 0) {
        die("fork");
    }

    if (first_child == 0) {
        run_ps_for_parent(parent_pid);
    }

    pthread_t columns_thread;
    if (pthread_create(&columns_thread, NULL, columns_thread_start, NULL) != 0) {
        fprintf(stderr, "No se pudo crear el pthread de columnas.\n");
        return EXIT_FAILURE;
    }

    if (pthread_join(columns_thread, NULL) != 0) {
        fprintf(stderr, "No se pudo ejecutar pthread_join().\n");
        return EXIT_FAILURE;
    }

    printf("El thread en el que se ejecuta main es: %ld\n", get_tid_linux());
    fflush(stdout);

    if (waitpid(first_child, NULL, 0) < 0) {
        die("waitpid");
    }

    check_rows();

    printf("Sudoku %s!\n", sudoku_is_valid() ? "resuelto" : "inválido");
    fflush(stdout);

    pid_t second_child = fork();
    if (second_child < 0) {
        die("fork");
    }

    if (second_child == 0) {
        run_ps_for_parent(getppid());
    }

    if (waitpid(second_child, NULL, 0) < 0) {
        die("waitpid");
    }

    return 0;
}
