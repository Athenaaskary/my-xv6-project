#ifndef _PINFO_H_
#define _PINFO_H_

struct pinfo {
    int pid;
    int pstate;
    int priority;
    uint64 tickets;
    char name[16];
};

#endif
