#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SECTOR_SIZE 512
#define POINTER_SIZE 4
#define DATA_SIZE (SECTOR_SIZE - POINTER_SIZE) // 508 bytes

#define END_OF_FILE 0xFFFFFFFF
#define FREE_BLOCK 0xFFFFFFFE

#define SUPERBLOCK_IDX 0
#define DIR_START_IDX 1
#define DIR_BLOCKS 4
#define DATA_START_IDX 5
#define TOTAL_BLOCKS 2048 // 1MB disk

// --- File System Structures ---
struct superblock {
    char magic[8];
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

// The disk image is built entirely in host RAM, then flushed to file.
uint8_t disk[TOTAL_BLOCKS * SECTOR_SIZE];
uint32_t next_free_block = DATA_START_IDX;

void format_disk() {
    memset(disk, 0, sizeof(disk));

    // 1. Write Superblock
    struct superblock *sb =
        (struct superblock *)&disk[SUPERBLOCK_IDX * SECTOR_SIZE];
    strcpy(sb->magic, "MYFS");
    sb->total_blocks = TOTAL_BLOCKS;
    sb->dir_blocks = DIR_BLOCKS;
    sb->data_start_block = DATA_START_IDX;

    // 2. Mark all data blocks as free
    for (int i = DATA_START_IDX; i < TOTAL_BLOCKS; i++) {
        struct data_block *db = (struct data_block *)&disk[i * SECTOR_SIZE];
        db->next_block = FREE_BLOCK;
    }
}

void inject_file(const char *host_filepath, const char *filename) {
    FILE *f = fopen(host_filepath, "rb");
    if (!f)
        return;

    // Get file size
    fseek(f, 0, SEEK_END);
    uint32_t filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // 1. Find a free directory entry in sectors 1-4
    struct dir_entry *entry = NULL;
    for (int i = 0; i < DIR_BLOCKS * (SECTOR_SIZE / sizeof(struct dir_entry));
         i++) {
        struct dir_entry *e =
            (struct dir_entry *)&disk[DIR_START_IDX * SECTOR_SIZE +
                                      i * sizeof(struct dir_entry)];
        if (!e->in_use) {
            entry = e;
            break;
        }
    }

    if (!entry) {
        printf("Error: No free directory entries for %s\n", filename);
        fclose(f);
        return;
    }

    // 2. Populate directory entry
    entry->in_use = true;
    strncpy(entry->name, filename, 18);
    entry->size = filesize;
    entry->start_block = next_free_block;

    // 3. Read file and create linked data blocks
    uint32_t bytes_read = 0;
    uint32_t current_block = next_free_block;

    while (bytes_read < filesize) {
        struct data_block *db =
            (struct data_block *)&disk[current_block * SECTOR_SIZE];

        uint32_t chunk = (filesize - bytes_read > DATA_SIZE)
                             ? DATA_SIZE
                             : (filesize - bytes_read);
        fread(db->data, 1, chunk, f);
        bytes_read += chunk;

        next_free_block++; // Move to the next block globally

        if (bytes_read < filesize) {
            db->next_block = next_free_block;
            current_block = next_free_block;
        } else {
            db->next_block = END_OF_FILE;
        }
    }

    printf("Injected: %s (%d bytes) starting at block %d\n", filename, filesize,
           entry->start_block);
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <disk.img> <source_dir/>\n", argv[0]);
        return 1;
    }

    format_disk();

    // Read the host directory
    DIR *dir = opendir(argv[2]);
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {
                char filepath[256];
                snprintf(filepath, sizeof(filepath), "%s/%s", argv[2],
                         ent->d_name);
                inject_file(filepath, ent->d_name);
            }
        }
        closedir(dir);
    } else {
        printf("Could not open directory %s\n", argv[2]);
    }

    // Write the compiled disk image to the host file system
    FILE *out = fopen(argv[1], "wb");
    fwrite(disk, 1, sizeof(disk), out);
    fclose(out);

    printf("Successfully built MYFS image: %s\n", argv[1]);
    return 0;
}