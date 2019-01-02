
#include <arch.h>
#include <zjunix/syscall.h>
#include <driver/vga.h>
#include <zjunix/slab.h>

unsigned int syscall4(unsigned int status, unsigned int cause, context* pt_context) {
    *GPIO_LED = 0x00ff;
    return 0;
}

unsigned int kernel_printf_syscall(unsigned int status, unsigned int cause, context* pt_context){
    const char *format = (char *)pt_context->a0;
    kernel_printf(format);
    return 0;
}

unsigned int kmalloc_syscall(unsigned int status, unsigned int cause, context* pt_context){
    kernel_printf("herer\n");
    const unsigned int size = (unsigned int)pt_context->a0;
    void * ptr = kmalloc(size);
    return (unsigned int)ptr;
}


