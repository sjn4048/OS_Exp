#include "ext2.h"

void ext2_output_buffer(void *data, int length)
{
    __u8 *buffer = (__u8 *)data;
    for (int i = 0; i < length; i++)
    {
        kernel_printf("%x%x ", buffer[i] >> 4, buffer[i] & 0xf);
        if (i % 16 == 15)
        {
            kernel_printf("\n");
        }
        if (i % (16 * 20) == 16 * 20 - 1)
        {
            kernel_getchar();
        }
    }
    kernel_printf("end\n");
    kernel_getchar();
}

void ext2_show_group_decs(struct ext2_group_desc *gd)
{
    kernel_printf("group_desc.bg_inode_bitmap: %d\n", gd->bg_inode_bitmap);
    kernel_printf("group_desc.bg_block_bitmap: %d\n", gd->bg_block_bitmap);
    kernel_getchar();
}

void ext2_show_current_dir()
{
    kernel_printf("current_dir.inode.id: %d\n", current_dir.inode.id);
    kernel_printf("current_dir.inode.info.i_mode: %d\n", current_dir.inode.info.i_mode);
    kernel_printf("current_dir.inode.info.i_block[0]: %d\n", current_dir.inode.info.i_block[0]);
    kernel_getchar();
}
