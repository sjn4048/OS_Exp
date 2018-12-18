//
// Created by zengkai on 2018/11/29.
//

#include "ext2.h"

/**
 * print information, warning or error message on output device
 * via calling different function
 * determined by platform
 * @param type: type of message, information/warning/error
 * @param format: format string
 * @param ...: valuable length parameter
 * @return number of parameter that successfully output
 */
int debug_cat(int type, const char *format, ...) {
#ifdef DEBUG_EXT2
#ifdef WINDOWS
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
        case DEBUG_NONE:
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

    // return the count of successively printed argument
    return count;
#elif SWORD
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
#endif //SYSTEM TYPE
#else
    // empty function
    return 0;
#endif //DEBUG
}
