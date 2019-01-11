//
// Created by zengkai on 2018/12/19.
//

#include "ext2.h"

/**
 * return the length of string
 * @param string: target string
 * @return length, -1 for FAIL
 */
int ext2_strlen(__u8 *string)
{
    int i = 0;
    while (string[i] != 0)
    {
        i++;
        if (i > 255)
        {
            return -1;
        }
    }
    return i;
}

/**
 * return whether a string is valid file name
 */
int ext2_is_valid_filename(__u8 *name)
{
    char self[2] = {'.', '\0'};
    char parent[3] = {'.', '.', '\0'};
    if (ext2_is_equal(name, self))
    {
        return EXT2_FALSE;
    }
    if (ext2_is_equal(name, parent))
    {
        return EXT2_FALSE;
    }

    int i = 0;
    while (name[i] != '\0')
    {
        if (name[i] == '/')
        {
            return EXT2_FALSE;
        }
        if (name[i] == '\\')
        {
            return EXT2_FALSE;
        }
        i++;
    }

    if (i == 0)
    {
        return EXT2_FALSE;
    }

    if (i > 255)
    {
        return EXT2_FALSE;
    }

    unsigned char path[255];
    path[0] = '/';
    kernel_memcpy(path + 1, name, ext2_strlen(name));
    path[ext2_strlen(name) + 1] = '\0';
    INODE inode;
    if (EXT2_SUCCESS == ext2_find_file_relative(path, &inode))
    {
        return EXT2_FALSE;
    }

    return EXT2_TRUE;
}

/**
 * count next directory name's length
 * @param path
 * @param current_ptr: this will be UPDATED to the end of the next directory
 * @return return the length of next directory's name
 *      (0 stands for next directory does no exist)
 *      (-1 stands for error happened)
 */
short ext2_count_next_dir_name(const __u8 *path, short *current_ptr)
{
    short ptr;
    kernel_memcpy(&ptr, current_ptr, sizeof(short));

    if (path[ptr] == '\0')
    {
        return 0;
    }
    else if (path[ptr] == '/')
    {
        ptr++;
    }
    else
    {
        return -1;
    }

    short count = 0;
    while (path[ptr] != '/' && path[ptr] != '\0')
    {
        count++;
        ptr++;
    }

    kernel_memcpy(current_ptr, &ptr, sizeof(short));

    return count;
}

/**
 * main loop of finding something
 * @param path: path, relative or absolute
 * @param sub_path: the start of traversing, determining relative or absolute
 * @param inode: result
 * @return success or not
 */
int ext2_find_file_loop(__u8 *path, struct ext2_path *sub_path, INODE *inode)
{
    short length;
    short name_ptr = 0;
    INODE parent_inode;

    // initialize parent inode
    if (sub_path->inode.id == 0)
    {
        kernel_memcpy(&parent_inode, &(current_dir.inode), sizeof(INODE));
    }
    else
    {
        kernel_memcpy(&parent_inode, &(sub_path->inode), sizeof(INODE));
    }

    while ((length = ext2_count_next_dir_name(path, &name_ptr)) > 0)
    {
        // copy name
        for (int i = 0; i < length; i++)
        {
            sub_path->name[length - 1 - i] = path[name_ptr - 1 - i];
        }
        sub_path->name[length] = '\0';

        // if parent inode's type is not directory
        if ((parent_inode.info.i_mode & EXT2_S_IFDIR) == 0)
        {
            return EXT2_FAIL;
        }

        // traverse blocks
        if (EXT2_FAIL == ext2_traverse_block(
                             sub_path->name, parent_inode.info.i_block, &(sub_path->inode)))
        {
            return EXT2_FAIL;
        }
        else
        {
            kernel_memcpy(&parent_inode, &(sub_path->inode), sizeof(INODE));
            continue;
        }
    }

    // success
    kernel_memcpy(inode, &sub_path->inode, sizeof(INODE));
    return EXT2_SUCCESS;
}

/**
 * find a file or directory by relative path
 * @param path: length(path) <= 255
 * @param inode: where store the result
 * @return success or not
 */
int ext2_find_file_relative(__u8 *path, INODE *inode)
{
    struct ext2_path sub_path;
    sub_path.inode.id = 0;
    if (EXT2_SUCCESS == ext2_find_file_loop(path, &sub_path, inode))
    {
        return EXT2_SUCCESS;
    }
    else
    {
        return EXT2_FAIL;
    }
}

/**
 * find a file or directory by absolute path
 * @param path: length(path) <= 255
 * @param inode: where store the result
 * @return success or not
 */
int ext2_find_file_absolute(__u8 *path, INODE *inode)
{
    struct ext2_path sub_path;
    sub_path.inode.id = 2;
    ext2_get_root_inode(&(sub_path.inode.info));

    if (EXT2_SUCCESS == ext2_find_file_loop(path, &sub_path, inode))
    {
        return EXT2_SUCCESS;
    }
    else
    {
        return EXT2_FAIL;
    }
}

/**
 * find a file or directory by relative or absolute path
 * @param path: length(path) <= 255
 * @param inode: where store the result
 * @return success or not
 */
int ext2_find(__u8 *path, INODE *inode)
{
    if (path[0] != '/')
    {
        return EXT2_FAIL;
    }

    // find file by relative path
    if (EXT2_SUCCESS == ext2_find_file_relative(path, inode))
    {
        return EXT2_SUCCESS;
    }

    // find file by absolute path
    if (EXT2_SUCCESS == ext2_find_file_absolute(path, inode))
    {
        return EXT2_SUCCESS;
    }

    return EXT2_FAIL;
}

/**
 * open file
 * @param file
 * @param path
 * @return success or not
 */
int ext2_open(__u8 *path, EXT2_FILE *file)
{
    if (EXT2_FAIL == ext2_find(path, &(file->inode)))
    {
        return EXT2_FAIL;
    }

    if (EXT2_S_IFREG != file->inode.info.i_mode & EXT2_S_IFREG)
    {
        return EXT2_FAIL;
    }

    file->pointer = 0;
    file->dirty = EXT2_NOT_DIRTY;

    if (file->inode.info.i_block[0] == 0)
    {
        return EXT2_FAIL;
    }

    sd_read_block(
        file->buffer,
        fs_info.par_start_address + file->inode.info.i_block[0] * (fs_info.block_size / 512),
        fs_info.block_size / 512);

    return EXT2_SUCCESS;
}

/**
 * close a file, write data back to sd card if it is dirty
 * @param file: file to close
 * @return success or not
 */
int ext2_close(EXT2_FILE *file)
{
    if (file->dirty == EXT2_DIRTY)
    {
        if (ext2_write_block(file) == EXT2_FAIL)
        {
            return EXT2_FAIL;
        }
    }
    return EXT2_SUCCESS;
}

/**
 * move file pointer
 * @param file: file structure
 * @param new_pointer: new location; illegal location leads to fail
 * @return success or not
 */
int ext2_lseek(EXT2_FILE *file, __u32 new_pointer)
{
    if (file == EXT2_NULL)
    {
        return EXT2_FAIL;
    }

    if (file->pointer / fs_info.block_size == new_pointer / fs_info.block_size)
    {
        file->pointer = new_pointer;
        return EXT2_SUCCESS;
    }

    if (new_pointer >= file->inode.info.i_size)
    {
        return EXT2_FAIL;
    }

    if (file->dirty == EXT2_DIRTY)
    {
        if (EXT2_FAIL == ext2_write_block(file))
        {
            return EXT2_FAIL;
        }
    }

    file->pointer = new_pointer;
    if (EXT2_FAIL == ext2_read_block(file))
    {
        return EXT2_FAIL;
    }

    return EXT2_SUCCESS;
}

/**
 * read bytes from file
 * @param file: specific file
 * @param buffer: store data
 * @param length: length of data you want to read
 * @param bytes_of_read: length of data successfully read
 * @return success or not
 */
int ext2_read(EXT2_FILE *file, __u8 *buffer, __u64 length, __u64 *bytes_of_read)
{
    if (file == EXT2_NULL)
    {
        return EXT2_FAIL;
    }

    // if the length is longer than the bytes left
    if (length > file->inode.info.i_size - file->pointer)
    {
        length = file->inode.info.i_size - file->pointer;
    }

    __u64 count = 0;
    while (count < length)
    {
        int i = 0;
        for (i = file->pointer % fs_info.block_size; i < fs_info.block_size && count < length; i++)
        {
            buffer[count++] = file->buffer[i];
        }
        if (i == fs_info.block_size)
        {
            if (EXT2_FAIL == ext2_lseek(file, (file->pointer / fs_info.block_size + 1) * fs_info.block_size))
            {
                return EXT2_FAIL;
            }
        }
    }

    if (bytes_of_read != EXT2_NULL)
    {
        kernel_memcpy(bytes_of_read, &count, sizeof(__u64));
    }
    return EXT2_SUCCESS;
}

/**
 * write bytes to file
 * @param file: specific file
 * @param buffer: store data
 * @param length: length of data you want to write
 * @param bytes_of_read: length of data successfully write
 * @return success or not
 */
int ext2_write(EXT2_FILE *file, __u8 *buffer, __u64 length, __u64 *bytes_of_write)
{
    if (file == EXT2_NULL)
    {
        return EXT2_FAIL;
    }

    // if the length is longer than the bytes left
    if (length > file->inode.info.i_size - file->pointer)
    {
        length = file->inode.info.i_size - file->pointer;
    }

    __u64 count = 0;
    while (count < length)
    {
        int i = 0;
        for (i = file->pointer % fs_info.block_size; i < fs_info.block_size && count < length; i++)
        {
            file->buffer[i] = buffer[count++];
            file->dirty = EXT2_DIRTY;
        }
        if (i == fs_info.block_size)
        {
            if (EXT2_FAIL == ext2_lseek(file, (file->pointer / fs_info.block_size + 1) * fs_info.block_size))
            {
                return EXT2_FAIL;
            }
        }
    }

    if (bytes_of_write != EXT2_NULL)
    {
        (*bytes_of_write) = count;
    }
    return EXT2_SUCCESS;
}

/**
 * remove file or directory by inode RECURSIVELY
 * @param inode
 * @return success or not
 */
int ext2_rm_inode(INODE inode)
{
    if ((inode.info.i_mode & EXT2_S_IFDIR) != 0)
    {
        // not a directory
        ext2_release_blocks(inode.info.i_block);
        ext2_set_inode_free(inode.id);
    }
    else
    {
        // it is a directory
        ext2_traverse_block_rm(inode.info.i_block);
        ext2_set_inode_free(inode.id);
    }
    return EXT2_SUCCESS;
}

/**
 * determine whether a file can be removed
 * @para name
 * @return true or false
 */
int ext2_is_removable(__u8 *name)
{
    char self[2] = {'.', '\0'};
    char parent[3] = {'.', '.', '\0'};
    char lf[11] = {'l', 'o', 's', 't', '+', 'f', 'o', 'u', 'n', 'd', '\0'};

    if (ext2_is_equal(name, self))
    {
        return EXT2_FALSE;
    }

    if (ext2_is_equal(name, parent))
    {
        return EXT2_FALSE;
    }

    if (ext2_is_equal(name, lf))
    {
        return EXT2_FALSE;
    }

    return EXT2_TRUE;
}