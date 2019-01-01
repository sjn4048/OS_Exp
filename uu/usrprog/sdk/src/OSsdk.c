#include <OSsdk.h>


unsigned int ENTRY_ADDR = 0;
const unsigned int TRANSFORM_ADDR = 0x80000900;

#define TRANSFORM(name) (((unsigned int (*)(unsigned int func, \
 unsigned int entry, void * arg1))                                          \
(TRANSFORM_ADDR))((unsigned int)name,ENTRY_ADDR,))

void sdk_init(unsigned int argc, void *args, unsigned int entry_point){
    ENTRY_ADDR = entry_point;
    asm volatile(
        "la $t0, main\n\t"
        "add $t0, $t0, $a2\n\t"
        "j $t0\n\t"
    );
}