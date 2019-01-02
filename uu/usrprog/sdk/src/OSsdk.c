#include <OSsdk.h>


unsigned int ENTRY_ADDR;
unsigned int TRANSFORM_ADDR;


// char printf_tmp[30];

void sdk_init(unsigned int argc, void *args, unsigned int entry_point){
    ENTRY_ADDR = entry_point;
    int (*kernel_printf)(const char* format, ...) = (int (*)(const char* format, ...))(0x80002838);
    TRANSFORM_ADDR = 0x80000900;
    TRANSFORM((unsigned int)main,0,0);
}