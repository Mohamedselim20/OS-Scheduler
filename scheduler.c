#include "headers.h"
#include <string.h>
#include <signal.h>
#include <limits.h>

int msgq_id;
int algorithm, time_slice, num_processes;
int clk;

struct RunningProcess {
    struct PCB pcb;
    int shmid;
    int pid;
};

void cleanup(int signum) {
    msgctl(msgq_id, IPC_RMID, NULL);
    destroyClk(true);
    exit(0);
}

void wait_for_clk_tick(int *last_clk) {
    while (getClk() == *last_clk) {
        // busy wait
    }
    *last_clk = getClk();
}

int main(int argc, char *argv[]) {
    signal(SIGINT, cleanup);
    initClk();

    if (argc < 3) {
        printf("Usage: %s <num_processes> <algorithm> [time_slice]\n", argv[0]);
        return -1;
    }

    num_processes = atoi(argv[1]);
    algorithm = atoi(argv[2]);
    time_slice = (argc == 4) ? atoi(argv[3]) : 0;

    key_t key_id = ftok("keyfile", 65);
    msgq_id = msgget(key_id, 0666 | IPC_CREAT);
    if (msgq_id == -1) {
        perror("Error creating message queue");
        exit(-1);
    }

    clk = getClk();
    Node *pq = NULL;
    struct Queue *rrQueue = createQueue(100);

    int finished = 0;
    struct processData proc;
    struct RunningProcess *running = NULL;
    int last_clk = clk;
    int rr_counter = 0;

    while (finished < num_processes) {
        while (msgrcv(msgq_id, &proc, sizeof(proc), 0, IPC_NOWAIT) != -1) {
            struct PCB *pcb = malloc(sizeof(struct PCB));
            setPCB(pcb, proc.arrivaltime, proc.runningtime, 0, proc.id, 0, 0, -1, 0, 0);

            if (algorithm == 1) {
                push(&pq, pcb, proc.priority);
            } else if (algorithm == 2) {
                push(&pq, pcb, proc.runningtime);
            } else if (algorithm == 3) {
                enqueue(rrQueue, pcb);
            }
        }

        wait_for_clk_tick(&last_clk);

        if (!running) {
            struct PCB *next = NULL;
            if (algorithm == 1 || algorithm == 2) {
                if (!isEmpty(&pq)) {
                    next = peek(&pq);
                    pop(&pq);
                }
            } else if (algorithm == 3) {
                if (!isEmptyQ(rrQueue)) {
                    next = dequeue(rrQueue);
                }
            }

            if (next) {
                key_t shmkey = ftok("keyfile", 100 + next->id);
                int shmid = shmget(shmkey, sizeof(int), IPC_CREAT | 0666);
                int *shmaddr = (int *)shmat(shmid, NULL, 0);
                *shmaddr = next->brust;

                pid_t pid = fork();
                if (pid == 0) {
                    char shm_id_str[20];
                    sprintf(shm_id_str, "%d", shmkey);
                    execl("./process.out", "process.out", shm_id_str, NULL);
                    exit(1);
                }

                struct RunningProcess *rp = malloc(sizeof(struct RunningProcess));
                rp->pcb = *next;
                rp->shmid = shmid;
                rp->pid = pid;
                rp->pcb.start = getClk();

                running = rp;
            }
        } else {
            int *shmaddr = (int *)shmat(running->shmid, NULL, 0);
            if (*shmaddr <= 0) {
                waitpid(running->pid, NULL, 0);
                finished++;
                shmdt(shmaddr);
                shmctl(running->shmid, IPC_RMID, NULL);
                free(running);
                running = NULL;
                rr_counter = 0;
            } else if (algorithm == 3) {
                rr_counter++;
                if (rr_counter >= time_slice) {
                    kill(running->pid, SIGSTOP);
                    enqueue(rrQueue, &running->pcb);
                    rr_counter = 0;
                    running = NULL;
                }
            } else if (algorithm == 2) {
                if (!isEmpty(&pq) && peek(&pq)->brust < *shmaddr) {
                    kill(running->pid, SIGSTOP);
                    push(&pq, &running->pcb, *shmaddr);
                    running = NULL;
                }
            }
        }
    }

    destroyClk(true);
    msgctl(msgq_id, IPC_RMID, NULL);
    return 0;
}
