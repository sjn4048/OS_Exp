#include <init.h>


#pragma GCC push_options
#pragma GCC optimize("O0")


unsigned int main(unsigned int argc, void *args) {
    // test4(23);
    char x[5] = "23";
    int size = sizeof(x) / sizeof(char);               
    int i = 0;                                         
    for (i = 0; i < size; i++) printf_tmp[i] = x[i];   
    printf_tmp[7] = 0; 
    printf(printf_tmp);
    return 0;
}

#pragma GCC pop_options