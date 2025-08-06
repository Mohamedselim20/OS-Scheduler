#include "headers.h"

void handl(int signum)
{
    destroyClk(false);
    raise(SIGKILL); // terminate the child process
}

int main(int argc, char * argv[])
{
    signal(SIGCHLD, handl);
    initClk();

    // Shared memory to get remaining time
    key_t shmKey1 = ftok("keyfile", 65);         // shared remaining time
    key_t prevClkKey = ftok("keyfile", 100);     // previous clock tick

    int remTimeID = shmget(shmKey1, sizeof(int), 0666);
    int prevClkID = shmget(prevClkKey, sizeof(int), 0666);

    if (remTimeID == -1 || prevClkID == -1)
    {
        perror("Error in getting the shared memory");
        exit(-1);
    }

    int *remainingTime = (int *) shmat(remTimeID, NULL, 0);
    int *prev = (int *) shmat(prevClkID, NULL, 0);

    if (remainingTime == (void *) -1 || prev == (void *) -1)
    {
        perror("Error in attaching the shared memory");
        exit(-1);
    }

    *prev = getClk(); // initialize prev

    while (*remainingTime > 0)
    {
        // Wait for a clock tick
        while (*prev == getClk())
            ; // busy wait until clock changes

        *prev = getClk();  // update to the new clock

        (*remainingTime)--;  // simulate doing work
        kill(getppid(), SIGUSR1);  // notify scheduler

        printf("Remaining time: %d pid: %d\n", *remainingTime, getpid());
        fflush(stdout);
    }

    // Detach and clean
    shmdt(remainingTime);
    shmdt(prev);
    destroyClk(false);

    return 0;
}
