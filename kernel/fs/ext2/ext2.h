//
// Created by zengk on 2018/11/29.
//

#ifndef EXT2_EXT2_H
#define EXT2_EXT2_H

#include "mode.h"

#ifdef DEBUG_EXT2

// provide debug support
#include "debug_cat.h"

#endif //DEBUG_EXT2

// provide type definition and transition
#include "types.h"

// connect to the lowest layer
// provide basic read and write function communicating with hardware
// basic data transfer unit size is 512 bytes
// the functions are encapsulated to a structure called <disk>
// provide different implement in Windows and SWORD
#include "physical_interface.h"

#endif //EXT2_EXT2_H
