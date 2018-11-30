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

// types and data structures
typedef unsigned long DWORD;
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;

// physical interface provide function communicating with hardware
struct physical_interface {
    // read 512 bytes from hardware
    // id is 512-byte-aligned address (module 512)
    int (*read)(int id, void *buffer);

    // write 512 bytes to hardware
    // id is 512-byte-aligned address (module 512)
    int (*write)(int id, void *buffer);
};

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

#endif //EXT2_EXT2_H
