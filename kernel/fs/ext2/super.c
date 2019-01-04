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
int ext2_sb_fill(struct ext2_super_block *sb, void *data)
{
    kernel_memcpy(sb, data, sizeof(struct ext2_super_block));
    return EXT2_SUCCESS;
}

/**
 * load a group descriptor into structures
 * @param group_desc: where store group descriptors
 * @param data: raw data
 * @return success or not
 */
int ext2_group_desc_fill(struct ext2_group_desc *group_desc, void *data)
{
    kernel_memcpy(group_desc, data, sizeof(struct ext2_group_desc));
    return EXT2_SUCCESS;
}

/**
 * find a block group whose number of used inode is lower than average
 * @param group_desc: result
 * @param id: id of result group descriptor
 * @return number of block group
 */
int ext2_find_sparse_bg(struct ext2_group_desc *group_desc, int *id)
{
    struct ext2_group_desc gd;
    int bg_count = fs_info.sb.s_inodes_count / fs_info.sb.s_inodes_per_group + 1;
    int bg_inode_count_avg = fs_info.sb.s_free_inodes_count / bg_count;

    // group descriptor is no longer than 1 block
    unsigned char buffer[4096];
    sd_read_block(buffer, fs_info.group_desc_address, 8);
    for (int i = 0; i < bg_count; i++)
    {
        ext2_group_desc_fill(&gd, buffer + i * 32);
        if (gd.bg_inode_bitmap != 0)
        {
            kernel_memcpy(group_desc, &gd, sizeof(struct ext2_group_desc));
            if (gd.bg_free_inodes_count > bg_inode_count_avg)
            {
                id[0] = i;
                return EXT2_SUCCESS;
            }
        }
        else
        {
            if (group_desc->bg_block_bitmap != 0 && i - 1 >= 0)
            {
                id[0] = i - 1;
                return EXT2_SUCCESS;
            }
            else
            {
                return EXT2_FAIL;
            }
        }
    }
}

/**
 * search on bitmap for 0
 * @return result, or -1 for non-existing
 */
int ext2_find_bitmap(unsigned char *bitmap)
{
    int mask;
    for (int i = 0; i < 4096; i++)
    {
        mask = 0x1;
        for (int j = 0; j < 8; j++)
        {
            if ((mask & bitmap[i]) == 0)
            {
                return i * 8 + j;
            }
            mask <<= 1;
        }
    }
    return -1;
}

/**
 * find the first free inode in a block group
 * @param group_desc: target block group
 * @return: success or not, NOTE: FAIL is -1 here
 */
int ext2_find_free_inode(struct ext2_group_desc *group_desc)
{
    unsigned char buffer[4096];
    sd_read_block(buffer, fs_info.par_start_address + group_desc->bg_inode_bitmap * 8, 8);
    return ext2_find_bitmap(buffer);
}

/**
 * find the first free block in a block group
 * if there is no free block, return -1
 * @param group_desc: target block group
 * @return success or not, NOTE: FAIL is -1 here
 */
int ext2_find_free_block(struct ext2_group_desc *group_desc)
{
    unsigned char buffer[4096];
    sd_read_block(buffer, fs_info.par_start_address + group_desc->bg_block_bitmap * 8, 8);
    return ext2_find_bitmap(buffer);
}

/**
 * set block used in bitmap and group descriptor
 * @param block_id: block's id
 * @return success or not
 */
int ext2_set_block_used(__u32 block_id)
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
    group_desc.bg_free_blocks_count -= 1;
    kernel_memcpy(
        buffer + (block_id / fs_info.sb.s_blocks_per_group) * 32 % 512,
        &group_desc, sizeof(struct ext2_group_desc));
    sd_write_block(
        buffer,
        fs_info.group_desc_address + (block_id / fs_info.sb.s_blocks_per_group) * 32 / 512, 1);
    // refresh block bitmap
    sd_read_block(
        buffer,
        fs_info.par_start_address + group_desc.bg_block_bitmap * 8 + block_id / 8 / 512, 1);
    buffer[block_id / 8 % 512] |= 1 << (7 - block_id % 8);
    sd_write_block(
        buffer,
        fs_info.par_start_address + group_desc.bg_block_bitmap * 8 + block_id / 8 / 512, 1);

    return EXT2_SUCCESS;
}
