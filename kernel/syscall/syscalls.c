
#include <arch.h>
#include <zjunix/syscall.h>
#include <driver/vga.h>
#include <zjunix/slab.h>

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
    kernel_printf("aaa%x\n",(unsigned int)ptr);
    return (unsigned int)ptr;
}
