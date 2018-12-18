//
// Created by zengkai on 2018/12/18.
//

#include "ext2.h"

/**
 * load an inode into structure
 * @param inode: where store inode
 * @param data: raw data
 * @return success or not
 */
int inode_fill(struct ext2_inode *inode, void *data) {
    *inode = *(struct ext2_inode *) data;
    return INODE_INODE_FILL_SUCCESS;
}