## Author

**Arul Thiruvasagam**
- GitHub: [@sulpboy](https://github.com/sulpboy)
- LinkedIn: [Arul Thiruvasagam](https://www.linkedin.com/in/arul-thiruvasagam-amirthaganesan-0b57b5388/)
- Email: arul24115@iiitd.ac.in
- Number: +91 9894196160
- 
# SimpleOS: MLFQ Implementation
> A custom Multi-Level Feedback Queue scheduler and Unix-like shell tools built natively for the RISC-V architecture.

This repository contains a modified version of the EGOS-2000 teaching operating system. The project focuses on bridging the gap between hardware-specific abstractions and user-facing applications by implementing a preemptive CPU scheduler in kernel space, alongside low-level system utilities in user space.

---

## Architecture & Features

### Preemptive MLFQ Scheduler (Kernel Space)
The default scheduler was entirely rewritten to implement a 5-level Multi-Level Feedback Queue (MLFQ). The implementation prevents starvation and dynamically adjusts process priorities based on CPU consumption, enforcing strict preemption via timer interrupts (`mtime_get()`).

### Custom Shell Utilities (User Space)
Implemented POSIX-like utilities (`grep`, `wcl`) entirely from scratch. These utilities bypass high-level C libraries and interact directly with the OS's internal inode and block-reading system calls to process files natively.

---

## Core Technical Contributions

My custom implementation spans across the hardware-specific Earth layer, the hardware-independent Grass layer, and User Space applications. The core changes can be found in these files:

- `grass/process.h` - Extended process PCB to track lifecycle stats (`cputime_ms`, `turnaround_ms`, etc.) and MLFQ queue states.
- `grass/process.c` - Implemented dynamic queue bookkeeping (`mlfq_update_level`, `mlfq_reset_level`) and process allocations.
- `grass/kernel.c` - Modified the main preemptive scheduler loop in `proc_yield` to enforce MLFQ priority rules and intercept timer interrupts.
- `apps/user/grep.c` - Written from scratch to read files via the disk inode layer and pattern match lines.
- `apps/user/wcl.c` - Written from scratch to correctly iterate over files and aggregate newline counts.

---

## Technical Specifications

- **Languages**: C, RISC-V Assembly
- **Core Abstractions**: Process Control Blocks (PCBs), Multi-Level Feedback Queues, Inode File Systems, Bare-Metal C Pointers, RISC-V Interrupts.
- **Environment**: RISC-V QEMU Emulator, GNU Make, GCC Cross-Compiler
