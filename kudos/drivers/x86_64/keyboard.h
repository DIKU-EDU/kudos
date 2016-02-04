/*
 * x86 Keyboard Driver
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <arch.h>

/* x86 Keyboard ports (PS2) */
#define PS2_KEYBOARD_DATA       0x60
#define PS2_KEYBOARD_STATUS     0x64

/* Status Port Bits */
#define PS2_KB_STATUS_OUTPUT_BUSY       0x1
#define PS2_KB_STATUS_INPUT_BUSY        0x2

/* Controller Commands */
#define PS2_KB_SELFTEST         0xAA
#define PS2_KB_INTERFACETEST    0xAB
#define PS2_DISABLE_PORT1       0xAD
#define PS2_DISABLE_PORT2       0xA7

/* Encoder Commands */
#define PS2_KB_SETLEDS          0xED
#define PS2_KB_IDENTIFY         0xF2
#define PS2_KB_ENABLESCAN       0xF4
#define PS2_KB_RESETWAIT        0xF5
#define PS2_KB_RESETSCAN        0xF6
#define PS2_KB_ACK              0xFA
#define PS2_KB_RESET            0xFF

/* Error Codes */
#define PS2_ERR_BAT_FAILED      0xFC
#define PS2_ERR_DIAG_FAILED     0xFD
#define PS2_ERR_RESEND_CMD      0xFE

#define INVALID_SCANCODE        0xFFFF

/* Prototypes */
void keyboard_init();

char keyboard_getkey();

#endif /* TTY_H */
