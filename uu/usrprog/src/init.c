#include <init.h>


#pragma GCC push_options
#pragma GCC optimize("O0")


unsigned int main(unsigned int argc, void *args) {

    char x[7] = "%xab\n";
    unsigned int ptr = malloc(16);
    printf(STRING(x),23);
    return 0;
}

#pragma GCC pop_options

