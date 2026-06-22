#include "kernel/types.h"
#include "kernel/pinfo.h"
#include "user/user.h"

static char *
state_name(int state)
{
  static char *states[] = {
    "UNUSED", "USED", "SLEEP", "RUNNABLE", "RUNNING", "ZOMBIE"
  };

  if(state < 0 || state >= 6)
    return "UNKNOWN";
  return states[state];
}

int
main(int argc, char *argv[])
{
  struct pinfo info[64];
  int count = getpinfo(info);

  if(count < 0) {
    printf("getpinfo failed\n");
    exit(1);
  }

  printf("PID\tNAME\tSTATE\t\tPRIORITY\tTICKETS\n");
  for(int i = 0; i < count; i++) {
    printf("%d\t%s\t%s\t%d\t\t%d\n", info[i].pid, info[i].name,
           state_name(info[i].pstate), info[i].priority, info[i].tickets);
  }

  exit(0);
}
