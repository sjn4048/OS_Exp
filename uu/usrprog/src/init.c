#include <init.h>
#include "myvi.h"

#pragma GCC push_options
#pragma GCC optimize("O0")


unsigned int main(unsigned int argc, void *args) {
asm volatile("addi $sp, $sp, -32\n\t"); 
    unsigned int ptr = malloc(16);
asm volatile("addi $sp, $sp, 32\n\t"); 
    // printf("%dab\n",23);
    // unsigned int ret =  myvi((char *)args);
    // unsigned int ret =  myvi("test.txt");
    return 5;

}

#pragma GCC pop_options

