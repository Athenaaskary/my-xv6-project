#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: chpri <pid> <priority>\n");
        printf("  priority: 0 (highest) to 100 (lowest)\n");
        exit(1);
    }

    int pid = atoi(argv[1]);
    int priority = atoi(argv[2]);

    if (pid <= 0) {
        printf("chpri: invalid pid '%s'\n", argv[1]);
        exit(1);
    }

    if (priority < 0 || priority > 100) {
        printf("chpri: priority must be between 0 and 100\n");
        exit(1);
    }

    if (setpriority(pid, priority) < 0) {
        printf("chpri: failed to set priority for pid %d\n", pid);
        printf("       (process may not exist)\n");
        exit(1);
    }

    printf("chpri: pid %d priority set to %d\n", pid, priority);
    exit(0);
}
