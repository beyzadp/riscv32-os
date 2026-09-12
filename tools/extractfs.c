#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 512
#define POINTER_SIZE 4
#define DATA_SIZE (SECTOR_SIZE - POINTER_SIZE)

#define END_OF_FILE 0xFFFFFFFF
#define FREE_BLOCK 0xFFFFFFFE
#define DIR_START_IDX 1
#define DIR_BLOCKS 4

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

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <disk.img> <output_dir/>\n", argv[0]);
        return 1;
    }

    FILE *img = fopen(argv[1], "rb");
    if (!img) {
        printf("Could not open %s\n", argv[1]);
        return 1;
    }

    // 1. Read the directory blocks
    uint8_t dir_buf[SECTOR_SIZE * DIR_BLOCKS];
    fseek(img, DIR_START_IDX * SECTOR_SIZE, SEEK_SET);
    fread(dir_buf, 1, sizeof(dir_buf), img);

    struct dir_entry *entries = (struct dir_entry *)dir_buf;
    int max_entries = sizeof(dir_buf) / sizeof(struct dir_entry);

    // 2. Loop through every file in the directory
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].in_use) {
            char out_path[256];
            snprintf(out_path, sizeof(out_path), "%s/%s", argv[2],
                     entries[i].name);

            FILE *out = fopen(out_path, "wb");
            if (!out) {
                printf("Failed to create %s\n", out_path);
                continue;
            }

            // 3. Follow the blocks and extract the data
            uint32_t current_block = entries[i].start_block;
            uint32_t bytes_left = entries[i].size;
            uint8_t block_buf[SECTOR_SIZE];

            while (current_block != END_OF_FILE &&
                   current_block != FREE_BLOCK && bytes_left > 0) {
                fseek(img, current_block * SECTOR_SIZE, SEEK_SET);
                fread(block_buf, 1, SECTOR_SIZE, img);

                struct data_block *db = (struct data_block *)block_buf;

                uint32_t chunk =
                    (bytes_left < DATA_SIZE) ? bytes_left : DATA_SIZE;
                fwrite(db->data, 1, chunk, out);

                bytes_left -= chunk;
                current_block = db->next_block;
            }

            fclose(out);
            printf("Extracted: %s (%d bytes)\n", entries[i].name,
                   entries[i].size);
        }
    }

    fclose(img);
    return 0;
}