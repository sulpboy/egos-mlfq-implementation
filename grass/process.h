#pragma once

#include "egos.h"
#include "syscall.h"
//change all to ulonglong
enum proc_status {
    PROC_UNUSED,
    PROC_LOADING,
    PROC_READY,
    PROC_RUNNING,
    PROC_RUNNABLE,
    PROC_PENDING_SYSCALL
};

#define MAX_NPROCESS        16
#define SAVED_REGISTER_NUM  32
#define SAVED_REGISTER_SIZE SAVED_REGISTER_NUM * 4
#define SAVED_REGISTER_ADDR (void*)(EGOS_STACK_TOP - SAVED_REGISTER_SIZE)

struct process {
    int pid;
    struct syscall syscall;
    enum proc_status status;
    uint mepc, saved_registers[SAVED_REGISTER_NUM];
    /* Student's code goes here (Preemptive Scheduler | System Call). */

    ulonglong created_ms;
    ulonglong firsttime_ms;
    ulonglong cputime_ms; //running on cpu time - add into func
    ulonglong timer_intrpt_count; // timer interrupt count - add into func on each timer interrupt
    ulonglong last_start_ms;
    ulonglong response_ms; //firsttime-created to be done in proc free func or smtg idk
    ulonglong turnaround_ms; // turnaround = exitms-createdms to be done in proc free function later

    //mlfq specific variables
    ulonglong  q_level; //queue level where process is at
    ulonglong  q_remaining_ms; //remaining runtime on that queue level for process



    /* Add new fields for lifecycle statistics, MLFQ or process sleep. */

    /* Student's code ends here. */
};

ulonglong mtime_get();

int proc_alloc();
void proc_free(int);
void proc_set_ready(int);
void proc_set_running(int);
void proc_set_runnable(int);
void proc_set_pending(int);

void mlfq_reset_level();
void mlfq_update_level(struct process* p, ulonglong runtime);
void proc_sleep(int pid, uint usec);
void proc_coresinfo();

extern uint core_to_proc_idx[NCORES];
