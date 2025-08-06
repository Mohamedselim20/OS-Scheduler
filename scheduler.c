#include "headers.h"
#include <math.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int msgq_id;

// Message struct
struct msgbuff {
    long mtype;
    struct processData data;
};

// Signal handler
void handler(int signum) {
    // Just notify that a child has decreased its remaining time
}

void createProcess(struct processData p) {
    // 1. Allocate shared memory for remaining time
    int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }

    int *remainingTime = (int *)shmat(shmid, NULL, 0);
    *remainingTime = p.runningtime;
    shmdt(remainingTime);

    // 2. Fork and exec
    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        char id_str[10];
        sprintf(id_str, "%d", shmid);
        execl("./process.out", "process.out", id_str, NULL);
        perror("Exec failed");
        exit(EXIT_FAILURE);
    } else {
        printf("Created process PID: %d for process ID: %d\n", pid, p.id);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./scheduler.out <num_processes> <algorithm> [timeslice]\n");
        return -1;
    }

    int num_processes = atoi(argv[0]);
    int algorithm = atoi(argv[1]);
    int time_slice = (argc == 3) ? atoi(argv[2]) : 0;

    signal(SIGUSR1, handler);

    initClk();

    // Setup message queue
    key_t key_id = ftok("keyfile", 65);
    msgq_id = msgget(key_id, 0666 | IPC_CREAT);
    if (msgq_id == -1) {
        perror("Error in msgget");
        exit(-1);
    }

    printf("Scheduler started. Waiting for processes...\n");

    int remaining = num_processes;
    while (remaining > 0) {
        struct msgbuff msg;
        int rec_val = msgrcv(msgq_id, &msg, sizeof(msg.data), 0, !IPC_NOWAIT);

        if (rec_val == -1) {
            perror("Error in receiving message");
            continue;
        }

        printf("Received process ID: %d, Arrival: %d, Runtime: %d, Priority: %d\n",
               msg.data.id, msg.data.arrivaltime, msg.data.runningtime, msg.data.priority);

        createProcess(msg.data);
        remaining--;
    }

    // Wait for all children
    while (wait(NULL) > 0);

    destroyClk(true);
    msgctl(msgq_id, IPC_RMID, NULL);

    return 0;
}
