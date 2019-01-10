#include <exc.h>
#include <zjunix/syscall.h>
#include "syscalls.h"
#include <driver/vga.h>
#include <intr.h>

sys_fn syscalls[256];

// simple Semaphore implemtation
int Semaphore = 1;

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
    register_syscall(18,fs_open_dir_syscall);
    register_syscall(19,fs_read_dir_syscall);
    register_syscall(20,get_filename_syscall);


    register_syscall(14,kernel_putchar_at_syscall);
    register_syscall(15,kernel_getchar_syscall);
    register_syscall(16,kernel_set_cursor_syscall);
    register_syscall(17,kernel_clear_screen_syscall);

    
}

// system syscall entry ( from exception )
void __syscall(unsigned int status, unsigned int cause, context* pt_context) {
    unsigned int code;
    code = pt_context->t0;
    pt_context->epc += 4;
    if (syscalls[code]) {
        pt_context->v0 = syscalls[code](status, cause, pt_context);
    }else{
        pt_context->v0 = 1;
    }
}

void register_syscall(int index, sys_fn fn) {
    index &= 255;
    syscalls[index] = fn;
}


// user syscall entry ( you can directly call this )
int syscall(unsigned int code){
    // simple Semaphore implemtation
    while (Semaphore == 0);
    disable_interrupts();
    Semaphore = 0;
    enable_interrupts();
    kernel_printf("into\n");
    asm volatile(
        "move $t0, %0\n\t"
        "syscall\n\t"
        "nop\n\t"
        : : "r"(code));
    // restore Semaphore
    disable_interrupts();
    Semaphore = 1;
    enable_interrupts();
    return 0;
}

