This is a ext2 file system, maybe.

### Structure

- ext2.h
    - debug_cat.c
    - init.c
    - windows_disk_driver.c
    - disk_tool.c
    - super.c
    - inode.c

### debug_cat.c

This module provides debug catalog tool.

In DEBUG mode, extra information will be printed via `debug_cat`.

In normal mode, `debug_cat` is an empty function and
this will keep you from removing this function one by one or
adding extra macro for them.

### init.c

This module will initialize some global variables.

Make sure to call it before using ext2.

### windows_disk_driver.c

This module will proceed data transfer, to simulate
the operation on a SD card.

The basic data transfer unit is 512 bytes, in other word, a sector.

### disk_tool.c

This module helps to find and decode the partition table.

### super.c

This module works on super block.

### inode.c

This module deals with inodes.