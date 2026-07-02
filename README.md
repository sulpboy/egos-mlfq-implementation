<div align="center">
  <h1>⚙️ SimpleOS</h1>
  <p><b>A lightweight OS featuring a Multi-Level Feedback Queue (MLFQ) scheduler and custom shell utilities</b></p>
</div>

<br/>

## 📖 Overview

**SimpleOS** is an enhanced version of the [EGOS-2000](https://egos.fun/) teaching operating system, featuring a highly robust preemptive scheduler and custom user-space applications. This project bridges the gap between hardware-specific abstractions and user-facing applications by introducing intelligent process management and native shell commands.

Built for the RISC-V architecture, this OS is capable of running natively on QEMU and demonstrates deep concepts in kernel programming, memory translation, context switching, and I/O file operations.

## ✨ Key Features

### 1. Preemptive MLFQ Scheduler (Kernel Space)
A fully functioning **Multi-Level Feedback Queue** integrated directly into the `grass` layer of the OS, designed to optimize CPU responsiveness and throughput. 

- **5 Priority Levels:** Processes dynamically migrate across 5 distinct levels based on their CPU burst times.
- **Fair Resource Allocation:** CPU-intensive tasks drop in priority, ensuring that I/O bound or interactive processes (like the OS shell) maintain maximum responsiveness.
- **Dynamic Starvation Prevention:** Implements a global priority reset every 10 seconds (or immediately on keyboard interrupts) to prevent low-priority processes from starving.
- **Detailed Lifecycle Tracking:** Tracks precise CPU time, turnaround time, response time, and timer interrupts for every process, generating a full lifecycle report on process termination.

### 2. Custom Shell Utilities (User Space)
Implemented POSIX-like command-line utilities using low-level EGOS system calls, avoiding standard C library abstractions (like `fopen` or `fgets`) to interface directly with the disk and inode layer.

- `grep`: Searches for substring patterns across files. It utilizes block-by-block file reading (`file_read`) and inode tracking (`dir_lookup`) to scan files larger than the block size.
- `wcl`: A lightweight line-counting utility (equivalent to bash's `wc -l`). It seamlessly supports reading multiple files sequentially and aggregating the total line count accurately. 

## 🛠️ Tech Stack & Architecture

- **Language:** C, RISC-V Assembly
- **Architecture:** 3-Layer Design (Earth: Hardware, Grass: Process/Memory, Apps: User Space)
- **Environment:** RISC-V QEMU Emulator, GNU Make, GCC Cross-Compiler

### 📂 Files Coded for this Assignment
My custom implementation spans across the hardware-specific Earth layer, the hardware-independent Grass layer, and User Space applications. The core changes can be found in these files:
- `grass/process.h` - Extended process PCB to track lifecycle stats (`cputime_ms`, `turnaround_ms`, etc.) and MLFQ queue states.
- `grass/process.c` - Implemented dynamic queue bookkeeping (`mlfq_update_level`, `mlfq_reset_level`) and process allocations (`proc_alloc`, `proc_free`).
- `grass/kernel.c` - Modified the main preemptive scheduler loop in `proc_yield` to enforce strict MLFQ priority rules and intercept timer interrupts in `intr_entry`.
- `apps/user/grep.c` - Written from scratch to read files via the disk inode layer and pattern match lines.
- `apps/user/wcl.c` - Written from scratch to correctly iterate over files and aggregate newline counts.

## 🚀 Quick Start

### Prerequisites
You need a Unix-based environment (Linux or WSL) and the RISC-V GNU compiler toolchain. 

### Building and Running
1. Clone the repository and compile the OS:
   ```bash
   make clean
   make
   ```
2. Boot SimpleOS inside the QEMU emulator:
   ```bash
   make qemu
   ```
3. Once booted, you will be prompted to choose a memory translation mechanism (select `0` for page tables). 
4. You are now inside the shell! Try running:
   ```bash
   > wcl [filename]
   > grep [pattern] [filename]
   ```

## 🧠 What I Learned
Through this project, I gained hands-on experience dealing with raw memory limits, cross-compilation, hardware interrupts, and low-level C programming. Implementing the MLFQ required careful manipulation of process control blocks (PCBs) and context saving/restoring during timer traps, solidifying my understanding of modern OS design.

---
*This project was completed as part of the Operating Systems course (Monsoon 2025). Built on top of the original [EGOS-2000](https://github.com/yhzhang0128/egos-2000/) teaching OS framework.*

> **P.S.** Debugging a custom scheduler operating natively in kernel space with bare-metal C pointers was an absolute, unadulterated horror. 10/10 would not recommend to my worst enemy... but at least it works! 👻🐛
