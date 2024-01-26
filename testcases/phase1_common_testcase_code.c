#include <usloss.h>
#include <phase1.h>

#include <stdio.h>
#include <assert.h>

#include "phase1helper.h"



/* make sure that we don't hit clock interrupts.  The student is not
 * required to implement a clock handler in Phase 1a
 */
static void dummy_clock_handler(int dev,void *arg)
{
    /* NOP */
}

void startup(int argc, char **argv)
{
    USLOSS_IntVec[USLOSS_CLOCK_INT] = dummy_clock_handler;

    phase1helper_init();
    phase1_init();
    startProcesses();
}



void phase2_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}

void phase3_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}

void phase4_start_service_processes()
{
    USLOSS_Console("%s() called -- currently a NOP\n", __func__);
}



void phase1_dispatcher_wrapper(int target_pid)
{
    USLOSS_Console("Phase 1A TEMPORARY HACK: init() manually switching to PID %d.\n", target_pid);
    TEMP_switchTo(target_pid);
}



void finish      (int argc, char **argv) {}
void test_setup  (int argc, char **argv) {}
void test_cleanup(int argc, char **argv) {}

