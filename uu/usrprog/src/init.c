#include <init.h>

int main(unsigned int argc, void *args) {
    syscall(4);
    return (int)argc;
}
