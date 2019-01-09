#ifndef _FILE_H_
#define _FILE_H_

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
#define LOCAL_DATA_BUF_NUM 4


struct __attribute__((__packed__)) dir_entry_attr {
    u8 name[8];                   /* Name */
    u8 ext[3];                    /* Extension */
    u8 attr;                      /* attribute bits */
    u8 lcase;                     /* Case for base and extension */
    u8 ctime_cs;                  /* Creation time, centiseconds (0-199) */
    u16 ctime;                    /* Creation time */
    u16 cdate;                    /* Creation date */
    u16 adate;                    /* Last access date */
    u16 starthi;                  /* Start cluster (Hight 16 bits) */
    u16 time;                     /* Last modify time */
    u16 date;                     /* Last modify date */
    u16 startlow;                 /* Start cluster (Low 16 bits) */
    u32 size;                     /* file size (in bytes) */
};

union dir_entry {
    u8 data[32];
    struct dir_entry_attr attr;
};

/* 4k byte buffer */
typedef struct buf_4k {
    unsigned char buf[4096];
    unsigned long cur;
    unsigned long state;
} BUF_4K;

typedef struct fat_file {
    unsigned char path[256];
    /* Current file pointer */
    unsigned long loc;
    /* Current directory entry position */
    unsigned long dir_entry_pos;
    unsigned long dir_entry_sector;
    /* current directory entry */
    union dir_entry entry;
    /* Buffer clock head */
    unsigned long clock_head;
    /* For normal FAT32, cluster size is 4k */
    BUF_4K data_buf[LOCAL_DATA_BUF_NUM];
} FILE;

#endif 