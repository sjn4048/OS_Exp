#include <OSsdk.h>


unsigned int ENTRY_ADDR;
unsigned int TRANSFORM_ADDR;


char * printf_tmp;

void sdk_init(unsigned int argc, void *args, unsigned int entry_point){
    ENTRY_ADDR = entry_point;
    printf_tmp = (char *)malloc(30);
    printf_tmp[29] = 0;
    TRANSFORM_ADDR = 0x80000900;
    TRANSFORM((unsigned int)main,0,0);
}