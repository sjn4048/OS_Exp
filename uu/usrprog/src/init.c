#include <init.h>

int test_jump(int arg){
    return arg;
}

unsigned int main(unsigned int argc, void *args) {
    test4(23);
    unsigned int ra;
    asm volatile("move %0,$ra\n\t" : "=r"(ra)); 
    return ra;
}

