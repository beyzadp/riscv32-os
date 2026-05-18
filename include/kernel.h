#pragma once

struct sbiret {
    long error;
    long value;
};

#define PROCS_MAX 8 // Maximum number of processes

#define PROC_UNUSED 0   // Unused process control structure
#define PROC_RUNNABLE 1 // Runnable process

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

void kernel_main(void);

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid);

#include "common.h"

struct trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t tp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t sp;
} __attribute__((packed));

#define READ_CSR(reg)                                                          \
    ({                                                                         \
        unsigned long __tmp;                                                   \
        __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));                  \
        __tmp;                                                                 \
    })

#define WRITE_CSR(reg, value)                                                  \
    do {                                                                       \
        uint32_t __tmp = (value);                                              \
        __asm__ __volatile__("csrw " #reg ", %0" ::"r"(__tmp));                \
    } while (0)

struct process {
    int pid;    // Process ID
    int state;  // Process state: PROC_UNUSED or PROC_RUNNABLE
    vaddr_t sp; // Stack pointer
    uint32_t *page_table;
    uint8_t stack[8192]; // Kernel stack
};

#define SAVE_CALLEE_REGS                                                       \
    "sw ra,  0  * 4(sp)\n"                                                     \
    "sw s0,  1  * 4(sp)\n"                                                     \
    "sw s1,  2  * 4(sp)\n"                                                     \
    "sw s2,  3  * 4(sp)\n"                                                     \
    "sw s3,  4  * 4(sp)\n"                                                     \
    "sw s4,  5  * 4(sp)\n"                                                     \
    "sw s5,  6  * 4(sp)\n"                                                     \
    "sw s6,  7  * 4(sp)\n"                                                     \
    "sw s7,  8  * 4(sp)\n"                                                     \
    "sw s8,  9  * 4(sp)\n"                                                     \
    "sw s9,  10 * 4(sp)\n"                                                     \
    "sw s10, 11 * 4(sp)\n"                                                     \
    "sw s11, 12 * 4(sp)\n"

#define RESTORE_CALLEE_REGS                                                    \
    "lw ra,  0  * 4(sp)\n"                                                     \
    "lw s0,  1  * 4(sp)\n"                                                     \
    "lw s1,  2  * 4(sp)\n"                                                     \
    "lw s2,  3  * 4(sp)\n"                                                     \
    "lw s3,  4  * 4(sp)\n"                                                     \
    "lw s4,  5  * 4(sp)\n"                                                     \
    "lw s5,  6  * 4(sp)\n"                                                     \
    "lw s6,  7  * 4(sp)\n"                                                     \
    "lw s7,  8  * 4(sp)\n"                                                     \
    "lw s8,  9  * 4(sp)\n"                                                     \
    "lw s9,  10 * 4(sp)\n"                                                     \
    "lw s10, 11 * 4(sp)\n"                                                     \
    "lw s11, 12 * 4(sp)\n"

#define SAVE_CALLER_REGS                                                       \
    "sw ra,  0*4(sp)\n"                                                        \
    "sw t0,  1*4(sp)\n"                                                        \
    "sw t1,  2*4(sp)\n"                                                        \
    "sw t2,  3*4(sp)\n"                                                        \
    "sw t3,  4*4(sp)\n"                                                        \
    "sw t4,  5*4(sp)\n"                                                        \
    "sw t5,  6*4(sp)\n"                                                        \
    "sw t6,  7*4(sp)\n"                                                        \
    "sw a0,  8*4(sp)\n"                                                        \
    "sw a1,  9*4(sp)\n"                                                        \
    "sw a2,  10*4(sp)\n"                                                       \
    "sw a3,  11*4(sp)\n"                                                       \
    "sw a4,  12*4(sp)\n"                                                       \
    "sw a5,  13*4(sp)\n"                                                       \
    "sw a6,  14*4(sp)\n"                                                       \
    "sw a7,  15*4(sp)\n"

#define RESTORE_CALLER_REGS                                                    \
    "lw ra,  0*4(sp)\n"                                                        \
    "lw t0,  1*4(sp)\n"                                                        \
    "lw t1,  2*4(sp)\n"                                                        \
    "lw t2,  3*4(sp)\n"                                                        \
    "lw t3,  4*4(sp)\n"                                                        \
    "lw t4,  5*4(sp)\n"                                                        \
    "lw t5,  6*4(sp)\n"                                                        \
    "lw t6,  7*4(sp)\n"                                                        \
    "lw a0,  8*4(sp)\n"                                                        \
    "lw a1,  9*4(sp)\n"                                                        \
    "lw a2,  10*4(sp)\n"                                                       \
    "lw a3,  11*4(sp)\n"                                                       \
    "lw a4,  12*4(sp)\n"                                                       \
    "lw a5,  13*4(sp)\n"                                                       \
    "lw a6,  14*4(sp)\n"                                                       \
    "lw a7,  15*4(sp)\n"

#define SAVE_ALL_REGS                                                          \
    SAVE_CALLER_REGS                                                           \
    "sw s0,  16*4(sp)\n"                                                       \
    "sw s1,  17*4(sp)\n"                                                       \
    "sw s2,  18*4(sp)\n"                                                       \
    "sw s3,  19*4(sp)\n"                                                       \
    "sw s4,  20*4(sp)\n"                                                       \
    "sw s5,  21*4(sp)\n"                                                       \
    "sw s6,  22*4(sp)\n"                                                       \
    "sw s7,  23*4(sp)\n"                                                       \
    "sw s8,  24*4(sp)\n"                                                       \
    "sw s9,  25*4(sp)\n"                                                       \
    "sw s10, 26*4(sp)\n"                                                       \
    "sw s11, 27*4(sp)\n"

#define RESTORE_ALL_REGS                                                       \
    RESTORE_CALLER_REGS                                                        \
    "lw s0,  16*4(sp)\n"                                                       \
    "lw s1,  17*4(sp)\n"                                                       \
    "lw s2,  18*4(sp)\n"                                                       \
    "lw s3,  19*4(sp)\n"                                                       \
    "lw s4,  20*4(sp)\n"                                                       \
    "lw s5,  21*4(sp)\n"                                                       \
    "lw s6,  22*4(sp)\n"                                                       \
    "lw s7,  23*4(sp)\n"                                                       \
    "lw s8,  24*4(sp)\n"                                                       \
    "lw s9,  25*4(sp)\n"                                                       \
    "lw s10, 26*4(sp)\n"                                                       \
    "lw s11, 27*4(sp)\n"

#define SATP_SV32 (1u << 31)
#define PAGE_V (1 << 0) // "Valid" bit (entry is enabled)
#define PAGE_R (1 << 1) // Readable
#define PAGE_W (1 << 2) // Writable
#define PAGE_X (1 << 3) // Executable
#define PAGE_U (1 << 4) // User (accessible in user mode)