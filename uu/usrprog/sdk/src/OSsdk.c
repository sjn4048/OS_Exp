#include <OSsdk.h>


int syscall(unsigned int code){
    asm volatile(
        "move $t0, %0\n\t"
        "syscall\n\t"
        "nop\n\t"
        : : "r"(code));
    return 0;
}

