//
// Created by zengkai on 2018/11/29.
//

#include "ext2.h"

/**
 * read data for disk sector by sector (normally 512 bytes)
 * @param id: the address of sector to read
 * @param buffer: where store data read from disk
 * @return bytes of data read
 */
int windows_disk_read(__u32 id, void *buffer) {
    // buffer and initialize
    char deviceName[128];
    memset(deviceName, 0, 128);
    sprintf(deviceName, VIRTUAL_DISK);
    DWORD length = SECTOR_SIZE;

    // get handle
    HANDLE ext2Disk = CreateFileA(deviceName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, 0, NULL);
    // move file pointer
    long address_piece[2];
    long long *address = (long long *) address_piece;
    *address = id * SECTOR_SIZE;
    SetFilePointer(ext2Disk, (LONG) address_piece[0], &address_piece[1], FILE_BEGIN);

    if (ext2Disk == INVALID_HANDLE_VALUE) {
        // if we do not successfully get the resource
        debug_cat(DEBUG_ERROR, "Access hardware failed. Error code: 0x%X\n", (unsigned long) GetLastError());
        return 0;
    } else {
        // we read from the hard disk
        DWORD bytesOfRead;
        ReadFile(ext2Disk, buffer, length, &bytesOfRead, 0);
        // if we have read nothing
        if (bytesOfRead != length) {
            debug_cat(DEBUG_ERROR, "ReadFile failed. Error code: 0x%X\n", (unsigned long) GetLastError());
            return 0;
        } else {
            debug_cat(DEBUG_LOG, "Successfully read %ld byte(s) data.\n", (unsigned long) bytesOfRead);
            return (int) bytesOfRead;
        }
    }
}

/**
 * write data to disk sector by sector (normally 512 bytes)
 * @param id: the address of sector to write
 * @param buffer: where store data that will be writen to disk
 * @return bytes of data written
 */
int windows_disk_write(__u32 id, void *buffer) {
    // TODO: windows_disk_write(__u32 id, void *buffer) {}
    return 0;
}
