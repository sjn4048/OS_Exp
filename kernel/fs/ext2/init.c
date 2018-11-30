//
// Created by zengk on 2018/11/30.
//

#include "ext2.h"

int ext2_init() {
#ifdef WINDOWS

    disk.read = windows_disk_read;
    disk.write = windows_disk_write;

#elif SWORD

    disk.read = sd_read_sector_blocking;
    disk.write = sd_write_sector_blocking;

#endif // system type

    return INIT_SUCCESS;
}
