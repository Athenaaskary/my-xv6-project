#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static int
parse_number(const char *text, int *value)
{
    int result = 0;

    if(text[0] == 0)
        return -1;
    for(int i = 0; text[i] != 0; i++) {
        if(text[i] < '0' || text[i] > '9')
            return -1;
        result = result * 10 + text[i] - '0';
    }

    *value = result;
    return 0;
}

int
main(int argc, char *argv[])
{
  if (argc != 3) {
        printf("Usage: chpri <pid> <priority>\n");
        printf("  priority: 0 (highest) to 100 (lowest)\n");
        exit(1);
    }

  int pid, priority;

  if (parse_number(argv[1], &pid) < 0 || pid <= 0) {
    printf("chpri: invalid pid '%s'\n", argv[1]);
    exit(1);
  }

  if (parse_number(argv[2], &priority) < 0 || priority > 100) {
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
