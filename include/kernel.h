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

#define SATP_SV32 (1u << 31)
#define PAGE_V (1 << 0) // "Valid" bit (entry is enabled)
#define PAGE_R (1 << 1) // Readable
#define PAGE_W (1 << 2) // Writable
#define PAGE_X (1 << 3) // Executable
#define PAGE_U (1 << 4) // User (accessible in user mode)

#define SSTATUS_SPIE (1 << 5)
#define SCAUSE_ECALL 8
#define PROC_EXITED 2

#define SECTOR_SIZE 512
#define VIRTQ_ENTRY_NUM 16
#define VIRTIO_DEVICE_BLK 2
#define VIRTIO_BLK_PADDR 0x10001000
#define VIRTIO_REG_MAGIC 0x00
#define VIRTIO_REG_VERSION 0x04
#define VIRTIO_REG_DEVICE_ID 0x08
#define VIRTIO_REG_PAGE_SIZE 0x28
#define VIRTIO_REG_QUEUE_SEL 0x30
#define VIRTIO_REG_QUEUE_NUM_MAX 0x34
#define VIRTIO_REG_QUEUE_NUM 0x38
#define VIRTIO_REG_QUEUE_PFN 0x40
#define VIRTIO_REG_QUEUE_READY 0x44
#define VIRTIO_REG_QUEUE_NOTIFY 0x50
#define VIRTIO_REG_DEVICE_STATUS 0x70
#define VIRTIO_REG_DEVICE_CONFIG 0x100
#define VIRTIO_STATUS_ACK 1
#define VIRTIO_STATUS_DRIVER 2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTQ_DESC_F_NEXT 1
#define VIRTQ_DESC_F_WRITE 2
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1

// Virtqueue Descriptor Table entry.
struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

// Virtqueue Available Ring.
struct virtq_avail {
    uint16_t flags;
    uint16_t index;
    uint16_t ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

// Virtqueue Used Ring entry.
struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

// Virtqueue Used Ring.
struct virtq_used {
    uint16_t flags;
    uint16_t index;
    struct virtq_used_elem ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

// Virtqueue.
struct virtio_virtq {
    struct virtq_desc descs[VIRTQ_ENTRY_NUM];
    struct virtq_avail avail;
    struct virtq_used used __attribute__((aligned(PAGE_SIZE)));
    int queue_index;
    volatile uint16_t *used_index;
    uint16_t last_used_index;
} __attribute__((packed));

// Virtio-blk request.
struct virtio_blk_req {
    // First descriptor: read-only from the device
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;

    // Second descriptor: writable by the device if it's a read operation
    // (VIRTQ_DESC_F_WRITE)
    uint8_t data[512];

    // Third descriptor: writable by the device (VIRTQ_DESC_F_WRITE)
    uint8_t status;
} __attribute__((packed));

// --- Custom File System Definitions ---

#define POINTER_SIZE 4
#define DATA_SIZE (SECTOR_SIZE - POINTER_SIZE) // 508 bytes

#define END_OF_FILE 0xFFFFFFFF
#define FREE_BLOCK 0xFFFFFFFE

#define SUPERBLOCK_IDX 0
#define DIR_START_IDX 1
#define DIR_BLOCKS 4
#define DATA_START_IDX 5

struct superblock {
    char magic[8]; // "MYFS\0\0\0\0"
    uint32_t total_blocks;
    uint32_t dir_blocks;
    uint32_t data_start_block;
    uint8_t padding[492];
} __attribute__((packed));

struct dir_entry {
    uint8_t in_use;
    char name[19];
    uint32_t size;
    uint32_t start_block;
    uint32_t reserved;
} __attribute__((packed));

struct data_block {
    uint8_t data[DATA_SIZE];
    uint32_t next_block;
} __attribute__((packed));

// --- SBI / Console I/O ---
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid);
void putchar(char ch);
long getchar(void);

// --- Trap and Syscall Handling ---
void handle_trap(struct trap_frame *f);
__attribute__((naked)) __attribute__((aligned(4))) void kernel_entry(void);
void handle_syscall(struct trap_frame *f);

// --- Memory Management ---
paddr_t alloc_pages(uint32_t n);
void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags);

// --- Process Management ---
__attribute__((naked)) void switch_context(uint32_t *prev_sp,
                                           uint32_t *next_sp);
__attribute__((naked)) void user_entry(void);
struct process *create_process(const void *image, size_t image_size);
void yield(void);
void delay(void);
void proc_a_entry(void);
void proc_b_entry(void);

// --- virtio-blk Driver ---
uint32_t virtio_reg_read32(unsigned offset);
uint64_t virtio_reg_read64(unsigned offset);
void virtio_reg_write32(unsigned offset, uint32_t value);
void virtio_reg_fetch_and_or32(unsigned offset, uint32_t value);
struct virtio_virtq *virtq_init(unsigned index);
void virtio_blk_init(void);
void virtq_kick(struct virtio_virtq *vq, int desc_index);
bool virtq_is_busy(struct virtio_virtq *vq);
void read_write_disk(void *buf, unsigned sector, int is_write);

// --- Custom File System (MYFS) ---
void fs_format(void);
void fs_init(void);
void fs_list_files(void);
int fs_read_file(const char *filename, char *buf, int max_len);
int fs_write_file(const char *filename, const char *data, int len);

// --- Entry Points ---
void kernel_main(void);
__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void);
