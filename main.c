#include "phase1helper.h"
#include "phase1.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct PCB {
    char name[MAXNAME+1]; // name of process
    int pid; // process ID 
    int priority; // priority 
    USLOSS_Context state;
    void *stack; // pointer to process stack
    int status; // process status: running, ready, or blocked
    int hasExited; // flag to check if process has terminated
    int used; // flag to check if process in use
    struct PCB *parent; 
    struct PCB *first_child; // pointer to its children
    struct PCB *next_sibling;  // pointer to next sibling
};

struct PQ {
    struct PCB *process;
    struct PQ *run_queue_next;
};

// priority queue
struct PQ *queue[7];

// current process running
struct PCB *curProcess;

// number of processes in process table
int processes = 1;

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

    struct PCB *initProcess = &pTable[1];
    
    // intializing init process's properties 
    strcpy(initProcess->name, "init"); 
    initProcess->pid = 1;                     
    initProcess->priority = 6; 
    initProcess->status = 0;
    initProcess->hasExited = 0;
    initProcess->parent = NULL;
    initProcess->first_child = NULL;
    initProcess->next_sibling = NULL;
    initProcess->used = 1;
    initProcess->stack = (char *) malloc(USLOSS_MIN_STACK);

    russ_ContextInit(initProcess->pid, &initProcess->state, initProcess->stack, USLOSS_MIN_STACK, init_main, initProcess->name);
}

/*
    Context switches from one process to another, saving old process's state.
*/
void TEMP_switchTo(int pid) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Someone attempted to call TEMP_switchTo while in user mode!\n");
        USLOSS_Halt(1);
    }

    // finds the slot where the new prcoess being context switched to is 
    int slot = pid % MAXPROC;

    if (curProcess == NULL) {
        curProcess = &pTable[slot];
        USLOSS_ContextSwitch(NULL, &pTable[slot].state);
    } else {
        struct PCB *oldProc = curProcess;
        curProcess = &pTable[slot];
        USLOSS_ContextSwitch(&oldProc->state, &pTable[slot].state);
    }
}

/*
    Creates a new process, which is the child of the currently running process
    and returns this child's pid if process table is not full. Returns -1 if the process
    table is full, if priority given for new process is not between 1-5, if the func is null,
    or if the name of the process exceeds MAXNAME. Returns -2 if the stack size of the
    new process is less than USLOSS_MIN_STACK. 
*/
int  spork(char *name, int(*func)(char *), char *arg, int stacksize, int priority) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Someone attempted to call spork while in user mode!\n");
        USLOSS_Halt(1);
    }

    if ((processes >= MAXPROC) || (priority < 1) || (priority > 5) || 
    (name == NULL) || (strlen(name) > MAXNAME) || (func == NULL))  {
        return -1;
    }
    else if (stacksize < USLOSS_MIN_STACK) {
        return -2;
    }
    
    // finds the next open slot in the process table to put new process
    int slot = PID % MAXPROC;
    int count = 1;
    while (count <= MAXPROC && pTable[slot].used == 1) {
        PID++;
        slot = PID % MAXPROC;
        count += 1;

        // checks if process table is full
        if (count == MAXPROC) {
            return -1;
        }
    }

    struct PCB *newProcess = &pTable[slot];  

    // increment number of process in process table 
    processes += 1;

    // set new process properties
    strcpy(newProcess->name, name);
    newProcess->priority = priority;
    newProcess->pid = PID; 
    newProcess->status = 0;
    newProcess->hasExited = 0;
    newProcess->used = 1;
    newProcess->parent = curProcess;
    newProcess->first_child = NULL;
    newProcess->next_sibling = NULL;
    newProcess->next_sibling = curProcess->first_child;
    curProcess->first_child = newProcess;
    newProcess->stack = (char *) malloc(stacksize);

    russ_ContextInit(newProcess->pid, &newProcess->state, newProcess->stack, USLOSS_MIN_STACK, func, arg);

    // PID for new proces
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


    // iterates through all of current process's children to determine if they have all
    // been terminated 
    for (child = curProcess->first_child; child != NULL; child = child->next_sibling) {
        if (child->hasExited) {

            // fix pointers to child processes 
            if (prevChild == NULL) {
                curProcess->first_child = child->next_sibling;
            }
            else {
                prevChild->next_sibling = child->next_sibling;
            }

            // find slot where child is located in process table
            int slot = child->pid % MAXPROC;
            processes--;
            int temp = child->pid;
            *status = child->status; // set the exit status of the child
            free(child->stack); // free child's memory
            memset(&pTable[slot], 0, sizeof(struct PCB)); // reset memory at the slot
            pTable[slot].used = 0;
            
            return temp; // return the PID of the joined child
        }
        prevChild = child;
    }
    return -2;
}

/*
    Quits the running 3wwand switches to the next process. Ensure
    process doesn't quit until all of its children have ended and parent
    has collected all of their statuses (using join()).   
*/
void quit_phase_1a(int status, int switchToPid) {
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0) {
        USLOSS_Console("ERROR: Someone attempted to call quit_phase_1a while in user mode!\n");
        USLOSS_Halt(1);
    }

    if (curProcess->first_child != 0) {
        USLOSS_Console("ERROR: Process pid %d called quit() while it still had children.\n", getpid());
        USLOSS_Halt(1);
    }

    // set to exited and status (for join)
    if (curProcess->pid != 1) {
        curProcess->hasExited = 1;
        curProcess->used = 0;
        curProcess->status = status;   
    
        int slot = switchToPid % MAXPROC;
        curProcess = &pTable[slot];
        
        USLOSS_ContextSwitch(NULL, &curProcess->state);
    }

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
    Prints information about all processes in the process table. 
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

        // print all processes that have not been joined and are still in process table
        if (pTable[i].pid != 0) {
            temp = &pTable[i];
            int ppid;
            if (temp->parent == NULL) {
                ppid = 0;
            }
            else {

                // gets parent's pid
                ppid = temp->parent->pid;
            }
            
            USLOSS_Console("%4d  %4d  %-17s %-10d", temp->pid, ppid, temp->name, temp->priority);

            // prints process status
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