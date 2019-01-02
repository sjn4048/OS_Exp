#include <OSsdk.h>


unsigned int ENTRY_ADDR = 0;
const unsigned int TRANSFORM_ADDR = 0x80000900;

// kernel_printf = ((int (*)(const char* format, ...))(0x80002838));


void sdk_init(unsigned int argc, void *args, unsigned int entry_point){
    ENTRY_ADDR = entry_point;
    unsigned int rsa;
    asm volatile("move %0,$ra\n\t" : "=r"(rsa)); 
    TRANSFORM(((int (*)(const char* format, ...))(0x80002838)),"%x\n",rsa);
    TRANSFORM(((int (*)(const char* format, ...))(0x80002838)),"%x\n",rsa);
    TRANSFORM((unsigned int)main,0,0);
    // asm volatile(
    //     "la $t0, main\n\t"
    //     "add $t0, $t0, $a2\n\t"
    //     "j $t0\n\t"
    // );
}