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
 * make directory API provided to outside
 */
int ext2_mkdir(__u8 *name)
{
    return ext2_mkdir_plus(name, &(current_dir.inode), EXT2_NULL, EXT2_TRUE);
}

/**
 * make directory
 * @param name: file name
 * @return success or not
 */
int ext2_mkdir_plus(__u8 *name, INODE *parent, INODE *result, int check_file_name)
{
    if (EXT2_TRUE == check_file_name)
    {
        if (!ext2_is_valid_filename(name))
        {
            log(LOG_FAIL, "Invalid file name: %s", name);
            return EXT2_FAIL;
        }
    }

    // Orlov
    int inode_id;
    struct ext2_group_desc group_desc;
    int group_desc_id;
    if (parent->id == 2)
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
        group_desc_id = (parent->id - 1) / fs_info.sb.s_inodes_per_group;
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
    if (EXT2_SUCCESS != ext2_append_to_end(parent, &dir))
    {
        return EXT2_FAIL;
    }

    // store back to disk
    parent->info.i_links_count += 1;
    if (EXT2_FAIL == ext2_store_inode(parent))
    {
        return EXT2_FAIL;
    }

    // store INODE
    INODE inode_new;
    kernel_memcpy(&(inode_new.info), &(parent->info), sizeof(struct ext2_inode_info));
    inode_new.id = dir.inode;
    inode_new.info.i_size = 0;
    inode_new.info.i_links_count = 2;
    inode_new.info.i_blocks = 0;
    for (int i = 0; i < 15; i++)
    {
        inode_new.info.i_block[i] = 0;
    }

    if (EXT2_FAIL == ext2_store_inode(&inode_new))
    {
        // occupy the inode to prevent from that it be allocate again
        return EXT2_FAIL;
    }

    // create . and ..
    if (EXT2_FAIL == ext2_new_block(&inode_new, 1))
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
    if (EXT2_SUCCESS != ext2_append_to_end(&inode_new, &_dir))
    {
        return EXT2_FAIL;
    }

    _dir.name[1] = '.';
    _dir.name[2] = '\0';
    _dir.inode = parent->id;
    _dir.rec_len = 12;
    _dir.name_len = 2;
    if (EXT2_SUCCESS != ext2_append_to_end(&inode_new, &_dir))
    {
        return EXT2_FAIL;
    }

    if (EXT2_FAIL == ext2_store_inode(&inode_new))
    {
        return EXT2_FAIL;
    }

    if (result != EXT2_NULL)
    {
        kernel_memcpy(result, &inode_new, sizeof(INODE));
    }
    kernel_printf("%s created\n", name);
    return EXT2_SUCCESS;
}

int ext2_create(__u8 *param)
{
    return ext2_create_plus(param, &(current_dir.inode), EXT2_NULL, EXT2_TRUE);
}

/**
 * create file
 * syntax: create [filename] [content]
 * @param param: filename and content, split by ' '
 * @return success or not
 */
int ext2_create_plus(__u8 *param, INODE *parent, INODE *result, int check_file_name)
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

    INODE inode;
    if (EXT2_TRUE == check_file_name)
    {
        if (!ext2_is_valid_filename(filename))
        {
            log(LOG_FAIL, "Invalid file name: %s", filename);
            return EXT2_FAIL;
        }

        if (EXT2_SUCCESS == ext2_traverse_block(filename, current_dir.inode.info.i_block, &inode))
        {
            log(LOG_FAIL, "%s exists.", filename);
            return EXT2_FAIL;
        }
    }

    // new inode
    __u8 buffer[512];
    sd_read_block(
        buffer,
        fs_info.group_desc_address + ((parent->id - 1) / fs_info.sb.s_inodes_per_group) * 32 / 512, 1);
    struct ext2_group_desc group_desc;
    ext2_group_desc_fill(&group_desc,
                         buffer + ((parent->id - 1) / fs_info.sb.s_inodes_per_group) * 32 % 512);
    inode.id = ext2_find_free_inode(&group_desc) +
               ((parent->id - 1) / fs_info.sb.s_inodes_per_group) * fs_info.sb.s_inodes_per_group + 1;

    // copy other message from parent
    kernel_memcpy(&(inode.info), &(parent->info), sizeof(struct ext2_inode));
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
    if (EXT2_SUCCESS != ext2_append_to_end(parent, &dir))
    {
        return EXT2_FAIL;
    }

    // store back to disk
    if (EXT2_FAIL == ext2_store_inode(parent))
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

    if (result != EXT2_NULL)
    {
        kernel_memcpy(result, &inode, sizeof(INODE));
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
    if (EXT2_FAIL == ext2_traverse_block(
                         param, current_dir.inode.info.i_block, &(inode)))
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
    if (EXT2_FAIL == ext2_rm_inode(inode))
    {
        log(LOG_FAIL, "Failed to remove inode.");
        return EXT2_FAIL;
    }

    // remove dir entry from current inode
    if (EXT2_FAIL == ext2_rm_dir_entry(param, &current_dir.inode, EXT2_NULL))
    {
        log(LOG_FAIL, "Failed to remove directory entry.");
        return EXT2_FAIL;
    }

    return EXT2_SUCCESS;
}

/**
 * decode the param provided by powershell
 */
int ext2_param_decode(__u8 *param, __u8 *src, __u8 *target, __u8 *dest, __u8 *target_rename)
{
    int count = 0;
    int internal = 0;
    int i;

    for (i = 0; i < 64; i++)
    {
        if (param[i] == ' ' || param[i] == '\0')
        {
            src[count - internal - 1] = '\0';
            target[internal] = '\0';
            i++;
            break;
        }
        else
        {
            src[count++] = param[i];
            if (param[i] == '/')
            {
                internal = 0;
            }
            else
            {
                target[internal++] = param[i];
            }
        }
    }

    for (count = 0; i < 64; i++)
    {
        if (param[i] == ' ' || param[i] == '\0')
        {
            target_rename[internal] = '\0';
            i++;
            break;
        }
        else
        {
            dest[count++] = param[i];
            if (param[i] == '/')
            {
                internal = 0;
            }
            else
            {
                target_rename[internal++] = param[i];
            }
        }
    }

    INODE inode;
    dest[count] = '\0';
    if (EXT2_SUCCESS == ext2_find(dest, &inode))
    {
        log(LOG_FAIL, "%s exists.", dest);
        return EXT2_FAIL;
    }
    dest[count - internal - 1] = '\0';
    return EXT2_SUCCESS;
}

/**
 * move an inode from source to destination (absolute path)
 * you can rename it when mv
 * @param param: source and destination
 * @return success or not
 */
int ext2_mv(__u8 *param)
{
    __u8 src[64];
    __u8 dest[64];
    __u8 target[64];
    __u8 target_rename[64];

    if (EXT2_FAIL == ext2_param_decode(param, src, target, dest, target_rename))
    {
        return EXT2_FAIL;
    }

    INODE inode;
    __u8 dest_target[64];
    int length = ext2_strlen(dest);
    kernel_memcpy(dest_target, dest, length);
    dest_target[length] = '/';
    int length2 = ext2_strlen(target_rename);
    kernel_memcpy(dest_target + length + 1, target_rename, length2);
    dest_target[length + length2 + 1] = '\0';
    log(LOG_STEP, "%s", dest_target);

    if (EXT2_SUCCESS == ext2_find(dest_target, &inode))
    {
        log(LOG_FAIL, "%s exists.", dest_target);
        return EXT2_FAIL;
    }

    if (EXT2_FAIL == ext2_find(src, &inode))
    {
        log(LOG_FAIL, "Cannot find %s", src);
        return EXT2_FAIL;
    }

    struct ext2_dir_entry dir;
    if (EXT2_FAIL == ext2_rm_dir_entry(target, &inode, &dir))
    {
        log(LOG_FAIL, "Cannot remove dentry from %s", src);
        return EXT2_FAIL;
    }

    if (EXT2_FAIL == ext2_find(dest, &inode))
    {
        log(LOG_FAIL, "Cannot find %s", dest);
        return EXT2_FAIL;
    }

    length = ext2_strlen(target_rename);
    kernel_memcpy(&(dir.name), target_rename, length);
    dir.name_len = length;
    dir.name[length] = '\0';
    dir.rec_len = ((dir.name_len + 8 - 1) / 4 + 1) * 4;

    if (EXT2_FAIL == ext2_append_to_end(&inode, &dir))
    {
        log(LOG_FAIL, "Cannot append dentry to %s", dest);
        return EXT2_FAIL;
    }

    log(LOG_OK, "Done");
    return EXT2_SUCCESS;
}

/**
 * copy an inode from source to destination
 * you can rename it when copy
 * @param param: source and destination
 * @return success or not
 */
int ext2_cp(__u8 *param)
{
    __u8 src[64];
    __u8 dest[64];
    __u8 target[64];
    __u8 target_rename[64];

    if (EXT2_FAIL == ext2_param_decode(param, src, target, dest, target_rename))
    {
        return EXT2_FAIL;
    }

    src[ext2_strlen(src)] = '/';
    INODE inode;

    __u8 dest_target[64];
    int length = ext2_strlen(dest);
    kernel_memcpy(dest_target, dest, length);
    dest_target[length] = '/';
    int length2 = ext2_strlen(target_rename);
    kernel_memcpy(dest_target + length + 1, target_rename, length2);
    dest_target[length + length2 + 1] = '\0';
    if (EXT2_SUCCESS == ext2_find_file_absolute(dest_target, &inode))
    {
        log(LOG_FAIL, "%s exists.", dest_target);
        return EXT2_FAIL;
    }

    if (EXT2_FAIL == ext2_find(src, &inode))
    {
        log(LOG_FAIL, "Cannot find %s", src);
        return EXT2_FAIL;
    }

    INODE inode_dest;
    if (EXT2_FAIL == ext2_find(dest, &inode_dest))
    {
        log(LOG_FAIL, "Cannot find %s", dest);
        return EXT2_FAIL;
    }

    if (EXT2_FAIL == ext2_cp_i2i(&inode, &inode_dest, target_rename))
    {
        log(LOG_FAIL, "Fail to copy.");
        return EXT2_FAIL;
    }

    return EXT2_SUCCESS;
}

int ext2_rename(__u8 *param)
{
    __u8 old_name[64];
    __u8 new_name[64];
    int count = 0;
    int i = 0;

    for (i = 0; i < 64; i++)
    {
        if (param[i] == ' ' || param[i] == '\0')
        {
            i++;
            old_name[count] = '\0';
            break;
        }
        else
        {
            old_name[count++] = param[i];
        }
    }
    for (count = 0; i < 64; i++)
    {
        if (param[i] == ' ' || param[i] == '\0')
        {
            i++;
            new_name[count] = '\0';
            break;
        }
        else
        {
            new_name[count++] = param[i];
        }
    }

    if (EXT2_FALSE == ext2_is_valid_filename(new_name))
    {
        log(LOG_FAIL, "Invalid name.");
        return EXT2_FAIL;
    }

    struct ext2_dir_entry dir;
    if (EXT2_FAIL == ext2_rm_dir_entry(old_name, &(current_dir.inode), &dir))
    {
        log(LOG_FAIL, "Cannot find that file");
        return EXT2_FAIL;
    }

    int length = ext2_strlen(new_name);
    kernel_memcpy(dir.name, new_name, length);
    dir.name[length] = '\0';
    dir.name_len = length;
    dir.rec_len = ((dir.name_len + 8 - 1) / 4 + 1) * 4;
    if (EXT2_FAIL == ext2_append_to_end(&(current_dir.inode), &dir))
    {
        log(LOG_FAIL, "Cannot add new entry");
        return EXT2_FAIL;
    }

    return EXT2_SUCCESS;
}