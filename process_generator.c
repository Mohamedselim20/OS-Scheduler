#include "headers.h"

void clearResources(int);

int msgq_id, shmid;

void arrivalsSharedMemory()
{
    key_t shmKey;
    shmKey = ftok("keyfile", 120);
    shmid = shmget(shmKey, sizeof(int) * 1000, IPC_CREAT | 0666);
    if (shmid == -1)
    {
        perror("Error in creating the shared memory");
        exit(-1);
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, clearResources);

    arrivalsSharedMemory();
    int *arrivals = (int *)shmat(shmid, 0, 0);
    for (int i = 0; i < 1000; i++)
        arrivals[i] = 0;

    key_t key_id;
    int send_val;
    key_id = ftok("keyfile", 65);
    msgq_id = msgget(key_id, 0666 | IPC_CREAT);
    if (msgq_id == -1)
    {
        perror("Error in creating the message queue\n");
        exit(-1);
    }
    else
        printf("Message Queue created successfully!\n");

    FILE *file = fopen("processes.txt", "r");
    if (file == NULL) {
        printf("Error! File not opened.\n");
        return 1;
    }

    int numberOfProcesses = 0;
    fscanf(file, "%*[^\n]");
    while (!feof(file)) {
        int i, a, r, p;
        fscanf(file, "%d\t%d\t%d\t%d", &i, &a, &r, &p);
        numberOfProcesses = i;
    }
    printf("Number of Processes: %d\n", numberOfProcesses);

    fclose(file);
    file = fopen("processes.txt", "r");
    if (file == NULL) {
        printf("Error! File not opened.\n");
        return 1;
    }

    struct processData *processes = malloc(sizeof(struct processData) * numberOfProcesses);
    fscanf(file, "%*[^\n]");
    int index = 0;
    printf("***Input***\n");
    printf("id   arrival   runtime   priority\n");
    while (index < numberOfProcesses) {
        int i, a, r, p;
        fscanf(file, "%d\t%d\t%d\t%d", &i, &a, &r, &p);
        processes[index].id = i;
        processes[index].arrivaltime = a;
        processes[index].runningtime = r;
        processes[index].priority = p;
        arrivals[a]++;
        printf("%d\t%d\t%d\t%d\n", i, a, r, p);
        index++;
    }
    fclose(file);

    int algorithm;
    printf("-------------------------------\n");
    printf("Choose a scheduling algorithm:\n");
    printf("1. Non-preemptive Highest Priority First (HPF).\n");
    printf("2. Shortest Remaining time Next (SRTN).\n");
    printf("3. Round Robin (RR).\n");
    scanf("%d", &algorithm);
    printf("-------------------------------\n");

    int timeSlot;
    if (algorithm == 3) {
        printf("Enter the time slot for Round Robin: ");
        scanf("%d", &timeSlot);
    }

    // Fork Scheduler
    printf("Forking Scheduler..\n");
    int pid = fork();
    if (pid == -1)
        perror("Error in creating scheduler process\n");
    else if (pid == 0) {
        system("gcc -Wall -o scheduler.out scheduler.c -lm -fno-stack-protector");
        printf("Scheduling..\n");
        char n_str[10], a_str[10], t_str[10];
        if (algorithm == 3)
            execl("./scheduler.out", "scheduler.out", string(numberOfProcesses, n_str), string(algorithm, a_str), string(timeSlot, t_str), NULL);
        else
            execl("./scheduler.out", "scheduler.out", string(numberOfProcesses, n_str), string(algorithm, a_str), NULL);
    }

    // Fork Clock
    if (pid != 0) {
        printf("Forking Clock..\n");
        int clkpid = fork();
        if (clkpid == -1)
            perror("Error in creating clock process\n");
        else if (clkpid == 0) {
            system("gcc clk.c -o clk.out -fno-stack-protector");
            execl("./clk.out", "clk", NULL);
        }
        else {
            initClk();

            int current = 0;
            while (current < numberOfProcesses) {
                int arrival = processes[current].arrivaltime;
                if (arrival == getClk()) {
                    send_val = msgsnd(msgq_id, &processes[current], sizeof(struct processData), !IPC_NOWAIT);
                    current++;
                }
            }
            int stat_loc;
            waitpid(pid, &stat_loc, 0);
        }
    }
}

void clearResources(int signum)
{
    printf("Clearing\n");
    msgctl(msgq_id, IPC_RMID, (struct msqid_ds *)0);
    raise(SIGKILL);
}
