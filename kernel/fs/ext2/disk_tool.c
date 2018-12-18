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
    debug_cat(DEBUG_LOG, "Successfully got partition entries\n");
    return FIND_PART_SUCCESS;
}
