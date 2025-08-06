#include "headers.h"

// Handle termination
void handl(int signum)
{
    destroyClk(false);
    raise(SIGKILL);  // terminate the process
}

int main(int argc, char * argv[])
{
    signal(SIGCHLD, handl);
    initClk();

    // ========== Parse shared memory ID from arguments ==========
    if (argc < 2)
    {
        fprintf(stderr, "Error: Missing shared memory ID argument.\n");
        exit(EXIT_FAILURE);
    }

    int shmid = atoi(argv[1]);
    int *remainingTime = (int *) shmat(shmid, NULL, 0);
    if (remainingTime == (void *) -1)
    {
        perror("Error attaching to shared memory");
        exit(EXIT_FAILURE);
