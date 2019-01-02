#include <init.h>


#pragma GCC push_options
#pragma GCC optimize("O0")

int test_jump(int arg){
    return arg;
}

char tmp[5];
unsigned int main(unsigned int argc, void *args) {
    // test4(23);
    tmp[0] = 'w';
    tmp[1] = 'a';
    tmp[2] = '2';
    tmp[3] = 0;
    printf(tmp);
    return 0;
}

#pragma GCC pop_options