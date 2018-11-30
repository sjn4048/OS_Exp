//
// Created by zengk on 2018/11/29.
//

#include "ext2.h"

#ifdef DEBUG_EXT2

#ifdef WINDOWS

int debug_cat(int type, const char *format, ...) {
    // header
    switch (type) {
        case DEBUG_LOG:
            printf("[ INFO  ] ");
            break;
        case DEBUG_WARNING:
            printf("[WARNING] ");
            break;
        case DEBUG_ERROR:
            printf("[ ERROR ] ");
            break;
        default:
            printf("Debug catalog failed.\n");
            break;
    }

    // pass variable length argument into printf
    va_list ap;
    va_start(ap, format);
    int count = vprintf(format, ap);
    va_end(ap);
    printf("\n");

    // return the count of successively printed argument
    return count;
}

#elif SWORD

int debug_cat(int type, const char *format, ...) {
    // header
    switch (type) {
        case DEBUG_LOG:
            kernel_printf("[ INFO  ] ");
            break;
        case DEBUG_WARNING:
            kernel_printf("[WARNING] ");
            break;
        case DEBUG_ERROR:
            kernel_printf("[ ERROR ] ");
            break;
        default:
            kernel_printf("Debug catalog failed.\n");
            break;
    }

    // pass variable length argument into printf
    va_list ap;
    va_start(ap, format);
    int count = kernel_vprintf(format, ap);
    va_end(ap);
    kernel_printf("\n");

    // return the count of successively printed argument
    return count;
}

#endif //system type

#else

int debug_cat(int type, const char *format, ...) {
    // empty function
    return 0;
}

#endif