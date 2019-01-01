#include <OSsdk.h>


unsigned int ENTRY_ADDR = 0;
const unsigned int TRANSFORM = 0x80000900;


unsigned int sys_test4(int arg){
    __syscall(4);
    return 0;
}

void sdk_init(unsigned int argc, void *args, unsigned int entry_point){
    ENTRY_ADDR = entry_point;
    asm volatile(
        "la $t0, main\n\t"
        "add $t0, $t0, $a2\n\t"
        "j $t0\n\t"
    );
}