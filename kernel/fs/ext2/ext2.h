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
#include <driver/ps2.h>

/* disk_tool.c */
int ext2_find_part(struct partition_entry *par, void *data);

/* super.c */
int ext2_sb_fill(struct ext2_super_block *sb, void *data);
int ext2_group_desc_fill(struct ext2_group_desc *group_desc, void *data);
int ext2_find_sparse_bg(struct ext2_group_desc *group_desc, int *id);
int ext2_find_free_inode(struct ext2_group_desc *group_desc);
int ext2_find_free_block(struct ext2_group_desc *group_desc);
int ext2_set_block_used(__u32 block_id);
int ext2_rm_dir_entry(__u8 *name, INODE *inode, struct ext2_dir_entry *dir);

/* inode.c */
int ext2_inode_fill(struct ext2_inode *inode, void *data);
int ext2_dir_entry_fill(struct ext2_dir_entry *dir, void *data);
int ext2_is_equal(__u8 *s1, __u8 *s2);
int ext2_find_inode(__u32 id, struct ext2_inode *inode);
int ext2_traverse_block(__u8 *target, __u32 *i_blocks, INODE *inode);
int ext2_get_root_inode(struct ext2_inode *inode);
int ext2_append_to_end(INODE *inode, struct ext2_dir_entry *dir);
int ext2_store_inode(INODE *inode);
int ext2_release_blocks(__u32 *blocks);
int ext2_set_inode_free(__u32 id);
int ext2_traverse_block_rm(__u32 *blocks);
int ext2_cp_i2i(INODE *src, INODE *dest, __u8 *name);

/* block.c */
int ext2_write_block(EXT2_FILE *file);
int ext2_read_block(EXT2_FILE *file);
int ext2_new_block(INODE *inode, int count);
int ext2_set_block_free(__u32 block_id);
int ext2_new_block_without_inode(int gd_id);

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
int ext2_rm_inode(INODE inode);
int ext2_is_removable(__u8 *name);

/* ext2.c */
int ext2_mkdir_plus(__u8 *name, INODE *parent, INODE *result);
int ext2_create_plus(__u8 *param, INODE *parent, INODE *result);

/* debug.c */
void ext2_output_buffer(void *data, int length);
void ext2_show_group_decs(struct ext2_group_desc *gd);
void ext2_show_current_dir();

#endif //EXT2_EXT2_H
