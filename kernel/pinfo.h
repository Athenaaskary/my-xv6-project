#ifndef _PINFO_H_
#define _PINFO_H_

struct pinfo {
    int pid;
    int pstate;
    int priority;
    int tickets;
    char name[16];
};

#endif
