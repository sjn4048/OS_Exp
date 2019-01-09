#include <OSsdk.h>


unsigned int ENTRY_ADDR;
unsigned int TRANSFORM_ADDR;



unsigned int sdk_init(unsigned int argc, void *args, unsigned int entry_point){
    ENTRY_ADDR = entry_point;
    TRANSFORM_ADDR = 0x80000900;
    printf = (int (*)(const char* format, ...))(0x8000282c);
    unsigned int ret = main(0,0);
    return ret;
    // TRANSFORM((unsigned int)main,0,0);
}

unsigned long get_entry_filesize(u8 *entry) {
    return get_u32(entry + 28);
}

unsigned long get_u32(u8 *ch) {
    return (*ch) + ((*(ch + 1)) << 8) + ((*(ch + 2)) << 16) + ((*(ch + 3)) << 24);
}