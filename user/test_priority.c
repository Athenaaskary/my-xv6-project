#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define WORKLOAD 50000000

static void
busy_work(int n)
{
    volatile int x = 0;
    for (int i = 0; i < n; i++)
        x += i;
    (void)x;
}

int
main(void)
{
    printf("=== Priority Scheduler Test ===\n");

    if (fork() == 0) {
        setpriority(getpid(), 90);
        busy_work(WORKLOAD);
        printf("[PID %d] Priority 90 (LOW) finished.\n", getpid());
        exit(0);
    }

    if (fork() == 0) {
        setpriority(getpid(), 50);
        busy_work(WORKLOAD);
        printf("[PID %d] Priority 50 (MEDIUM) finished.\n", getpid());
        exit(0);
    }

    if (fork() == 0) {
        setpriority(getpid(), 10);
        busy_work(WORKLOAD);
        printf("[PID %d] Priority 10 (HIGH) finished.\n", getpid());
        exit(0);
    }

    wait(0);
    wait(0);
    wait(0);

    printf("=== Test Complete ===\n");
    exit(0);
}
