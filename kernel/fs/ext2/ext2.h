//
// Created by zengk on 2018/11/29.
//

#ifndef EXT2_EXT2_H
#define EXT2_EXT2_H

// macro definition
#define WINDOWS // platform
#define DEBUG_EXT2 // debug mode
#define INIT_SUCCESS 0
#define INIT_FAIL 1
#define DEBUG_LOG 100
#define DEBUG_WARNING 101
#define DEBUG_ERROR 102
#define FIND_PART_SUCCESS 200
#define FIND_PART_FAIL 201
#define SUPER_FILL_SB_SUCCESS 300
#define SUPER_FILL_SB_FAIL 301

// types and data structures
typedef unsigned long DWORD;
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef __u16 __le16;
typedef __u32 __le32;

// physical interface provide function communicating with hardware
struct physical_interface {
    // read 512 bytes from hardware
    // id is 512-byte-aligned address (module 512)
    int (*read)(int id, void *buffer);

    // write 512 bytes to hardware
    // id is 512-byte-aligned address (module 512)
    int (*write)(int id, void *buffer);
};

// partition entry in MBR (main boot record)
struct partition_entry {
    __u8 status; // 0x00 means inactive, 0x80 means active, 0x01-0x7f stand for invalid
    __u8 start_head;
    __u8 start_sector; // sector in bits 5-0; bits 7-6 are high bits of cylinder
    __u8 start_cylinder; // 10 bits in total
    __u8 partition_type;
    __u8 end_head;
    __u8 end_sector; // sector in bits 5-0; bits 7-6 are high bits of cylinder
    __u8 end_cylinder; // 10 bits in total
    __u32 start_address; // logical block addressing of first absolute sector in the partition
    __u32 total_sectors; // number of sectors in partition
};

// structure of super block, waited to be commented
struct ext2_super_block {
    __le32 s_inodes_count;
    __le32 s_blocks_count;
    __le32 s_r_blocks_count;
    __le32 s_free_blocks_count;
    __le32 s_free_inodes_count;
    __le32 s_first_data_block;
    __le32 s_log_block_size;
    __le32 s_log_frag_size;
    __le32 s_blocks_per_group;
    __le32 s_frags_per_group;
    __le32 s_inodes_per_group;
    __le32 s_mtime;
    __le32 s_wtime;
    __le16 s_mnt_count;
    __le16 s_max_mnt_count;
    __le16 s_magic;
    __le16 s_state;
    __le16 s_errors;
    __le16 s_minor_rev_level;
    __le32 s_lastcheck;
    __le32 s_checkinterval;
    __le32 s_creator_os;
    __le32 s_rev_level;
    __le16 s_def_resuid;
    __le16 s_def_resgid;

    __le32 s_first_ino;
    __le16 s_inode_size;
    __le16 s_block_group_nr;
    __le32 s_feature_compat;
    __le32 s_feature_incompat;
    __le32 s_feature_ro_compat;
    __u8 s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    __le32 s_algorithm_usage_bitmap;

    __u8 s_prealloc_blocks;
    __u8 s_prealloc_dir_blocks;
    __u16 s_padding1;

    __u8 s_journal_uuid[16];
    __u32 s_journal_inum;
    __u32 s_journal_dev;
    __u32 s_last_orphan;
    __u32 s_hash_seed[4];
    __u8 s_def_hash_version;
    __u8 s_reserved_char_pad;
    __u16 s_reserved_word_pad;
    __le32 s_default_mount_opts;
    __le32 s_first_meta_bg;

    __u32 s_reserved[190];
};

// global variable
struct physical_interface disk;

// platform specification
#ifdef WINDOWS

#include "stdio.h"
#include "stdarg.h"
#include "windows.h"

#define VIRTUAL_DISK "\\\\.\\PhysicalDrive2" // this is the virtual disk used while my developing
#define SECTOR_SIZE 512 // this is the sector size of virtual disk

int windows_disk_read(int id, void *buffer);

int windows_disk_write(int id, void *buffer);

#elif SWORD

#include "driver/vga.h"
#include "zjunix/utils.h"
#include "driver/sd.h"

#endif // system type

// init.c
int ext2_init();

// debug_cat.c
int debug_cat(int type, const char *format, ...);

// disk_tool.c
int find_part(struct partition_entry *par);

// super.c
int sb_fill(struct ext2_super_block *sb, void *data);


#endif //EXT2_EXT2_H
