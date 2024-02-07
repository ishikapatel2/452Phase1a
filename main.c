#include "phase1helper.h"
#include "phase1.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct PCB {
    char name[MAXNAME+1];
    int pid; // process ID 
    int priority; // priority 
    USLOSS_Context state;
    void *stack;
    int status; // running, ready, or blocked
    int hasExited; // tracks if process has terminated
    struct PCB *parent;
    struct PCB *first_child; 
    struct PCB *next_sibling;
};

struct PQ {
    struct PCB *process;
    struct PQ *run_queue_next;
};

struct PQ *queue[7];

// current process running
struct PCB *curProcess;

// number of processes in process table
int processes = 0;

// process table
struct PCB pTable[MAXPROC];

// increments PID value every time new process is created
int PID = 2;

/*
    Called exactly once and initializes process table, queue, and init process.
*/
void phase1_init(void) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Someone attempted to call phase1_init while in user mode!\n");
        USLOSS_Halt(1);
    }

    // intitilizes table and queue
    memset(pTable, 0, sizeof(pTable));
    memset(queue, 0, sizeof(queue));

    curProcess = NULL;

    struct PCB initProcess;
    
    strcpy(initProcess.name, "init"); 
    initProcess.pid = 1;                     
    initProcess.priority = 6; 
    initProcess.status = 0;
    initProcess.hasExited = 0;
    initProcess.parent = NULL;
    initProcess.first_child = NULL;
    initProcess.next_sibling = NULL;
    initProcess.stack = (char *) malloc(USLOSS_MIN_STACK);

    russ_ContextInit(initProcess.pid, &initProcess.state, initProcess.stack, USLOSS_MIN_STACK, init_main, initProcess.name);

    pTable[1] = initProcess;
    processes += 1;

    struct PQ temp;
    temp.process = &initProcess;
    temp.run_queue_next = NULL;
    queue[6] = &temp; 
}

/*
    Finds the slot the process with pid, id in the pTable
*/
// int findProcess(int id) {
//     int slot = id % MAXPROC;
//     struct PCB *p = &pTable[slot];

//     while (p->pid != id && slot < 50) {
//         slot += 1;
//         p = &pTable[slot%MAXPROC];
//     }

//     return slot % MAXPROC;

// }

int findProcess(int id) {
    int startSlot = id % MAXPROC;
    int slot = startSlot;
    struct PCB *p = &pTable[slot];

    do {
        if (p->pid == id) {
            // Process with matching id found
            return slot;
        }
        // Move to the next slot and wrap around if necessary
        slot = (slot + 1) % MAXPROC;
        p = &pTable[slot];
    } while (slot != startSlot); // Ensure we don't loop indefinitely

    return -1; // should never return -1 
}


/*
    Context switches from one process to another.
*/
void TEMP_switchTo(int pid) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Someone attempted to call TEMP_switchTo while in user mode!\n");
        USLOSS_Halt(1);
    }

    int temp = findProcess(pid);

    if (curProcess == NULL) {
        curProcess = &pTable[temp];
        USLOSS_ContextSwitch(NULL, &pTable[temp].state);
    } else {
        struct PCB *oldProc = curProcess;
        curProcess = &pTable[temp];
        // USLOSS_Console("%p, %p\n", &oldProc->state, &pTable[temp].state);
        USLOSS_ContextSwitch(&oldProc->state, &pTable[temp].state);
    }
     
}

/*
    Creates a new process, which is the child of the currently running process
    and returns this child's pid if process table is not full. 
*/
int  spork(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Someone attempted to call spork while in user mode!\n");
        USLOSS_Halt(1);
    }

    if ((processes > MAXPROC) || (priority < 1) || (priority > 5) || 
    (name == NULL) || (strlen(name) > MAXNAME) || (func == NULL))  {
        return -1;
    }
    else if (stacksize < USLOSS_MIN_STACK) {
        return -2;
    }

    // add new process to process table
    // if slot is full, keep incrementing until empty slot is found

    int slot = (PID % MAXPROC);
    while (pTable[slot].pid != 0) {
        slot = (slot + 1) % MAXPROC;

        if (slot == PID % MAXPROC) {

            // should never return -1
            return -1;
        }
    }
    
    struct PCB *newProcess = &pTable[slot];

    // set new process properties
    strcpy(newProcess->name, name);
    newProcess->priority = priority;
    newProcess->pid = PID; 
    newProcess->status = 0;
    newProcess->hasExited = 0;
    newProcess->parent = curProcess;
    newProcess->first_child = NULL;
    newProcess->next_sibling = NULL;

    newProcess->next_sibling = curProcess->first_child;
    curProcess->first_child = newProcess;
    
    newProcess->stack = (char *) malloc(stacksize);
    russ_ContextInit(newProcess->pid, &newProcess->state, newProcess->stack, USLOSS_MIN_STACK, func, arg);

    // increment the number of processes currently in the table
    processes += 1;

    // increment PID
    PID += 1;
    
    return newProcess->pid;
}

/*
    Delivers the "status" of the child (the parameter that the child passed to quit())
    back to the parent. If the current process has a dead child, join() reports its
    status. Returns the pid of the joined process if child has been terminated.
    Returns -3 if there is no status. Returns -2 if child has not been terminated.
*/
int  join(int *status) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Someone attempted to call join while in user mode!\n");
        USLOSS_Halt(1);
    }

    if (status == NULL) {
        return -3;
    } 
    
    struct PCB *child;
    struct PCB *prevChild = NULL;
 
    for (child = curProcess->first_child; child != NULL; child = child->next_sibling) {
        
        // USLOSS_Console("start here f%p\n", child);
        if (child->hasExited) {

            // fix pointers to child processes
            if (prevChild == NULL) {
                curProcess->first_child = child->next_sibling;
            }
            else {
                prevChild->next_sibling = child->next_sibling;
            }

            // find slot where child is located on ptable
            int slot = findProcess(child->pid);

            // USLOSS_Console("%p\n", child);

            int temp = child->pid;
            *status = child->status; // Set the exit status of the child
            free(child->stack); // free child's memory
            memset(&pTable[slot], 0, sizeof(struct PCB));
            
            return temp; // Return the PID of the joined child
        }
        prevChild = child;
    }
    return -2;
}

/*
    Quits the running process and switches to the next process. Ensure
    process doesn't quit until all of its children have ended and parent
    has collected all of their statuses (using join()).   
*/
void quit_phase_1a(int status, int switchToPid) {

    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Someone attempted to call quit_phase_1a while in user mode!\n");
        USLOSS_Halt(1);
    }

    int slot = findProcess(switchToPid);

    if (curProcess->first_child != 0) {
        USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", getpid());
        USLOSS_Halt(1);
    }

    // set to exited and status (for join)
    curProcess->hasExited = 1;
    curProcess->status = status;
    
    curProcess = &pTable[slot];
    // USLOSS_Console("%p, %p\n", &oldProc->state, &pTable[temp].state);
    
    USLOSS_ContextSwitch(NULL, &curProcess->state);
    exit(status);
}

/*
    Returns the pid of current running process.    
*/
int  getpid(void) {
    if (curProcess == NULL)
        return 1;
    return curProcess->pid;
}

/*
    Prints inforation about all processes from the process table in any state. 
*/
void dumpProcesses() {
    if ((USLOSS_PsrGet() && USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Someone attempted to call dumpProcesses while in user mode!\n");
        USLOSS_Halt(1);
    }

    int i = 0;
    struct PCB *temp = &pTable[i];
    USLOSS_Console("%4s  %4s  %-17s %-10s%-6s\n", "PID", "PPID", "NAME", "PRIORITY", "STATE");
    while (i < MAXPROC) {
        if (pTable[i].pid != 0) {
            temp = &pTable[i];
            int ppid;
            if (temp->parent == NULL) {
                ppid = 0;
            }
            else {
                ppid = temp->parent->pid;
            }
            
            USLOSS_Console("%4d  %4d  %-17s %-10d", temp->pid, ppid, temp->name, temp->priority);
            if (temp->status == 0 && temp->pid == curProcess->pid) {
                USLOSS_Console("Running\n");
            }
            else if (temp->status == 0) {
                USLOSS_Console("Runnable\n");
            }
            else {
                USLOSS_Console("Terminated(%d)\n", temp->status);
            }
        }
        i += 1;
    }
}