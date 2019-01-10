#include <OSsdk.h>





unsigned int sdk_init(unsigned int argc, char args[][10]){
    printf = (int (*)(const char* format, ...))(0x8000283c);
    unsigned int ret = main(argc,args);
    return ret;
    // TRANSFORM((unsigned int)main,0,0);
}

unsigned long get_entry_filesize(u8 *entry) {
    return get_u32(entry + 28);
}

unsigned long get_u32(u8 *ch) {
    return (*ch) + ((*(ch + 1)) << 8) + ((*(ch + 2)) << 16) + ((*(ch + 3)) << 24);
}


