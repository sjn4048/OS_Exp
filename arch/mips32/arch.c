#include "arch.h"

// Machine params

unsigned int* const holder1 = (unsigned int*)0xbfc04000;
unsigned int* const holder2 = (unsigned int*)0xbfc04000;
unsigned int* const holder3 = (unsigned int*)0xbfc04000;
unsigned int* const holder4 = (unsigned int*)0xbfc04000;
unsigned int* const holder5 = (unsigned int*)0xbfc04000;
unsigned int* const holder6 = (unsigned int*)0xbfc04000;
unsigned int* const holder7 = (unsigned int*)0xbfc04000;
unsigned int* const holder8 = (unsigned int*)0xbfc04000;
unsigned int* const holder9 = (unsigned int*)0xbfc04000;
unsigned int* const holder10 = (unsigned int*)0xbfc04000;
unsigned int* const holder11 = (unsigned int*)0xbfc04000;
unsigned int* const holder12 = (unsigned int*)0xbfc04000;
unsigned int* const holder13 = (unsigned int*)0xbfc04000;
unsigned int* const holder14 = (unsigned int*)0xbfc04000;
unsigned int* const holder15 = (unsigned int*)0xbfc04000;
unsigned int* const holder16 = (unsigned int*)0xbfc04000;
unsigned int* const holder17 = (unsigned int*)0xbfc04000;
unsigned int* const holder18 = (unsigned int*)0xbfc04000;
unsigned int* const holder19 = (unsigned int*)0xbfc04000;
unsigned int* const holder110 = (unsigned int*)0xbfc04000;

unsigned int* const CHAR_VRAM = (unsigned int*)0xbfc04000;
unsigned int* const GRAPHIC_VRAM = (unsigned int*)0xbfe0000;
unsigned int* const GPIO_SWITCH = (unsigned int*)0xbfc09000;     // switch read-only
unsigned int* const GPIO_BUTTON = (unsigned int*)0xbfc09004;     // button read-only
unsigned int* const GPIO_SEG = (unsigned int*)0xbfc09008;        // Seg R/W
unsigned int* const GPIO_LED = (unsigned int*)0xbfc0900c;        // LED R/W
unsigned int* const GPIO_PS2_DATA = (unsigned int*)0xbfc09010;   // PS/2 data register, R/W
unsigned int* const GPIO_PS2_CTRL = (unsigned int*)0xbfc09014;   // PS/2 control register, R/W
unsigned int* const GPIO_UART_DATA = (unsigned int*)0xbfc09018;  // UART data register, R/W
unsigned int* const GPIO_UART_CTRL = (unsigned int*)0xbfc0901c;  // UART control register, R/W
unsigned int* const GPIO_CURSOR = (unsigned int*)0xbfc09020;     // Cursor 8-bit frequency 8-bit row 8-bit col
unsigned int* const VGA_MODE = (unsigned int*)0xbfc09024;        // enable graphic mode

// kernel sp
volatile unsigned int kernel_sp = 0x81000000;

unsigned int get_phymm_size() {
    return MACHINE_MMSIZE;
}