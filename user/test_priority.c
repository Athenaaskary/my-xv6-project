#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define WORKLOAD 50000000
#define FAIR_TICKS 30

struct priority_result {
    int priority;
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

static void
busy_work(int n)
{
    volatile int x = 0;
    for (int i = 0; i < n; i++)
        x += i;
    (void)x;
}

static int
start_order_worker(int priority, int ready_write, int start_read,
                   int result_write)
{
    int pid = fork();
    if (pid != 0)
        return pid;

    if (setpriority(getpid(), priority) < 0) {
        printf("FAIL: setpriority(%d)\n", priority);
        exit(1);
    }

    write(ready_write, "R", 1);
    char start;
    if (read(start_read, &start, 1) != 1)
        exit(1);

    busy_work(WORKLOAD);
    struct priority_result result = {priority, 0};
    write(result_write, &result, sizeof(result));
    exit(0);
}

static int
run_order_test(void)
{
    int ready[2], start[2], result_pipe[2];
    if (pipe(ready) < 0 || pipe(start) < 0 || pipe(result_pipe) < 0)
        return -1;

    start_order_worker(90, ready[1], start[0], result_pipe[1]);
    start_order_worker(50, ready[1], start[0], result_pipe[1]);
    start_order_worker(10, ready[1], start[0], result_pipe[1]);

    close(ready[1]);
    close(start[0]);
    close(result_pipe[1]);

    char ready_bytes[3];
    if (read_full(ready[0], ready_bytes, sizeof(ready_bytes)) < 0)
        return -1;
    write(start[1], "GGG", 3);
    close(start[1]);

    int expected[] = {10, 50, 90};
    int passed = 1;
    for (int i = 0; i < 3; i++) {
        struct priority_result result;
        if (read_full(result_pipe[0], &result, sizeof(result)) < 0)
            return -1;
        printf("Finished #%d: priority %d\n", i + 1, result.priority);
        if (result.priority != expected[i])
            passed = 0;
    }

    close(ready[0]);
    close(result_pipe[0]);
    wait(0);
    wait(0);
    wait(0);

    printf("Strict priority order: %s\n", passed ? "PASS" : "FAIL");
    return passed ? 0 : -1;
}

static int
start_fair_worker(int worker, int ready_write, int start_read,
                  int result_write)
{
    int pid = fork();
    if (pid != 0)
        return pid;

    if (setpriority(getpid(), 40) < 0)
        exit(1);
    write(ready_write, "R", 1);

    char start;
    if (read(start_read, &start, 1) != 1)
        exit(1);

    int begin = uptime();
    volatile uint64 work = 0;
    while (uptime() - begin < FAIR_TICKS) {
        for (int i = 0; i < 1000; i++)
            work++;
    }

    struct priority_result result = {worker, work};
    write(result_write, &result, sizeof(result));
    exit(0);
}

static int
run_equal_priority_test(void)
{
    int ready[2], start[2], result_pipe[2];
    if (pipe(ready) < 0 || pipe(start) < 0 || pipe(result_pipe) < 0)
        return -1;

    start_fair_worker(1, ready[1], start[0], result_pipe[1]);
    start_fair_worker(2, ready[1], start[0], result_pipe[1]);

    close(ready[1]);
    close(start[0]);
    close(result_pipe[1]);

    char ready_bytes[2];
    if (read_full(ready[0], ready_bytes, sizeof(ready_bytes)) < 0)
        return -1;
    write(start[1], "GG", 2);
    close(start[1]);

    struct priority_result first, second;
    if (read_full(result_pipe[0], &first, sizeof(first)) < 0 ||
        read_full(result_pipe[0], &second, sizeof(second)) < 0)
        return -1;

    close(ready[0]);
    close(result_pipe[0]);
    wait(0);
    wait(0);

    uint64 minimum = first.work < second.work ? first.work : second.work;
    uint64 maximum = first.work > second.work ? first.work : second.work;
    int fairness = maximum == 0 ? 0 : (int)(minimum * 100 / maximum);

    printf("Equal-priority worker 1: %lu units\n",
           first.priority == 1 ? first.work : second.work);
    printf("Equal-priority worker 2: %lu units\n",
           first.priority == 2 ? first.work : second.work);
    printf("Round Robin fairness: %d%% (%s)\n", fairness,
           fairness >= 60 ? "PASS" : "FAIL");
    return fairness >= 60 ? 0 : -1;
}

int
main(void)
{
    printf("=== Priority Scheduler Test ===\n");
    int order_ok = run_order_test() == 0;
    int fairness_ok = run_equal_priority_test() == 0;

    printf("=== Priority Test: %s ===\n",
           order_ok && fairness_ok ? "PASS" : "FAIL");
    exit(order_ok && fairness_ok ? 0 : 1);
}
