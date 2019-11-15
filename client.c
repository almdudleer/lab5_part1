#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MODE
#define MODE 1// 0: printf, 1: shared memory, 2: message queue, 3: mmap
#endif

#define KEY 4536

#if MODE == 1
#include <sys/shm.h>
#define SHM_SIZE 1024
int shmid;
char* data;

int init() {

    if ((shmid = shmget(KEY, SHM_SIZE, 0644)) == -1) {
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

char* receive() {
    return data;
};

int terminate() {
    if (shmdt(data) == -1) {
        perror("shmdt");
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
messsage_buf buf;

int init() {
    if ((msqid = msgget(KEY, 0664)) == -1) {
        perror("msqget");
        return -1;
    }
    return 0;
}

char* receive() {
    if (msgrcv(msqid, &buf, MSGSZ, 1, 0) == -1) {
        perror("msgrcv");
        return NULL;
    }
    return buf.mtext;
}

int terminate() { return 0; }
#elif MODE == 3
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

int fdin;
void* src;
struct stat statbuf;

int init() {
    if ((fdin = open("file.tmp", O_RDONLY)) < 0) {
        perror("open");
        return -1;
    }
    if (fstat(fdin, &statbuf) < 0) {
        perror("open");
        return -1;
    }
    if ((src = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fdin, 0))== MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    return 0;
}

char* receive(){
    char* str = malloc(statbuf.st_size);
    strcpy(str, src);
    return str;
}

int terminate() {
    return 0;
}
#else
int init() {
    printf("No such mode: %d. Recompile with MODE = 1, 2 or 3.", MODE);
    exit(0);
}

char* receive(){ return 0; }

int terminate() { return 0; }
#endif

int main() {
    if (init() == -1) exit(1);
    printf("%s", receive());
    if (terminate() == -1) return 1;
    return 0;
}