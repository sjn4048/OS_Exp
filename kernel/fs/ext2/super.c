//
// Created by zengkai on 2018/11/30.
//

#include "ext2.h"

/**
 * load superblock data into structure
 * @param sb: where store superblock
 * @param data: raw data
 * @return success or not
 */
int sb_fill(struct ext2_super_block *sb, void *data) {
    *sb = *(struct ext2_super_block *) data;
    return SUCCESS;
}

/**
 * load a group descriptor into structures
 * @param group_desc: where store group descriptors
 * @param data: raw data
 * @return success or not
 */
int group_desc_fill(struct ext2_group_desc *group_desc, void *data) {
    *group_desc = *(struct ext2_group_desc *) data;
    return SUCCESS;
}
