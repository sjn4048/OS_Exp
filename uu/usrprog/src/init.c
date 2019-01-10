#include <init.h>
// #include "myvi.h"
#include "ls.h"

#pragma GCC push_options
#pragma GCC optimize("O0")


unsigned int main(unsigned int argc, char params[][10]) {
    // unsigned int ptr = malloc(16);
    // printf("malloc ptr : %x\n",ptr);

    // unsigned int ret =  myvi((char *)args);
    // unsigned int ret =  myvi("test.txt");
    
    // unsigned int ret = ls((char *)args);
    unsigned int ret = ls(params[1]);

    return 10;

}

#pragma GCC pop_options

