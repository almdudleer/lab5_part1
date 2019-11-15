#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#ifndef MODE
#define MODE 1// 0: printf (incompatible with client), 1: shared memory, 2: message queue, 3: mmap
#endif

#define KEY 4536

pid_t pid;
uid_t uid;
gid_t gid;
time_t start_time;

#if MODE == 0
int send_to_client(char* s) {
    return printf("%s", s);
};
#elif MODE == 1
#include <sys/shm.h>
#define SHM_SIZE 1024
int shmid;
char* data;

int init() {

    if ((shmid = shmget(KEY, SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
        perror("shmget");
        return -1;
    }

    data = shmat(shmid, NULL, 0);
    if (data == (char*) (-1)) {
        perror("shmat");
        return -1;
    }

    return 0;
}

int send_to_client(char* s) {
    strncpy(data, s, SHM_SIZE);
    return 0;
};

int terminate() {
    if (shmdt(data) == -1) {
        perror("shmdt");
        return -1;
    }
    if (-1 == (shmctl(shmid, IPC_RMID, NULL))) {
        perror("shmctl");
        return -1;
    }
    return 0;
}
#elif MODE == 2
#include <sys/msg.h>

#define MSGSZ 128

typedef struct msgbuf {
    long mtype;
    char mtext[MSGSZ];
} messsage_buf;

int msqid;

int init() {
    if ((msqid = msgget(KEY, 0644 | IPC_CREAT)) == -1) {
        perror("msgget");
        return -1;
    }
    return 0;
}

int send_to_client(char* s) {
    messsage_buf buf;
    strcpy(buf.mtext, s);
    buf.mtype = 1;
    if ((msgsnd(msqid, &buf, MSGSZ, IPC_NOWAIT) == -1)) {
        perror("msgsnd");
        return -1;
    }
    return 0;
}

int terminate() { return 0; }
#elif MODE == 3
#include <sys/mman.h>
#include <fcntl.h>

int fdout;
void* dst;

int init() {
    if ((fdout = open("file.tmp", O_RDWR | O_CREAT | O_TRUNC, 0664)) < 0) {
        perror("fopen");
        return -1;
    }
    return 0;
}

int send_to_client(char* s) {
    if (lseek(fdout, strlen(s), SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }
    if (write(fdout, "", 1) != 1) {
        perror("write");
        return -1;
    }
    if ((dst = mmap(0, strlen(s), PROT_READ | PROT_WRITE, MAP_SHARED, fdout, 0)) == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    strcpy(dst, s);
    return 0;
}

int terminate() { return 0; }
#else
int init() {
    printf("No such mode: %d. Recompile with MODE = 0, 1, 2 or 3.", MODE);
    exit(0);
}

int send_to_client(){ return 0; }

int terminate() { return 0; }
#endif

int main() {
    if (init() == -1) exit(1);

    pid = getpid();
    uid = getuid();
    gid = getgid();
    start_time = time(0);
    printf("Started server, pid: %d, uid: %d, gid: %d\n", pid, uid, gid);

    char cont = 1;
    double load[3];
    time_t uptime = 0;
    while (cont) {
        if (uptime != time(0) - start_time) {
            uptime = time(0) - start_time;
            getloadavg(load, 3);
            char s[100];
            sprintf(s, "%lu: %f %f %f\n", uptime, load[0], load[1], load[2]);
            send_to_client(s);
            if (uptime == 200) cont = 0;
        }
    }

    if (terminate() == -1) return 1;
    return 0;
}