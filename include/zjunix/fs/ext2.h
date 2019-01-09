//
// Created by zengkai on 2019/1/1.
//

#ifndef _ZJUNIX_FS_EXT2_H
#define _ZJUNIX_FS_EXT2_H

#define EXT2_DEBUG

/* all macro definition of ext2 file system */
#define EXT2_SUCCESS 0x1
#define EXT2_FAIL 0x0
#define EXT2_NULL 0
#define EXT2_TRUE 1
#define EXT2_FALSE 0

#define EXT2_NOT_FOUND 0x2

#define EXT2_DIRTY 1
#define EXT2_NOT_DIRTY 0

#define EXT2_S_IFDIR 0x4000
#define EXT2_S_IFREG 0x8000
#define EXT2_FT_REG_FILE 0x0001
#define EXT2_FT_DIR 0x0002

/* type and structure definition */
typedef unsigned long DWORD;
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;
typedef unsigned long __u64;
typedef __u16 __le16;
typedef __u32 __le32;

/* partition entry in MBR (main boot record) */
struct partition_entry
{
    /* 0x00: inactive
     * 0x80: active
     * 0x01-0x7f: invalid */
    __u8 status;
    __u8 start_head;
    /* sector in bits 5-0; bits 7-6 are high bits of cylinder */
    __u8 start_sector;
    /* 10 bits in total (high order 2 bits are located in start_sector's high order 2 bits) */
    __u8 start_cylinder;
    __u8 partition_type;
    __u8 end_head;
    /* similar to start_sector and start_cylinder */
    __u8 end_sector;
    __u8 end_cylinder;
    __u32 start_address;
    __u32 total_sectors;
};

/* structure of super block, waited to be commented
 * in block group #1, superblock is following 1024B boot code
 * the total size is 1 block, if block size >= 2KB (padding 0)
 * the total size is 2 blocks, if block size = 1KB */
struct ext2_super_block
{
    /* total inodes count (used, free or reserved)
     * = sum(inodes in each block group)
     * <= s_inode_per_group * number of block groups */
    __le32 s_inodes_count;
    /* total blocks count (used, free or reserved)
     * = sum(blocks in each block group)
     * <= s_block_per_group * number of block groups */
    __le32 s_blocks_count;
    /* total number of blocks reserved for super user */
    __le32 s_r_blocks_count;
    /* total number of free blocks, including those reserved for super user */
    __le32 s_free_blocks_count;
    /* total number of free inodes */
    __le32 s_free_inodes_count;
    /* identify the first data block
     * (superblock belongs to data block, so this identify the the block contains superblock)
     * for file system with a block size > 1KB, = 0
     * for file system with a block size = 1KB, = 1
     * the superblock is ALWAYS starting at the 1024B of the disk */
    __le32 s_first_data_block;
    /* must be POSITIVE
     * block size (byte) = 1024 << s_log_block_size */
    __le32 s_log_block_size;
    /* positive: fragment size (byte) = 1024 << s_log_frag_size
     * negative: fragment size (byte) = 1024 >> (-s_log_frag_size) */
    __le32 s_log_frag_size;
    __le32 s_blocks_per_group;
    /* determine the size of the block bitmap of each block group */
    __le32 s_frags_per_group;
    /* determine the size of the inode bitmap of each block group
     * <= block size (bit), because the inode bitmap must fit within a single block
     * = a perfect multiple of the number of inodes that can fit a single block */
    __le32 s_inodes_per_group;
    /* UNIX time, as defined by POSIX, of the last time when the file system was mounted*/
    __le32 s_mtime;
    /* UNIX time, as defined by POSIX, of the last time when the last write access to the file system */
    __le32 s_wtime;
    /* how many time the file system was mounted since the last time it was fully verified */
    __le16 s_mnt_count;
    /* the maximum number of times that the file system may be mounted before a full check is performed */
    __le16 s_max_mnt_count;
    /* 0xEF53 for ext2, magic number */
    __le16 s_magic;
    /* when the file system is mounted, s_state is set to EXT2_ERROR_FS (2)
     * after the file system is cleanly unmounted, s_state is set to EXT2_VALID_FS (1) */
    __le16 s_state;
    /* indicate what should do when an error is detected
     * EXT2_ERRORS_CONTINUE (1): do nothing
     * EXT2_ERRORS_RO (2): remount read-only
     * EXT2_ERRORS_PANIC (3): cause a kernel panic */
    __le16 s_errors;
    /* the minor revision level within its revision level */
    __le16 s_minor_rev_level;
    /* UNIX time, as defined by POSIX, of the last time when system check */
    __le32 s_lastcheck;
    /* maximum UNIX time internal, as defined by POSIX, allowed between file system checks */
    __le32 s_checkinterval;
    /* OS that created the file system
     * EXT2_OS_LINUX (0): Linux
     * EXT2_OS_HURD (1): GNU HURD
     * EXT2_OS_MASIX (2): MASIX
     * EXT2_OS_FREEBSD (3): FreeBSD
     * EXT2_OS_LITES (4): Lites */
    __le32 s_creator_os;
    /* revision level
     * EXT2_GOOD_OLD_REV (0): Revision 0
     * EXT2_DYNAMIC_REV (1): Revision 1 with extended attributes */
    __le32 s_rev_level;
    /* default user ID for reserved blocks */
    __le16 s_def_resuid;
    /* default group ID for reserved blocks */
    __le16 s_def_resgid;
    /* index of the first inode usable for standard files
     * in revision 0, this is fixed to 11
     * in revision 1, this may be set to any value */
    __le32 s_first_ino;
    /* in revision 0, 128B
     * in revision 1, (perfect power of 2) and (<= block size) */
    __le16 s_inode_size;
    /* indicate the block group number hosting this superblock */
    __le16 s_block_group_nr;
    /* compatible features
     * EXT2_FEATURE_COMPAT_DIR_PREALLOC (0x0001): block pre-allocate for new directories
     * EXT2_FEATURE_COMPAT_IMAGIC_INODES (0x0002): ?
     * EXT2_FEATURE_COMPAT_HAS_JOURNAL (0x0004): an Ext3 journal exists
     * EXT2_FEATURE_COMPAT_EXT_ATTR (0x0008): extended inode attributes are present
     * EXT2_FEATURE_COMPAT_RESIZE_INO (0x0010): non-standard inode size are used
     * EXT2_FEATURE_COMPAT_DIR_INDEX (0x0020): directory index (HTree) */
    __le32 s_feature_compat;
    /* incompatible features
     * system should refuse to mount this file system if any incompatible feature is unsupported
     * EXT2_FEATURE_INCOMPAT_COMPRESSION (0x0001): disk compression is used
     * EXT2_FEATURE_INCOMPAT_FILETYPE (0x0002): file attributes are included in directories attributes
     * EXT2_FEATURE_INCOMPAT_RECOVER (0x0004): ?
     * EXT2_FEATURE_INCOMPAT_JOURNAL_DEV (0x0008): ?
     * EXT2_FEATURE_INCOMPAT_META_BG (0x0010): ? */
    __le32 s_feature_incompat;
    /* read-only features
     * the file system should be mounted as read-only if any of the indicated feature is unsupported
     * EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER (0x0001): sparse superblock
     * EXT2_FEATURE_RO_COMPAT_LARGE_FILE (0x0002): 64-bit file size
     * EXT2_FEATURE_RO_COMPAT_BTREE_DIR (0x0004): binary tree sorted directory files */
    __le32 s_feature_ro_compat;
    /* unique for each file system (as much as possible) */
    __u8 s_uuid[16];
    /* mostly UNUSED, should consist of only ISO-Latin-1 character and be 0 terminated */
    char s_volume_name[16];
    /* directory path where the file system was last mounted */
    char s_last_mounted[64];
    /* used by compression algorithms to determine the compression method(s) used
     * EXT2_LZV1_ALG: 0x0001
     * EXT2_LZRW3A_ALG: 0x0002
     * EXT2_GZIP_ALG: 0x0004
     * EXT2_BZIP2_ALG: 0x0008
     * EXT2_LZP_ALG: 0x0010 */
    __le32 s_algorithm_usage_bitmap;

    /* performance hints */
    /* the number of blocks the implementation should attempt to pre-allocate when creating a new regular file */
    __u8 s_prealloc_blocks;
    /* the number of blocks the implementation should attempt to pre-allocate when creating a new directory */
    __u8 s_prealloc_dir_blocks;
    __u16 s_padding1;

    /* this part is for Ext3 journal, so details are omitted */
    /* the uuid of the journal superblock */
    __u8 s_journal_uuid[16];
    /* inode number of the journal file */
    __u32 s_journal_inum;
    /* device number of the journal file */
    __u32 s_journal_dev;
    /* inode number pointing to the first inode in the list of inodes to delete */
    __u32 s_last_orphan;

    /* direction indexing support */
    /* the seeds used for the hash algorithm for directory indexing */
    __u32 s_hash_seed[4];
    /* the default hash version used for directory indexing */
    __u8 s_def_hash_version;
    __u8 s_reserved_char_pad;
    __u16 s_reserved_word_pad;

    /* other options */
    __le32 s_default_mount_opts;
    __le32 s_first_meta_bg;

    /* reserve for future revisions */
    __u32 s_reserved[190];
};

/* structure of block group descriptor */
struct ext2_group_desc
{
    /* the first block of the block bitmap */
    __le32 bg_block_bitmap;
    /* the first block of the inode bitmap */
    __le32 bg_inode_bitmap;
    /* the first block of the inode table */
    __le32 bg_inode_table;
    __le16 bg_free_blocks_count;
    __le16 bg_free_inodes_count;
    /* the number of inodes allocated to directories */
    __le16 bg_used_dirs_count;

    /* no use, maybe */
    __le16 bg_pad;
    __le32 bg_reserved[3];
};

/* structure of inode */
struct ext2_inode
{
    /* indicate the format of the file and the access right
     * -- file format --
     * EXT2_S_IFSOCK (0xC000): socket
     * EXT2_S_IFLNK (0xA000): symbolic link
     * EXT2_S_IFREG (0x8000): regular file
     * EXT2_S_IFBLK (0x6000): block device
     * EXT2_S_IFDIR (0x4000): directory
     * EXT2_S_IFCHR (0x2000): character device
     * EXT2_S_IFIFO (0x1000): fifo
     * -- process execution user/group override --
     * EXT2_S_ISUID (0x0800): set process user ID
     * EXT2_S_ISGID (0x0400): set process group ID
     * EXT2_S_ISVTX (0x0200): sticky bit
     * -- access right --
     * EXT2_S_IRUSR (0x0100): user read
     * EXT2_S_IWUSR (0x0080): user write
     * EXT2_S_IXUSR (0x0040): user execute
     * EXT2_S_IRGRP (0x0020): group read
     * EXT2_S_IWGRP (0x0010): group write
     * EXT2_S_IXGRP (0x0008): group execute
     * EXT2_S_IROTH (0x0004): other read
     * EXT2_S_IWOTH (0x0002): other write
     * EXT2_S_IXOTH (0x0001): other execute */
    __le16 i_mode;
    /* user ID associated with the file */
    __le16 i_uid;
    /* in revision 0, this SIGNED value indicating the size of the file in bytes
     * in revision 1 and only for regular files, this the lower 32-bit of the file size
     * the upper 32-bit is located in the i_dir_acl */
    __le32 i_size;
    /* the number of second since January 1st 1970 of the last time this node was accessed */
    __le32 i_atime;
    /* the number of second since January 1st 1970 of when the inode was created */
    __le32 i_ctime;
    /* the number of second since January 1st 1970 of the last time this node was modified */
    __le32 i_mtime;
    /* the number of second since January 1st 1970 of the last time this node was deleted */
    __le32 i_dtime;
    /* POSIX group having access to this file */
    __le16 i_gid;
    /* most files will have a link count of 1, and additional count for each hard link */
    __le16 i_links_count;
    /* total number of 512B blocks (regardless what the size of file system block size is)
     * reserved to contain the data of this file (regardless used or not) */
    __le32 i_blocks;
    /* how the file system should behave when accessing the data for this inode
     * EXT2_SECRM_FL (0x000000001): secure deletion
     * EXT2_UNRM_FL (0x00000002): record for undelete
     * EXT2_COMPR_FL (0x00000004): compressed file
     * EXT2_SYNC_FL (0x00000008): synchronous updates
     * EXT2_IMMUTABLE_FL (0x00000010): immutable file
     * EXT2_APPEND_FL (0x00000020): append only
     * EXT2_NODUMP_FL (0x00000040): do not dump/delete file
     * EXT2_NOATIME_FL (0x00000080): do not update .i_atime
     * -- reserved for compression usage --
     * EXT2_DIRTY_FL (0x00000100): dirty
     * EXT2_COMPRBLK_FL (0x00000200): compressed blocks
     * EXT2_NOCOMPR_FL (0x00000400): access raw compressed data
     * EXT2_ECOMPR_FL (0x00000800): compression error
     * -- other option --
     * EXT2_BTREE_FL (0x00001000): B-tree format directory
     * EXT2_INDEX_FL (0x00002000): hash indexed directory
     * EXT2_IMAGIC_FL (0x00004000): AFS directory
     * EXT2_JOURNAL_DATA_FL (0x00008000): journal file data
     * EXT2_RESERVED_FL (0x80000000): reserved for ext2 library */
    __le32 i_flags;
    /* OS dependant value: no use, maybe */
    union {
        struct
        {
            __le32 l_i_reserved1;
        } linux1;
        struct
        {
            __le32 h_i_translator;
        } hurd1;
        struct
        {
            __le32 m_i_reserved1;
        } masix1;
    } osd1;
    /* 15 entries of "pointers", where store block numbers
     * the first 12 blocks are direct blocks (store block IDs of data block)
     * the 13th entry is indirect block, pointing to a block where store an array of block IDs
     * the 14th entry is double-indirect block, pointing to a block where store an array of indirect block IDs
     * the 15th entry is triple-indirect block, pointing to a block containing an array of double-indirect block IDs
     * 0 terminated */
    __le32 i_block[15];
    /* file version, used by NFS */
    __le32 i_generation;
    /* block ID containing the extended attributes
     * in revision 0, = 0 */
    __le32 i_file_acl;
    /* in revision 0, = 0
     * in revision 1, this is high 32bit value of the 64bit file size, for regular files */
    __le32 i_dir_acl;
    /* the location of file fragment */
    __le32 i_faddr;
    /* OS dependant structure 2: no use, maybe */
    union {
        struct
        {
            __u8 l_i_frag;
            __u8 l_i_fsize;
            __u16 i_pad1;
            __le16 l_i_uid_high;
            __le16 l_i_gid_high;
            __u32 l_i_reserved2;
        } linux2;
        struct
        {
            __u8 h_i_frag;
            __u8 h_i_fsize;
            __le16 h_i_mode_high;
            __le16 h_i_uid_high;
            __le16 h_i_gid_high;
            __le32 h_i_author;
        } hurd2;
        struct
        {
            __u8 m_i_frag;
            __u8 m_i_fsize;
            __u16 m_pad1;
            __u32 m_i_reserved2[2];
        } masix2;
    } osd2;
};

/* structure of inode in memory */
typedef struct ext2_inode_info
{
    struct ext2_inode info;
    __u32 id;
} INODE;

/* directory entry */
struct ext2_dir_entry
{
    /* inode ID
     * 0 indicates that the entry is not used (deleted)
     * 1 is lost+found, I guess
     * 2 is the root */
    __le32 inode;
    /* unsigned displacement to the next entry from the start of the current entry
     * the directories must be aligned on 4 bytes boundaries */
    __le16 rec_len;
    /* how many bytes of characters are contained in the name */
    __u8 name_len;
    /* file type
     * in revision 0, this is the upper 8-bit of 16bit .name_len
     * EXT2_FT_UNKNOWN (0): unknown file type
     * EXT2_FT_REG_FILE (1): regular file
     * EXT2_FT_DIR (2): directory
     * EXT2_FT_CHRDEV (3): character device
     * EXT2_FT_BLKDEV (4): block device
     * EXT2_FT_FIFO (5): buffer file
     * EXT2_FT_SOCK (6): socket file
     * EXT2_FT_SYMLINK (7): symbolic link */
    __u8 file_type;
    /* name of the entry
     * ISO-Latin-1 character set is expected in most system
     * no longer than 255 bytes after encoding */
    char name[256];
};

/* ext2 file */
typedef struct ext2_file
{
    /* start address */
    INODE inode;
    /* pointers, starts from 0 */
    __u32 pointer;
    /* 4096 bytes buffer */
    __u8 buffer[4096];
    /* dirty
     * 1 for dirty, and need to be written back to disk */
    __u8 dirty;
} EXT2_FILE;

/* structure for optimize file path */
struct ext2_path
{
    __u8 name[256];
    INODE inode;
};

/* global variable */
/* meta information of file system */
struct FS_INFO
{
    struct ext2_super_block sb;
    __u32 par_start_address;
    __u32 group_desc_address;
    __u16 block_size;
} fs_info;
/* current directory */
struct ext2_path current_dir;

/* functions provided to outside */
int ext2_init();
int ext2_cat(__u8 *path);
int ext2_ls();
int ext2_cd(__u8 *path);
int ext2_mkdir(__u8 *name);
int ext2_create(__u8 *param);
int ext2_rm(__u8 *param);

#endif //_ZJUNIX_FS_EXT2_H
