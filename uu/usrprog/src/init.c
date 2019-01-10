#include <init.h>
#include "myvi.h"

#pragma GCC push_options
#pragma GCC optimize("O0")


unsigned int main(unsigned int argc, void *args) {
    // unsigned int ptr = malloc(16);
    // printf("malloc ptr : %x\n",ptr);
    // unsigned int ret =  myvi((char *)args);
    unsigned int ret =  myvi("test.txt");
    // int i = 23;
    return 10;

}

#pragma GCC pop_options

