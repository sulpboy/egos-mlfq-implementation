/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: kernel ≈ 2 handlers
 *   intr_entry() handles timer and device interrupts.
 *   excp_entry() handles system calls and faults (e.g., invalid memory access).
 */

#include "process.h"
#include <string.h>
static ulonglong yield_count = 0;
static ulonglong intr_log_count = 0;
static ulonglong yield_log_count = 0;
#define DEBUG_INTR 1
#define DEBUG_YIELD 0
static int interrupt_counter = 0;
static const int PRINT_INTERVAL = 1000;
uint core_in_kernel;
uint core_to_proc_idx[NCORES];
struct process proc_set[MAX_NPROCESS + 1];
/* proc_set[0] is a place holder for idle cores. */
#define MLFQ_NLEVELS 4
#define curr_proc_idx core_to_proc_idx[core_in_kernel]
#define curr_pid      proc_set[curr_proc_idx].pid
#define curr_status   proc_set[curr_proc_idx].status
#define curr_saved    proc_set[curr_proc_idx].saved_registers

static void intr_entry(uint);
static void excp_entry(uint);

static const char* proc_status_str(enum proc_status s) {
    switch (s) {
    case PROC_UNUSED:          return "UNUSED";
    case PROC_LOADING:         return "LOADING";
    case PROC_READY:           return "READY";
    case PROC_RUNNABLE:        return "RUNNABLE";
    case PROC_RUNNING:         return "RUNNING";
    case PROC_PENDING_SYSCALL: return "PENDING_SYSCALL";
    default:                   return "UNKNOWN";
    }
}

//function to increment cputime, and update mlfq level check
static void account_runtime(void) {
    struct process* p = &proc_set[curr_proc_idx];

    // skip idle / unused
    if (p->pid == 0 || p->status != PROC_RUNNING)
        return;

    ulonglong now  = mtime_get();
    ulonglong dt   = now - p->last_start_ms;

    p->cputime_ms += dt;          // still in microseconds; name is just a name
    mlfq_update_level(p, dt);     // use dt for queue bookkeeping too
}

void kernel_entry() {
    /* With the kernel lock, only one core can enter this point at any time. */
    asm("csrr %0, mhartid" : "=r"(core_in_kernel));

    /* Defensive: ensure core_to_proc_idx is initialized for this core.
     * If not, fall back to the first kernel process slot (1). */
    if (core_to_proc_idx[core_in_kernel] == 0)
        core_to_proc_idx[core_in_kernel] = 1;

    /* Save the process context. */
    asm("csrr %0, mepc" : "=r"(proc_set[curr_proc_idx].mepc));
    memcpy(curr_saved, SAVED_REGISTER_ADDR, SAVED_REGISTER_SIZE);

    uint mcause;
    asm("csrr %0, mcause" : "=r"(mcause));
    (mcause & (1 << 31)) ? intr_entry(mcause & 0x3FF) : excp_entry(mcause);

    /* Restore the process context. */
    asm("csrw mepc, %0" ::"r"(proc_set[curr_proc_idx].mepc));
    memcpy(SAVED_REGISTER_ADDR, curr_saved, SAVED_REGISTER_SIZE);
}

#define INTR_ID_TIMER   7
#define EXCP_ID_ECALL_U 8
#define EXCP_ID_ECALL_M 11
static void proc_yield();
static void proc_try_syscall(struct process* proc);

static void excp_entry(uint id) {
    account_runtime(); 
    if (id >= EXCP_ID_ECALL_U && id <= EXCP_ID_ECALL_M) {
        /* Copy the system call arguments from user space to the kernel. */
        uint syscall_paddr = earth->mmu_translate(curr_pid, SYSCALL_ARG);
        if (!syscall_paddr) {
            INFO("WARN: excp_entry mmu_translate failed pid=%d idx=%d",
                 curr_pid, curr_proc_idx);
            /* Gracefully handle bad user pointer: mark pending and yield. */
            proc_set_pending(curr_pid);
            proc_yield();
            return;
        }
        memcpy(&proc_set[curr_proc_idx].syscall, (void*)syscall_paddr,
               sizeof(struct syscall));
        proc_set[curr_proc_idx].syscall.status = PENDING;

        proc_set_pending(curr_pid);
        proc_set[curr_proc_idx].mepc += 4;
        proc_try_syscall(&proc_set[curr_proc_idx]);
        proc_yield();
        return;
    }
    /* Student's code goes here (System Call & Protection | Virtual Memory). */

    /* Kill the current process if curr_pid is a user application. */

    /* Student's code ends here. */
    FATAL("excp_entry: kernel got exception %d", id);
}



static void intr_entry(uint id) {
    if (id != INTR_ID_TIMER) FATAL("excp_entry: kernel got interrupt %d", id);
    /* Student's code goes here (Preemptive Scheduler). */
    account_runtime(); 
    /* Update the process lifecycle statistics. */
    /* Increment timer interrupt count for the currently running user
     * process (account_runtime updated cputime_ms). */
    struct process* p = &proc_set[curr_proc_idx];
    /*
    if (p->pid!=4){
        printf("%d",p->pid);
    }
    */
//increasing timer interrupt count
    if (p->pid != 0 && p->status == PROC_RUNNING) {
        p->timer_intrpt_count++;
    }
    /* Student's code ends here. */
    proc_yield();
}

static void proc_yield() {


    /*INFO("yield #%llu: core=%u, curr_idx=%u, pid=%d, status=%s",
         (unsigned long long)yield_count,
         core_in_kernel,
         curr_proc_idx,
         curr_pid,
         proc_status_str(curr_status));
    */
    /* Ensure this core has a valid current process index. If not,
     * initialize it to 1 (first user slot) to avoid selecting the
     * idle slot at index 0. */
    if (core_to_proc_idx[core_in_kernel] == 0)
        core_to_proc_idx[core_in_kernel] = 1;

    if (curr_status == PROC_RUNNING) {
        struct process* p = &proc_set[curr_proc_idx];
        ulonglong now = mtime_get();
        proc_set_runnable(curr_pid);
    }
    /* Student's code goes here (Multiple Projects). */

    /* [Preemptive Scheduler]
     * Measure and record lifecycle statistics for the *current* process.
     * Modify the loop below to find the next process to schedule with MLFQ.
     * [System Call & Protection]
     * Do not schedule a process that should still be sleeping at this time. */
    //reset mlfq level, and select next process according to lowest level
    mlfq_reset_level();
    int next_idx = -1;
    int best_level = MLFQ_NLEVELS + 1;
    /* Iterate over process slots 1..MAX_NPROCESS (exclude 0 which is idle).
     * Use a 1-based wrap so we never select index 0. */
    for (uint offset = 1; offset <= MAX_NPROCESS; offset++) {
        uint idx = ((curr_proc_idx - 1 + offset) % MAX_NPROCESS) + 1; /* 1..MAX_NPROCESS */
        struct process* p = &proc_set[idx];

        if (p->status == PROC_PENDING_SYSCALL) proc_try_syscall(p);
        //select based on lowest level
        if (p->status == PROC_READY || p->status == PROC_RUNNABLE) {
            if (p->q_level < best_level) {
                best_level = p->q_level;
                next_idx   = idx;
            }
        }
    }

    if (next_idx >= 1 && next_idx <= MAX_NPROCESS) {
        /* [Preemptive Scheduler]
         * Measure and record lifecycle statistics for the *next* process.
        //just measuring lifecycle stats


         * [System Call & Protection | Multicore & Locks]
         * Modify mstatus.MPP to enter machine or user mode after mret. */
        struct process* p = &proc_set[next_idx];
        ulonglong now = mtime_get();
        p->last_start_ms=now;
        if(p->firsttime_ms==(ulonglong)-1){
            p->firsttime_ms=now;
        }




    } else {
        /* [Multicore & Locks]
         * Release the kernel lock.
         * [Multicore & Locks | System Call & Protection]
         * Set curr_proc_idx to 0; Reset the timer;
         * Enable interrupts by setting the mstatus.MIE bit to 1;
         * Wait for the next interrupt using the wfi instruction. */

        FATAL("proc_yield: no process to run on core %d", core_in_kernel);
    }
    /* Student's code ends here. */
    /*if((curr_proc_idx!=next_idx )&&((next_idx==6)||(curr_proc_idx==5))){
        printf("what");
    }*/
    curr_proc_idx = next_idx;

    earth->mmu_switch(curr_pid);
    earth->mmu_flush_cache();
    if (curr_status == PROC_READY) {
        /* Setup argc, argv and program counter for a newly created process. */
        curr_saved[0]                = APPS_ARG;
        curr_saved[1]                = APPS_ARG + 4;
        proc_set[curr_proc_idx].mepc = APPS_ENTRY;
    }
    proc_set_running(curr_pid);
    earth->timer_reset(core_in_kernel);
}

static void proc_try_send(struct process* sender) {
    for (uint i = 0; i < MAX_NPROCESS; i++) {
        struct process* dst = &proc_set[i];
        if (dst->pid == sender->syscall.receiver &&
            dst->status != PROC_UNUSED) {
            /* Return if dst is not receiving or not taking msg from sender. */
            if (!(dst->syscall.type == SYS_RECV &&
                  dst->syscall.status == PENDING))
                return;
            if (!(dst->syscall.sender == GPID_ALL ||
                  dst->syscall.sender == sender->pid))
                return;

            dst->syscall.status = DONE;
            dst->syscall.sender = sender->pid;
            /* Copy the system call arguments within the kernel PCB. */
            memcpy(dst->syscall.content, sender->syscall.content,
                   SYSCALL_MSG_LEN);
            return;
        }
    }
    FATAL("proc_try_send: unknown receiver pid=%d", sender->syscall.receiver);
}

static void proc_try_recv(struct process* receiver) {
    if (receiver->syscall.status == PENDING) return;

    /* Copy the system call struct from the kernel back to user space. */
    uint syscall_paddr = earth->mmu_translate(receiver->pid, SYSCALL_ARG);
    if (!syscall_paddr) {
        INFO("WARN: proc_try_recv mmu_translate failed receiver.pid=%d", receiver->pid);
        return;
    }
    memcpy((void*)syscall_paddr, &receiver->syscall, sizeof(struct syscall));

    /* Set the receiver and sender back to RUNNABLE. */
    proc_set_runnable(receiver->pid);
    proc_set_runnable(receiver->syscall.sender);
}

static void proc_try_syscall(struct process* proc) {
    switch (proc->syscall.type) {
    case SYS_RECV:
        proc_try_recv(proc);
        break;
    case SYS_SEND:
        proc_try_send(proc);
        break;
    default:
        FATAL("proc_try_syscall: unknown syscall type=%d", proc->syscall.type);
    }
}