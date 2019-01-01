#include <init.h>

int test_jump(int arg){
    return arg;
}

int main(unsigned int argc, void *args) {
    test4(23);
    sys_fork(0);
    TRANSFORM(test_jump,4);
    return (int)argc;
}

