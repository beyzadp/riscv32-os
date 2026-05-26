├── disk/     - File system contents
├── common.c  - Kernel/user common library: printf, memset, ...
├── common.h  - Kernel/user common library: definitions of structs and constants
├── kernel.c  - Kernel: process management, system calls, device drivers, file system
├── kernel.h  - Kernel: definitions of structs and constants
├── kernel.ld - Kernel: linker script (memory layout definition)
├── shell.c   - Command-line shell
├── user.c    - User library: functions for system calls
├── user.h    - User library: definitions of structs and constants
├── user.ld   - User: linker script (memory layout definition)
└── run.sh    - Build script

Makefile targets
- `make shell`: links `build/shell.elf`, converts it to `build/shell.bin`, then wraps it as `build/shell.bin.o` for the kernel to embed
- `make all`: builds the kernel and user shell; the kernel link depends on `build/shell.bin.o`
- `make run`: builds then boots QEMU with `build/kernel.elf`
- `make clean`: removes `build/`
