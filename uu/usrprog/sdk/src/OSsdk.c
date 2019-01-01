#include <OSsdk.h>

int __syscall(unsigned int code){
    asm volatile(
        "move $t0, %0\n\t"
        "syscall\n\t"
        "nop\n\t"
        : : "r"(code));
    return 0;
}

unsigned int sys_test4(int arg){
    asm volatile(
        "move $t0, %0\n\t"
        "syscall\n\t"
        "nop\n\t"
        : : "r"(4));
    return 0;
}

void sdk_init(unsigned int argc, void *args, unsigned int entry_point){
    ENTRY_ADDR = entry_point;
    asm volatile(
        "la $t0, main"
        "add $t0, $t0, $a2"
        "j $t0"
    );
}