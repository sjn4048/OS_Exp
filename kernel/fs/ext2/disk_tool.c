//
// Created by zengkai on 2018/11/30.
//

#include "ext2.h"

/**
 * get partition table of the disk
 * @param par: where store partition table
 * @return success or not
 */
int ext2_find_part(struct partition_entry *par, void *data)
{
    int pointer = 446; // partition table starts at 446 bytes
    for (int i = 0; i < 4; i++)
    { // four entry in the table
        kernel_memcpy(par + i, (__u8 *)data + pointer, 16);
        pointer += 16;
    }

    return EXT2_SUCCESS;
}
