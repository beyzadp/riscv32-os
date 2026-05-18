#include "kernel.h"
#include "common.h"

extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];
extern char __kernel_base[];

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
                         : "=r"(a0), "=r"(a1)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}

#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        printf("PANIC: %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);  \
        while (1) {                                                            \
        }                                                                      \
    } while (0)

void handle_trap(struct trap_frame *f) {
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    PANIC("unexpected trap scause=%x, stval=%x, sepc=%x\n", scause, stval,
          user_pc);
}

__attribute__((naked)) __attribute__((aligned(4))) void kernel_entry(void) {
    __asm__ __volatile__(
        "csrrw sp, sscratch, sp\n"
        "addi sp, sp, -4 * 31\n" // Allocate space for the trap_frame struct
        SAVE_ALL_REGS

        "csrr a0, sscratch\n"
        "sw a0, 4 * 30(sp)\n"

        "mv a0, sp\n"
        "call handle_trap\n"

        RESTORE_ALL_REGS "lw sp,  4 * 30(sp)\n"
        "sret\n");
}

paddr_t alloc_pages(uint32_t n) {
    static paddr_t next_paddr = (paddr_t)__free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t)__free_ram_end)
        PANIC("out of memory");

    memset((void *)paddr, 0, n * PAGE_SIZE);
    return paddr;
}

__attribute__((naked)) void switch_context(uint32_t *prev_sp,
                                           uint32_t *next_sp) {
    __asm__ __volatile__(
        // Save callee-saved registers onto the current process's stack.
        "addi sp, sp, -13 * 4\n" // Allocate stack space for 13 4-byte registers

        SAVE_CALLEE_REGS

        // Switch the stack pointer.
        "sw sp, (a0)\n" // *prev_sp = sp;
        "lw sp, (a1)\n" // Switch stack pointer (sp) here

        // Restore callee-saved registers from the next process's stack.
        RESTORE_CALLEE_REGS "addi sp, sp, 13 * 4\n" // We've popped 13 4-byte
                                                    // registers from the stack
        "ret\n");
}

struct process procs[PROCS_MAX]; // All process control structures.

void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags) {
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);

    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr);

    uint32_t vpn1 = (vaddr >> 22) & 0x3ff;
    if ((table1[vpn1] & PAGE_V) == 0) {
        // Create the 1st level page table if it doesn't exist.
        uint32_t pt_paddr = alloc_pages(1);
        table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10) | PAGE_V;
    }

    // Set the 2nd level page table entry to map the physical page.
    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
    uint32_t *table0 = (uint32_t *)((table1[vpn1] >> 10) * PAGE_SIZE);
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V;
}

struct process *create_process(uint32_t pc) {
    // Find an unused process control structure.
    struct process *proc = NULL;
    int i;
    for (i = 0; i < PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }

    if (!proc)
        PANIC("no free process slots");

    // Stack callee-saved registers. These register values will be restored in
    // the first context switch in switch_context.
    uint32_t *sp = (uint32_t *)&proc->stack[sizeof(proc->stack)];
    *--sp = 0;            // s11
    *--sp = 0;            // s10
    *--sp = 0;            // s9
    *--sp = 0;            // s8
    *--sp = 0;            // s7
    *--sp = 0;            // s6
    *--sp = 0;            // s5
    *--sp = 0;            // s4
    *--sp = 0;            // s3
    *--sp = 0;            // s2
    *--sp = 0;            // s1
    *--sp = 0;            // s0
    *--sp = (uint32_t)pc; // ra

    uint32_t *page_table = (uint32_t *)alloc_pages(1);
    for (paddr_t paddr = (paddr_t)__kernel_base;
         paddr < (paddr_t)__free_ram_end; paddr += PAGE_SIZE)
        map_page(page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X);

    // Initialize fields.
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t)sp;
    proc->page_table = page_table;
    return proc;
}

void delay(void) {
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop"); // do nothing
}

struct process *current_proc; // Currently running process
struct process *idle_proc;    // Idle process

void yield(void) {
    // Search for a runnable process
    struct process *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    // If there's no runnable process other than the current one, return and
    // continue processing
    if (next == current_proc)
        return;

    // Context switch
    struct process *prev = current_proc;
    current_proc = next;

    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n"
        "csrw sscratch, %[sscratch]\n"
        :
        // Don't forget the trailing comma!
        : [satp] "r"(SATP_SV32 | ((uint32_t)next->page_table / PAGE_SIZE)),
          [sscratch] "r"((uint32_t)&next->stack[sizeof(next->stack)]));

    switch_context(&prev->sp, &next->sp);
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        yield();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        yield();
    }
}

void kernel_main(void) {
    memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

    WRITE_CSR(stvec, (uint32_t)kernel_entry);
    //  __asm__ __volatile__("unimp");

    printf("Hello, world!\n");
    idle_proc = create_process((uint32_t)NULL);
    idle_proc->pid = 0; // idle
    current_proc = idle_proc;

    proc_a = create_process((uint32_t)proc_a_entry);
    proc_b = create_process((uint32_t)proc_b_entry);

    yield();
    PANIC("switched to idle process");
    // printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

    // paddr_t paddr0 = alloc_pages(2);
    // paddr_t paddr1 = alloc_pages(1);
    // printf("alloc_pages test: paddr0=%x\n", paddr0);
    // printf("alloc_pages test: paddr1=%x\n", paddr1);

    // PANIC("Something went wrong: %d", -1);
    // printf("This will never be printed\n");

    // proc_a = create_process((uint32_t)proc_a_entry);
    // proc_b = create_process((uint32_t)proc_b_entry);
    // proc_a_entry();

    // uint32_t ra_val;
    //  Ask the compiler to move the current value of the 'ra' register into our
    //  C variable
    //__asm__ __volatile__("mv %0, ra" : "=r"(ra_val));

    // printf("About to return! The RA register holds: 0x%x\n", ra_val);

    // for (;;) {
    //    __asm__ __volatile__("wfi"); // Wait For Interrupt (saves CPU power)
    //}
}
__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n" // Set the stack pointer
        "j kernel_main\n"       // Jump to the kernel main function
        :
        : [stack_top] "r"(
            __stack_top) // Pass the stack top address as %[stack_top]
    );
}