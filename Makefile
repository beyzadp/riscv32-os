# Toolchain variables
CC = /usr/bin/clang
QEMU = qemu-system-riscv32

# Directories
SRC_DIR = src
BUILD_DIR = build

# Compiler flags
CFLAGS = -std=c11 -O2 -g3 -Wall -Wextra --target=riscv32-unknown-elf \
         -fno-stack-protector -ffreestanding -nostdlib -I$(SRC_DIR)/common

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

# QEMU arguments
QEMU_FLAGS = -machine virt -bios default -nographic -serial mon:stdio --no-reboot -kernel $(KERNEL_TARGET)

# Phony targets
.PHONY: all run clean

# Default target
all: $(KERNEL_TARGET) $(USER_TARGET)

# Rule to link the kernel
$(KERNEL_TARGET): $(KERNEL_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(KERNEL_LDFLAGS) -o $@ $^

# Rule to link the user shell
$(USER_TARGET): $(USER_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(USER_LDFLAGS) -o $@ $^

# Rule to compile .c files into .o files inside the build directory
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Target to build and run
run: all
	$(QEMU) $(QEMU_FLAGS)

# Target to clean up all built files
clean:
	rm -rf $(BUILD_DIR)