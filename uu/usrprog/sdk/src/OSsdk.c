#include <OSsdk.h>


unsigned int ENTRY_ADDR = 0;
const unsigned int TRANSFORM_ADDR = 0x80000900;


int (*kernel_printf)(const char* format, ...) = (int (*)(const char* format, ...))(0x80002838);


void sdk_init(unsigned int argc, void *args, unsigned int entry_point){
    ENTRY_ADDR = entry_point;
    unsigned int ra;
    asm volatile("move %0,$ra\n\t" : "=r"(ra)); 
    // kernel_printf("%x",ra);
    TRANSFORM((unsigned int)main,0,0);

}