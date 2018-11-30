//
// Created by zengk on 2018/11/29.
//

#include "ext2.h"

int windows_disk_read(int id, void *buffer) {
    // buffer and initialize
    char deviceName[128];
    memset(deviceName, 0, 128);
    sprintf(deviceName, VIRTUAL_DISK);
    DWORD length = SECTOR_SIZE;

    // get handle
    HANDLE ext2Disk = CreateFileA(deviceName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, 0, NULL);
    // move file pointer
    SetFilePointer(ext2Disk, (LONG) id * SECTOR_SIZE, NULL, FILE_BEGIN);

    if (ext2Disk == INVALID_HANDLE_VALUE) {
        // if we do not successfully get the resource
        debug_cat(DEBUG_ERROR, "Access hardware failed. Error code: 0x%X", (unsigned long) GetLastError());
        return 0;
    } else {
        // we read from the hard disk
        DWORD bytesOfRead;
        ReadFile(ext2Disk, buffer, length, &bytesOfRead, 0);
        // if we have read nothing
        if (bytesOfRead != length) {
            debug_cat(DEBUG_ERROR, "ReadFile failed. Error code: 0x%X", (unsigned long) GetLastError());
            return 0;
        } else {
            debug_cat(DEBUG_LOG, "Successfully read %ld byte(s) data.", (unsigned long) bytesOfRead);
            return (int) bytesOfRead;
        }
    }
}

int windows_disk_write(int id, void *buffer) {
    return 0;
}
