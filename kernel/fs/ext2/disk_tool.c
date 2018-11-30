//
// Created by zengk on 2018/11/30.
//

#include "ext2.h"

int find_part(struct partition_entry *par) {
    unsigned char buffer[512];
    if (512 == disk.read(0, buffer)) { // read MBR from disk
        int pointer = 446; // partition table starts at 446 bytes
        for (int i = 0; i < 4; i++) { // four entry in the table
            par[i] = *(struct partition_entry *) (buffer + pointer);
            pointer += 16;
        }
        debug_cat(DEBUG_LOG, "Successfully got partition entries");
        return FIND_PART_SUCCESS;
    } else {
        debug_cat(DEBUG_ERROR, "Failed to get partition entries");
        return FIND_PART_FAIL;
    }
}
