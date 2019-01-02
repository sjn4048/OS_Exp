#include <init.h>


#pragma GCC push_options
#pragma GCC optimize("O0")

int test_jump(int arg){
    return arg;
}

char printf_tmp[20];
unsigned int main(unsigned int argc, void *args) {
    // test4(23);
    printf_tmp[0] = 'w';
    printf_tmp[2] = 'w';
    printf_tmp[1] = 'w';
    printf_tmp[3] = 'w';
    printf_tmp[4] = 'w';
    printf_tmp[5] = 'w';
    printf_tmp[6] = 0;
    printf((unsigned int)printf_tmp + ENTRY_ADDR);
    return 0;
}

#pragma GCC pop_options