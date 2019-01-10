#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <sysdef.h>
#include <init.h>
#include <file.h>


unsigned int sdk_init(unsigned int argc, char args[][10]);

unsigned long get_entry_filesize(u8 *entry);
unsigned long get_u32(u8 *ch);


#define STRING(x) ({                                   \
    int size = sizeof(x) / sizeof(char);               \
    int i = 0;                                         \
    for (i = 0; i < size; i++) printf_tmp[i] = x[i];   \
    printf_tmp;                                        \
})


int (*printf)(const char* format, ...); // = (int (*)(const char* format, ...))(0x8000283c);


// #define TRANSFORM(name, _arg1, _arg2) (((unsigned int (*)(unsigned int func, \
//  unsigned int entry, void * arg1, void * arg2))                                          \
// (TRANSFORM_ADDR))((unsigned int)name, ENTRY_ADDR, (void *)_arg1, (void *)_arg2))

#endif