#include <init.h>

int main(unsigned int argc, void *args) {
    syscall(4);
    // asm volatile(
    //     "li $t0, 4\n\t"
    //     "syscall\n\t"
    //     "nop\n\t"
    // );
    return (int)argc;
}

