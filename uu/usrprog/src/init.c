#include <init.h>


#pragma GCC push_options
#pragma GCC optimize("O0")


unsigned int main(unsigned int argc, void *args) {
    // test4(23);
    char x[5] = "23";
    
    printf(STRING(x));
    return 0;
}

#pragma GCC pop_options