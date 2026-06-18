#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int pid = fork();
    int start = uptime();

    if (pid == 0) {
        settickets(10);

        for(long i = 0; i < 500000000; i++);
        printf("POOR (10 tickets) finished at %d ticks.\n", uptime() - start);
        exit(0);
    } else {
        settickets(100);
        for(long i = 0; i < 500000000; i++);
        printf("RICH (100 tickets) finished at %d ticks.\n", uptime() - start);
        wait(0);
        exit(0);
    }
}
