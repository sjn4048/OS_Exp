//
// Created by zengkai on 2018/11/30.
//

#include "ext2.h"

/**
 * get partition table of the disk
 * @param par: where store partition table
 * @return success or not
 */
int find_part(struct partition_entry *par, void *data) {
    int pointer = 446; // partition table starts at 446 bytes
    for (int i = 0; i < 4; i++) { // four entry in the table
        par[i] = *(struct partition_entry *) (data + pointer);
        pointer += 16;
    }

    return SUCCESS;
}

/**
 * read multiple sectors from disk
 * CAN NOT be used before ext2_init
 * @param id: sector ID
 * @param buffer: where store data
 * @param len: number of sectors
 * @return bytes of data read
 */
int disk_read(__u32 id, __u8 *buffer, int len) {
    int count = 0;
    int delta;
    for (int i = 0; i < len; i++) {
        delta = disk.read(id + i, buffer + 512 * i);
        if (delta == 0) {
            return 0;
        } else {
            count += delta;
        }
    }
    return count;
}

/**
 * write multiple sectors tp disk
 * CAN NOT be used before ext2_init
 * @param id: sector ID
 * @param buffer: where store data
 * @param len: number of sectors
 * @return bytes of data written
 */
int disk_write(__u32 id, __u8 *buffer, int len) {
    int count = 0;
    int delta;
    for (int i = 0; i < len; i++) {
        delta = disk.write(id + i, buffer + 512 * i);
        if (delta == 0) {
            return 0;
        } else {
            count += delta;
        }
    }
    return count;
}