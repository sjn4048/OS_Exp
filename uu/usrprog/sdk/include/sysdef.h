#ifndef _SYSDEF_H_
#define _SYSDEF_H_

#define test4(arg1) SYSCALL_DEFINE1(_test4, arg1)

#define SYSCALL_DEFINE0(name) sys##name

#define SYSCALL_DEFINE1(name, _arg1) do {                          \
    asm volatile("move $a0, %0\n\t" : : "r"((unsigned int)_arg1)); \
             sys##name;                                            \
} while(0)

#define SYSCALL_DEFINE2(name, _arg1, _arg2) do {                   \
    asm volatile("move $a0, %0\n\t" : : "r"((unsigned int)_arg1)); \
    asm volatile("move $a1, %0\n\t" : : "r"((unsigned int)_arg2)); \
             sys##name;                                            \
} while(0)

#define SYSCALL_DEFINE3(name, _arg1, _arg2, _arg3) do {                   \
    asm volatile("move $a0, %0\n\t" : : "r"((unsigned int)_arg1)); \
    asm volatile("move $a1, %0\n\t" : : "r"((unsigned int)_arg2)); \
    asm volatile("move $a2, %0\n\t" : : "r"((unsigned int)_arg3)); \
             sys##name;                                            \
} while(0)

#define sys_test4  __syscall(4) 

#define __syscall(code)    \
        asm volatile(       \
        "move $t0, %0\n\t"  \
        "syscall\n\t"       \
        "nop\n\t"           \
        : : "r"((unsigned int)code))     \


#endif
