#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

static void die(const char *m){ perror(m); exit(1); }

static int send_fd(int sock, int fd) {
    struct msghdr msg; memset(&msg, 0, sizeof(msg));
    char buf[1] = {0};
    struct iovec io = { .iov_base = buf, .iov_len = 1 };
    msg.msg_iov = &io; msg.msg_iovlen = 1;

    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));
    msg.msg_controllen = cmsg->cmsg_len;

    if (sendmsg(sock, &msg, 0) < 0) return -1;
    return 0;
}

static int recv_fd(int sock) {
    struct msghdr msg; memset(&msg, 0, sizeof(msg));
    char m_buffer[1];
    struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };
    msg.msg_iov = &io; msg.msg_iovlen = 1;

    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    if (recvmsg(sock, &msg, 0) < 0) return -1;

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (!cmsg) return -1;

    int fd;
    memcpy(&fd, CMSG_DATA(cmsg), sizeof(int));
    return fd;
}

static int uds_server_accept(const char *path) {
    int s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) die("socket");

    struct sockaddr_un addr; memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);

    unlink(path);
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) die("bind");
    if (listen(s, 1) < 0) die("listen");

    int c = accept(s, NULL, NULL);
    if (c < 0) die("accept");
    close(s);
    return c;
}

static int uds_client_connect(const char *path) {
    int c = socket(AF_UNIX, SOCK_STREAM, 0);
    if (c < 0) die("socket");

    struct sockaddr_un addr; memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);

    for (int i = 0; i < 200; i++) {
        if (connect(c, (struct sockaddr*)&addr, sizeof(addr)) == 0) return c;
        usleep(20000);
    }
    die("connect");
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <n> <x>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    char x = argv[2][0];
    if (n <= 0) { fprintf(stderr, "n debe ser > 0\n"); return 1; }

    const char *SHM_NAME = "/lab2_ipc_shm";
    const size_t SHM_SIZE = 4096;

    const char *FIFO_NAME = "/tmp/lab2_ipc_fifo";
    const char *SOCK_PATH = "/tmp/lab2_ipc_sock";

    if (mkfifo(FIFO_NAME, 0666) == -1 && errno != EEXIST) die("mkfifo");

    int created = 0;
    int shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (shm_fd >= 0) {
        created = 1;
        if (ftruncate(shm_fd, (off_t)SHM_SIZE) < 0) die("ftruncate");
    } else {
        if (errno != EEXIST) die("shm_open");
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
        if (shm_fd < 0) die("shm_open2");
    }

    if (created) {
        int fifo_w = open(FIFO_NAME, O_WRONLY);
        if (fifo_w < 0) die("open fifo_w");
        char b = '1';
        if (write(fifo_w, &b, 1) != 1) die("write fifo");
        close(fifo_w);

        int conn = uds_server_accept(SOCK_PATH);
        if (send_fd(conn, shm_fd) < 0) die("send_fd");
        close(conn);
    } else {
        int fifo_r = open(FIFO_NAME, O_RDONLY);
        if (fifo_r < 0) die("open fifo_r");
        char b;
        if (read(fifo_r, &b, 1) != 1) die("read fifo");
        close(fifo_r);

        int conn = uds_client_connect(SOCK_PATH);
        int received_fd = recv_fd(conn);
        if (received_fd < 0) die("recv_fd");
        close(conn);

        printf("[IPC %c] Recibí FD=%d (NO lo uso para mmap)\n", x, received_fd);
        close(received_fd);
    }

    char *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) die("mmap");

    if (created) {
        memset(shm_ptr, '.', SHM_SIZE);
        shm_ptr[SHM_SIZE - 1] = '\0';
    }

    int pfd[2];
    if (pipe(pfd) < 0) die("pipe");

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        close(pfd[1]);
        char msg;
        size_t idx = 0;
        while (read(pfd[0], &msg, 1) == 1) {
            if (msg == x) {
                if (idx < SHM_SIZE - 1) shm_ptr[idx++] = x;
            }
        }
        close(pfd[0]);
        munmap(shm_ptr, SHM_SIZE);
        close(shm_fd);
        return 0;
    } else {
        close(pfd[0]);
        for (size_t i = 0; i < SHM_SIZE - 1; i++) {
            if ((i % (size_t)n) == 0) {
                if (write(pfd[1], &x, 1) != 1) die("write");
            }
        }
        close(pfd[1]);
        wait(NULL);

        printf("[IPC %c] Contenido final (primeros 120 chars):\n", x);
        fwrite(shm_ptr, 1, 120, stdout);
        printf("\n");

        munmap(shm_ptr, SHM_SIZE);
        close(shm_fd);

        return 0;
    }
}
