//
// Created by zengkai on 2018/11/30.
//

#include "ext2.h"

/**
 * initialize ext2 file system
 *   1. select disk interface: Windows or SWORD
 * @return success or not
 */
int ext2_init() {
#ifdef WINDOWS
    disk.read = windows_disk_read;
    disk.write = windows_disk_write;
#elif SWORD
    disk.read = sd_read_sector_blocking;
    disk.write = sd_write_sector_blocking;
#endif // system type
    debug_cat(DEBUG_LOG, "Successfully initialized\n");
    return INIT_SUCCESS;
}
