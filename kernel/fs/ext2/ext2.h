//
// Created by zengkai on 2018/11/29.
//

#ifndef EXT2_EXT2_H
#define EXT2_EXT2_H

//#define DEBUG_EXT2 // debug mode

#include <zjunix/fs/ext2.h>
#include <driver/vga.h>
#include <zjunix/utils.h>
#include <driver/sd.h>
#include <zjunix/memory.h>
#include <zjunix/log.h>

/* disk_tool.c */
int ext2_find_part(struct partition_entry *par, void *data);

/* super.c */
int ext2_sb_fill(struct ext2_super_block *sb, void *data);
int ext2_group_desc_fill(struct ext2_group_desc *group_desc, void *data);
int ext2_find_sparse_bg(struct ext2_group_desc *group_desc, int *id);
int ext2_find_free_inode(struct ext2_group_desc *group_desc);
int ext2_find_free_block(struct ext2_group_desc *group_desc);
int ext2_set_block_used(__u32 block_id);

/* inode.c */
int ext2_inode_fill(struct ext2_inode *inode, void *data);
int ext2_dir_entry_fill(struct ext2_dir_entry *dir, void *data);
int ext2_is_equal(__u8 *s1, __u8 *s2);
int ext2_find_inode(__u32 id, struct ext2_inode *inode);
int ext2_traverse_block(__u8 *target, __u32 *i_blocks, INODE *inode);
int ext2_get_root_inode(struct ext2_inode *inode);
int ext2_append_to_end(INODE *inode, struct ext2_dir_entry *dir);
int ext2_store_inode(INODE *inode);

/* block.c */
int ext2_write_block(EXT2_FILE *file);
int ext2_read_block(EXT2_FILE *file);
int ext2_new_block(INODE *inode, int count);

/* file.c */
int ext2_find(__u8 *path, INODE *inode);
int ext2_open(__u8 *path, EXT2_FILE *file);
int ext2_close(EXT2_FILE *file);
int ext2_lseek(EXT2_FILE *file, __u32 new_pointer);
int ext2_read(EXT2_FILE *file, __u8 *buffer, __u64 length, __u64 *bytes_of_read);
int ext2_write(EXT2_FILE *file, __u8 *buffer, __u64 length, __u64 *bytes_of_write);
int ext2_is_valid_filename(__u8 *name);
int ext2_strlen(__u8 *string);
int ext2_find_file_relative(__u8 *path, INODE *inode);

#endif //EXT2_EXT2_H
