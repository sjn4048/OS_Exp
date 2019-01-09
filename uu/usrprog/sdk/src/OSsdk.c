#include <OSsdk.h>


unsigned int ENTRY_ADDR;
unsigned int TRANSFORM_ADDR;



void sdk_init(unsigned int argc, void *args, unsigned int entry_point){
    ENTRY_ADDR = entry_point;
    TRANSFORM_ADDR = 0x80000900;
    printf = (int (*)(const char* format, ...))(0x8000283c);
    main(0,0);
    // TRANSFORM((unsigned int)main,0,0);
}

