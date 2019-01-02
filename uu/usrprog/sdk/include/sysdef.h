#ifndef _SYSDEF_H_
#define _SYSDEF_H_

#define SYSCALL_DEFINE0(name) sys##name

#define SYSCALL_DEFINE1(name, _arg1) ({                          \
    asm volatile("move $a0, %0\n\t" : : "r"((unsigned int)_arg1)); \
             sys##name;                                            \
})

#define SYSCALL_DEFINE2(name, _arg1, _arg2) ({                   \
    asm volatile("move $a0, %0\n\t" : : "r"((unsigned int)_arg1)); \
    asm volatile("move $a1, %0\n\t" : : "r"((unsigned int)_arg2)); \
             sys##name;                                            \
})

#define SYSCALL_DEFINE3(name, _arg1, _arg2, _arg3) ({                   \
    asm volatile("move $a0, %0\n\t" : : "r"((unsigned int)_arg1)); \
    asm volatile("move $a1, %0\n\t" : : "r"((unsigned int)_arg2)); \
    asm volatile("move $a2, %0\n\t" : : "r"((unsigned int)_arg3)); \
             sys##name;                                            \
})

#define get_ret_value ({                           \
    unsigned int tmp = 0;                                   \
    asm volatile("move %0, $v0\n\t" : "=r"(tmp));  \
    tmp;  })

#define sys_test4 __syscall(4)

#define sys_fork ({                    \
    __syscall(8);                              \
    get_ret_value;   })

#define sys_malloc ({                    \
    __syscall(7);                                \
    get_ret_value;   })

#define sys_printf __syscall(9)

#define test4(arg1) SYSCALL_DEFINE1(_test4, arg1)
#define fork() SYSCALL_DEFINE0(_fork)
#define printf(arg1, arg2) SYSCALL_DEFINE2(_printf, arg1, arg2)
#define malloc(arg1)  SYSCALL_DEFINE1(_malloc, arg1)

#define __syscall(code)     \
        asm volatile(       \
        "move $t0, %0\n\t"  \
        "syscall\n\t"       \
        "nop\n\t"           \
        : : "r"((unsigned int)code))     \


#endif
