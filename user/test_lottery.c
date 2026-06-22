#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pinfo.h"
#include "user/user.h"

#define TRIALS 3
#define TRIAL_TICKS 30

struct lottery_result {
    int tickets;
    uint64 work;
};

static int
read_full(int fd, void *buffer, int size)
{
    int total = 0;
    while (total < size) {
        int n = read(fd, (char *)buffer + total, size - total);
        if (n <= 0)
            return -1;
        total += n;
    }
    return 0;
}

static int
current_tickets(void)
{
    struct pinfo info[64];
    int count = getpinfo(info);
    int pid = getpid();

    for (int i = 0; i < count; i++)
        if (info[i].pid == pid)
            return info[i].tickets;
    return -1;
}

static int
test_ticket_inheritance(void)
{
    if (settickets(37) < 0)
        return -1;

    int pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0) {
        int inherited = current_tickets();
        printf("Inherited tickets: %d (expected 37)\n", inherited);
        exit(inherited == 37 ? 0 : 1);
    }

    int status = -1;
    wait(&status);
    settickets(1);
    printf("Ticket inheritance: %s\n", status == 0 ? "PASS" : "FAIL");
    return status == 0 ? 0 : -1;
}

static int
start_worker(int tickets, int ready_write, int deadline_read,
             int result_write)
{
    int pid = fork();
    if (pid != 0)
        return pid;

    if (settickets(tickets) < 0)
        exit(1);
    write(ready_write, "R", 1);

    int deadline;
    if (read_full(deadline_read, &deadline, sizeof(deadline)) < 0)
        exit(1);

    volatile uint64 work = 0;
    while (uptime() < deadline) {
        for (int i = 0; i < 1000; i++)
            work++;
    }

    struct lottery_result result = {tickets, work};
    write(result_write, &result, sizeof(result));
    exit(0);
}

static int
run_trial(int trial, uint64 *low_total, uint64 *high_total)
{
    int ready[2], deadline_pipe[2], result_pipe[2];
    if (pipe(ready) < 0 || pipe(deadline_pipe) < 0 || pipe(result_pipe) < 0)
        return -1;

    start_worker(10, ready[1], deadline_pipe[0], result_pipe[1]);
    start_worker(100, ready[1], deadline_pipe[0], result_pipe[1]);

    close(ready[1]);
    close(deadline_pipe[0]);
    close(result_pipe[1]);

    char ready_bytes[2];
    if (read_full(ready[0], ready_bytes, sizeof(ready_bytes)) < 0)
        return -1;

    int deadline = uptime() + TRIAL_TICKS;
    write(deadline_pipe[1], &deadline, sizeof(deadline));
    write(deadline_pipe[1], &deadline, sizeof(deadline));
    close(deadline_pipe[1]);

    struct lottery_result first, second;
    if (read_full(result_pipe[0], &first, sizeof(first)) < 0 ||
        read_full(result_pipe[0], &second, sizeof(second)) < 0)
        return -1;

    close(ready[0]);
    close(result_pipe[0]);
    wait(0);
    wait(0);

    uint64 low = first.tickets == 10 ? first.work : second.work;
    uint64 high = first.tickets == 100 ? first.work : second.work;
    *low_total += low;
    *high_total += high;

    int ratio100 = low == 0 ? 0 : (int)(high * 100 / low);
    printf("Trial %d: T100=%lu, T10=%lu, ratio=%d.%d\n",
           trial, high, low, ratio100 / 100, ratio100 % 100);
    return 0;
}

int
main(void)
{
    printf("=== Lottery Scheduler Test ===\n");

    int validation_ok = settickets(0) < 0;
    printf("Reject zero tickets: %s\n", validation_ok ? "PASS" : "FAIL");

    int inheritance_ok = test_ticket_inheritance() == 0;
    uint64 low_total = 0;
    uint64 high_total = 0;
    int trials_ok = 1;

    for (int trial = 1; trial <= TRIALS; trial++)
        if (run_trial(trial, &low_total, &high_total) < 0)
            trials_ok = 0;

    int ratio100 = low_total == 0 ? 0 : (int)(high_total * 100 / low_total);
    int share_ok = trials_ok && ratio100 >= 400;
    printf("Aggregate CPU ratio T100:T10 = %d.%d (expected near 10.0)\n",
           ratio100 / 100, ratio100 % 100);
    printf("Proportional CPU share: %s\n", share_ok ? "PASS" : "FAIL");

    int passed = validation_ok && inheritance_ok && share_ok;
    printf("=== Lottery Test: %s ===\n", passed ? "PASS" : "FAIL");
    exit(passed ? 0 : 1);
}
