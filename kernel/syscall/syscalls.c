#include <arch.h>
#include <zjunix/syscall.h>
#include <driver/vga.h>
#include <zjunix/slab.h>
#include <zjunix/fs/fat.h>
#include <driver/ps2.h>

unsigned int syscall4(unsigned int status, unsigned int cause, context* pt_context) {
    *GPIO_LED = 0x00ff;
    return 0;
}

unsigned int kernel_printf_syscall(unsigned int status, unsigned int cause, context* pt_context){
    const char *a0 = (char *)pt_context->a0;
    const char *a1 = (char *)pt_context->a1;
    kernel_printf(a0,a1);
    return 0;
}

unsigned int kmalloc_syscall(unsigned int status, unsigned int cause, context* pt_context){
    const unsigned int size = (unsigned int)pt_context->a0;
    void * ptr = kmalloc(size);
    // kernel_printf("aaa%x\n",(unsigned int)ptr);
    return (unsigned int)ptr;
}

unsigned int fs_open_syscall(unsigned int status, unsigned int cause, context* pt_context){
    FILE *file = (FILE *)pt_context->a0;
    u8 *filename = (u8 *)pt_context->a1;
    u32 ret = fs_open(file, filename);
    return ret;
}

unsigned int fs_read_syscall(unsigned int status, unsigned int cause, context* pt_context){
    FILE *file = (FILE *)pt_context->a0;
    u8 *buf = (u8 *)pt_context->a1;
    u32 count = (u32)pt_context->a1;
    u32 ret = fs_read(file, buf, count);
    return ret;
}

unsigned int fs_close_syscall(unsigned int status, unsigned int cause, context* pt_context){
    FILE *file = (FILE *)pt_context->a0;
    u32 ret = fs_close(file);
    return ret;
}

unsigned int fs_create_syscall(unsigned int status, unsigned int cause, context* pt_context){
    u8 *filename = (u8 *)pt_context->a0;
    u32 ret = fs_create(filename);
    return ret;
}

unsigned int fs_lseek_syscall(unsigned int status, unsigned int cause, context* pt_context){
    FILE *file = (FILE *)pt_context->a0;
    u32 new_loc = (u32)pt_context->a1;
    fs_lseek(file, new_loc);
    return 0;
}

unsigned int fs_write_syscall(unsigned int status, unsigned int cause, context* pt_context){
    FILE *file = (FILE *)pt_context->a0;
    const unsigned char * buf = (u8 *)pt_context->a1;
    u32 count = (u32)pt_context->a2;
    unsigned int ret = fs_write(file, buf, count);
    return ret;
}

unsigned int kernel_putchar_at_syscall(unsigned int status, unsigned int cause, context* pt_context){
    const int ch = (int)pt_context->a0;
    const int fc = (int)pt_context->a1;
    const int bg = (int)pt_context->a2;
    const int row = (int)pt_context->a3;
    const int col = (int)pt_context->t5;
    kernel_putchar_at(ch, fc, bg, row, col);
    return 0;
}

unsigned int kernel_getchar_syscall(unsigned int status, unsigned int cause, context* pt_context){
    unsigned int ret = kernel_getchar();
    return ret;
}

unsigned int kernel_set_cursor_syscall(unsigned int status, unsigned int cause, context* pt_context){
    kernel_set_cursor();
    return 0;
}

unsigned int kernel_clear_screen_syscall(unsigned int status, unsigned int cause, context* pt_context){
    int scope = (int)pt_context->a0;
    kernel_clear_screen(scope);
    return 0;
}


