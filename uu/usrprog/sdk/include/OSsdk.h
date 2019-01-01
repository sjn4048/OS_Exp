#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <sysdef.h>

unsigned int sys_test4(int arg);

extern unsigned int ENTRY_ADDR;
extern const unsigned int TRANSFORM;


void sdk_init(unsigned int argc, void *args, unsigned int entry_point);



#endif