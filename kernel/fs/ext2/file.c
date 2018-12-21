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
int find_file_loop(__u8 *path, struct ext2_path *sub_path, struct ext2_inode *inode) {
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
        struct ext2_inode *parent_inode;
        if (sub_path->parent == NULL) {
            parent_inode = &(current_dir->inode);
        } else {
            parent_inode = &(sub_path->parent->inode);
        }

        // if parent inode's type is not directory
        if (parent_inode->i_mode & EXT2_S_IFDIR == 0) {
            memory.free(sub_path->name);
            sub_path->name = NULL;
            if (sub_path->parent != NULL) {
                sub_path = sub_path->parent;
            }
            memory.free(sub_path->child);
            goto fail;
        }

        // traverse blocks
        if (FAIL == traverse_block(sub_path->name, parent_inode->i_block, &(sub_path->inode))) {
            goto fail;
        } else {
            continue;
        }
    }

    // success
    (*inode) = sub_path->inode;
    clean_sub_path(sub_path);
    return SUCCESS;

    fail:
    clean_sub_path(sub_path);
    return FAIL;
}

/**
 * find a file or directory by relative path
 * @param path: length(path) <= 255
 * @param inode: where store the result
 * @return success or not
 */
int find_file_relative(__u8 *path, struct ext2_inode *inode) {
    struct ext2_path *sub_path = NULL;
    if (SUCCESS == find_file_loop(path, sub_path, inode)) {
        return SUCCESS;
    } else {
        return FAIL;
    }
}

/**
 * find a file or directory by absolute path
 * @param path: length(path) <= 255
 * @param inode: where store the result
 * @return success or not
 */
int find_file_absolute(__u8 *path, struct ext2_inode *inode) {
    struct ext2_path *sub_path = (struct ext2_path *) memory.allocate(sizeof(struct ext2_path));
    sub_path->name = (__u8 *) memory.allocate(sizeof(__u8));
    sub_path->name[0] = '\0';
    sub_path->parent = NULL;
    sub_path->child = NULL;
    get_root_inode(&(sub_path->inode));

    if (SUCCESS == find_file_loop(path, sub_path, inode)) {
        return SUCCESS;
    } else {
        return FAIL;
    }
}

/**
 * find a file or directory by relative or absolute path
 * @param path: length(path) <= 255
 * @param inode: where store the result
 * @return success or not
 */
int ext2_find(__u8 *path, struct ext2_inode *inode) {

    if (path[0] != '/') {
        debug_cat(DEBUG_ERROR, "find_file: path must start with a '/'\n");
        return FAIL;
    }

    // find file by relative path
    debug_cat(DEBUG_LOG, "find_file: try to find by relative path.\n");
    if (SUCCESS == find_file_relative(path, inode)) {
        debug_cat(DEBUG_LOG, "find_file: found that.\n");
        return SUCCESS;
    }

    // find file by absolute path
    debug_cat(DEBUG_LOG, "find_file: try to find by absolute path.\n");
    if (SUCCESS == find_file_absolute(path, inode)) {
        debug_cat(DEBUG_LOG, "find_file: found that.\n");
        return SUCCESS;
    }

    debug_cat(DEBUG_ERROR, "find_file: cannot find that file or directory.\n");
    return FAIL;
}