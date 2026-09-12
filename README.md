# riscv32-os

A small RISC-V32 operating system written in C, following the "Operating System in 1000 Lines" tutorial and extended with a custom single-level filesystem (MYFS) and a command-line shell. Runs under QEMU (`qemu-system-riscv32`, `virt` machine).

## Layout

```
include/   - Shared/kernel/user headers: typedefs, syscall numbers, structs, function declarations
src/
  common/  - Shared library: printf, memset, memcpy, strcpy, strcmp
  kernel/  - Kernel: trap/syscall handling, process management, virtio-blk driver, MYFS filesystem
  user/    - User-space syscall wrappers, program entry point, and the shell
tools/     - Host-side disk utilities (mkfs, extractfs) for building/inspecting disk.img
disk/      - Starts empty; anything you place here is copied into disk.img's MYFS at build time
```

## Filesystem: MYFS

A minimal, single-level (no subdirectories) filesystem used for `disk.img`:

- **Sector 0** — superblock (`struct superblock`): magic string `"MYFS"`, total block count, directory block count, data start block.
- **Sectors 1-4** — directory blocks (`struct dir_entry`, 32 bytes each, 16 per sector): filename, size, start block, in-use flag.
- **Sector 5 onward** — data blocks (`struct data_block`): 508 bytes of file data + a 4-byte pointer to the next block (linked-list chaining), terminated by `END_OF_FILE`.

The same three structs (`superblock`, `dir_entry`, `data_block`) are defined in `include/kernel.h` (kernel-side) and duplicated in `tools/mkfs.c` / `tools/extractfs.c` (host-side) — all three must be kept in sync by hand if the on-disk format changes.

## Shell commands

Once booted, the embedded shell (`src/user/shell.c`) supports:

| Command | Description |
|---|---|
| `hello` | Prints a test greeting |
| `ls` | Lists files stored in MYFS |
| `cat <filename>` | Prints the contents of a file |
| `write <filename> <content>` | Writes `<content>` to `<filename>` |
| `exit` | Exits the shell process |

## Building & running

Requires `clang` (targeting `riscv32-unknown-elf`), `llvm-objcopy`, `qemu-system-riscv32`, and a host `gcc` (for the `mkfs`/`extractfs` tools).

Makefile targets:
- `make shell` — links `build/shell.elf`, converts it to `build/shell.bin`, then wraps it as `build/shell.bin.o` for the kernel to embed
- `make all` — builds the kernel and user shell; the kernel link depends on `build/shell.bin.o`
- `make run` — builds everything, generates `disk.img` from `disk/` via `mkfs`, then boots QEMU with `build/kernel.elf`
- `make extract` — builds `build/extractfs` and extracts the files inside `disk.img` back into `disk/`
- `make clean` — removes `build/` and `disk.img`

## License

[MIT](LICENSE)
