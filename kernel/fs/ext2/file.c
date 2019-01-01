//
// Created by zengkai on 2018/12/19.
//

#include "ext2.h"

/**
 * count next directory name's length
 * @param path
 * @param current_ptr: this will be UPDATED to the end of the next directory
 * @return return the length of next directory's name
 *      (0 stands for next directory does no exist)
 *      (-1 stands for error happened)
 */
short count_next_dir_name(const __u8 *path, short *current_ptr) {
    if (path[*current_ptr] == '\0') {
        return 0;
    } else if (path[*current_ptr] == '/') {
        (*current_ptr)++;
    } else {
        return -1;
    }

    short count = 0;
    while (path[*current_ptr] != '/' && path[*current_ptr] != '\0') {
        count++;
        (*current_ptr)++;
    }

    return count;
}

/**
 * free the memory which allocated when traversing the path
 * @param sub_path: structures
 */
void clean_sub_path(struct ext2_path *sub_path) {
    while (sub_path->parent != NULL) {
        if (sub_path->name != NULL) {
            memory.free(sub_path->name);
        }
        sub_path = sub_path->parent;
        memory.free(sub_path->child);
    }
    if (sub_path->name != NULL) {
        memory.free(sub_path->name);
    }
    memory.free(sub_path);
}

/**
 * main loop of finding something
 * @param path: path, relative or absolute
 * @param sub_path: the start of traversing, determining relative or absolute
 * @param inode: result
 * @return success or not
 */
int find_file_loop(__u8 *path, struct ext2_path *sub_path, INODE *inode) {
    short length;
    short name_ptr = 0;
    while ((length = count_next_dir_name(path, &name_ptr)) > 0) {
        // new structure
        if (sub_path == NULL) {
            // the first directory
            sub_path = (struct ext2_path *) memory.allocate(sizeof(struct ext2_path));
            if (sub_path == NULL) {
                debug_cat(DEBUG_ERROR, "find_file_relative: failed due to memory error.\n");
                goto fail;
            }
            sub_path->parent = NULL;
            sub_path->child = NULL;
        } else {
            // if this is not the first directory, we need to perform some operations on pointers
            sub_path->child = (struct ext2_path *) memory.allocate(sizeof(struct ext2_path));
            if (sub_path->child == NULL) {
                debug_cat(DEBUG_ERROR, "find_file_relative: failed due to memory error.\n");
                goto fail;
            }
            sub_path->child->parent = sub_path;
            sub_path->child->child = NULL;
            sub_path = sub_path->child;
        }

        // copy name
        sub_path->name = memory.allocate(sizeof(__u8) * (length + 1));
        if (sub_path->name == NULL) {
            debug_cat(DEBUG_ERROR, "find_file_relative: failed due to memory error.\n");
            goto fail;
        }
        for (int i = 0; i < length; i++) {
            sub_path->name[length - 1 - i] = path[name_ptr - 1 - i];
        }
        sub_path->name[length] = '\0';

        // find that file
        INODE *parent_inode;
        if (sub_path->parent == NULL) {
            parent_inode = &(current_dir.inode);
        } else {
            parent_inode = &(sub_path->parent->inode);
        }

        // if parent inode's type is not directory
        if (parent_inode->info.i_mode & EXT2_S_IFDIR == 0) {
            memory.free(sub_path->name);
            sub_path->name = NULL;
            if (sub_path->parent != NULL) {
                sub_path = sub_path->parent;
            }
            memory.free(sub_path->child);
            goto fail;
        }

        // traverse blocks
        if (EXT2_FAIL == traverse_block(sub_path->name, parent_inode->info.i_block, &(sub_path->inode))) {
            goto fail;
        } else {
            continue;
        }
    }

    // success
    (*inode) = sub_path->inode;
    clean_sub_path(sub_path);
    return EXT2_SUCCESS;

    fail:
    clean_sub_path(sub_path);
    return EXT2_FAIL;
}

/**
 * find a file or directory by relative path
 * @param path: length(path) <= 255
 * @param inode: where store the result
 * @return success or not
 */
int find_file_relative(__u8 *path, INODE *inode) {
    struct ext2_path *sub_path = NULL;
    if (EXT2_SUCCESS == find_file_loop(path, sub_path, inode)) {
        return EXT2_SUCCESS;
    } else {
        return EXT2_FAIL;
    }
}

/**
 * find a file or directory by absolute path
 * @param path: length(path) <= 255
 * @param inode: where store the result
 * @return success or not
 */
int find_file_absolute(__u8 *path, INODE *inode) {
    struct ext2_path *sub_path = (struct ext2_path *) memory.allocate(sizeof(struct ext2_path));
    sub_path->name = (__u8 *) memory.allocate(sizeof(__u8));
    sub_path->name[0] = '\0';
    sub_path->parent = NULL;
    sub_path->child = NULL;
    get_root_inode(&(sub_path->inode.info));

    if (EXT2_SUCCESS == find_file_loop(path, sub_path, inode)) {
        return EXT2_SUCCESS;
    } else {
        return EXT2_FAIL;
    }
}

/**
 * find a file or directory by relative or absolute path
 * @param path: length(path) <= 255
 * @param inode: where store the result
 * @return success or not
 */
int ext2_find(__u8 *path, INODE *inode) {
    if (path[0] != '/') {
        debug_cat(DEBUG_ERROR, "find_file: path must start with a '/'\n");
        return EXT2_FAIL;
    }

    // find file by relative path
    debug_cat(DEBUG_LOG, "find_file: try to find by relative path.\n");
    if (EXT2_SUCCESS == find_file_relative(path, inode)) {
        debug_cat(DEBUG_LOG, "find_file: found that.\n");
        return EXT2_SUCCESS;
    }

    // find file by absolute path
    debug_cat(DEBUG_LOG, "find_file: try to find by absolute path.\n");
    if (EXT2_SUCCESS == find_file_absolute(path, inode)) {
        debug_cat(DEBUG_LOG, "find_file: found that.\n");
        return EXT2_SUCCESS;
    }

    debug_cat(DEBUG_ERROR, "find_file: cannot find that file or directory.\n");
    return EXT2_FAIL;
}

/**
 * open file
 * @param file
 * @param path
 * @return success or not
 */
int ext2_open(__u8 *path, EXT2_FILE *file) {
    if (EXT2_FAIL == ext2_find(path, &(file->inode))) {
        debug_cat(DEBUG_ERROR, "ext2_open: failed because cannot find that file.\n");
        return EXT2_FAIL;
    }

    if (EXT2_S_IFREG != file->inode.info.i_mode & EXT2_S_IFREG) {
        debug_cat(DEBUG_ERROR, "ext2_open: failed because that is not a regular file.\n");
        return EXT2_FAIL;
    }

    file->pointer = 0;
    file->dirty = EXT2_NOT_DIRTY;
    file->buffer = (__u8 *) memory.allocate(sizeof(__u8) * fs_info.block_size);
    if (file->buffer == NULL) {
        debug_cat(DEBUG_ERROR, "ext2_open: failed due to memory fail.\n");
        return EXT2_FAIL;
    }

    if (file->inode.info.i_block[0] == 0) {
        debug_cat(DEBUG_ERROR, "ext2_open: failed because the first block ID is 0.\n");
        return EXT2_FAIL;
    }

    if (0 ==
        disk_read(fs_info.par_start_address + file->inode.info.i_block[0] * (fs_info.block_size / 512), file->buffer,
                  fs_info.block_size / 512)) {
        debug_cat(DEBUG_ERROR, "ext2_open: failed due to disk fail.\n");
        return EXT2_FAIL;
    }

    debug_cat(DEBUG_LOG, "ext2_open: opened.\n");
    return EXT2_SUCCESS;
}

/**
 * move file pointer
 * @param file: file structure
 * @param new_pointer: new location; illegal location leads to fail
 * @return success or not
 */
int ext2_lseek(EXT2_FILE *file, __u32 new_pointer) {
    if (file == NULL) {
        debug_cat(DEBUG_ERROR, "ext2_lseek: empty file pointer.\n");
        return EXT2_FAIL;
    }

    if (file->pointer / fs_info.block_size == new_pointer / fs_info.block_size) {
        file->pointer = new_pointer;
        debug_cat(DEBUG_LOG, "ext2_lseek: pointer moved.\n");
        return EXT2_SUCCESS;
    }

    if (new_pointer >= file->inode.info.i_size) {
        debug_cat(DEBUG_ERROR, "ext2_lseek: new pointer is out of range [0, %d].\n", file->inode.info.i_size - 1);
        return EXT2_FAIL;
    }

    if (file->dirty == EXT2_DIRTY) {
        if (EXT2_FAIL == write_block(file)) {
            debug_cat(DEBUG_ERROR, "ext2_lseek: failed to write dirty data back to disk.\n");
            return EXT2_FAIL;
        }
    }

    file->pointer = new_pointer;
    if (EXT2_FAIL == read_block(file)) {
        debug_cat(DEBUG_ERROR, "ext2_lseek: failed to read data from disk.\n");
        return EXT2_FAIL;
    }

    debug_cat(DEBUG_LOG, "ext2_lseek: pointer moved.\n");
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
int ext2_read(EXT2_FILE *file, __u8 *buffer, __u64 length, __u64 *bytes_of_read) {
    if (file == NULL) {
        debug_cat(DEBUG_ERROR, "ext2_read: empty file pointer.\n");
        return EXT2_FAIL;
    }

    // if the length is longer than the bytes left
    if (length > file->inode.info.i_size - file->pointer) {
        length = file->inode.info.i_size - file->pointer;
    }

    __u64 count = 0;
    while (count < length) {
        int i = 0;
        for (i = file->pointer % fs_info.block_size; i < fs_info.block_size && count < length; i++) {
            buffer[count++] = file->buffer[i];
        }
        if (i == fs_info.block_size) {
            if (EXT2_FAIL == ext2_lseek(file, (file->pointer / fs_info.block_size + 1) * fs_info.block_size)) {
                debug_cat(DEBUG_ERROR, "ext2_read: failed to move pointer.\n");
                return EXT2_FAIL;
            }
        }
    }

    if (bytes_of_read != NULL) {
        (*bytes_of_read) = count;
    }
    debug_cat(DEBUG_LOG, "ext2_read: read %d byte(s) of data.\n", count);
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
int ext2_write(EXT2_FILE *file, const __u8 *buffer, __u64 length, __u64 *bytes_of_write) {
    if (file == NULL) {
        debug_cat(DEBUG_ERROR, "ext2_write: empty file pointer.\n");
        return EXT2_FAIL;
    }

    // if the length is longer than the bytes left
    if (length > file->inode.info.i_size - file->pointer) {
        length = file->inode.info.i_size - file->pointer;
    }

    __u64 count = 0;
    while (count < length) {
        int i = 0;
        for (i = file->pointer % fs_info.block_size; i < fs_info.block_size && count < length; i++) {
            file->buffer[i] = buffer[count++];
            file->dirty = EXT2_DIRTY;
        }
        if (i == fs_info.block_size) {
            if (EXT2_FAIL == ext2_lseek(file, (file->pointer / fs_info.block_size + 1) * fs_info.block_size)) {
                debug_cat(DEBUG_ERROR, "ext2_write: failed to move pointer.\n");
                return EXT2_FAIL;
            }
        }
    }

    if (bytes_of_write != NULL) {
        (*bytes_of_write) = count;
    }
    debug_cat(DEBUG_LOG, "ext2_read: read %d byte(s) of data.\n", count);
    return EXT2_SUCCESS;
}