# Toolchain variables
CC = /usr/bin/clang
HOST_CC = gcc
OBJCOPY = llvm-objcopy
QEMU = qemu-system-riscv32

# Directories
SRC_DIR = src
BUILD_DIR = build

# Compiler flags (cleaned up duplicated include paths)
CFLAGS = -std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf \
         -fno-stack-protector -ffreestanding -nostdlib -Iinclude -I$(SRC_DIR)/common -I$(SRC_DIR)/kernel

# Linker flags for Kernel and User
KERNEL_LDFLAGS = -fuse-ld=lld -Wl,-T$(SRC_DIR)/kernel/kernel.ld -Wl,-Map=$(BUILD_DIR)/kernel.map
USER_LDFLAGS = -fuse-ld=lld -Wl,-T$(SRC_DIR)/user/user.ld -Wl,-Map=$(BUILD_DIR)/user.map

# Source files
KERNEL_SRCS = $(wildcard $(SRC_DIR)/kernel/*.c) $(wildcard $(SRC_DIR)/common/*.c)
USER_SRCS = $(wildcard $(SRC_DIR)/user/*.c) $(wildcard $(SRC_DIR)/common/*.c)

# Object files (patsubst replaces the 'src/' prefix with 'build/' and '.c' with '.o')
KERNEL_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(KERNEL_SRCS))
USER_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(USER_SRCS))

# Output executables
KERNEL_TARGET = $(BUILD_DIR)/kernel.elf
USER_TARGET = $(BUILD_DIR)/shell.elf

# QEMU arguments (Updated to use disk.img)
QEMU_FLAGS = -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
             -d unimp,guest_errors,int,cpu_reset -D qemu.log \
             -drive id=drive0,file=disk.img,format=raw,if=none \
             -device virtio-blk-device,drive=drive0,bus=virtio-mmio-bus.0 \
             -kernel $(KERNEL_TARGET)

# Phony targets
.PHONY: all shell run clean extract

# Default target
all: $(KERNEL_TARGET) $(USER_TARGET)

# Build user shell artifacts
shell: $(BUILD_DIR)/shell.bin.o

# Build the host-side file system generation tool inside the build directory
$(BUILD_DIR)/mkfs: tools/mkfs.c
	@mkdir -p $(dir $@)
	$(HOST_CC) -O2 -Wall -o $@ $<

# Build the host-side file system extraction tool inside the build directory
$(BUILD_DIR)/extractfs: tools/extractfs.c
	@mkdir -p $(dir $@)
	$(HOST_CC) -O2 -Wall -o $@ $<

# Create a 1MB disk image and inject files from the disk/ directory
disk.img: $(BUILD_DIR)/mkfs $(wildcard disk/*)
	@mkdir -p disk
	dd if=/dev/zero of=$@ bs=512 count=2048
	./$(BUILD_DIR)/mkfs $@ disk/

# Target to extract files from the disk image back to the host folder
extract: $(BUILD_DIR)/extractfs
	@mkdir -p disk
	./$(BUILD_DIR)/extractfs disk.img disk/

# Rule to link the kernel
$(KERNEL_TARGET): $(KERNEL_OBJS) $(BUILD_DIR)/shell.bin.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(KERNEL_LDFLAGS) -o $@ $^

# Rule to link the user shell
$(USER_TARGET): $(USER_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(USER_LDFLAGS) -o $@ $^

$(BUILD_DIR)/shell.bin: $(USER_TARGET)
	@mkdir -p $(dir $@)
	$(OBJCOPY) --set-section-flags .bss=alloc,contents -O binary $< $@

$(BUILD_DIR)/shell.bin.o: $(BUILD_DIR)/shell.bin
	@mkdir -p $(dir $@)
	$(OBJCOPY) -Ibinary -Oelf32-littleriscv $< $@

# Rule to compile .c files into .o files inside the build directory
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Target to build and run (depends on disk.img)
run: all disk.img
	$(QEMU) $(QEMU_FLAGS)

# Target to clean up all built files and the disk image
clean:
	rm -rf $(BUILD_DIR) disk.img