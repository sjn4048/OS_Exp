#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <sysdef.h>
#include <init.h>

extern unsigned int ENTRY_ADDR;
extern unsigned int TRANSFORM_ADDR;

void sdk_init(unsigned int argc, void *args, unsigned int entry_point);

// extern char printf_tmp[30];

#define STRING(x) ({                                   \
    int size = sizeof(x) / sizeof(char);               \
    int i = 0;                                         \
    for (i = 0; i < size; i++) printf_tmp[i] = x[i];   \
    printf_tmp;                                        \
})

#define TRANSFORM(name, _arg1, _arg2) (((unsigned int (*)(unsigned int func, \
 unsigned int entry, void * arg1, void * arg2))                                          \
(TRANSFORM_ADDR))((unsigned int)name, ENTRY_ADDR, (void *)_arg1, (void *)_arg2))

#endif