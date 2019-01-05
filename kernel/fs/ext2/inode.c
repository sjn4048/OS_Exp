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
int ext2_inode_fill(struct ext2_inode *inode, void *data)
{
    kernel_memcpy(inode, data, sizeof(struct ext2_inode));
    return EXT2_SUCCESS;
}

/**
 * load a directory entry into structure
 * @param dir: where store directory
 * @param data: raw data
 * @return success or not
 */
int ext2_dir_entry_fill(struct ext2_dir_entry *dir, void *data)
{
    kernel_memcpy(dir, (__u8 *)data, 8);
    // +1 for '\0'

    // copy file name
    kernel_memcpy(dir->name, (__u8 *)data + 8, dir->name_len);
    // append end
    dir->name[dir->name_len] = '\0';
    return EXT2_SUCCESS;
}

/**
 * compare two string (used to compare file name or path)
 * @param s1: string 1
 * @param s2: string 2
 * @return EXT2_TRUE or EXT2_FALSE
 */
int ext2_is_equal(__u8 *s1, __u8 *s2)
{
    while (*s1 != '\0' && *s2 != '\0')
    {
        if (*s1 == *s2)
        {
            s1++;
            s2++;
        }
        else
        {
            return EXT2_FALSE;
        }
    }

    if (*s1 == '\0' && *s2 == '\0')
    {
        return EXT2_TRUE;
    }
    else
    {
        return EXT2_FALSE;
    }
}

/**
 * find inode by inode ID
 * @param id: inode ID
 * @param inode: result
 * @return: success or not
 */
int ext2_find_inode(__u32 id, struct ext2_inode *inode)
{
    if (id < 2 || id > fs_info.sb.s_inodes_count)
    {
        // no such inode, inode ID is in [2, inode_count]
        return EXT2_FAIL;
    }
    else
    {
        int bg_id = (id - 1) / fs_info.sb.s_inodes_per_group;
        int inode_index = (id - 1) % fs_info.sb.s_inodes_per_group;

        // load group descriptor
        __u8 buffer[512];
        struct ext2_group_desc group_desc;
        sd_read_block(buffer, fs_info.group_desc_address + (bg_id * 32 / 512), 1);
        ext2_group_desc_fill(&group_desc, buffer + (bg_id * 32 % 512));

        // load inode bitmap
        sd_read_block(
            buffer,
            fs_info.par_start_address + group_desc.bg_inode_bitmap * 8 + inode_index / 8 / 512, 1);

        if (0 == (buffer[inode_index / 8] & (1 << (inode_index % 8))))
        {
            // that inode is not used
            return EXT2_FAIL;
        }

        // read that inode entry
        sd_read_block(buffer,
                      fs_info.par_start_address +
                          group_desc.bg_inode_table * (fs_info.block_size / 512) +
                          inode_index * fs_info.sb.s_inode_size / 512,
                      1);
        // load that inode
        ext2_inode_fill(inode, buffer + inode_index * fs_info.sb.s_inode_size % 512);

        return EXT2_SUCCESS;
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
int ext2_traverse_direct_block(__u8 *target, __u32 *i_blocks, int length, INODE *inode)
{
    // block by block
    __u8 buffer[4096];
    int offset;
    struct ext2_dir_entry dir;
    for (int i = 0; i < length; i++)
    {
        if (i_blocks[i] == 0)
        {
            // this block is empty
            return EXT2_FAIL;
        }

        sd_read_block(buffer,
                      fs_info.par_start_address + i_blocks[i] * (fs_info.block_size / 512),
                      fs_info.block_size / 512);

        offset = 0;
        while (offset < 4096)
        {
            if (EXT2_FAIL == ext2_dir_entry_fill(&dir, buffer + offset))
            {
                return EXT2_FAIL;
            }
            offset += dir.rec_len;

            if (target == EXT2_NULL)
            {
                kernel_printf("    %s\n", dir.name);
                continue;
            }
            else
            {
                if (EXT2_FALSE == ext2_is_equal((__u8 *)dir.name, target))
                {
                    continue;
                }
            }

            // we can continue
            inode->id = dir.inode;
            if (EXT2_FAIL == ext2_find_inode(inode->id, &(inode->info)))
            {
                return EXT2_FAIL;
            }
            else
            {
                return EXT2_SUCCESS;
            }
        }
    }

    return EXT2_FAIL | EXT2_NOT_FOUND;
}

/**
 * traverse indirect blocks to find something called target
 * @param target: target's name
 * @param i_blocks: indirect blocks
 * @param length: length of indirect blocks
 * @param inode: where store result
 * @return success or not, finish or not
 */
int ext2_traverse_indirect_block(__u8 *target, const __u32 *i_blocks, int length, INODE *inode)
{
    __u8 buffer[4096];
    for (int i = 0; i < length; i++)
    {
        if (i_blocks[i] == 0)
        {
            return EXT2_FAIL;
        }

        sd_read_block(buffer,
                      fs_info.par_start_address + i_blocks[i] * (fs_info.block_size / 512),
                      fs_info.block_size / 512);
        __u32 *entries = (__u32 *)buffer;
        int result = ext2_traverse_direct_block(target, entries, 1024, inode);
        if (EXT2_SUCCESS == result)
        {
            return EXT2_SUCCESS;
        }
        else if (EXT2_NOT_FOUND == (result & EXT2_NOT_FOUND))
        {
            continue;
        }
        else
        {
            return EXT2_FAIL;
        }
    }

    return EXT2_FAIL | EXT2_NOT_FOUND;
}

/**
 * traverse double indirect blocks to find something called target
 * @param target: target's name
 * @param i_blocks: double indirect blocks
 * @param length: length of double indirect blocks
 * @param inode: where store result
 * @return success or not, finish or not
 */
int ext2_traverse_double_indirect_block(__u8 *target, const __u32 *i_blocks, int length, INODE *inode)
{
    __u8 buffer[4096];
    for (int i = 0; i < length; i++)
    {
        if (i_blocks[i] == 0)
        {
            return EXT2_FAIL;
        }

        sd_read_block(buffer,
                      fs_info.par_start_address + i_blocks[i] * (fs_info.block_size / 512),
                      fs_info.block_size / 512);
        __u32 *entries = (__u32 *)buffer;
        int result = ext2_traverse_indirect_block(target, entries, 1024, inode);
        if (EXT2_SUCCESS == result)
        {
            return EXT2_SUCCESS;
        }
        else if (EXT2_NOT_FOUND == (result & EXT2_NOT_FOUND))
        {
            continue;
        }
        else
        {
            return EXT2_FAIL;
        }
    }

    return EXT2_FAIL | EXT2_NOT_FOUND;
}

/**
 * traverse triple indirect blocks to find something called target
 * @param target: target's name
 * @param i_blocks: triple indirect blocks
 * @param length: length of triple indirect blocks
 * @param inode: where store result
 * @return success or not, finish or not
 */
int ext2_traverse_triple_indirect_block(__u8 *target, const __u32 *i_blocks, int length, INODE *inode)
{
    __u8 buffer[4096];
    for (int i = 0; i < length; i++)
    {
        if (i_blocks[i] == 0)
        {
            return EXT2_FAIL;
        }

        sd_read_block(buffer,
                      fs_info.par_start_address + i_blocks[i] * (fs_info.block_size / 512),
                      fs_info.block_size / 512);

        __u32 *entries = (__u32 *)buffer;
        int result = ext2_traverse_indirect_block(target, entries, 1024, inode);
        if (EXT2_SUCCESS == result)
        {
            return EXT2_SUCCESS;
        }
        else if (EXT2_NOT_FOUND == (result & EXT2_NOT_FOUND))
        {
            continue;
        }
        else
        {
            return EXT2_FAIL;
        }
    }

    return EXT2_FAIL | EXT2_NOT_FOUND;
}

/**
 * traverse 15 blocks of pointers to find target
 * @param target: target name
 * @param i_blocks: 15 blocks
 * @param inode: store result
 * @return success or not
 */
int ext2_traverse_block(__u8 *target, __u32 *i_blocks, INODE *inode)
{
    int result = ext2_traverse_direct_block(target, i_blocks, 12, inode);
    if (EXT2_NOT_FOUND == (result & EXT2_NOT_FOUND))
    {
        result = ext2_traverse_indirect_block(target, i_blocks + 12, 1, inode);
        if (EXT2_NOT_FOUND == (result & EXT2_NOT_FOUND))
        {
            result = ext2_traverse_double_indirect_block(target, i_blocks + 13, 1, inode);
            if (EXT2_NOT_FOUND == (result & EXT2_NOT_FOUND))
            {
                result = ext2_traverse_triple_indirect_block(target, i_blocks + 14, 1, inode);
                if (EXT2_SUCCESS == result)
                {
                    return EXT2_SUCCESS;
                }
                else
                {
                    return EXT2_FAIL;
                }
            }
            else if (EXT2_SUCCESS == result)
            {
                return EXT2_SUCCESS;
            }
            else
            {
                return EXT2_FAIL;
            }
        }
        else if (EXT2_SUCCESS == result)
        {
            return EXT2_SUCCESS;
        }
        else
        {
            return EXT2_FAIL;
        }
    }
    else if (EXT2_SUCCESS == result)
    {
        return EXT2_SUCCESS;
    }
    else
    {
        return EXT2_FAIL;
    }
}

/**
 * get root inode from disk
 * @param inode: store inode
 * @return success or not
 */
int ext2_get_root_inode(struct ext2_inode *inode)
{
    __u8 buffer[512];
    struct ext2_group_desc group_desc;
    sd_read_block(buffer, fs_info.group_desc_address, 1);
    ext2_group_desc_fill(&group_desc, buffer);

    // fine root inode
    sd_read_block(buffer,
                  fs_info.par_start_address + group_desc.bg_inode_table * fs_info.block_size / 512,
                  1);

    ext2_inode_fill(inode, buffer + fs_info.sb.s_inode_size);
    return EXT2_SUCCESS;
}

/**
 * append new directory to a direct block of an inode
 * @param block: direct block
 * @param len: block count
 * @param dir: new directory
 * @param inode: master inode
 * @return success or not
 */
int ext2_append_to_end_direct(__u32 *block, int len, struct ext2_dir_entry *dir, INODE *inode)
{
    int target;
    unsigned char buffer[4096];
    // find end - direct
    for (int i = 0; i < len; i++)
    {
        if (block[i] == 0)
        {
            // allocate a new block for it
            block[i] = ext2_new_block(inode, 1);
            if (block[i] == 0)
            {
                return EXT2_FAIL;
            }
            sd_read_block(buffer, fs_info.par_start_address + block[i] * 8, 8);
            for (int i = 0; i < 4096; i++)
            {
                buffer[i] = 0;
            }
        }
        else
        {
            sd_read_block(buffer, fs_info.par_start_address + block[i] * 8, 8);
        }

        int offset = 0;
        struct ext2_dir_entry temp;
        while (offset < 4096)
        {
            EXT2_FAIL == ext2_dir_entry_fill(&temp, buffer + offset);

            if (temp.rec_len == 0)
            {
                dir->rec_len = 4096;
                kernel_memcpy(buffer, dir, 8);
                kernel_memcpy(buffer + 8, dir->name, dir->name_len);
                sd_write_block(buffer, fs_info.par_start_address + block[i] * 8, 8);
                return EXT2_SUCCESS;
            }
            else if ((offset + temp.rec_len >= 4096 && 4096 - offset - ((temp.name_len + 8 - 1) / 4 + 1) * 4 >= dir->rec_len) ||
                     temp.rec_len - ((temp.name_len + 8 - 1) / 4 + 1) * 4 >= dir->rec_len)
            {
                dir->rec_len = temp.rec_len - (((8 + temp.name_len - 1) / 4 + 1) * 4);
                temp.rec_len = ((8 + temp.name_len - 1) / 4 + 1) * 4;
                kernel_memcpy(buffer + offset, &temp, 8);
                kernel_memcpy(buffer + offset + 8, &(temp.name), temp.name_len);
                kernel_memcpy(buffer + offset + temp.rec_len, dir, 8);
                kernel_memcpy(buffer + offset + temp.rec_len + 8, dir->name, dir->name_len);
                sd_write_block(buffer, fs_info.par_start_address + block[i] * 8, 8);
                return EXT2_SUCCESS;
            }
            else
            {
                offset += temp.rec_len;
            }
        }
    }

    return EXT2_FAIL | EXT2_NOT_FOUND;
}

/**
 * append new directory to a indirect block of an inode
 * @param block: direct block
 * @param len: block count
 * @param dir: new directory
 * @param inode: master inode
 * @return success or not
 */
int ext2_append_to_end_indirect(__u32 *block, int len, struct ext2_dir_entry *dir, INODE *inode)
{
    unsigned char buffer[4096];
    int result;
    for (int i = 0; i < len; i++)
    {
        if (block[i] == 0)
        {
            // allocate a new block for it
            block[i] = ext2_new_block(inode, 1);
            if (block[i] == 0)
            {
                return EXT2_FAIL;
            }
            sd_read_block(buffer, fs_info.par_start_address + block[i] * 8, 8);
            for (int i = 0; i < 4096; i++)
            {
                buffer[i] = 0;
            }
        }
        else
        {
            sd_read_block(buffer, fs_info.par_start_address + block[i] * 8, 8);
        }

        __u32 *buffer_32 = (__u32 *)buffer;
        result = ext2_append_to_end_direct(buffer_32, 1024, dir, inode);
        if ((result | EXT2_NOT_FOUND) == EXT2_NOT_FOUND)
        {
            continue;
        }
        else
        {
            return result;
        }
    }

    return EXT2_FAIL | EXT2_NOT_FOUND;
}

/**
 * append new directory to a DOUBLE indirect block of an inode
 * @param block: direct block
 * @param len: block count
 * @param dir: new directory
 * @param inode: master inode
 * @return success or not
 */
int ext2_append_to_end_double_indirect(__u32 *block, int len, struct ext2_dir_entry *dir, INODE *inode)
{
    unsigned char buffer[4096];
    int result;
    for (int i = 0; i < len; i++)
    {
        if (block[i] == 0)
        {
            // allocate a new block for it
            block[i] = ext2_new_block(inode, 1);
            if (block[i] == 0)
            {
                return EXT2_FAIL;
            }
            sd_read_block(buffer, fs_info.par_start_address + block[i] * 8, 8);
            for (int i = 0; i < 4096; i++)
            {
                buffer[i] = 0;
            }
        }
        else
        {
            sd_read_block(buffer, fs_info.par_start_address + block[i] * 8, 8);
        }

        __u32 *buffer_32 = (__u32 *)buffer;
        result = ext2_append_to_end_indirect(buffer_32, 1024, dir, inode);
        if ((result | EXT2_NOT_FOUND) == EXT2_NOT_FOUND)
        {
            continue;
        }
        else
        {
            return result;
        }
    }

    return EXT2_FAIL | EXT2_NOT_FOUND;
}

/**
 * append new directory to a TRIPLE indirect block of an inode
 * @param block: direct block
 * @param len: block count
 * @param dir: new directory
 * @param inode: master inode
 * @return success or not
 */
int ext2_append_to_end_triple_indirect(__u32 *block, int len, struct ext2_dir_entry *dir, INODE *inode)
{
    unsigned char buffer[4096];
    int result;
    for (int i = 0; i < len; i++)
    {
        if (block[i] == 0)
        {
            // allocate a new block for it
            block[i] = ext2_new_block(inode, 1);
            if (block[i] == 0)
            {
                return EXT2_FAIL;
            }
            sd_read_block(buffer, fs_info.par_start_address + block[i] * 8, 8);
            for (int i = 0; i < 4096; i++)
            {
                buffer[i] = 0;
            }
        }
        else
        {
            sd_read_block(buffer, fs_info.par_start_address + block[i] * 8, 8);
        }

        __u32 *buffer_32 = (__u32 *)buffer;
        result = ext2_append_to_end_double_indirect(buffer_32, 1024, dir, inode);
        if ((result | EXT2_NOT_FOUND) == EXT2_NOT_FOUND)
        {
            continue;
        }
        else
        {
            return result;
        }
    }

    return EXT2_FAIL | EXT2_NOT_FOUND;
}

/**
 * append new directory to an inode
 * @param inode: target
 * @param dir: new directory
 * @return success or not
 */
int ext2_append_to_end(INODE *inode, struct ext2_dir_entry *dir)
{
    int result = ext2_append_to_end_direct(inode->info.i_block, 12, dir, inode);
    if ((result & EXT2_NOT_FOUND) == EXT2_NOT_FOUND)
    {
        result = ext2_append_to_end_indirect(inode->info.i_block + 12, 1, dir, inode);
        if ((result & EXT2_NOT_FOUND) == EXT2_NOT_FOUND)
        {
            result = ext2_append_to_end_double_indirect(inode->info.i_block + 13, 1, dir, inode);
            if ((result & EXT2_NOT_FOUND) == EXT2_NOT_FOUND)
            {
                result = ext2_append_to_end_triple_indirect(inode->info.i_block + 14, 1, dir, inode);
                if ((result & EXT2_NOT_FOUND) == EXT2_NOT_FOUND)
                {
                    return EXT2_FAIL;
                }
                else
                {
                    return result;
                }
            }
            else
            {
                return result;
            }
        }
        else
        {
            return result;
        }
    }
    else
    {
        return result;
    }
}

/**
 * store inode back to inode table
 * @param inode: target
 * @return success or not
 */
int ext2_store_inode(INODE *inode)
{
    __u8 buffer[512];
    int bg_number = (inode->id - 1) / fs_info.sb.s_inodes_per_group;
    int number = (inode->id - 1) % fs_info.sb.s_inodes_per_group;
    sd_read_block(buffer, fs_info.group_desc_address + bg_number * 32 / 512, 1);
    struct ext2_group_desc group_desc;
    ext2_group_desc_fill(&group_desc, buffer + bg_number * 32 % 512);

    // bitmap
    sd_read_block(buffer, fs_info.par_start_address + group_desc.bg_inode_bitmap * 8 + number / 8 / 512, 1);
    buffer[number / 8 % 512] |= (1 << (number % 8));
    sd_write_block(buffer, fs_info.par_start_address + group_desc.bg_inode_bitmap * 8 + number / 8 / 512, 1);

    // inode table
    sd_read_block(buffer, fs_info.par_start_address + group_desc.bg_inode_table * 8 + number * 256 / 512, 1);
    kernel_memcpy(buffer + number * 256 % 512, &(inode->info), 256);
    sd_write_block(buffer, fs_info.par_start_address + group_desc.bg_inode_table * 8 + number * 256 / 512, 1);

    return EXT2_SUCCESS;
}
