#ifndef _ZJUNIX_SYSCALL_H
#define _ZJUNIX_SYSCALL_H

#include <zjunix/pc.h>

typedef unsigned int (*sys_fn)(unsigned int status, unsigned int cause, context* pt_context);

extern sys_fn syscalls[256];
extern int Semaphore;

void init_syscall();
void __syscall(unsigned int status, unsigned int cause, context* pt_context);
void register_syscall(int index, sys_fn fn);
int syscall(unsigned int code);

#endif // ! _ZJUNIX_SYSCALL_H
