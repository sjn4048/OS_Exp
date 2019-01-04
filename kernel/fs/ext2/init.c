//
// Created by zengkai on 2018/11/30.
//

#include "ext2.h"

/**
 * seek for partition that type is 0x83 (Linux)
 * @param buffer
 * @return success or not
 */
int ext2_find_linux_par(__u8 *buffer)
{
    sd_read_block(buffer, 0, 1);
    // get partition table
    struct partition_entry par[4];
    ext2_find_part(par, buffer);
    // find Linux partition (do not support extended partition now)
    fs_info.par_start_address = 0;
    for (int i = 0; i < 4; i++)
    {
        // NOTE: delete check for par[i].status == 0x80 because sometimes it is 0
        if (par[i].partition_type == 0x83)
        {
            // this partition is active and partition type is Linux
            fs_info.par_start_address = par[i].start_address;
            return EXT2_SUCCESS;
        }
    }

    return EXT2_FAIL;
}

/**
 * look up ext2 file system on a Linux partition
 * @param buffer
 * @return success or not
 */
int ext2_find_ext2_fs(__u8 *buffer)
{
    // skip 1024B boot code
    struct ext2_super_block sb;
    sd_read_block(buffer, fs_info.par_start_address + 2, 2);
    ext2_sb_fill(&sb, buffer);
    if (sb.s_magic == 0xEF53)
    {
        kernel_memcpy(&(fs_info.sb), &sb, sizeof(struct ext2_super_block));
        if (fs_info.sb.s_log_block_size == 0)
        {
            // block size = 1KB
            fs_info.block_size = 1024;
            fs_info.group_desc_address = fs_info.par_start_address + 4;
        }
        else
        {
            // block size >= 2KB
            fs_info.block_size = (__u16)(1024 << fs_info.sb.s_log_block_size);
            fs_info.group_desc_address = fs_info.par_start_address + fs_info.block_size / 512;
        }
        return EXT2_SUCCESS;
    }
    return EXT2_FAIL;
}

/**
 * load root inode
 * @return success or not
 */
int ext2_load_root_inode()
{
    // initialize current path (root)
    current_dir.name[0] = '\0';
    current_dir.inode.id = 2;

    if (EXT2_SUCCESS == ext2_get_root_inode(&(current_dir.inode.info)))
    {
        return EXT2_SUCCESS;
    }
    else
    {
        return EXT2_FAIL;
    }
}

/**
 * initialize ext2 file system
 *   1. select disk interface: Windows or SWORD
 *   2. figure out whether there is a Ext2 file system or not
 *   3. initialize sb_info
 * @return success or not
 */
int ext2_init()
{
    __u8 buffer[1024];

    // find Linux partition
    if (EXT2_SUCCESS != ext2_find_linux_par(buffer))
    {
        goto error;
    }

    // whether this Linux partition is Ext2
    if (EXT2_SUCCESS != ext2_find_ext2_fs(buffer))
    {
        goto error;
    }

    // find root inode
    if (EXT2_SUCCESS == ext2_load_root_inode())
    {
        return EXT2_SUCCESS;
    }
    else
    {
        goto error;
    }

// cannot find Ext2 file system in primary partitions
error:
    return EXT2_FAIL;
}
