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
    return SUCCESS;
}

/**
 * load a directory entry into structure
 * @param dir: where store directory
 * @param data: raw data
 * @return success or not
 */
int dir_entry_fill(struct ext2_dir_entry *dir, void *data) {
    // free old
    if (dir->name != NULL) {
        memory.free(dir->name);
    }

    *dir = *(struct ext2_dir_entry *) data;
    // +1 for '\0'
    dir->name = memory.allocate(dir->name_len + 1);
    if (dir->name == NULL) {
        debug_cat(DEBUG_ERROR, "dir_entry_fill: failed due to memory error.\n");
        return FAIL;
    }
    // copy file name
    memory.copy(dir->name, data + 8, dir->name_len);
    // append end
    dir->name[dir->name_len] = '\0';
    return SUCCESS;
}

/**
 * compare two string (used to compare file name or path)
 * @param s1: string 1
 * @param s2: string 2
 * @return true or false
 */
int is_equal(__u8 *s1, __u8 *s2) {
    while (*s1 != '\0' && *s2 != '\0') {
        if (*s1 == *s2) {
            s1++;
            s2++;
        } else {
            return FALSE;
        }
    }

    if (*s1 == '\0' && *s2 == '\0') {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * find inode by inode ID
 * @param id: inode ID
 * @param inode: result
 * @return: success or not
 */
int find_inode(__u32 id, struct ext2_inode *inode) {
    if (id <= 2 || id > fs_info.inode_count) {
        // no such inode, inode ID is in [2, inode_count]
        return FAIL;
    } else {
        int bg_id = (id - 1) / fs_info.inode_per_bg;
        int inode_index = (id - 1) % fs_info.inode_per_bg;

        // load group descriptor
        __u8 buffer[512];
        struct ext2_group_desc group_desc;
        if (0 == disk_read(fs_info.group_desc_address + (bg_id * 32 / 512), buffer, 1)) {
            debug_cat(DEBUG_ERROR, "find_inode: failed due to disk error.\n");
            return FAIL;
        }
        group_desc_fill(&group_desc, buffer + (bg_id * 32 % 512));

        // load inode bitmap
        if (0 == disk_read(
                fs_info.par_start_address + group_desc.bg_inode_bitmap * (fs_info.block_size / 512) + inode_index / 512,
                buffer, 1)) {
            debug_cat(DEBUG_ERROR, "find_inode: failed due to disk error.\n");
            return FAIL;
        }

        if (0 == (buffer[inode_index / 8] & (1 << (7 - inode_index % 8)))) {
            // that inode is not used
            return FAIL;
        }

        // read that inode entry
        if (0 == disk_read(fs_info.par_start_address + group_desc.bg_inode_table * (fs_info.block_size / 512) +
                           inode_index * fs_info.inode_size / 512, buffer, 1)) {
            debug_cat(DEBUG_ERROR, "find_inode: failed due to disk error.\n");
            return FAIL;
        }

        // load that inode
        inode_fill(inode, buffer + inode_index * fs_info.inode_size % 512);
        return SUCCESS;
    }
}

/**
 * traverse direct blocks to find something called target
 * @param target: target's name
 * @param i_blocks: direct blocks
 * @param length: length of direct blocks
 * @param inode: where store result
 * @return success or not, finish or not
 */
int traverse_direct_block(__u8 *target, const __u32 *i_blocks, int length, INODE *inode) {
    // block by block
    __u8 buffer[4096];
    int offset;
    struct ext2_dir_entry dir;
    dir.name = NULL;
    for (int i = 0; i < length; i++) {
        if (i_blocks[i] == 0) {
            // this block is empty
            return FAIL;
        }

        if (0 == disk_read(fs_info.par_start_address + i_blocks[i] * (fs_info.block_size / 512), buffer,
                           fs_info.block_size / 512)) {
            debug_cat(DEBUG_ERROR, "traverse_direct_block: failed due to disk error.\n");
            return FAIL;
        }

        offset = 0;
        while (offset < 4096) {
            if (FAIL == dir_entry_fill(&dir, buffer + offset)) {
                return FAIL;
            }

            offset += dir.rec_len;
            if (FALSE == is_equal((__u8 *) dir.name, target)) {
                continue;
            }

            // we can continue
            inode->id = dir.inode;
            if (FAIL == find_inode(inode->id, &(inode->info))) {
                return FAIL;
            } else {
                return SUCCESS;
            }
        }
    }

    return FAIL | NOT_FOUND;
}

/**
 * traverse indirect blocks to find something called target
 * @param target: target's name
 * @param i_blocks: indirect blocks
 * @param length: length of indirect blocks
 * @param inode: where store result
 * @return success or not, finish or not
 */
int traverse_indirect_block(__u8 *target, const __u32 *i_blocks, int length, INODE *inode) {
    __u8 buffer[4096];
    for (int i = 0; i < length; i++) {
        if (i_blocks[i] == 0) {
            return FAIL;
        }

        if (0 == disk_read(fs_info.par_start_address + i_blocks[i] * (fs_info.block_size / 512), buffer,
                           fs_info.block_size / 512)) {
            debug_cat(DEBUG_ERROR, "traverse_indirect_block: failed due to disk error.\n");
            return FAIL;
        }
        __u32 *entries = (__u32 *) buffer;
        int result = traverse_direct_block(target, entries, 1024, inode);
        if (SUCCESS == result) {
            return SUCCESS;
        } else if (NOT_FOUND == result & NOT_FOUND) {
            continue;
        } else {
            return FAIL;
        }
    }

    return FAIL | NOT_FOUND;
}

/**
 * traverse double indirect blocks to find something called target
 * @param target: target's name
 * @param i_blocks: double indirect blocks
 * @param length: length of double indirect blocks
 * @param inode: where store result
 * @return success or not, finish or not
 */
int traverse_double_indirect_block(__u8 *target, const __u32 *i_blocks, int length, INODE *inode) {
    __u8 buffer[4096];
    for (int i = 0; i < length; i++) {
        if (i_blocks[i] == 0) {
            return FAIL;
        }

        if (0 == disk_read(fs_info.par_start_address + i_blocks[i] * (fs_info.block_size / 512), buffer,
                           fs_info.block_size / 512)) {
            debug_cat(DEBUG_ERROR, "traverse_double_indirect_block: failed due to disk error.\n");
            return FAIL;
        }

        __u32 *entries = (__u32 *) buffer;
        int result = traverse_indirect_block(target, entries, 1024, inode);
        if (SUCCESS == result) {
            return SUCCESS;
        } else if (NOT_FOUND == result & NOT_FOUND) {
            continue;
        } else {
            return FAIL;
        }
    }

    return FAIL | NOT_FOUND;
}

/**
 * traverse triple indirect blocks to find something called target
 * @param target: target's name
 * @param i_blocks: triple indirect blocks
 * @param length: length of triple indirect blocks
 * @param inode: where store result
 * @return success or not, finish or not
 */
int traverse_triple_indirect_block(__u8 *target, const __u32 *i_blocks, int length, INODE *inode) {
    __u8 buffer[4096];
    for (int i = 0; i < length; i++) {
        if (i_blocks[i] == 0) {
            return FAIL;
        }

        if (0 == disk_read(fs_info.par_start_address + i_blocks[i] * (fs_info.block_size / 512), buffer,
                           fs_info.block_size / 512)) {
            debug_cat(DEBUG_ERROR, "traverse_triple_indirect_block: failed due to disk error.\n");
            return FAIL;
        }

        __u32 *entries = (__u32 *) buffer;
        int result = traverse_indirect_block(target, entries, 1024, inode);
        if (SUCCESS == result) {
            return SUCCESS;
        } else if (NOT_FOUND == result & NOT_FOUND) {
            continue;
        } else {
            return FAIL;
        }
    }

    return FAIL | NOT_FOUND;
}

/**
 * traverse 15 blocks of pointers to find target
 * @param target: target name
 * @param i_blocks: 15 blocks
 * @param inode: store result
 * @return success or not
 */
int traverse_block(__u8 *target, __u32 *i_blocks, INODE *inode) {
    int result = traverse_direct_block(target, i_blocks, 12, inode);
    if (NOT_FOUND == result & NOT_FOUND) {
        result = traverse_indirect_block(target, i_blocks + 12, 1, inode);
        if (NOT_FOUND == result & NOT_FOUND) {
            result = traverse_double_indirect_block(target, i_blocks + 13, 1, inode);
            if (NOT_FOUND == result & NOT_FOUND) {
                result = traverse_triple_indirect_block(target, i_blocks + 14, 1, inode);
                if (SUCCESS == result) {
                    return SUCCESS;
                } else {
                    return FAIL;
                }
            } else if (SUCCESS == result) {
                return SUCCESS;
            } else {
                return FAIL;
            }
        } else if (SUCCESS == result) {
            return SUCCESS;
        } else {
            return FAIL;
        }
    } else if (SUCCESS == result) {
        return SUCCESS;
    } else {
        return FAIL;
    }
}

/**
 * get root inode from disk
 * @param inode: store inode
 * @return success or not
 */
int get_root_inode(struct ext2_inode *inode) {
    __u8 buffer[512];
    struct ext2_group_desc group_desc;
    if (disk_read(fs_info.group_desc_address, buffer, 1) == 0) {
        debug_cat(DEBUG_ERROR, "get_root_inode: failed due to disk error.\n");
        return FAIL;
    }
    group_desc_fill(&group_desc, buffer);

    // fine root inode
    if (disk_read(fs_info.par_start_address + group_desc.bg_inode_table * fs_info.block_size / 512, buffer, 1) == 0) {
        debug_cat(DEBUG_ERROR, "get_root_inode: failed due to disk error.\n");
        return FAIL;
    }

    inode_fill(inode, buffer + fs_info.inode_size);
    return SUCCESS;
}