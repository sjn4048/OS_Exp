#include <init.h>
// #include "myvi.h"
#include "ls.h"

#pragma GCC push_options
#pragma GCC optimize("O0")


unsigned int main(unsigned int argc, void *args) {
    // unsigned int ptr = malloc(16);
    // printf("malloc ptr : %x\n",ptr);
    printf("malloc ptr\n");
    // unsigned int ret =  myvi((char *)args);
    // unsigned int ret =  myvi("test.txt");
    unsigned int ret = ls("/");

    // int i = 23;
    return 10;

}

#pragma GCC pop_options

