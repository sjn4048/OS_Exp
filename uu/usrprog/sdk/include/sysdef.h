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

#define SYSCALL_DEFINE4(name, _arg1, _arg2, _arg3, _arg4) ({                   \
    asm volatile("move $a0, %0\n\t" : : "r"((unsigned int)_arg1)); \
    asm volatile("move $a1, %0\n\t" : : "r"((unsigned int)_arg2)); \
    asm volatile("move $a2, %0\n\t" : : "r"((unsigned int)_arg3)); \
    asm volatile("move $a3, %0\n\t" : : "r"((unsigned int)_arg4)); \
             sys##name;                                            \
})

#define SYSCALL_DEFINE5(name, _arg1, _arg2, _arg3, _arg4, _arg5) ({                   \
    asm volatile("move $a0, %0\n\t" : : "r"((unsigned int)_arg1)); \
    asm volatile("move $a1, %0\n\t" : : "r"((unsigned int)_arg2)); \
    asm volatile("move $a2, %0\n\t" : : "r"((unsigned int)_arg3)); \
    asm volatile("move $a3, %0\n\t" : : "r"((unsigned int)_arg4)); \
    asm volatile("move $t5, %0\n\t" : : "r"((unsigned int)_arg5)); \
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

#define sys_fs_open ({                    \
    __syscall(5);                                \
    get_ret_value;   })

#define sys_fs_read ({                    \
    __syscall(6);                                \
    get_ret_value;   })

#define sys_fs_close ({                    \
    __syscall(3);                                \
    get_ret_value;   })

#define sys_fs_create ({                    \
    __syscall(11);                                \
    get_ret_value;   })

#define sys_fs_lseek ({                    \
    __syscall(12);                                \
    get_ret_value;   })

#define sys_fs_write ({                    \
    __syscall(13);                                \
    get_ret_value;   })

#define sys_kernel_putchar_at ({                    \
    __syscall(14);                                \
    get_ret_value;   })

#define sys_kernel_getchar ({                    \
    __syscall(15);                                \
    get_ret_value;   })

#define sys_kernel_set_cursor ({                    \
    __syscall(16);                                \
    get_ret_value;   })

#define sys_kernel_clear_screen ({                    \
    __syscall(17);                                \
    get_ret_value;   })



#define kernel_set_cursor() SYSCALL_DEFINE0(_kernel_set_cursor)
#define kernel_getchar() SYSCALL_DEFINE0(_kernel_getchar)
#define kernel_clear_screen(arg1) SYSCALL_DEFINE1(_kernel_clear_screen, arg1)
#define kernel_putchar_at(arg1, arg2, arg3, arg4, arg5) \
                SYSCALL_DEFINE5(_kernel_putchar_at, arg1, arg2, arg3, arg4, arg5)

#define fs_write(arg1, arg2, arg3) SYSCALL_DEFINE3(_fs_write, arg1, arg2, arg3)
#define fs_lseek(arg1, arg2) SYSCALL_DEFINE2(_fs_lseek, arg1, arg2)
#define fs_create(arg1) SYSCALL_DEFINE1(_fs_create, arg1)
#define fs_close(arg1) SYSCALL_DEFINE1(_fs_close, arg1)
#define fs_read(arg1, arg2, arg3) SYSCALL_DEFINE3(_fs_read, arg1, arg2, arg3)
#define fs_open(arg1, arg2) SYSCALL_DEFINE2(_fs_open, arg1, arg2)
#define test4(arg1) SYSCALL_DEFINE1(_test4, arg1)
#define fork() SYSCALL_DEFINE0(_fork)
#define malloc(arg1)  SYSCALL_DEFINE1(_malloc, arg1)

#define __syscall(code)     \
        asm volatile(       \
        "move $t0, %0\n\t"  \
        "syscall\n\t"       \
        "nop\n\t"           \
        : : "r"((unsigned int)code))     \


#endif
