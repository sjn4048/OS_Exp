
#include <arch.h>
#include <zjunix/syscall.h>

unsigned int syscall4(unsigned int status, unsigned int cause, context* pt_context) {
    *GPIO_LED = 0x00ff;
    return 0;
}