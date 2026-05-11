#!/bin/bash
set -xue

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "$SCRIPT_DIR/.." && pwd)

QEMU=qemu-system-riscv32

# Path to clang and compiler flags
CC=/usr/bin/clang
CFLAGS="-std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf -fuse-ld=lld -fno-stack-protector -ffreestanding -nostdlib"

# Build the kernel
$CC $CFLAGS -Wl,-T"$ROOT_DIR/src/kernel/kernel.ld" -Wl,-Map=kernel.map -o kernel.elf \
    "$ROOT_DIR/src/kernel/kernel.c" "$ROOT_DIR/src/common/common.c"


# Start QEMU
$QEMU -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
    -kernel kernel.elf
