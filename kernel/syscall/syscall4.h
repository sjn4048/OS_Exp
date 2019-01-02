#ifndef _SYSCALL4_H
#define _SYSCALL4_H

unsigned int syscall4(unsigned int status, unsigned int cause, context* pt_context);
unsigned int kernel_printf_syscall(unsigned int status, unsigned int cause, context* pt_context);
unsigned int kmalloc_syscall(unsigned int status, unsigned int cause, context* pt_context);

#endif  // ! _SYSCALL4_H