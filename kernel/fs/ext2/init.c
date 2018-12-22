//
// Created by zengkai on 2018/11/30.
//

#include "ext2.h"

void *memory_allocate(unsigned int size) { return malloc(size); }

void *memory_copy(void *dest, void *src, int len) { return memcpy(dest, src, (size_t) len); }

void *memory_set(void *dest, int value, int len) { return memset(dest, value, (size_t) len); }

int find_linux_par(__u8 *buffer) {
    if (disk_read(0, buffer, 1) == 0) {
        debug_cat(DEBUG_ERROR, "find_linux_par: failed due to disk error.\n");
        return FAIL;
    }
    // get partition table
    struct partition_entry par[4];
    find_part(par, buffer);
    // find Linux partition (do not support extended partition now)
    fs_info.par_start_address = 0;
    for (int i = 0; i < 4; i++) {
        if (par[i].status == 0x80 && par[i].partition_type == 0x83) {
            // this partition is active and partition type is Linux
            fs_info.par_start_address = par[i].start_address;
            return SUCCESS;
        }
    }

    return FAIL;
}

int find_ext2_fs(__u8 *buffer) {
    // skip 1024B boot code
    struct ext2_super_block sb;
    if (disk_read(fs_info.par_start_address + 2, buffer, 2) == 0) {
        debug_cat(DEBUG_ERROR, "find_ext2_fs: failed due to disk error.\n");
        return FAIL;
    }
    sb_fill(&sb, buffer);
    if (sb.s_magic == 0xEF53) {
        // check magic number
        // compute the address of group descriptors, block size and inode size
        if (sb.s_log_block_size == 0) {
            // block size = 1KB
            fs_info.block_size = 1024;
            fs_info.group_desc_address = fs_info.par_start_address + 4;
        } else {
            // block size >= 2KB
            fs_info.block_size = (__u16) (1024 << sb.s_log_block_size);
            fs_info.group_desc_address = fs_info.par_start_address + fs_info.block_size / 512;
        }
        fs_info.inode_size = sb.s_inode_size;
        fs_info.inode_count = sb.s_inodes_count;
        fs_info.inode_per_bg = sb.s_inodes_per_group;
        return SUCCESS;
    }
    return FAIL;
}

int load_root_inode() {
    // initialize current path (root)
    current_dir.name = NULL;
    current_dir.parent = NULL;
    current_dir.child = NULL;
    current_dir.inode.id = 2;

    if (SUCCESS == get_root_inode(&(current_dir.inode.info))) {
        return SUCCESS;
    } else {
        return FAIL;
    }
}

/**
 * initialize ext2 file system
 *   1. select disk interface: Windows or SWORD
 *   2. figure out whether there is a Ext2 file system or not
 *   3. initialize sb_info
 * @return success or not
 */
int ext2_init() {
    debug_cat(DEBUG_LOG, "ext2_init: initializing...\n");

#ifdef WINDOWS
    disk.read = windows_disk_read;
    disk.write = windows_disk_write;

    memory.allocate = memory_allocate;
    memory.free = free;
    memory.copy = memory_copy;
    memory.set = memory_set;
#elif SWORD
    disk.read = sd_read_sector_blocking;
    disk.write = sd_write_sector_blocking;

    memory.allocate = kmalloc;
    memory.free = kfree;
    memory.copy = kernel_memcpy;
    memory.set = kernel_memset;
#endif // system type

    __u8 buffer[1024];

    // find Linux partition
    if (SUCCESS == find_linux_par(buffer)) {
        debug_cat(DEBUG_LOG, "ext2_init: found Linux partition at %d.\n", fs_info.par_start_address);
    } else {
        debug_cat(DEBUG_ERROR, "ext2_init: cannot find an active primary Linux partition.\n");
        goto error;
    }

    // whether this Linux partition is Ext2
    if (SUCCESS == find_ext2_fs(buffer)) {
        debug_cat(DEBUG_LOG, "ext2_init: confirmed that Linux partition is an Ext2 file system.\n");
        debug_cat(DEBUG_LOG, "ext2_init: loaded essential information into memory.\n");
    } else {
        debug_cat(DEBUG_ERROR, "ext2_init: this Linux partition is not in Ext2 file system.\n");
        goto error;
    }

    // find root inode
    if (SUCCESS == load_root_inode()) {
        debug_cat(DEBUG_LOG, "ext2_init: loaded root directory inode.\n");
        debug_cat(DEBUG_LOG, "ext2_init: initialized Ext2 file system.\n");
        return SUCCESS;
    } else {
        debug_cat(DEBUG_ERROR, "ext2_init: this Linux partition is not in Ext2 file system.\n");
        goto error;
    }

    // cannot find Ext2 file system in primary partitions
    error:
    debug_cat(DEBUG_ERROR, "ext2_init: fail to initialize the Ext2 file system.\n");
    return FAIL;
}
