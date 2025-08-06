#include "headers.h"

void handle_exit(int signum) {
    destroyClk(false);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <shm_key>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, handle_exit);
    signal(SIGUSR1, SIG_IGN);

    initClk();

    key_t shmkey = atoi(argv[1]);
    int shmid = shmget(shmkey, sizeof(int), 0666);
    if (shmid == -1) {
        perror("shmget failed in process");
        exit(-1);
    }

    int *remaining_time = (int *)shmat(shmid, NULL, 0);
    if (remaining_time == (void *)-1) {
        perror("shmat failed in process");
        exit(-1);
    }

    int last_clk = getClk();
    while (*remaining_time > 0) {
        while (getClk() == last_clk) {
            // wait
        }
        last_clk = getClk();

        (*remaining_time)--;
        printf("Remaining time: %d, PID: %d\n", *remaining_time, getpid());
        fflush(stdout);
    }

    shmdt(remaining_time);
    destroyClk(false);
    return 0;
}
