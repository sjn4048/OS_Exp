//
// Created by zengkai on 2019/1/1.
//

#include "ext2.h"

/**
 * output file to screen
 * if the file is longer than 200 bytes, only the first 200 charactor will be printed
 * @param path: path to the file to print, relative or absolute
 * @return success or not
 */
int ext2_cat(__u8 *path)
{
    __u8 path_buffer[64];
    int count = 0;
    for (int i = 0; i < 64; i++)
    {
        if (path[i] != ' ' && path[i] != '\0')
        {
            path_buffer[count++] = path[i];
        }
        else
        {
            path_buffer[count] = '\0';
            break;
        }
    }

    // open it
    EXT2_FILE file;
    if (EXT2_FAIL == ext2_open(path_buffer, &file))
    {
        log(LOG_FAIL, "Cannot find file located at %s", path_buffer);
        return EXT2_FAIL;
    }

    if ((file.inode.info.i_mode & EXT2_S_IFREG) == 0)
    {
        log(LOG_FAIL, "%s is not a regular file and cannot be cat", path_buffer);
        return EXT2_FAIL;
    }

    int start = 0;
    int length = 0;
    if (path[count++] == ' ')
    {
        while (path[count] >= '0' && path[count] <= '9')
        {
            start = start * 10 + path[count++] - '0';
        }

        while (path[count] < '0' || path[count] > '9')
        {
            count++;
        }

        while (path[count] >= '0' && path[count] <= '9')
        {
            length = length * 10 + path[count++] - '0';
        }
    }
    else
    {
        length = file.inode.info.i_size;
    }

    if (length > 300)
    {
        length = 300;
    }
    if (length == 0)
    {
        log(LOG_FAIL, "Length = 0");
        return EXT2_FAIL;
    }

    log(LOG_STEP, "cat path=%s, start=%d, length=%d", path_buffer, start, length);

    if (file.inode.info.i_size < start)
    {
        log(LOG_FAIL, "This is file is %d bytes long, and your want to read starting at %d.",
            file.inode.info.i_size, start);
        return EXT2_FAIL;
    }

    __u8 *buffer;
    if (start + length > file.inode.info.i_size)
    {
        length = file.inode.info.i_size - start;
    }
    buffer = (__u8 *)kmalloc(length + 1);

    // move pointer
    if (EXT2_FAIL == ext2_lseek(&file, start))
    {
        log(LOG_FAIL, "Cannot move pointer.");
        return EXT2_FAIL;
    }

    __u64 bytes_of_read;
    if (EXT2_FAIL == ext2_read(&file, buffer, length, &bytes_of_read) ||
        length != bytes_of_read)
    {
        log(LOG_FAIL, "Failed to read data from %s", path_buffer);
    }

    buffer[length] = '\0';
    kernel_printf("%s\n", buffer);
    kernel_printf("-------- Totally read %d bytes of data. --------\n", bytes_of_read);
    kfree(buffer);
    return 0;
}

/**
 * list all files and directories in current directory
 * @return success or not
 */
int ext2_ls()
{
    return ext2_traverse_block(EXT2_NULL, current_dir.inode.info.i_block, EXT2_NULL);
}

/**
 * dive in to a directory, if it is a directory
 * @param path: path to the directory, relative or absolute
 * @return success or not
 */
int ext2_cd(__u8 *path)
{
    INODE inode;
    if (EXT2_SUCCESS != ext2_find(path, &inode))
    {
        log(LOG_FAIL, "Cannot find %s", path);
        return EXT2_FAIL;
    }

    if ((inode.info.i_mode & EXT2_S_IFDIR) == 0)
    {
        log(LOG_FAIL, "%s is not a directory and cannot be cd", path);
        return EXT2_FAIL;
    }

    kernel_memcpy(&(current_dir.inode), &inode, sizeof(INODE));
    log(LOG_OK, "Done, inode number: %d", current_dir.inode.id);
    return EXT2_SUCCESS;
}

/**
 * make directory
 * @param name: file name
 * @return success or not
 */
int ext2_mkdir(__u8 *name)
{
    if (!ext2_is_valid_filename(name))
    {
        log(LOG_FAIL, "Invalid file name: %s", name);
        return EXT2_FAIL;
    }

    // Orlov
    int inode_id;
    struct ext2_group_desc group_desc;
    int group_desc_id;
    if (current_dir.inode.id == 2)
    {
        if (EXT2_FAIL == ext2_find_sparse_bg(&group_desc, &group_desc_id))
        {
            return EXT2_FAIL;
        }

        if ((inode_id = ext2_find_free_inode(&group_desc)) == -1)
        {
            return EXT2_FAIL;
        }
    }
    else
    {
        group_desc_id = (current_dir.inode.id - 1) / fs_info.sb.s_inodes_per_group;
        unsigned char buffer[512];
        sd_read_block(buffer, fs_info.group_desc_address + group_desc_id * 32 / 512, 1);
        ext2_group_desc_fill(&group_desc, buffer + group_desc_id * 32 % 512);
        if ((inode_id = ext2_find_free_inode(&group_desc)) == -1)
        {
            return EXT2_FAIL;
        }
    }
    inode_id = group_desc_id * fs_info.sb.s_inodes_per_group + inode_id + 1;

    // conduct directory entry
    struct ext2_dir_entry dir;
    dir.inode = inode_id;
    dir.name_len = ext2_strlen(name);
    if (dir.name_len == -1)
    {
        return EXT2_FAIL;
    }
    kernel_memcpy(dir.name, name, dir.name_len);
    dir.name[dir.name_len] = '\0';
    dir.rec_len = ((dir.name_len + 8 - 1) / 4 + 1) * 4;
    dir.file_type = EXT2_FT_DIR;

    // append to parent
    if (EXT2_SUCCESS != ext2_append_to_end(&current_dir.inode, &dir))
    {
        return EXT2_FAIL;
    }

    // store back to disk
    current_dir.inode.info.i_links_count += 1;
    if (EXT2_FAIL == ext2_store_inode(&(current_dir.inode)))
    {
        return EXT2_FAIL;
    }

    // store INODE
    INODE inode;
    kernel_memcpy(&(inode.info), &(current_dir.inode.info), sizeof(struct ext2_inode_info));
    inode.id = dir.inode;
    inode.info.i_size = 0;
    inode.info.i_links_count = 2;
    inode.info.i_blocks = 0;
    for (int i = 0; i < 15; i++)
    {
        inode.info.i_block[i] = 0;
    }

    if (EXT2_FAIL == ext2_store_inode(&inode))
    {
        // occupy the inode to prevent from that it be allocate again
        return EXT2_FAIL;
    }

    // create . and ..
    if (EXT2_FAIL == ext2_new_block(&inode, 1))
    {
        return EXT2_FAIL;
    }
    struct ext2_dir_entry _dir;
    _dir.inode = dir.inode;
    _dir.name_len = 1;
    _dir.name[0] = '.';
    _dir.name[1] = '\0';
    _dir.file_type = EXT2_FT_DIR;
    _dir.rec_len = 12;
    if (EXT2_SUCCESS != ext2_append_to_end(&inode, &_dir))
    {
        return EXT2_FAIL;
    }

    _dir.name[1] = '.';
    _dir.name[2] = '\0';
    _dir.inode = current_dir.inode.id;
    _dir.rec_len = 12;
    _dir.name_len = 2;
    if (EXT2_SUCCESS != ext2_append_to_end(&inode, &_dir))
    {
        return EXT2_FAIL;
    }

    if (EXT2_FAIL == ext2_store_inode(&inode))
    {
        return EXT2_FAIL;
    }

    kernel_printf("%s created\n", name);
    return EXT2_SUCCESS;
}

/**
 * create file
 * syntax: create [filename] [content]
 * @param param: filename and content, split by ' '
 * @return success or not
 */
int ext2_create(__u8 *param)
{
    char filename[64];
    char content[64]; // 64 because of ps.c

    // get filename
    int i = 0;
    int j = 0;
    int is_empty = EXT2_FALSE;
    while (i < 64 && param[i] != ' ' && param[i] != '\0')
    {
        filename[j++] = param[i++];
    }
    filename[j] = '\0';

    if (param[i] != '\0')
    {
        // get content
        i++;
        j = 0;
        while (i < 64 && param[i] != '\0')
        {
            content[j++] = param[i++];
        }
        content[j] = '\0';
    }
    else
    {
        is_empty = EXT2_TRUE;
    }

    // new inode
    __u8 buffer[512];
    INODE inode;
    sd_read_block(
        buffer,
        fs_info.group_desc_address + ((current_dir.inode.id - 1) / fs_info.sb.s_inodes_per_group) * 32 / 512, 1);
    struct ext2_group_desc group_desc;
    ext2_group_desc_fill(&group_desc,
                         buffer + ((current_dir.inode.id - 1) / fs_info.sb.s_inodes_per_group) * 32 % 512);
    inode.id = ext2_find_free_inode(&group_desc) +
               ((current_dir.inode.id - 1) / fs_info.sb.s_inodes_per_group) * fs_info.sb.s_inodes_per_group + 1;

    // copy other message from parent
    kernel_memcpy(&(inode.info), &(current_dir.inode.info), sizeof(struct ext2_inode));
    inode.info.i_mode &= 0x0fff;              // clear file format info
    inode.info.i_mode |= EXT2_S_IFREG;        // set it to a regular file
    inode.info.i_size = ext2_strlen(content); // clear all blocks
    inode.info.i_links_count = 1;             // 1 for himself
    inode.info.i_blocks = 0;                  // no block belong to him
    for (int i = 0; i < 15; i++)
    {
        inode.info.i_block[i] = 0;
    }

    // allocate new block for the inode
    if (EXT2_FAIL == ext2_new_block(&inode, 1))
    {
        return EXT2_FAIL;
    }

    // store the inode
    if (EXT2_FAIL == ext2_store_inode(&inode))
    {
        return EXT2_FAIL;
    }

    // conduct directory entry
    struct ext2_dir_entry dir;
    dir.inode = inode.id;
    dir.name_len = ext2_strlen(filename);
    if (dir.name_len == -1)
    {
        return EXT2_FAIL;
    }
    kernel_memcpy(dir.name, filename, dir.name_len);
    dir.name[dir.name_len] = '\0';
    dir.rec_len = ((dir.name_len + 8 - 1) / 4 + 1) * 4;
    dir.file_type = EXT2_FT_REG_FILE;

    // append to parent
    if (EXT2_SUCCESS != ext2_append_to_end(&current_dir.inode, &dir))
    {
        return EXT2_FAIL;
    }

    // store back to disk
    if (EXT2_FAIL == ext2_store_inode(&(current_dir.inode)))
    {
        return EXT2_FAIL;
    }

    // if we add some content to the inode
    if (!is_empty)
    {
        sd_read_block(buffer, fs_info.par_start_address + inode.info.i_block[0] * 8, 1);
        kernel_memcpy(buffer, content, j);
        sd_write_block(buffer, fs_info.par_start_address + inode.info.i_block[0] * 8, 1);
    }

    return EXT2_SUCCESS;
}

/**
 * remove file or directory RECURSIVELY
 * @param param: the file you want to remove (relative path only)
 * @return success or not
 */
int ext2_rm(__u8 *param)
{
    INODE inode;
    if (EXT2_FAIL == ext2_find_file_relative(param, &inode))
    {
        log(LOG_FAIL, "Cannot find %s", param);
        return EXT2_FAIL;
    }

    if (EXT2_FALSE == ext2_is_removable(param))
    {
        log(LOG_FAIL, "%s cannot be removed.", param);
        return EXT2_FAIL;
    }

    // remove
    if (EXT2_FAIL == ext2_rm_inode(inode)) {
        log(LOG_FAIL, "Failed to remove that.");
        return EXT2_FAIL;
    }

    // remove dir entry from current inode
    if (EXT2_FAIL == ext2_rm_dir_entry(param)) {
        log(LOG_FAIL, "Failed to remove that.");
        return EXT2_FAIL;
    }

    return EXT2_SUCCESS;
}