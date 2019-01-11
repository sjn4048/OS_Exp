#include "ps.h"
#include <driver/ps2.h>
#include <driver/sd.h>
#include <driver/vga.h>
#include <zjunix/bootmm.h>
#include <zjunix/buddy.h>
#include <zjunix/fs/fat.h>
#include <zjunix/fs/ext2.h>
#include <zjunix/slab.h>
#include <zjunix/time.h>
#include <zjunix/log.h>
#include <zjunix/syscall.h>
#include <zjunix/utils.h>
#include "../usr/ls.h"
#include "exec.h"
#include "myvi.h"

char ps_buffer[64];
int ps_buffer_index;

void test_syscall4()
{
    syscall(4);
    // asm volatile(
    //     "li $a0, 0x00ff\n\t"
    //     "li $v0, 4\n\t"
    //     "syscall\n\t"
    //     "nop\n\t");
}

void test_proc()
{
    unsigned int timestamp;
    unsigned int currTime;
    unsigned int data;
    asm volatile("mfc0 %0, $9, 6\n\t"
                 : "=r"(timestamp));
    data = timestamp & 0xff;
    while (1)
    {
        asm volatile("mfc0 %0, $9, 6\n\t"
                     : "=r"(currTime));
        if (currTime - timestamp > 100000000)
        {
            timestamp += 100000000;
            *((unsigned int *)0xbfc09018) = data;
        }
    }
}

int proc_demo_create()
{

    return 0;
}

void ps()
{
    kernel_printf("Press any key to enter shell.\n");
    kernel_getchar();
    char c;
    ps_buffer_index = 0;
    ps_buffer[0] = 0;
    kernel_clear_screen(31);
    kernel_puts("\n", 0xfff, 0);
    kernel_puts("PowerShell\n", 0xfff, 0);
    kernel_puts("PS>", 0xfff, 0);
    while (1)
    {
        c = kernel_getchar();
        if (c == '\n')
        {
            ps_buffer[ps_buffer_index] = 0;
            if (kernel_strcmp(ps_buffer, "exit") == 0)
            {
                ps_buffer_index = 0;
                ps_buffer[0] = 0;
                kernel_printf("\n  PowerShell exit.\n");
            }
            else
                parse_cmd();
            ps_buffer_index = 0;
            kernel_puts("PS>", 0xfff, 0);
        }
        else if (c == 0x08)
        {
            if (ps_buffer_index)
            {
                ps_buffer_index--;
                kernel_putchar_at(' ', 0xfff, 0, cursor_row, cursor_col - 1);
                cursor_col--;
                kernel_set_cursor();
            }
        }
        else
        {
            if (ps_buffer_index < 63)
            {
                ps_buffer[ps_buffer_index++] = c;
                kernel_putchar(c, 0xfff, 0);
            }
        }
    }
}

void parse_cmd()
{
    unsigned int result = 0;
    char dir[32];
    char c;
    kernel_putchar('\n', 0, 0);
    char sd_buffer[8192];
    int i = 0;
    char *param;
    for (i = 0; i < 63; i++)
    {
        if (ps_buffer[i] == ' ')
        {
            ps_buffer[i] = 0;
            break;
        }
    }
    if (i == 63)
        param = ps_buffer;
    else
        param = ps_buffer + i + 1;
    if (ps_buffer[0] == 0)
    {
        return;
    }
    else if (kernel_strcmp(ps_buffer, "clear") == 0)
    {
        kernel_clear_screen(31);
    }
    else if (kernel_strcmp(ps_buffer, "echo") == 0)
    {
        kernel_printf("%s\n", param);
    }
    else if (kernel_strcmp(ps_buffer, "gettime") == 0)
    {
        char buf[10];
        get_time(buf, sizeof(buf));
        kernel_printf("%s\n", buf);
    }
    else if (kernel_strcmp(ps_buffer, "syscall4") == 0)
    {
        test_syscall4();
    }
    else if (kernel_strcmp(ps_buffer, "sdwi") == 0)
    {
        for (i = 0; i < 512; i++)
            sd_buffer[i] = i;
        sd_write_block(sd_buffer, 7, 1);
        kernel_puts("sdwi\n", 0xfff, 0);
    }
    else if (kernel_strcmp(ps_buffer, "sdr") == 0)
    {
        sd_read_block(sd_buffer, 7, 1);
        for (i = 0; i < 512; i++)
        {
            kernel_printf("%d ", sd_buffer[i]);
        }
        kernel_putchar('\n', 0xfff, 0);
    }
    else if (kernel_strcmp(ps_buffer, "sdwz") == 0)
    {
        for (i = 0; i < 512; i++)
        {
            sd_buffer[i] = 0;
        }
        sd_write_block(sd_buffer, 7, 1);
        kernel_puts("sdwz\n", 0xfff, 0);
    }
    else if (kernel_strcmp(ps_buffer, "mminfo") == 0)
    {
        bootmap_info("bootmm");
        buddy_info();
    }
    else if (kernel_strcmp(ps_buffer, "mmtest") == 0)
    {
        kernel_printf("kmalloc : %x, size = 1KB\n", kmalloc(1024));
    }

    // -------------------------------------------------
    // ----------- process schedule commands -----------
    else if (kernel_strcmp(ps_buffer, "ps") == 0)
    {
        result = print_proc();
        kernel_printf("ps return with %d\n", result);
    }
    else if (kernel_strcmp(ps_buffer, "rbtree") == 0)
    {
        result = print_rbtree_test();
        kernel_printf("rbtree return with %d\n", result);
    }
    else if (kernel_strcmp(ps_buffer, "kill") == 0)
    {
        int pid = param[0] - '0';
        kernel_printf("Killing process %d\n", pid);
        result = pc_kill(pid);
        kernel_printf("kill return with %d\n", result);
    }
    else if (kernel_strcmp(ps_buffer, "time") == 0)
    {
        pc_create("time", system_time_proc, 0, 0, 0, 1, 0);
    }
    else if (kernel_strcmp(ps_buffer, "proc") == 0)
    {
        result = proc_demo_create();
        kernel_printf("proc return with %d\n", result);
    }
    else if (kernel_strcmp(ps_buffer, "exec") == 0)
    {
        result = exec_from_file(param);
        kernel_printf("exec return with %d\n", result);
    }
    else if (kernel_strcmp(ps_buffer, "kill_cur") == 0)
    {
        result = pc_kill_current(param);
        kernel_printf("pc_kill_current return with %d\n", result);
    }

    // ----------- process schedule commands -----------
    // -------------------------------------------------

    // ====================================================+
    // ----------- ext2 file system commands -----------

    else if (kernel_strcmp(ps_buffer, "cat") == 0)
    {
        result = ext2_cat(param);
    }
    else if (kernel_strcmp(ps_buffer, "ls") == 0)
    {
        result = ext2_ls();
    }
    else if (kernel_strcmp(ps_buffer, "cd") == 0)
    {
        result = ext2_cd(param);
    }
    else if (kernel_strcmp(ps_buffer, "mkdir") == 0)
    {
        result = ext2_mkdir(param);
    }
    else if (kernel_strcmp(ps_buffer, "create") == 0)
    {
        result = ext2_create(param);
    }
    else if (kernel_strcmp(ps_buffer, "rm") == 0)
    {
        result = ext2_rm(param);
    }
    else if (kernel_strcmp(ps_buffer, "mv") == 0)
    {
        result = ext2_mv(param);
    }
    else if (kernel_strcmp(ps_buffer, "cp") == 0)
    {
        result = ext2_cp(param);
    }
    else if (kernel_strcmp(ps_buffer, "rename") == 0)
    {
        result = ext2_rename(param);
    }

    // ----------- ext2 file system commands -----------
    // ====================================================+

    else if (kernel_strcmp(ps_buffer, "vi") == 0)
    {
        result = myvi(param);
        kernel_printf("vi return with %d\n", result);
    }
    else
    {
        kernel_puts(ps_buffer, 0xfff, 0);
        kernel_puts(": command not found\n", 0xfff, 0);
    }
}
