#pragma once

struct sbiret {
    long error;
    long value;
};

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

void *memset(void *buf, char c, size_t n);
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid);
void kernel_main(void);