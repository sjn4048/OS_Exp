//
// Created by zengk on 2018/11/29.
//

#ifndef EXT2_PHYSICAL_INTERFACE_H
#define EXT2_PHYSICAL_INTERFACE_H

#include "mode.h"
#include "types.h"

struct physical_interface {
    // read 512 bytes from hardware
    // id is 512-byte-aligned address (module 512)
    int (*read)(int id, void *buffer);

    // write 512 bytes to hardware
    // id is 512-byte-aligned address (module 512)
    int (*write)(int id, void *buffer);
};

#ifdef WINDOWS

#include "windows_disk_driver.h"

struct physical_interface disk = {windows_disk_read, windows_disk_write};

#elif SWORD

#include "driver/sd.h"
struct physical_interface disk = {sd_read_sector_blocking, sd_write_sector_blocking};

#endif // select platform

#endif //EXT2_PHYSICAL_INTERFACE_H
