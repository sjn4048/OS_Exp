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
    HANDLE ext2Disk = CreateFile(deviceName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0,
                                 NULL);
    // move file pointer
    long long address = (long long) id * SECTOR_SIZE;
    unsigned long address_low = (unsigned long) (address & 0xffffffff);
    unsigned long address_high = (unsigned long) (address >> 32);
    SetFilePointer(ext2Disk, (LONG) address_low, (PLONG) &address_high, FILE_BEGIN);

    if (ext2Disk == INVALID_HANDLE_VALUE) {
        // if we do not successfully get the resource
        debug_cat(DEBUG_ERROR, "windows_disk_read: failed to access hardware. error code: 0x%X.\n",
                  (unsigned long) GetLastError());
        return 0;
    } else {
        // we read from the hard disk
        DWORD bytesOfRead;
        ReadFile(ext2Disk, buffer, length, &bytesOfRead, 0);
        CloseHandle(ext2Disk);
        // if we have read nothing
        if (bytesOfRead != length) {
            debug_cat(DEBUG_ERROR, "windows_disk_read: failed to read file. error code: 0x%X.\n",
                      (unsigned long) GetLastError());
            return 0;
        } else {
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
    // buffer and initialize
    char deviceName[128];
    memset(deviceName, 0, 128);
    sprintf(deviceName, VIRTUAL_DISK);
    DWORD length = SECTOR_SIZE;

    // get handle
    HANDLE ext2Disk = CreateFile(deviceName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                 OPEN_EXISTING, 0, NULL);
    // move file pointer
    long long address = (long long) id * SECTOR_SIZE;
    unsigned long address_low = (unsigned long) (address & 0xffffffff);
    unsigned long address_high = (unsigned long) (address >> 32);

    if (ext2Disk == INVALID_HANDLE_VALUE) {
        // if we do not successfully get the resource
        debug_cat(DEBUG_ERROR, "windows_disk_write: failed to access hardware. error code: 0x%X.\n",
                  (unsigned long) GetLastError());
        return 0;
    } else {
        SetFilePointer(ext2Disk, (LONG) address_low, (PLONG) &address_high, FILE_BEGIN);

//        DWORD bytes_returned;
//        if (0 == DeviceIoControl(ext2Disk, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes_returned, NULL)) {
//            debug_cat(DEBUG_ERROR, "windows_disk_write: failed to lock the device. Error code: 0x%X.\n",
//                      (unsigned long) GetLastError());
//            return 0;
//        }
        // we read from the hard disk
        DWORD bytesOfWrite;
        WriteFile(ext2Disk, buffer, length, &bytesOfWrite, 0);

//        if (0 == DeviceIoControl(ext2Disk, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytes_returned, NULL)) {
//            debug_cat(DEBUG_ERROR, "windows_disk_write: failed to unlock the device. Error code: 0x%X.\n",
//                      (unsigned long) GetLastError());
//            return 0;
//        }
        CloseHandle(ext2Disk);
        // if we have read nothing
        if (bytesOfWrite != length) {
            debug_cat(DEBUG_ERROR, "windows_disk_write: failed to write file. error code: 0x%X.\n",
                      (unsigned long) GetLastError());
            return 0;
        } else {
            return (int) bytesOfWrite;
        }
    }
}
