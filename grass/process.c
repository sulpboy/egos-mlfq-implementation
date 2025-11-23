/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: helper functions for process management
 */

#include "process.h"

#define MLFQ_NLEVELS          5
#define MLFQ_RESET_PERIOD     10000000         /* 10 seconds */
#define MLFQ_LEVEL_RUNTIME(x) (x + 1) * 100000 /* e.g., 100ms for level 0 */
extern struct process proc_set[MAX_NPROCESS + 1];
extern ulonglong mtime_get(void);

static void proc_set_status(int pid, enum proc_status status) {
    for (uint i = 0; i < MAX_NPROCESS; i++)
        if (proc_set[i].pid == pid) proc_set[i].status = status;
}


void proc_set_ready(int pid) { proc_set_status(pid, PROC_READY); }
void proc_set_running(int pid) { proc_set_status(pid, PROC_RUNNING); }
void proc_set_runnable(int pid) { proc_set_status(pid, PROC_RUNNABLE); }
void proc_set_pending(int pid) { proc_set_status(pid, PROC_PENDING_SYSCALL); }

int proc_alloc() {
    static uint curr_pid = 0;
    for (uint i = 1; i <= MAX_NPROCESS; i++)
        if (proc_set[i].status == PROC_UNUSED) {
            proc_set[i].pid    = ++curr_pid;
            proc_set[i].status = PROC_LOADING;
            /* Student's code goes here (Preemptive Scheduler | System Call). */

            /* Initialization of lifecycle statistics, MLFQ or process sleep. */
            proc_set[i].created_ms=mtime_get();
            proc_set[i].cputime_ms=0;
            proc_set[i].timer_intrpt_count=0;
            proc_set[i].firsttime_ms=(ulonglong)-1;
            //we need a common running time to gvet exit_ms. to get turnaroundms=exit-creation

            //queue init
            proc_set[i].q_level=0;
            proc_set[i].q_remaining_ms=MLFQ_LEVEL_RUNTIME(0);

            /* Student's code ends here. */
            return curr_pid;
        }

    FATAL("proc_alloc: reach the limit of %d processes", MAX_NPROCESS);
}

void proc_free(int pid) {
    /* Student's code goes here (Preemptive Scheduler). */
    ulonglong now = mtime_get();

    /* Print the lifecycle statistics of the terminated process or processes. */
    if (pid != GPID_ALL) {
        for (uint i = 0; i < MAX_NPROCESS; i++){
            if (proc_set[i].pid == pid && proc_set[i].status != PROC_UNUSED){
                proc_set[i].turnaround_ms=now-proc_set[i].created_ms;
                proc_set[i].response_ms=(proc_set[i].firsttime_ms==(ulonglong)-1)? 0:proc_set[i].firsttime_ms-proc_set[i].created_ms;
printf("PID %d: turnaround=%llu ms, response=%llu ms, "
               "cpu=%llu ms, timer_intr=%llu\n",
               proc_set[i].pid,
                    (unsigned long long)proc_set[i].turnaround_ms,
(unsigned long long)proc_set[i].response_ms,
               (unsigned long long)proc_set[i].cputime_ms,
               (unsigned long long)proc_set[i].timer_intrpt_count);


                break;
            }
        }

        earth->mmu_free(pid);
        proc_set_status(pid, PROC_UNUSED);
    } else {
        /* Free all user processes. */
        for (uint i = 0; i < MAX_NPROCESS; i++){
            if (proc_set[i].pid >= GPID_USER_START &&
                proc_set[i].status != PROC_UNUSED) {
                proc_set[i].turnaround_ms = now - proc_set[i].created_ms;
                proc_set[i].response_ms = (proc_set[i].firsttime_ms == (ulonglong)-1) ? 0 : proc_set[i].firsttime_ms - proc_set[i].created_ms;
                printf("PID %d: turnaround=%llu ms, response=%llu ms, "
                       "cpu=%llu ms, timer_intr=%llu\n",
                       proc_set[i].pid,
                       (unsigned long long)proc_set[i].turnaround_ms,
                       (unsigned long long)proc_set[i].response_ms,
                       (unsigned long long)proc_set[i].cputime_ms,
                       (unsigned long long)proc_set[i].timer_intrpt_count);

                earth->mmu_free(proc_set[i].pid);
                proc_set[i].status = PROC_UNUSED;
            }
        }
    }
    /* Student's code ends here. */
}

void mlfq_update_level(struct process* p, ulonglong runtime) {
    /* Student's code goes here (Preemptive Scheduler). */
    /* Update only MLFQ quantum bookkeeping. CPU-time accounting is
     * performed in account_runtime() in kernel.c. runtime is in
     * microseconds/mticks. */
    if (runtime >= p->q_remaining_ms) {
        if (p->q_level < MLFQ_NLEVELS - 1) {
            p->q_level++;
            p->q_remaining_ms = MLFQ_LEVEL_RUNTIME(p->q_level);
        } else {
            /* at lowest priority, refill its quantum */
            p->q_remaining_ms = MLFQ_LEVEL_RUNTIME(p->q_level);
        }
    } else {
        p->q_remaining_ms -= runtime;
    }
    /* Student's code ends here. */
}

void mlfq_reset_level() {
    /* Student's code goes here (Preemptive Scheduler). */
    //the below code puts shell at level 0 if user trying to type/input in terminal/shell
    if (!earth->tty_input_empty()) {
        for (uint i = 0; i < MAX_NPROCESS; i++){
        if (proc_set[i].pid == GPID_SHELL && proc_set[i].status != PROC_UNUSED){
            proc_set[i].q_level=0;
            proc_set[i].q_remaining_ms=MLFQ_LEVEL_RUNTIME(0);
            break;
        }
        }

        /* Reset the level of GPID_SHELL if there is pending keyboard input. */
    }

    //printf("reset");
    static ulonglong MLFQ_last_reset_time = 0;
    /* Reset the level of all processes every MLFQ_RESET_PERIOD microseconds. */
    ulonglong now = mtime_get();
    if(MLFQ_last_reset_time==0){
        MLFQ_last_reset_time=now; // because we want initial time, else it will reset in the start as now >>> 0, and therefore now-0 > Reset period.
    }
    if(now - MLFQ_last_reset_time >=MLFQ_RESET_PERIOD){
        for(uint i=0;i<MAX_NPROCESS;i++){
            if (proc_set[i].status != PROC_UNUSED) {
                proc_set[i].q_level = 0;
                proc_set[i].q_remaining_ms = MLFQ_LEVEL_RUNTIME(0);
            }
        }
        /* update last reset time to avoid repeated resets */
        MLFQ_last_reset_time = now;
    }
    /* Student's code ends here. */
}

void proc_sleep(int pid, uint usec) {
    /* Student's code goes here (System Call & Protection). */

    /* Update the sleep-related fields in the struct process for process pid. */

    /* Student's code ends here. */
}

void proc_coresinfo() {
    /* Student's code goes here (Multicore & Locks). */

    /* Print out the pid of the process running on each CPU core. */

    /* Student's code ends here. */
}