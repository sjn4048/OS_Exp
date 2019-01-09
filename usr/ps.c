#include "ps.h"
#include <driver/ps2.h>
#include <driver/sd.h>
#include <driver/vga.h>
#include <zjunix/bootmm.h>
#include <zjunix/buddy.h>
#include <zjunix/fs/fat.h>
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
char name[10];
void test_syscall4() {
    syscall(4);
    // asm volatile(
    //     "li $a0, 0x00ff\n\t"
    //     "li $v0, 4\n\t"
    //     "syscall\n\t"
    //     "nop\n\t");
}

void test_proc() {
    unsigned int timestamp;
    unsigned int currTime;
    unsigned int data;
    asm volatile("mfc0 %0, $9, 6\n\t" : "=r"(timestamp));
    data = timestamp & 0xff;
    while (1) {
        asm volatile("mfc0 %0, $9, 6\n\t" : "=r"(currTime));
        if (currTime - timestamp > 100000000) {
            timestamp += 100000000;
            *((unsigned int *)0xbfc09018) = data;
        }
    }
}

int proc_demo_create() {
    
    return 0;
}

void ps() {
    kernel_printf("Press any key to enter shell.\n");
    kernel_getchar();
    char c;
    ps_buffer_index = 0;
    ps_buffer[0] = 0;
    kernel_clear_screen(31);
    kernel_puts("\n", 0xfff, 0);
    kernel_puts("PowerShell\n", 0xfff, 0);
    kernel_puts("PS>", 0xfff, 0);
    while (1) {
        c = kernel_getchar();
        if (c == '\n') {
            ps_buffer[ps_buffer_index] = 0;
            if (kernel_strcmp(ps_buffer, "exit") == 0) {
                ps_buffer_index = 0;
                ps_buffer[0] = 0;
                kernel_printf("\n  PowerShell exit.\n");
            } else
                parse_cmd();
            ps_buffer_index = 0;
            kernel_puts("PS>", 0xfff, 0);
        }  else {
            if (ps_buffer_index < 63) {
                ps_buffer[ps_buffer_index++] = c;
                kernel_putchar(c, 0xfff, 0);
            }
        }
    }
}

void parse_cmd() {
    unsigned int result = 0;
    char dir[32];
    char c;
    kernel_putchar('\n', 0, 0);
    char sd_buffer[8192];
    int i = 0;
    int j;
    char *param;
    for (i = 0; i < 63; i++) {
        if (ps_buffer[i] == ' ') {
            ps_buffer[i] = 0;
            break;
        }
    }
    if (i == 63)
        param = ps_buffer;
    else
        param = ps_buffer + i + 1;
    if (ps_buffer[0] == 0) {
        return;
    } else if (kernel_strcmp(ps_buffer, "clear") == 0) {
        kernel_clear_screen(31);
    } else if (kernel_strcmp(ps_buffer, "echo") == 0) {
        kernel_printf("%s\n", param);
    } else if (kernel_strcmp(ps_buffer, "gettime") == 0) {
        char buf[10];
        get_time(buf, sizeof(buf));
        kernel_printf("%s\n", buf);
    } else if (kernel_strcmp(ps_buffer, "syscall4") == 0) {
        test_syscall4();
    } else if (kernel_strcmp(ps_buffer, "sdwi") == 0) {
        for (i = 0; i < 512; i++)
            sd_buffer[i] = i;
        sd_write_block(sd_buffer, 7, 1);
        kernel_puts("sdwi\n", 0xfff, 0);
    } else if (kernel_strcmp(ps_buffer, "sdr") == 0) {
        sd_read_block(sd_buffer, 7, 1);
        for (i = 0; i < 512; i++) {
            kernel_printf("%d ", sd_buffer[i]);
        }
        kernel_putchar('\n', 0xfff, 0);
    } else if (kernel_strcmp(ps_buffer, "sdwz") == 0) {
        for (i = 0; i < 512; i++) {
            sd_buffer[i] = 0;
        }
        sd_write_block(sd_buffer, 7, 1);
        kernel_puts("sdwz\n", 0xfff, 0);
    } else if (kernel_strcmp(ps_buffer, "mminfo") == 0) {
        bootmap_info("bootmm");
        buddy_info();
    } else if (kernel_strcmp(ps_buffer, "mmtest") == 0) {
        kernel_printf("kmalloc : %x, size = 1KB\n", kmalloc(1024));
    } 

    // -------------------------------------------------
    // ----------- process schedule commands -----------
    else if (kernel_strcmp(ps_buffer, "ps") == 0) {
        result = print_proc();
        kernel_printf("ps return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "rbtree") == 0) {
        result = print_rbtree_test();
        kernel_printf("rbtree return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "kill") == 0) {
        kernel_printf("Killing process %d\n", atoi(param));
        result = pc_kill(atoi(param));
        kernel_printf("kill return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "time") == 0) {
        pc_create("time",system_time_proc,0,0,0,1,0);
    } else if (kernel_strcmp(ps_buffer, "proc") == 0) {
        result = proc_demo_create();
        kernel_printf("proc return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "exec") == 0) {
        result = exec_from_file(param);
        kernel_printf("exec return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "kill_cur") == 0) {
        result = pc_kill_current(param);
        kernel_printf("pc_kill_current return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "change_latency") == 0) {
        kernel_printf_color(VGA_GREEN, "changing CPU interrupt interval to %d\n", atoi(param));
        change_sysctl_sched_latency(atoi(param));
        kernel_printf("change_sysctl_sched_latency return with 0\n");
    } else if (kernel_strcmp(ps_buffer, "tprog1") == 0) {
        for(j = 0;j < 10;j++) name[j] = param[j];
        name[9] = 0;
        if (name[0] != 't'){
            pc_create(name,test_program,1,name,0,1,0);
        }
        else{
            pc_create("default program",test_program,0,0,0,1,0);
        }
        kernel_printf("test_program return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "tprog2") == 0) {
        for(j = 0;j < 10;j++) name[j] = param[j];
        name[9] = 0;
        if (name[0] != 't'){
            pc_create(name,test_program,1,name,0,0,0);
        }
        else{
            pc_create("default program",test_program,0,0,0,0,0);
        }
        kernel_printf("test_program return with %d\n", result);
    }
    
    // ----------- process schedule commands -----------
    // -------------------------------------------------

    else if (kernel_strcmp(ps_buffer, "cat") == 0) {
        result = fs_cat(param);
        kernel_printf("cat return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "ls") == 0) {
        result = ls(param);
        kernel_printf("ls return with %d\n", result);
    } else if (kernel_strcmp(ps_buffer, "vi") == 0) {
        result = myvi(param);
        kernel_printf("vi return with %d\n", result);
    } else {
        kernel_puts(ps_buffer, 0xfff, 0);
        kernel_puts(": command not found\n", 0xfff, 0);
    }
}
