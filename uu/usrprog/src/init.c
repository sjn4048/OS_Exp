#include <init.h>

int test_jump(int arg){
    return arg;
}

char tmp[] = "13";
int tes = 2;
unsigned int main(unsigned int argc, void *args) {
    test4(23);
    unsigned int init_gp;
    asm volatile("la %0, _gp\n\t" : "=r"(init_gp)); 
    return init_gp;
}

