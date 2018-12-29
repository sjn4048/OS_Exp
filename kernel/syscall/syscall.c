#include <exc.h>
#include <zjunix/syscall.h>
#include "syscall4.h"

sys_fn syscalls[256];

void init_syscall() {
    register_exception_handler(8, __syscall);

    // register all syscalls here
    register_syscall(4, syscall4);
}

void __syscall(unsigned int status, unsigned int cause, context* pt_context) {
    unsigned int code;
    code = pt_context->v0;
    pt_context->epc += 4;
    if (syscalls[code]) {
        syscalls[code](status, cause, pt_context);
    }
}

void register_syscall(int index, sys_fn fn) {
    index &= 255;
    syscalls[index] = fn;
}

int syscall(unsigned int code){
    asm volatile(
        "move $v0, %0\n\t"
        "syscall\n\t"
        "nop\n\t"
        : : "r"(code));
    return 0;
}