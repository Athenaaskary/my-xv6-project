#include "kernel/types.h"
#include "kernel/pinfo.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  struct pinfo info[64];
  int count = getpinfo(info);

  if(count < 0) {
    printf("getpinfo failed\n");
    exit(1);
  }

  printf("PID  NAME       STATE\n");
  for(int i = 0; i < count; i++) {
    printf("%d    %s      %d\n", info[i].pid, info[i].name, info[i].pstate);
  }

  exit(0);
}
