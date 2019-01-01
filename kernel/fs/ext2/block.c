//
// Created by zengkai on 2018/12/22.
//

#include "ext2.h"

/**
 * find block in indirect blocks
 * @param bp_id: this block pointer index is relative
 * @return block ID or FAIL
 */
__u32 find_in_indirect(__u32 bp_id, __u32 indirect_block_ptr) {
    __u8 buffer[4096];
    if (0 == disk_read(fs_info.par_start_address + indirect_block_ptr * (fs_info.block_size / 512), buffer,
                       fs_info.block_size / 512)) {
        debug_cat(DEBUG_ERROR, "find_in_indirect: failed due to disk fail.\n");
        return EXT2_FAIL;
    }

    __u32 *block_ids = (__u32 *) buffer;
    return block_ids[bp_id];
}

/**
 * find block in double indirect blocks
 * @param bp_id: this block pointer index is relative
 * @return block ID or FAIL
 */
__u32 find_in_double_indirect(__u32 bp_id, __u32 double_indirect_block_ptr) {
    __u8 buffer[4096];
    if (0 == disk_read(fs_info.par_start_address + double_indirect_block_ptr * (fs_info.block_size / 512), buffer,
                       fs_info.block_size / 512)) {
        debug_cat(DEBUG_ERROR, "find_in_double_indirect: failed due to disk fail.\n");
        return EXT2_FAIL;
    }

    __u32 *block_ids = (__u32 *) buffer;
    __u32 result;
    for (int i = 0; i < fs_info.block_size / 32; i++) {
        if ((result = find_in_indirect(bp_id, block_ids[i])) != EXT2_FAIL) {
            return result;
        }
    }

    return EXT2_FAIL;
}

/**
 * find block in triple indirect blocks
 * @param bp_id: this block pointer index is relative
 * @return block ID or FAIL
 */
__u32 find_in_triple_indirect(__u32 bp_id, __u32 double_indirect_block_ptr) {
    __u8 buffer[4096];
    if (0 == disk_read(fs_info.par_start_address + double_indirect_block_ptr * (fs_info.block_size / 512), buffer,
                       fs_info.block_size / 512)) {
        debug_cat(DEBUG_ERROR, "find_in_triple_indirect: failed due to disk fail.\n");
        return EXT2_FAIL;
    }

    __u32 *block_ids = (__u32 *) buffer;
    __u32 result;
    for (int i = 0; i < fs_info.block_size / 32; i++) {
        if ((result = find_in_double_indirect(bp_id, block_ids[i])) != EXT2_FAIL) {
            return result;
        }
    }

    return EXT2_FAIL;
}

/**
 * find block ID by file pointer (in bytes)
 * @param bp_id: this block pointer index is absolute
 * @param block_ptr: 15 block point
 * @return block ID or FAIL
 */
__u32 find_block(__u32 bp_id, __u32 *block_ptr) {
    if (bp_id < 12) {
        // direct pointer
        return block_ptr[bp_id];
    } else if (12 <= bp_id && bp_id < 12 + fs_info.block_size / 32) {
        // indirect pointer
        return find_in_indirect(bp_id - 12, block_ptr[12]);
    } else if (12 + fs_info.block_size / 32 <= bp_id &&
               bp_id < 12 + fs_info.block_size / 32 + (fs_info.block_size / 32) * (fs_info.block_size / 32)) {
        // double indirect pointer
        return find_in_double_indirect(bp_id - (12 + fs_info.block_size / 32), block_ptr[13]);
    } else {
        // triple indirect pointer
        return find_in_triple_indirect(
                bp_id - (12 + fs_info.block_size / 32 + (fs_info.block_size / 32) * (fs_info.block_size / 32)),
                block_ptr[14]);
    }
}


/**
 * write buffer block into disk
 * @param file: file structure
 * @return success or not
 */
int write_block(EXT2_FILE *file) {
    if (file->dirty == EXT2_NOT_DIRTY) {
        // if the data is not dirty, it's not necessary to write.
        return EXT2_SUCCESS;
    }

    if (file->pointer >= file->inode.info.i_size) {
        return EXT2_FAIL;
    }

    __u32 b_id = find_block(file->pointer / fs_info.block_size, file->inode.info.i_block);
    if (b_id == EXT2_FAIL) {
        return EXT2_FAIL;
    }

    if (EXT2_FAIL == disk_write(fs_info.par_start_address + b_id * (fs_info.block_size / 512), file->buffer,
                           fs_info.block_size / 512)) {
        return EXT2_FAIL;
    }

    file->dirty = EXT2_NOT_DIRTY;
    return EXT2_SUCCESS;
}

/**
 * read buffer block from disk
 * @param file: file structure
 * @return success or not
 */
int read_block(EXT2_FILE *file) {
    if (file->pointer >= file->inode.info.i_size) {
        return EXT2_FAIL;
    }

    __u32 b_id = find_block(file->pointer / fs_info.block_size, file->inode.info.i_block);
    if (b_id == EXT2_FAIL) {
        return EXT2_FAIL;
    }

    if (EXT2_FAIL == disk_read(fs_info.par_start_address + b_id * (fs_info.block_size / 512), file->buffer,
                           fs_info.block_size / 512)) {
        return EXT2_FAIL;
    }

    file->dirty = EXT2_NOT_DIRTY;
    return EXT2_SUCCESS;
}