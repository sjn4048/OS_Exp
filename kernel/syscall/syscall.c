#include <exc.h>
#include <zjunix/syscall.h>
#include "syscalls.h"
#include <driver/vga.h>

sys_fn syscalls[256];

void init_syscall() {
    register_exception_handler(8, __syscall);

    // register all syscalls here // 8  \ 10
    register_syscall(4, syscall4);
    register_syscall(9,kernel_printf_syscall);
    register_syscall(7,kmalloc_syscall);


    register_syscall(5,fs_open_syscall);
    register_syscall(6,fs_read_syscall);
    register_syscall(3,fs_close_syscall);
    register_syscall(11,fs_create_syscall);
    register_syscall(12,fs_lseek_syscall);
    register_syscall(13,fs_write_syscall);


    register_syscall(14,kernel_putchar_at_syscall);
    register_syscall(15,kernel_getchar_syscall);
    register_syscall(16,kernel_set_cursor_syscall);
    register_syscall(17,kernel_clear_screen_syscall);

    
}

void __syscall(unsigned int status, unsigned int cause, context* pt_context) {
    unsigned int code;
    code = pt_context->t0;
    pt_context->epc += 4;
    if (syscalls[code]) {
        pt_context->v0 = syscalls[code](status, cause, pt_context);
    }
}

void register_syscall(int index, sys_fn fn) {
    index &= 255;
    syscalls[index] = fn;
}

int syscall(unsigned int code){
    asm volatile(
        "move $t0, %0\n\t"
        "syscall\n\t"
        "nop\n\t"
        : : "r"(code));
    return 0;
}

