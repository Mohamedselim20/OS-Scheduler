#include "headers.h"

void handl(int signum)
{
    destroyClk(false);
    raise(SIGKILL);  // kill self
}

int main(int argc, char * argv[])
{
    signal(SIGCHLD, handl);
    initClk();

    if (argc < 2)
    {
        fprintf(stderr, "Error: Missing shared memory ID.\n");
        exit(EXIT_FAILURE);
    }

    int shmid = atoi(argv[1]);
    int *remainingTime = (int *) shmat(shmid, NULL, 0);
    if (remainingTime == (void *) -1)
    {
        perror("Error attaching shared memory");
        exit(EXIT_FAILURE);
    }

    int prevClk = getClk();

    while (*remainingTime > 0)
    {
        while (prevClk == getClk()); // wait one tick
        prevClk = getClk();

        (*remainingTime)--;

        kill(getppid(), SIGUSR1);

        printf("Remaining time: %d, PID: %d\n", *remainingTime, getpid());
        fflush(stdout);
    }

    // Cleanup
    shmdt(remainingTime);
    shmctl(shmid, IPC_RMID, NULL);
    destroyClk(false);

    return 0;
}
