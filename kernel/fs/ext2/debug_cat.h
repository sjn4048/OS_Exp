//
// Created by zengk on 2018/11/29.
//

#ifndef EXT2_DEBUG_CAT_H
#define EXT2_DEBUG_CAT_H

#include "mode.h"

#ifdef WINDOWS

#include "stdio.h"
#include "stdarg.h"

#elif SWORD

#include "driver/vga.h"
#include "zjunix/utils.h"

#endif // system type

// type for debug_cat
#define DEBUG_LOG 0
#define DEBUG_WARNING 1
#define DEBUG_ERROR 2

// use like <printf>
// CAUTION: this function does not check the number of variable length arguments
int debug_cat(int type, const char *format, ...);

#endif //EXT2_DEBUG_CAT_H
