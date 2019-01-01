#ifndef _SYSDEF_H_
#define _SYSDEF_H_

#define test4(arg1) SYSCALL_DEFINE1(_test4, arg1)

#define SYSCALL_DEFINE1(name, arg1) do {                          \
    asm volatile("move $a0, %0\n\t" : : "r"((unsigned int)arg1)); \
             sys##name;                                           \
} while(0);


#define sys_test4 ( __syscall(4) )

#define __syscall(code) ( \
    asm volatile(           \
        "move $t0, %0\n\t"  \
        "syscall\n\t"       \
        "nop\n\t"           \
        : : "r"(code));     \
)

// #define SYSCALL_DEFINE1(name, ...) SYSCALL_DEFINEx(1, _##name, __VA_ARGS__)
// #define SYSCALL_DEFINE2(name, ...) SYSCALL_DEFINEx(2, _##name, __VA_ARGS__)
// #define SYSCALL_DEFINE3(name, ...) SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)
// #define SYSCALL_DEFINE4(name, ...) SYSCALL_DEFINEx(4, _##name, __VA_ARGS__)
// #define SYSCALL_DEFINE5(name, ...) SYSCALL_DEFINEx(5, _##name, __VA_ARGS__)
// #define SYSCALL_DEFINE6(name, ...) SYSCALL_DEFINEx(6, _##name, __VA_ARGS__)


#endif
