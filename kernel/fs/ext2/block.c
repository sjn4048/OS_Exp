//
// Created by zengkai on 2018/12/22.
//

#include "ext2.h"

/**
 * find block in indirect blocks
 * @param bp_id: this block pointer index is relative
 * @return block ID or FAIL
 */
__u32 ext2_find_in_indirect(__u32 bp_id, __u32 indirect_block_ptr)
{
    __u8 buffer[4096];
    sd_read_block(buffer,
                  fs_info.par_start_address + indirect_block_ptr * (fs_info.block_size / 512),
                  fs_info.block_size / 512);

    __u32 *block_ids = (__u32 *)buffer;
    return block_ids[bp_id];
}

/**
 * find block in double indirect blocks
 * @param bp_id: this block pointer index is relative
 * @return block ID or FAIL
 */
__u32 ext2_find_in_double_indirect(__u32 bp_id, __u32 double_indirect_block_ptr)
{
    __u8 buffer[4096];
    sd_read_block(buffer,
                  fs_info.par_start_address + double_indirect_block_ptr * (fs_info.block_size / 512),
                  fs_info.block_size / 512);

    __u32 *block_ids = (__u32 *)buffer;
    __u32 result;
    for (int i = 0; i < fs_info.block_size / 32; i++)
    {
        if ((result = ext2_find_in_indirect(bp_id, block_ids[i])) != EXT2_FAIL)
        {
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
__u32 ext2_find_in_triple_indirect(__u32 bp_id, __u32 double_indirect_block_ptr)
{
    __u8 buffer[4096];
    sd_read_block(buffer,
                  fs_info.par_start_address + double_indirect_block_ptr * (fs_info.block_size / 512),
                  fs_info.block_size / 512);

    __u32 *block_ids = (__u32 *)buffer;
    __u32 result;
    for (int i = 0; i < fs_info.block_size / 32; i++)
    {
        if ((result = ext2_find_in_double_indirect(bp_id, block_ids[i])) != EXT2_FAIL)
        {
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
__u32 ext2_find_block(__u32 bp_id, __u32 *block_ptr)
{
    if (bp_id < 12)
    {
        // direct pointer
        return block_ptr[bp_id];
    }
    else if (12 <= bp_id && bp_id < 12 + fs_info.block_size / 32)
    {
        // indirect pointer
        return ext2_find_in_indirect(bp_id - 12, block_ptr[12]);
    }
    else if (12 + fs_info.block_size / 32 <= bp_id &&
             bp_id < 12 + fs_info.block_size / 32 + (fs_info.block_size / 32) * (fs_info.block_size / 32))
    {
        // double indirect pointer
        return ext2_find_in_double_indirect(bp_id - (12 + fs_info.block_size / 32), block_ptr[13]);
    }
    else
    {
        // triple indirect pointer
        return ext2_find_in_triple_indirect(
            bp_id - (12 + fs_info.block_size / 32 + (fs_info.block_size / 32) * (fs_info.block_size / 32)),
            block_ptr[14]);
    }
}

/**
 * write buffer block into disk
 * @param file: file structure
 * @return success or not
 */
int ext2_write_block(EXT2_FILE *file)
{
    if (file->dirty == EXT2_NOT_DIRTY)
    {
        // if the data is not dirty, it's not necessary to write.
        return EXT2_SUCCESS;
    }

    if (file->pointer >= file->inode.info.i_size)
    {
        return EXT2_FAIL;
    }

    __u32 b_id = ext2_find_block(file->pointer / fs_info.block_size, file->inode.info.i_block);
    if (b_id == EXT2_FAIL)
    {
        return EXT2_FAIL;
    }

    sd_write_block(file->buffer,
                   fs_info.par_start_address + b_id * (fs_info.block_size / 512),
                   fs_info.block_size / 512);

    file->dirty = EXT2_NOT_DIRTY;
    return EXT2_SUCCESS;
}

/**
 * read buffer block from disk
 * @param file: file structure
 * @return success or not
 */
int ext2_read_block(EXT2_FILE *file)
{
    if (file->pointer >= file->inode.info.i_size)
    {
        return EXT2_FAIL;
    }

    __u32 b_id = ext2_find_block(file->pointer / fs_info.block_size, file->inode.info.i_block);
    if (b_id == EXT2_FAIL)
    {
        return EXT2_FAIL;
    }

    sd_read_block(file->buffer,
                  fs_info.par_start_address + b_id * (fs_info.block_size / 512),
                  fs_info.block_size / 512);

    file->dirty = EXT2_NOT_DIRTY;
    return EXT2_SUCCESS;
}

/**
 * allocate new block which is independent to inode
 * only support count = 1
 * @param gd_id: group descriptor ID
 * @return block id, or -1
 */
int ext2_new_block_without_inode(int gd_id)
{
    // find free blocks
    unsigned char buffer[4096];
    struct ext2_group_desc group_desc;
    int result;
    while (1)
    {
        sd_read_block(buffer, fs_info.group_desc_address + gd_id * 32 / 512, 1);
        ext2_group_desc_fill(&group_desc, buffer + gd_id * 32 % 512);
        sd_read_block(buffer, fs_info.par_start_address + group_desc.bg_block_bitmap * 8, 8);
        result = ext2_find_free_block(&group_desc);
        if (result != -1)
        {
            return gd_id * fs_info.sb.s_blocks_per_group + result;
        }
        else
        {
            gd_id += 1;
        }
    }
}

/**
 * attach new block to direct block
 * @param pointer: new block
 * @param len_pointer: new block count
 * @param block: inode's i_block
 * @param len_block: length of i_block (this function may be called by indirect block as well)
 * @return how many new block are attached, or -1
 */
int ext2_attach_b2i_direct(__u32 *pointer, int len_pointer, __u32 *block, int len_block)
{
    int count = 0;
    for (int i = 0; i < len_block && count < len_pointer; i++)
    {
        if (block[i] == 0)
        {
            block[i] = pointer[count++];
            if (EXT2_FAIL == ext2_set_block_used(block[i]))
            {
                return -1;
            }
        }
    }
    return count;
}

/**
 * attach new block to indirect block
 * @param pointer: new block
 * @param len_pointer: new block count
 * @param block: inode's i_block
 * @param len_block: length of i_block
 * @return how many new block are attached, -1
 */
int ext2_attach_b2i_indirect(__u32 *pointer, int len_pointer, __u32 *block, int len_block)
{
    int count = 0;
    int sum = 0;
    __u8 buffer[4096];
    for (int i = 0; i < len_block && sum < len_pointer; i++)
    {
        if (block[i] == 0)
        {
            // NOTE: trick here, allocate new block near to current directory
            block[i] = ext2_new_block_without_inode((current_dir.inode.id - 1) / fs_info.sb.s_inodes_per_group);
            if (EXT2_FAIL == ext2_set_block_used(block[i]))
            {
                return -1;
            }
            sd_read_block(
                buffer,
                fs_info.par_start_address + block[i] * 8, 8);
            for (int j = 0; i < 4096; i++)
            {
                buffer[j] = 0;
            }
        }
        else
        {
            sd_read_block(
                buffer,
                fs_info.par_start_address + block[i] * 8, 8);
        }

        __u32 *buffer_32 = (__u32 *)buffer;
        count = ext2_attach_b2i_direct(pointer + sum, len_pointer - sum, buffer_32, 1024);
        if (count == -1)
        {
            return -1;
        }
        sum += count;

        sd_write_block(
            buffer,
            fs_info.par_start_address + block[i] * 8, 8);
    }

    return sum;
}

/**
 * attach new block to DOUBLE indirect block
 * @param pointer: new block
 * @param len_pointer: new block count
 * @param block: inode's i_block
 * @param len_block: length of i_block
 * @return how many new block are attached, -1
 */
int ext2_attach_b2i_double_indirect(__u32 *pointer, int len_pointer, __u32 *block, int len_block)
{
    int count = 0;
    int sum = 0;
    __u8 buffer[4096];
    for (int i = 0; i < len_block && sum < len_pointer; i++)
    {
        if (block[i] == 0)
        {
            // NOTE: trick here, allocate new block near to current directory
            block[i] = ext2_new_block_without_inode((current_dir.inode.id - 1) / fs_info.sb.s_inodes_per_group);
            if (EXT2_FAIL == ext2_set_block_used(block[i]))
            {
                return -1;
            }
            sd_read_block(
                buffer,
                fs_info.par_start_address + block[i] * 8, 8);
            for (int j = 0; i < 4096; i++)
            {
                buffer[j] = 0;
            }
        }
        else
        {
            sd_read_block(
                buffer,
                fs_info.par_start_address + block[i] * 8, 8);
        }

        __u32 *buffer_32 = (__u32 *)buffer;
        count = ext2_attach_b2i_indirect(pointer + sum, len_pointer - sum, buffer_32, 1024);
        if (count == -1)
        {
            return -1;
        }
        sum += count;

        sd_write_block(
            buffer,
            fs_info.par_start_address + block[i] * 8, 8);
    }

    return sum;
}

/**
 * attach new block to TRIPLE indirect block
 * @param pointer: new block
 * @param len_pointer: new block count
 * @param block: inode's i_block
 * @param len_block: length of i_block
 * @return how many new block are attached, -1
 */
int ext2_attach_b2i_triple_indirect(__u32 *pointer, int len_pointer, __u32 *block, int len_block)
{
    int count = 0;
    int sum = 0;
    __u8 buffer[4096];
    for (int i = 0; i < len_block && sum < len_pointer; i++)
    {
        if (block[i] == 0)
        {
            // NOTE: trick here, allocate new block near to current directory
            block[i] = ext2_new_block_without_inode((current_dir.inode.id - 1) / fs_info.sb.s_inodes_per_group);
            if (EXT2_FAIL == ext2_set_block_used(block[i]))
            {
                return -1;
            }
            sd_read_block(
                buffer,
                fs_info.par_start_address + block[i] * 8, 8);
            for (int j = 0; i < 4096; i++)
            {
                buffer[j] = 0;
            }
        }
        else
        {
            sd_read_block(
                buffer,
                fs_info.par_start_address + block[i] * 8, 8);
        }

        __u32 *buffer_32 = (__u32 *)buffer;
        count = ext2_attach_b2i_double_indirect(pointer + sum, len_pointer - sum, buffer_32, 1024);
        if (count == -1)
        {
            return -1;
        }
        sum += count;

        sd_write_block(
            buffer,
            fs_info.par_start_address + block[i] * 8, 8);
    }

    return sum;
}

/**
 * attach new block to inode
 * @param pointers: new blocks
 * @param len: new blocks count
 * @param inode: attach to it
 * @return success or not
 */
int ext2_attach_block_to_inode(__u32 *pointers, int len, INODE *inode)
{
    int count;
    int sum = 0;

    // direct
    count = ext2_attach_b2i_direct(pointers, len, inode->info.i_block, 12);
    if (count == -1)
    {
        return EXT2_FAIL;
    }
    sum += count;

    // indirect
    if (sum < len)
    {
        count = ext2_attach_b2i_indirect(pointers + sum, len - sum, inode->info.i_block + 12, 1);
        if (count == -1)
        {
            return EXT2_FAIL;
        }
        sum += count;
    }

    // double indirect
    if (sum < len)
    {
        count = ext2_attach_b2i_double_indirect(pointers + sum, len - sum, inode->info.i_block + 13, 1);
        if (count == -1)
        {
            return EXT2_FAIL;
        }
        sum += count;
    }

    if (sum < len)
    {
        return EXT2_FAIL;
    }
    else
    {
        return EXT2_SUCCESS;
    }
}

/**
 * allocate new block for inode
 * FIXME: bad performance for big count
 * @param inode: proposer
 * @return success or not
 */
int ext2_new_block(INODE *inode, int count)
{
    if (count < 1)
    {
        return EXT2_FAIL;
    }
    __u32 *pointers = (__u32 *)kmalloc(sizeof(__u32) * count);

    // find free blocks
    unsigned char buffer[512];
    struct ext2_group_desc group_desc;
    int target = -1;
    int gd_id = (inode->id - 1) / fs_info.sb.s_inodes_per_group - 1; // -1 for +1
    for (int i = 0; i < count; i++)
    {
        while (1)
        {
            if (target == -1)
            {
                // next block group
                gd_id += 1;
                sd_read_block(buffer, fs_info.group_desc_address + gd_id * 32 / 512, 1);
                ext2_group_desc_fill(&group_desc, buffer + gd_id * 32 % 512);
            }
            target = ext2_find_free_block(&group_desc);
            if (target != -1)
            {
                pointers[i] = gd_id * fs_info.sb.s_blocks_per_group + target;
                break;
            }
        }
    }

    // assign them to inode
    if (EXT2_FAIL == ext2_attach_block_to_inode(pointers, count, inode))
    {
        kfree(pointers);
        return EXT2_FAIL;
    }
    else
    {
        // clean these blocks
        for (int i = 0; i < count; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                sd_read_block(buffer, fs_info.par_start_address + pointers[i] * 8 + j, 1);
                for (int k = 0; k < 512; k++)
                {
                    buffer[k] = 0;
                }
                sd_write_block(buffer, fs_info.par_start_address + pointers[i] * 8 + j, 1);
            }
        }
        inode->info.i_blocks += 8 * count;
        kfree(pointers);
        return EXT2_SUCCESS;
    }
}

/**
 * set a block free in bitmap
 * @param block id
 * @return success or not
 */
int ext2_set_block_free(__u32 block_id)
{
    struct ext2_group_desc group_desc;
    unsigned char buffer[512];
    // refresh group descriptor
    sd_read_block(
        buffer,
        fs_info.group_desc_address + (block_id / fs_info.sb.s_blocks_per_group) * 32 / 512, 1);
    ext2_group_desc_fill(
        &group_desc,
        buffer + (block_id / fs_info.sb.s_blocks_per_group) * 32 % 512);
    group_desc.bg_free_blocks_count += 1;
    kernel_memcpy(
        buffer + (block_id / fs_info.sb.s_blocks_per_group) * 32 % 512,
        &group_desc, sizeof(struct ext2_group_desc));
    sd_write_block(
        buffer,
        fs_info.group_desc_address + (block_id / fs_info.sb.s_blocks_per_group) * 32 / 512, 1);
    // refresh block bitmap
    sd_read_block(
        buffer,
        fs_info.par_start_address + group_desc.bg_block_bitmap * 8 + block_id % fs_info.sb.s_blocks_per_group / 8 / 512, 1);
    buffer[block_id % fs_info.sb.s_blocks_per_group / 8 % 512] &= ~(1 << (block_id % 8));
    sd_write_block(
        buffer,
        fs_info.par_start_address + group_desc.bg_block_bitmap * 8 + block_id % fs_info.sb.s_blocks_per_group / 8 / 512, 1);

    return EXT2_SUCCESS;
}

/**
 * release direct block
 * @param blocks: direct blocks
 * @return -1 means that haven't ended yet
 */
int ext2_release_direct_blocks(__u32 *blocks, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (blocks[i] == 0)
        {
            return 0;
        }
        else
        {
            ext2_set_block_free(blocks[i]);
        }
    }

    return -1;
}

/**
 * release indirect block
 * @param blocks: direct blocks
 * @return -1 means that haven't ended yet
 */
int ext2_release_indirect_blocks(__u32 *blocks, int len)
{
    __u8 buffer[4096];
    for (int i = 0; i < len; i++)
    {
        if (blocks[i] != 0)
        {
            sd_read_block(buffer, fs_info.par_start_address + blocks[i] * 8, 8);
            __u32 *buffer_32 = (__u32 *)buffer;
            ext2_release_direct_blocks(buffer_32, 1024);
            // release it self
            ext2_release_direct_blocks(blocks + i, 1);
        }
        else
        {
            return 0;
        }
    }

    return -1;
}

/**
 * release double indirect block
 * @param blocks: direct blocks
 * @return -1 means that haven't ended yet
 */
int ext2_release_double_indirect_blocks(__u32 *blocks, int len)
{
    __u8 buffer[4096];
    for (int i = 0; i < len; i++)
    {
        if (blocks[i] != 0)
        {
            sd_read_block(buffer, fs_info.par_start_address + blocks[i] * 8, 8);
            __u32 *buffer_32 = (__u32 *)buffer;
            ext2_release_indirect_blocks(buffer_32, 1024);
            // release it self
            ext2_release_direct_blocks(blocks + i, 1);
        }
        else
        {
            return 0;
        }
    }

    return -1;
}

/**
 * release triple indirect block
 * @param blocks: direct blocks
 * @return -1 means that haven't ended yet
 */
int ext2_release_triple_indirect_blocks(__u32 *blocks, int len)
{
    __u8 buffer[4096];
    for (int i = 0; i < len; i++)
    {
        if (blocks[i] != 0)
        {
            sd_read_block(buffer, fs_info.par_start_address + blocks[i] * 8, 8);
            __u32 *buffer_32 = (__u32 *)buffer;
            ext2_release_double_indirect_blocks(buffer_32, 1024);
            // release it self
            ext2_release_direct_blocks(blocks + i, 1);
        }
        else
        {
            return 0;
        }
    }

    return -1;
}

/**
 * release the blocks belong to a inode
 * @param blocks
 * @return success or not
 */
int ext2_release_blocks(__u32 *blocks)
{
    if (-1 == ext2_release_direct_blocks(blocks, 12))
    {
        if (-1 == ext2_release_indirect_blocks(blocks + 12, 1))
        {
            if (-1 == ext2_release_double_indirect_blocks(blocks + 13, 1))
            {
                if (-1 == ext2_release_triple_indirect_blocks(blocks + 14, 1))
                {
                    return EXT2_SUCCESS;
                }
            }
        }
    }
    return EXT2_SUCCESS;
}