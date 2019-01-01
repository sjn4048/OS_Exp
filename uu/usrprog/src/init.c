#include <head.h>

int main(unsigned int argc, void *args) {
    asm volatile(
        "syscall 4\n\t"
    );
    return (int)argc;
}
