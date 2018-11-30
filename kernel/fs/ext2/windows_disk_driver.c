//
// Created by zengk on 2018/11/29.
//

#include "ext2.h"

int windows_disk_read(int id, void *buffer) {
    // buffer and initialize
    char deviceName[128];
    memset(deviceName, 0, 128);
    sprintf(deviceName, VIRTUAL_DISK);
    DWORD start = (DWORD) id * SECTOR_SIZE;
    DWORD length = SECTOR_SIZE;

    // get handle
    HANDLE ext2Disk = CreateFileA(
            deviceName,// device name
            GENERIC_READ, // access mode: read
            FILE_SHARE_READ | FILE_SHARE_WRITE, // share mode
            NULL, // security attributes, 0 stands for "cannot be inherited by any child processes"
            OPEN_EXISTING, // open existing object, if target does not exist, failed.
            0, // dwFlagsAndAttributes
            NULL); // hTemplateFile

    if (ext2Disk == INVALID_HANDLE_VALUE) {
        // if we do not successfully get the resource
        debug_cat(DEBUG_ERROR, "Access hardware failed. Error code: 0x%X", (unsigned long) GetLastError());
        return 0;
    } else {
        // we read from the hard disk
        DWORD bytesOfRead = start;
        ReadFile(ext2Disk, buffer, length, &bytesOfRead, 0);
        // if we have read nothing
        if (bytesOfRead <= start) {
            debug_cat(DEBUG_ERROR, "ReadFile failed. Error code: 0x%X", (unsigned long) GetLastError());
            return 0;
        } else {
            debug_cat(DEBUG_LOG, "Successfully read %ld byte(s) data.", (unsigned long) (bytesOfRead - start));
            return (int) (bytesOfRead - start);
        }
    }
}

int windows_disk_write(int id, void *buffer) {
    return 0;
}
