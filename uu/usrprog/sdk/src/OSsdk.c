#include <OSsdk.h>


unsigned int ENTRY_ADDR = 0;
const unsigned int TRANSFORM_ADDR = 0x80000900;



void sdk_init(unsigned int argc, void *args, unsigned int entry_point){
    ENTRY_ADDR = entry_point;
    TRANSFORM((unsigned int)main,0,0);
    asm volatile(
        "la $t0, main\n\t"
        "add $t0, $t0, $a2\n\t"
        "j $t0\n\t"
    );
}