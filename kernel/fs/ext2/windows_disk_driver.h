//
// Created by zengk on 2018/11/29.
//

#ifndef EXT2_WINDOWS_DISK_DRIVER_H
#define EXT2_WINDOWS_DISK_DRIVER_H

#include "types.h"
#include "debug_cat.h"
#include "windows.h"
#include "stdio.h"

#define VIRTUAL_DISK "\\\\.\\PhysicalDrive2" // this is the virtual disk used while my developing
#define SECTOR_SIZE 512 // this is the sector size of virtual disk

// read from disk directly
// @id cannot be negative
// @buffer cannot be NULL
// @return is the length that successfully read
int windows_disk_read(int id, void *buffer);

// write to disk directly
// @id cannot be negative
// @buffer cannot be NULL
// @return is the length that successfully wrote
int windows_disk_write(int id, void *buffer);

#endif //EXT2_WINDOWS_DISK_DRIVER_H
