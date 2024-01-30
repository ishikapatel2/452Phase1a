#include "phase1helper.h"
#include "phase1.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


struct PCB {
    char name[MAXNAME];
    int pid;
    int priority;
    USLOSS_Context *state;
    struct PCB *parent;
    struct PCB *child;
    struct PCB *run_queue_next;
};


// process table
struct PCB pTable[MAXPROC];

// current process running
struct PCB *curProcess;

/*
    Called exactly once (when the simulator starts up). Initialize data structures
    here including setting up the process table entry for the starting process, init.

    Create the process table entry for init in slot 1 but don't run it yet.
*/
void phase1_init(void) {

    // intitilizes table
    memset(pTable, 0, sizeof(pTable));

    curProcess = NULL;

    // check for out of memory error
    char *stack = (char *) malloc(USLOSS_MIN_STACK);

    // init is a kernel mode process 
    struct PCB initProcess;
    strcpy(initProcess.name, "init"); 
    initProcess.pid = 1;                     
    initProcess.priority = 6; 
    initProcess.parent = NULL;
    initProcess.child = NULL;
    initProcess.run_queue_next = NULL;


    // check for out of memory error
    initProcess.state = (USLOSS_Context *) malloc(sizeof(USLOSS_Context));

    russ_ContextInit(initProcess.pid, initProcess.state, stack, USLOSS_MIN_STACK, init_main, initProcess.name);

    pTable[0] = initProcess;
}

/*

    Creates a new process, which is the child of the currently running process.

    Does not call dispatcher after it creates a new process. Instead, testcase
    will be responsible for choosing when to switch to another process. 

    Keep running parent process until testcases tells you otherwise.
*/
int  spork(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {
    return 0;
}

/*
    Similar to UNIX wait() syscall.

    Delivers the "status" of the child (the parameter that the child passed to quit())
    back to the parent. 


    No blocking. If the current process has a dead child, then join() should report 
    its status, exactly like in phase 1b. Likewise, if the current process has no 
    children at all (alive or dead), then join() should return, exactly like in phase 1b. 

   You may assume you don't have to handle the situation where a parent 
   blocks, waiting for its child to die.
*/
int  join(int *status) {
    return 0;
}

int  getpid(void) {
    if (curProcess == NULL)
        return -1;

    return curProcess->pid;
}


void dumpProcesses() {

}

/*
    Since you don’t have a dispatcher, the calling process has to tell 
    you which process will run next.

    Never needs to wake up a blocked parent process.


*/
void quit_phase_1a(int status, int switchToPid) {
    exit(status);
}


/*
    Testcases will choose exactly when to switch from one process to another.
    To tell you when to switch, they will call the following method. Switch to
    indicated process.

    These context switches, must call USLOSS_ContextSwitch() and save the old process
    state, so that it is possible to switch back to them later.

*/
void TEMP_switchTo(int pid) {

}
